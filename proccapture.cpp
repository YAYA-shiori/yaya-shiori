//
// AYA version 5
//
// 外部プログラムを起動し、標準入力を渡して、標準出力・標準エラー出力を取り込む（EXECUTE_CAPTURE）
//
// 設計の要点
// ・標準入力の書き込みと、標準出力・標準エラー出力の読み出しは並行して行う（片方だけを順に処理すると、
//   パイプが一杯になって子プロセスと互いに待ち合うため）。Windows は書き込み専用のスレッドを立て、
//   POSIX は子プロセスを二重に fork して、中間の子に書かせる
// ・POSIX の中間の子は孫（= 実行するコマンド）を待って終了コードをパイプで返す。ホストが SIGCHLD を
//   無視していても終了コードを取れる（EXECUTE_WAIT と同じ作り）。孫は自分のプロセスグループに入れ、
//   中断・時間切れ・上限超過のときはグループごと終了させる
// ・標準入力を指定しないときは、すぐ EOF になる入力（NUL・/dev/null）につなぐ
// ・子プロセスが終わっても、孫プロセスがパイプを握っていると EOF が来ない。終了後はデータが途切れてから
//   一定時間（PCAP_DRAIN_IDLE_MS）だけ待って打ち切る
//

#if defined(WIN32)
# include "stdafx.h"
# include <process.h>
#endif

#include <string>
#include <vector>

#if defined(POSIX)
# include <cstring>
# include <pthread.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <errno.h>
# include <stdlib.h>
# include <time.h>
#endif
#include <string.h>

#include "proccapture.h"
#include "globaldef.h"

// 子プロセスが終わった後、出力が途切れてから打ち切るまでの時間
static const unsigned long PCAP_DRAIN_IDLE_MS = 500;
// 標準入力の書き込みスレッドを待つ時間（Windows）
static const unsigned long PCAP_WRITER_JOIN_MS = 1000;

/* -----------------------------------------------------------------------
 *  読んだ分を out に足す
 *  VC6 の std::string は足りない分しか確保し直さないため、容量は倍々で確保する
 * -----------------------------------------------------------------------
 */
static void AppendBytes(std::string &out, const char *buf, size_t len)
{
	if ( out.capacity() < out.size() + len ) {
		out.reserve((out.size() + len) * 2);
	}
	out.append(buf, len);
}

/* -----------------------------------------------------------------------
 *  出力が上限を超えていたら、上限までに切り詰める
 *  返値：true=超えていた
 * -----------------------------------------------------------------------
 */
static bool CutOver(std::string &s, size_t maxsize)
{
	if ( s.size() > maxsize ) {
		s.erase(maxsize);
		return true;
	}
	return false;
}

#if defined(WIN32)

/* -----------------------------------------------------------------------
 *  CProcSpawnLock（Windows）
 * -----------------------------------------------------------------------
 */
namespace {
	class CSpawnCS {
	public:
		CRITICAL_SECTION cs;
		CSpawnCS(void) { ::InitializeCriticalSection(&cs); }
		~CSpawnCS(void) { ::DeleteCriticalSection(&cs); }
	};
	CSpawnCS g_spawn_cs;
}

CProcSpawnLock::CProcSpawnLock(void)
{
	::EnterCriticalSection(&g_spawn_cs.cs);
}

CProcSpawnLock::~CProcSpawnLock(void)
{
	::LeaveCriticalSection(&g_spawn_cs.cs);
}

#ifndef CREATE_NO_WINDOW
#define CREATE_NO_WINDOW 0x08000000
#endif

/* -----------------------------------------------------------------------
 *  標準入力の書き込みスレッド
 *  文脈（バッファとハンドル）はスレッドが所有し、書き終わったら（または相手が閉じたら）
 *  ハンドルを閉じて自分で解放する。呼び出し側は文脈に触れない
 * -----------------------------------------------------------------------
 */
struct CStdinWriter
{
	HANDLE	h;
	char	*data;
	size_t	len;
};

static unsigned __stdcall StdinWriterProc(void *arg)
{
	CStdinWriter *x = static_cast<CStdinWriter*>(arg);
	size_t pos = 0;
	while ( pos < x->len ) {
		size_t rest = x->len - pos;
		DWORD chunk = static_cast<DWORD>( rest > 65536 ? 65536 : rest );
		DWORD written = 0;
		if ( ! ::WriteFile(x->h, x->data + pos, chunk, &written, NULL) || written == 0 ) {
			break;
		}
		pos += written;
	}
	::CloseHandle(x->h);
	free(x->data);
	delete x;
	return 0;
}

static void CloseH(HANDLE &h)
{
	if ( h != NULL && h != INVALID_HANDLE_VALUE ) {
		::CloseHandle(h);
	}
	h = NULL;
}

/* -----------------------------------------------------------------------
 *  パイプに溜まっている分を s に足す（limit まで）
 *  返値：読んだバイト数。パイプが閉じられた（もう読むものがない）ときは eof を立てる
 * -----------------------------------------------------------------------
 */
static size_t ReadAvailWin32(HANDLE h, std::string &s, size_t limit, bool &eof)
{
	size_t total = 0;
	char buf[16384];
	while ( s.size() < limit ) {
		DWORD avail = 0;
		if ( ! ::PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL) ) {
			eof = true;
			break;
		}
		if ( avail == 0 ) {
			break;
		}
		size_t want = avail < sizeof(buf) ? avail : sizeof(buf);
		if ( want > limit - s.size() ) {
			want = limit - s.size();
		}
		DWORD got = 0;
		if ( ! ::ReadFile(h, buf, static_cast<DWORD>(want), &got, NULL) || got == 0 ) {
			eof = true;
			break;
		}
		AppendBytes(s, buf, got);
		total += got;
	}
	return total;
}

static void RunWin32(CProcCapture &c)
{
	c.status = PCAP_FAILED;
	c.exitcode = -1;

	const size_t limit = c.maxsize + 1;		// 上限を超えたかを見分けるため、1バイト多く読む

	HANDLE inR = NULL, inW = NULL, outR = NULL, outW = NULL, errR = NULL, errW = NULL;
	HANDLE hNul = INVALID_HANDLE_VALUE;

	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	bool created = false;

	{
		// パイプを作ってから、子の側のハンドルを閉じるまで、他の起動を入れない
		CProcSpawnLock lock;

		bool ok = true;
		hNul = ::CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa, OPEN_EXISTING, 0, NULL);

		if ( c.has_input ) {
			ok = ::CreatePipe(&inR, &inW, &sa, 65536) != FALSE;
			if ( ok ) {
				::SetHandleInformation(inW, HANDLE_FLAG_INHERIT, 0);
			}
		}
		if ( ok ) {
			ok = ::CreatePipe(&outR, &outW, &sa, 65536) != FALSE;
			if ( ok ) {
				::SetHandleInformation(outR, HANDLE_FLAG_INHERIT, 0);
			}
		}
		if ( ok && c.stderr_mode == PCAP_STDERR_SEPARATE ) {
			ok = ::CreatePipe(&errR, &errW, &sa, 65536) != FALSE;
			if ( ok ) {
				::SetHandleInformation(errR, HANDLE_FLAG_INHERIT, 0);
			}
		}

		if ( ok ) {
			HANDLE nul = ( hNul != INVALID_HANDLE_VALUE ) ? hNul : NULL;

			STARTUPINFOW si;
			memset(&si, 0, sizeof(si));
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			si.hStdInput  = c.has_input ? inR : nul;
			si.hStdOutput = outW;
			si.hStdError  = ( c.stderr_mode == PCAP_STDERR_SEPARATE ) ? errW
				: ( c.stderr_mode == PCAP_STDERR_MERGE ) ? outW : nul;

			// CreateProcessW はコマンドラインに書き換え可能なバッファを要求する
			std::vector<wchar_t> cmdbuf(c.cmdline.begin(), c.cmdline.end());
			cmdbuf.push_back(L'\0');

			created = ::CreateProcessW(c.app.size() ? c.app.c_str() : NULL, &cmdbuf[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL,
				c.dir.size() ? c.dir.c_str() : NULL, &si, &pi) != FALSE;
		}

		// 子の側のハンドルは、起動したあと（失敗したときも）親が持たないようにする
		CloseH(inR);
		CloseH(outW);
		CloseH(errW);
		CloseH(hNul);
	}

	if ( ! created ) {
		CloseH(inW);
		CloseH(outR);
		CloseH(errR);
		return;
	}
	::CloseHandle(pi.hThread);

	// 標準入力は書き込みスレッドに任せる（ハンドルの所有もスレッドに移る）
	HANDLE hWriter = NULL;
	bool writer_failed = false;
	if ( c.has_input ) {
		CStdinWriter *x = new CStdinWriter;
		x->h = inW;
		x->len = c.input.size();
		x->data = static_cast<char*>( malloc(x->len ? x->len : 1) );
		if ( x->data ) {
			if ( x->len ) {
				memcpy(x->data, c.input.data(), x->len);
			}
			unsigned tid = 0;
			hWriter = reinterpret_cast<HANDLE>( _beginthreadex(NULL, 0, StdinWriterProc, x, 0, &tid) );
		}
		if ( hWriter == NULL ) {
			::CloseHandle(x->h);
			free(x->data);
			delete x;
			writer_failed = true;
		}
		inW = NULL;
	}

	int why = writer_failed ? PCAP_FAILED : PCAP_OK;
	bool exited = false;
	bool out_eof = false;
	bool err_eof = ( errR == NULL );
	const DWORD start = ::GetTickCount();
	DWORD last_data = start;

	while ( why == PCAP_OK ) {
		size_t got = ReadAvailWin32(outR, c.out, limit, out_eof);
		if ( ! err_eof ) {
			got += ReadAvailWin32(errR, c.err, limit, err_eof);
		}
		const DWORD now = ::GetTickCount();
		if ( got ) {
			last_data = now;
		}

		if ( c.out.size() > c.maxsize || c.err.size() > c.maxsize ) {
			why = PCAP_TRUNCATED;
			break;
		}

		if ( exited ) {
			// 終了後は、パイプが閉じるか、データが途切れて一定時間たつまで読む
			if ( ( out_eof && err_eof ) || now - last_data >= PCAP_DRAIN_IDLE_MS ) {
				break;
			}
		}
		// 中断と時間切れは、終了後に出力を読んでいる間も判定する（孫が出力し続けても止められるように）
		if ( c.cancel && *c.cancel ) {
			why = PCAP_CANCELED;
			break;
		}
		if ( c.timeout_ms && now - start >= c.timeout_ms ) {
			why = PCAP_TIMEOUT;
			break;
		}

		if ( ! got ) {
			// 読めたときは待たずに次を読む（待つとパイプの容量ずつしか進まない）
			if ( exited ) {
				::Sleep(10);
			}
			else if ( ::WaitForSingleObject(pi.hProcess, 15) != WAIT_TIMEOUT ) {
				exited = true;
				last_data = ::GetTickCount();
			}
		}
	}

	// 子が終わっていれば、中断・時間切れ・上限超過でも終了コードは返す
	if ( exited ) {
		DWORD status = 0;
		if ( ::GetExitCodeProcess(pi.hProcess, &status) ) {
			c.exitcode = static_cast<yaya::int_t>(status);
		}
	}
	if ( why != PCAP_OK ) {
		// 中断・時間切れ・上限超過では子プロセスを終了させる（孫プロセスは終了させない）
		::TerminateProcess(pi.hProcess, 1);
		::WaitForSingleObject(pi.hProcess, 1000);
	}

	if ( CutOver(c.out, c.maxsize) || CutOver(c.err, c.maxsize) ) {
		why = ( why == PCAP_OK ) ? PCAP_TRUNCATED : why;
	}
	c.status = why;

	// 書き込みスレッドは、子が終わればパイプが壊れて抜ける。待っても終わらなければ手放す（スレッドが自分で片付ける）
	if ( hWriter ) {
		::WaitForSingleObject(hWriter, PCAP_WRITER_JOIN_MS);
		::CloseHandle(hWriter);
	}
	::CloseHandle(pi.hProcess);
	CloseH(outR);
	CloseH(errR);
}

#elif defined(POSIX)

/* -----------------------------------------------------------------------
 *  CProcSpawnLock（POSIX）
 * -----------------------------------------------------------------------
 */
static pthread_mutex_t g_spawn_mutex = PTHREAD_MUTEX_INITIALIZER;

CProcSpawnLock::CProcSpawnLock(void)
{
	pthread_mutex_lock(&g_spawn_mutex);
}

CProcSpawnLock::~CProcSpawnLock(void)
{
	pthread_mutex_unlock(&g_spawn_mutex);
}

static unsigned long MonotonicMsPosix(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return static_cast<unsigned long>(ts.tv_sec) * 1000UL + static_cast<unsigned long>(ts.tv_nsec / 1000000);
}

static void CloseFd(int &fd)
{
	if ( fd >= 0 ) {
		close(fd);
		fd = -1;
	}
}

/* -----------------------------------------------------------------------
 *  fd を 3 以上に移し、exec で閉じるようにする
 *  ホストが標準入出力を閉じていると pipe が 0～2 を返し、子の dup2 で壊れるため
 * -----------------------------------------------------------------------
 */
static bool MoveHighCloexec(int &fd)
{
	if ( fd < 0 ) {
		return false;
	}
	if ( fd < 3 ) {
		int nfd = fcntl(fd, F_DUPFD, 3);
		close(fd);
		if ( nfd < 0 ) {
			fd = -1;
			return false;
		}
		fd = nfd;
	}
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	return true;
}

static bool MakePipe(int fds[2])
{
	fds[0] = fds[1] = -1;
	if ( pipe(fds) != 0 ) {
		fds[0] = fds[1] = -1;
		return false;
	}
	if ( ! MoveHighCloexec(fds[0]) || ! MoveHighCloexec(fds[1]) ) {
		CloseFd(fds[0]);
		CloseFd(fds[1]);
		return false;
	}
	return true;
}

static void SetNonblock(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

/* -----------------------------------------------------------------------
 *  fd から読めるだけ s に足す（limit まで）
 *  返値：true=閉じられた（もう読むものがない）
 * -----------------------------------------------------------------------
 */
static bool ReadFdPosix(int fd, std::string &s, size_t limit, size_t &got)
{
	char buf[16384];
	while ( s.size() < limit ) {
		size_t want = sizeof(buf);
		if ( want > limit - s.size() ) {
			want = limit - s.size();
		}
		ssize_t n = read(fd, buf, want);
		if ( n > 0 ) {
			AppendBytes(s, buf, static_cast<size_t>(n));
			got += static_cast<size_t>(n);
			continue;
		}
		if ( n == 0 ) {
			return true;
		}
		if ( errno == EINTR ) {
			continue;
		}
		return errno != EAGAIN && errno != EWOULDBLOCK;
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  中間の子（fork 直後。async-signal-safe な関数だけを呼ぶ）
 *
 *  孫（コマンド）を起動し、標準入力のデータを書き、孫の終了を待って、pid と終了コードを pst に書く
 * -----------------------------------------------------------------------
 */
static void IntermediateChild(const char *s_exe, char *const argv[], const char *s_dir, const char *inbuf, size_t inlen, bool has_input,
	int stderr_mode, int devnull, int pin[2], int pout[2], int perr[2], int pst[2])
{
	close(pout[0]);
	if ( perr[0] >= 0 ) {
		close(perr[0]);
	}
	close(pst[0]);

	// 孫の終了コードを受け取れるように、SIGCHLD を既定に戻す
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);

	pid_t g = fork();
	if ( g < 0 ) {
		_exit(1);
	}

	if ( g == 0 ) {
		// 孫。自分のプロセスグループに入り、標準入出力をつないで実行する
		setpgid(0, 0);
		// ホスト（や CI のランナー）が SIGPIPE を無視していても、コマンドには引き継がない
		// （無視したままだと `yes | head` の yes が静かに終わらず、標準エラー出力にエラーを書く）。
		// sa はここまで SIG_DFL のまま
		sigaction(SIGPIPE, &sa, NULL);
		int in_fd  = has_input ? pin[0] : devnull;
		int err_fd = ( stderr_mode == PCAP_STDERR_SEPARATE ) ? perr[1]
			: ( stderr_mode == PCAP_STDERR_MERGE ) ? pout[1] : devnull;
		dup2(in_fd, 0);
		dup2(pout[1], 1);
		dup2(err_fd, 2);
		// ほかの fd は FD_CLOEXEC なので exec で閉じる
		if ( s_dir && chdir(s_dir) != 0 ) {
			_exit(127);
		}
		execv(s_exe, argv);
		_exit(127);
	}

	setpgid(g, g);

	// 孫の標準入出力は孫だけが持つ
	close(pout[1]);
	if ( perr[1] >= 0 ) {
		close(perr[1]);
	}
	if ( has_input ) {
		close(pin[0]);
	}
	close(devnull);

	// ここから先は SIGPIPE を無視する（孫には引き継がない）
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	int gpid = static_cast<int>(g);
	ssize_t w;
	do {
		w = write(pst[1], &gpid, sizeof(gpid));
	} while ( w < 0 && errno == EINTR );

	int st = 0;
	bool reaped = false;

	if ( has_input ) {
		// 孫が読まなくても止まらないよう、書けるだけ書いては孫の終了を確かめる
		fcntl(pin[1], F_SETFL, fcntl(pin[1], F_GETFL) | O_NONBLOCK);
		size_t pos = 0;
		while ( pos < inlen ) {
			ssize_t n = write(pin[1], inbuf + pos, inlen - pos);
			if ( n > 0 ) {
				pos += static_cast<size_t>(n);
				continue;
			}
			if ( n < 0 && errno == EINTR ) {
				continue;
			}
			if ( n < 0 && errno != EAGAIN && errno != EWOULDBLOCK ) {
				break;
			}
			pid_t r = waitpid(g, &st, WNOHANG);
			if ( r == g ) {
				reaped = true;
				break;
			}
			if ( r < 0 && errno != EINTR ) {
				break;
			}
			struct pollfd pfd;
			pfd.fd = pin[1];
			pfd.events = POLLOUT;
			pfd.revents = 0;
			poll(&pfd, 1, 20);
		}
		close(pin[1]);
	}

	int code = -1;
	if ( ! reaped ) {
		pid_t r;
		do {
			r = waitpid(g, &st, 0);
		} while ( r < 0 && errno == EINTR );
		reaped = ( r == g );
	}
	if ( reaped && WIFEXITED(st) ) {
		code = WEXITSTATUS(st);
	}
	do {
		w = write(pst[1], &code, sizeof(code));
	} while ( w < 0 && errno == EINTR );
	_exit(0);
}

static void KillGroupPosix(pid_t m, int gpid)
{
	if ( gpid > 0 ) {
		kill(-static_cast<pid_t>(gpid), SIGKILL);
		kill(static_cast<pid_t>(gpid), SIGKILL);
	}
	kill(m, SIGKILL);
}

static void RunPosix(CProcCapture &c)
{
	c.status = PCAP_FAILED;
	c.exitcode = -1;

	const char *s_dir = NULL;
	if ( c.dir.size() ) {
		struct stat sb;
		if ( stat(c.dir.c_str(), &sb) != 0 || ! S_ISDIR(sb.st_mode) ) {
			return;
		}
		s_dir = c.dir.c_str();
	}

	// 実行するもの。argv が空なら /bin/sh -c cmd、あるなら シェルを通さず argv[0] を直接実行する。
	// fork の後は async-signal-safe な関数しか呼べないので、引数の配列は fork の前に作る
	const char *s_exe = "/bin/sh";
	std::vector<char*> argv;
	if ( c.argv.empty() ) {
		argv.push_back(const_cast<char*>("sh"));
		argv.push_back(const_cast<char*>("-c"));
		argv.push_back(const_cast<char*>(c.cmd.c_str()));
	}
	else {
		for ( size_t i = 0; i < c.argv.size(); ++i ) {
			argv.push_back(const_cast<char*>(c.argv[i].c_str()));
		}
		s_exe = c.argv[0].c_str();
	}
	argv.push_back(NULL);

	const size_t limit = c.maxsize + 1;		// 上限を超えたかを見分けるため、1バイト多く読む
	const char *inbuf = c.input.data();
	const size_t inlen = c.input.size();

	int pin[2] = { -1, -1 };
	int pout[2] = { -1, -1 };
	int perr[2] = { -1, -1 };
	int pst[2] = { -1, -1 };
	int devnull = -1;
	pid_t m = -1;

	{
		// パイプを作ってから、子の側の fd を閉じるまで、他の起動を入れない
		CProcSpawnLock lock;

		devnull = open("/dev/null", O_RDWR);
		bool ok = devnull >= 0 && MoveHighCloexec(devnull);
		ok = ok && MakePipe(pout) && MakePipe(pst);
		ok = ok && ( ! c.has_input || MakePipe(pin) );
		ok = ok && ( c.stderr_mode != PCAP_STDERR_SEPARATE || MakePipe(perr) );

		if ( ok ) {
			m = fork();
			if ( m == 0 ) {
				IntermediateChild(s_exe, &argv[0], s_dir, inbuf, inlen, c.has_input, c.stderr_mode, devnull, pin, pout, perr, pst);
				_exit(1);
			}
		}

		// 親は子の側の fd と、標準入力の書き込み側（中間の子が書く）を持たない
		CloseFd(pout[1]);
		CloseFd(perr[1]);
		CloseFd(pst[1]);
		CloseFd(pin[0]);
		CloseFd(pin[1]);
		CloseFd(devnull);
	}

	if ( m < 0 ) {
		CloseFd(pout[0]);
		CloseFd(perr[0]);
		CloseFd(pst[0]);
		return;
	}

	SetNonblock(pout[0]);
	SetNonblock(pst[0]);
	if ( perr[0] >= 0 ) {
		SetNonblock(perr[0]);
	}

	int why = PCAP_OK;
	bool out_eof = false;
	bool err_eof = ( perr[0] < 0 );
	bool st_eof = false;
	char sbuf[sizeof(int) * 2];
	size_t sgot = 0;
	int gpid = -1;
	bool have_code = false;
	int code = -1;
	const unsigned long start = MonotonicMsPosix();
	unsigned long last_data = start;

	while ( true ) {
		struct pollfd pf[3];
		int n = 0;
		if ( ! out_eof ) {
			pf[n].fd = pout[0]; pf[n].events = POLLIN; pf[n].revents = 0; ++n;
		}
		if ( ! err_eof ) {
			pf[n].fd = perr[0]; pf[n].events = POLLIN; pf[n].revents = 0; ++n;
		}
		if ( ! st_eof ) {
			pf[n].fd = pst[0]; pf[n].events = POLLIN; pf[n].revents = 0; ++n;
		}
		if ( n ) {
			poll(pf, n, 20);
		}

		size_t got = 0;
		if ( ! out_eof ) {
			out_eof = ReadFdPosix(pout[0], c.out, limit, got);
		}
		if ( ! err_eof ) {
			err_eof = ReadFdPosix(perr[0], c.err, limit, got);
		}
		if ( ! st_eof && ! have_code ) {
			while ( sgot < sizeof(sbuf) ) {
				ssize_t r = read(pst[0], sbuf + sgot, sizeof(sbuf) - sgot);
				if ( r > 0 ) {
					sgot += static_cast<size_t>(r);
					continue;
				}
				if ( r == 0 ) {
					st_eof = true;
				}
				else if ( errno == EINTR ) {
					continue;
				}
				break;
			}
			if ( sgot >= sizeof(int) && gpid < 0 ) {
				memcpy(&gpid, sbuf, sizeof(int));
			}
			if ( sgot == sizeof(sbuf) ) {
				memcpy(&code, sbuf + sizeof(int), sizeof(int));
				have_code = true;
			}
		}

		const unsigned long now = MonotonicMsPosix();
		if ( got || ! have_code ) {
			last_data = now;
		}

		if ( c.out.size() > c.maxsize || c.err.size() > c.maxsize ) {
			why = PCAP_TRUNCATED;
			break;
		}

		if ( have_code ) {
			// 終了後は、パイプが閉じるか、データが途切れて一定時間たつまで読む
			if ( ( out_eof && err_eof ) || now - last_data >= PCAP_DRAIN_IDLE_MS ) {
				break;
			}
		}
		else if ( st_eof ) {
			// 中間の子が終了コードを返さずに死んだ
			why = PCAP_FAILED;
			break;
		}
		// 中断と時間切れは、終了後に出力を読んでいる間も判定する（孫が出力し続けても止められるように）
		if ( c.cancel && *c.cancel ) {
			why = PCAP_CANCELED;
			break;
		}
		if ( c.timeout_ms && now - start >= c.timeout_ms ) {
			why = PCAP_TIMEOUT;
			break;
		}
	}

	// 孫が終わっていれば、中断・時間切れ・上限超過でも終了コードは返す
	if ( have_code ) {
		c.exitcode = static_cast<yaya::int_t>(code);
	}
	if ( why != PCAP_OK ) {
		// 中断・時間切れ・上限超過・異常は、プロセスグループごと終了させる。
		// 孫が終わった後も、パイプを握っている子孫がグループに残っていればグループの番号は再利用されない
		KillGroupPosix(m, gpid);
	}

	// 中間の子を回収する（ホストが SIGCHLD を無視していると ECHILD になる）
	int status = 0;
	pid_t r;
	do {
		r = waitpid(m, &status, 0);
	} while ( r < 0 && errno == EINTR );

	if ( CutOver(c.out, c.maxsize) || CutOver(c.err, c.maxsize) ) {
		why = ( why == PCAP_OK ) ? PCAP_TRUNCATED : why;
	}
	c.status = why;

	CloseFd(pout[0]);
	CloseFd(perr[0]);
	CloseFd(pst[0]);
}

#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  ProcCaptureRun
 * -----------------------------------------------------------------------
 */
void ProcCaptureRun(CProcCapture &c)
{
	c.out.erase();
	c.err.erase();
	c.exitcode = -1;
	c.status = PCAP_FAILED;

#if defined(WIN32)
	RunWin32(c);
#elif defined(POSIX)
	RunPosix(c);
#endif
}
