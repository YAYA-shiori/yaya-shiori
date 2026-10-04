// 
// AYA version 5
//
// 1つのDLLを扱うクラス　CLib1
// written by umeici. 2004
// 

#if defined(WIN32)
# include "stdafx.h"
#endif

#include <string>
#include <vector>
#include <map>

#if defined(POSIX)
# include <cstring>
# include <dlfcn.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <errno.h>
# include <time.h>
#endif
#include <string.h>

#include "lib.h"

#include "ayavm.h"

#include "aya5.h"
#include "ccct.h"
#include "log.h"
#include "manifest.h"
#include "misc.h"
#include "wsex.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

#if defined(POSIX)
static std::string str_getenv(const std::string& name) {
    char* var = getenv(name.c_str());
    if (var == NULL) {
        return std::string();
    }
    else {
        return std::string(var);
    }
}
// dllサーチパス関連。
// POSIX上では、ある特定の場所にDLLと同名のライブラリを置く事でSAORIに対応する。
// DLLと同名とは云っても、それはシンボリックリンクであるべきで、例えば次のようにである。
// % pwd
// /home/foo/.saori
// % ls -l
// -rwxr-xr-x x foo bar xxxxx 1 1 00:00 libssu.so
// lrwxr-xr-x x foo bar xxxxx 1 1 00:00 ssu.dll -> libssu.so
//
// パスは環境変数 SAORI_FALLBACK_PATH から取得する。これはコロン区切りの絶対パスである。
static std::vector<std::string> posix_dll_search_path;
static bool posix_dll_search_path_is_ready = false;
static std::string posix_search_fallback_dll(const std::string& dllfile) {
    // dllfileは探したいファイルDLL名。パス区切り文字は/。
    // 代替ライブラリが見付かればその絶対パスを、
    // 見付けられなければ空文字列を返す。
    
    if (!posix_dll_search_path_is_ready) {
	// SAORI_FALLBACK_PATHを見る。
		std::string path = str_getenv("SAORI_FALLBACK_PATH");
        if (path.length() > 0) {
            while (true) {
				std::string::size_type colon_pos = path.find(':');
                if (colon_pos == std::string::npos) {
                    posix_dll_search_path.emplace_back(path);
                    break;
                }
                else {
                    posix_dll_search_path.emplace_back(path.substr(0, colon_pos));
                    path.erase(0, colon_pos+1);
                }
            }
        }
        posix_dll_search_path_is_ready = true;
    }

	std::string::size_type pos_slash = dllfile.rfind('/');
	std::string fname(
		dllfile.begin() + (pos_slash == std::string::npos ? 0 : pos_slash),
		dllfile.end());

    for (std::vector<std::string>::const_iterator ite = posix_dll_search_path.begin();
	 ite != posix_dll_search_path.end(); ite++ ) {
		std::string fpath = *ite + '/' + fname;
	struct stat sb;
	if (stat(fpath.c_str(), &sb) == 0) {
	    // 代替ライブラリが存在するようだ。これ以上のチェックは省略。
	    return fpath;
	}
    }
    return std::string();
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::LoadLib
 *  機能概要：  DLLをロードします
 *
 *  返値　　：　0/1=失敗/成功(既にロードされている含む)
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::LoadLib(void)
{
	if (hDLL != NULL)
		return 1;

	char	*dllpathname = Ccct::Ucs2ToMbcs(name, CHARSET_DEFAULT);
	if (dllpathname == NULL)
		return 0;

	module_t hDLLFromGet = ::GetModuleHandleA(dllpathname);
	isAlreadyLoaded = hDLLFromGet != NULL;
	if ( hDLLFromGet ) {
		hDLL = hDLLFromGet;
	}
	else {
		hDLL = ::LoadLibraryA(dllpathname);
	}
	free(dllpathname);
	dllpathname= NULL;
	
	return (hDLL != NULL) ? 1 : 0;
}
#elif defined(POSIX)
int CLib1::LoadLib() {
    if (hDLL != NULL) {
	return 1;
    }

	std::string libfile = narrow(name);
    fix_filepath(libfile);

    // 環境変数 SAORI_FALLBACK_ALWAYS が定義されていて、且つ
    // 空でも"0"でもなければ、このdllファイルを開いてみる事は
    // 初めからやらない。そうでなければ、試しにdlopenしてみる。
    char* env_fallback_always = getenv("SAORI_FALLBACK_ALWAYS");
    bool fallback_always = false;
    if (env_fallback_always != NULL) {
		std::string str_fallback_always(env_fallback_always);
	if (str_fallback_always.length() > 0 &&
	    str_fallback_always != "0") {
	    fallback_always = true;
	}
    }
    bool do_fallback = true;
    if (!fallback_always) {
	void* handle = dlopen(libfile.c_str(), RTLD_LAZY);
	if (handle != NULL) {
	    // load, unload, requestを取出してみる。
	    void* sym_load = dlsym(handle, "load");
	    void* sym_unload = dlsym(handle, "unload");
	    void* sym_request = dlsym(handle, "request");
	    if (sym_load != NULL && sym_unload != NULL && sym_request != NULL) {
		// なんと正常に読めた。
		do_fallback = false;
	    }
	    dlclose(handle);
	}
    }
    if (do_fallback) {
	// 代替ライブラリを探す。
		std::string fallback_lib = posix_search_fallback_dll(get_fname(libfile));
	if (fallback_lib.length() == 0) {
	    // 無い。
	    char* cstr_path = getenv("SAORI_FALLBACK_PATH");
		std::string fallback_path =
		(cstr_path == NULL ?
		 "(environment variable `SAORI_FALLBACK_PATH' is empty)" : cstr_path);
		std::string message =
		libfile+": This is not usable in this platform.\n"+
		"Fallback library doesn't exist: "+fallback_path+"\n";
	    vm.logger().Write(widen(message));
	    return 0;
	}
	else {
		std::string message =
		"SAORI: using "+fallback_lib+" instead of "+libfile+"\n";
	    vm.logger().Write(widen(message));
	    
	    libfile = fallback_lib;
	}
    }

    hDLL = dlopen(libfile.c_str(), RTLD_LAZY);
    if (hDLL != NULL) {
        char *p = strdup(libfile.c_str());
        filename = basename(p);
        free(p);
        auto pos = filename.rfind(".dll");
        if (pos != decltype(filename)::npos) {
            filename = filename.substr(0, pos);
        }
        return 1;
    }
    else {
        return 0;
    }
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::Load
 *  機能概要：  loadを実行します
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Load(void)
{
	if (isBasic)
		return LoadBasic();

	if (!LoadLib())
		return 0;

	// アドレス取得
	if ( ! isAlreadyLoaded ) {
		bool (*loadlib)(yaya::global_t h, long len) = NULL;
		int charset = CHARSET_UTF8;

		if (loadlib == NULL) {
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "loadu");
		}
		if (loadlib == NULL) {
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "_loadu");
		}

		if (loadlib == NULL) {
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "load");
			charset = CHARSET_DEFAULT;
		}
		if (loadlib == NULL) {
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "_load");
			charset = CHARSET_DEFAULT;
		}
		if (loadlib == NULL) {
			return 0;
		}

		// DLLパス文字列作成
		yaya::string_t	drive, dir, fname, ext;
		SplitPathParts(name, drive, dir, fname, ext);
		yaya::string_t	dllpath = drive;
		dllpath += dir;

		// パス文字列をMBCSに変換
		char	*t_dllpath = Ccct::Ucs2ToMbcs(dllpath, charset);
		if (t_dllpath == NULL)
			return 0;

		long	len = (long)strlen(t_dllpath);

		// パス文字列をヒープにコピー
		HGLOBAL	gmem = ::GlobalAlloc(GMEM_FIXED, len);
		if (!gmem) {
			free(t_dllpath);
			t_dllpath = NULL;
			return 0;
		}

		memcpy(gmem, t_dllpath, len);
		free(t_dllpath);
		t_dllpath = NULL;

		// 実行
		(*loadlib)(gmem, len);
	}
	
	return 1;
}
#elif defined(POSIX)
int CLib1::Load(void) {
	if (isBasic) {
		return LoadBasic();
	}

    if (!LoadLib()) {
		return 0;
    }
    
    // アドレス取得
	long (*loadlib)(char* h, long len) = NULL;

	if (loadlib == NULL) {
        std::string func_name = filename + "_saori_load";
        loadlib = (long(*)(char*,long))dlsym(hDLL, func_name.c_str());
    }
    if (loadlib == NULL) {
	 return 0;
    }
    
    // DLLパス文字列作成
	yaya::string_t::size_type pos_slash = name.rfind(L'/');
	std::string dllpath;
    if (pos_slash == yaya::string_t::npos) {
		dllpath = ".";
    }
    else {
		dllpath = narrow(name.substr(0, pos_slash+1));
    }

    long len = dllpath.length();

    // パス文字列をヒープにコピー
    char* gmem = static_cast<char*>(malloc(len));
    memcpy(gmem, dllpath.c_str(), len);

    // 実行
    id = (*loadlib)(gmem, len);
    
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::Unload
 *  機能概要：  unloadを実行します
 *
 *  返値　　：　0/1/2=失敗/成功/ロードされていない、もしくは既にunloadされている
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Unload(void)
{
	if (isBasic)
		return 1;

	if (hDLL == NULL)
		return 2;

	// アドレス取得
	if ( ! isAlreadyLoaded ) {
		bool (*unloadlib)(void) = NULL;

		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "unload");
		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "_unload");
		if (unloadlib == NULL)
			return 0;

		// 実行
		(*unloadlib)();
	}
	UnloadLib();

	return 1;
}
#elif defined(POSIX)
int CLib1::Unload(void) {
	if (isBasic) {
		return 1;
	}

    if (hDLL == NULL) {
	return 2;
    }

    // アドレス取得
	int (*unloadlib)(long) = NULL;

	if (unloadlib == NULL) {
        std::string func_name = filename + "_saori_unload";
        unloadlib = (int(*)(long))dlsym(hDLL, func_name.c_str());
    }
    if (unloadlib == NULL) {
	 return 0;
    }

    // 実行
    (*unloadlib)(id);
	UnloadLib();
    
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::Release
 *  機能概要：  DLLをリリースします
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CLib1::UnloadLib(void)
{
	if (hDLL == NULL)
		return;

	requestlib = NULL;

	if ( ! isAlreadyLoaded ) {
		FreeLibrary(hDLL);
	}
	hDLL = NULL;
}
#elif defined(POSIX)
void CLib1::UnloadLib(void) {
    if (hDLL == NULL) {
	return;
    }

    dlclose(hDLL);
    hDLL = NULL;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::Request
 *  機能概要：  requestを実行します
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Request(const yaya::string_t &istr, yaya::string_t &ostr)
{
	if (isBasic)
		return RequestBasic(istr, ostr);

	ostr.erase();

	if (hDLL == NULL)
		return 0;

	// アドレス取得
	if (requestlib == NULL)
		requestlib = (HGLOBAL (*)(HGLOBAL h, long *len))GetProcAddress(hDLL, "request");
	if (requestlib == NULL)
		requestlib = (HGLOBAL (*)(HGLOBAL h, long *len))GetProcAddress(hDLL, "_request");
	if (requestlib == NULL)
		return 0;

	// 文字列をマルチバイト文字コードに変換
	char	*t_istr = Ccct::Ucs2ToMbcs(istr, charset);
	if (t_istr == NULL)
		return 0;

	long	len = (long)strlen(t_istr);

	// request文字列をヒープにコピー
	HGLOBAL	igmem = ::GlobalAlloc(GMEM_FIXED, len);
	if (!igmem) {
		free(t_istr);
		t_istr = NULL;
		return 0;
	}
	memcpy(igmem, t_istr, len);
	free(t_istr);
	t_istr = NULL;

	// 実行
	HGLOBAL	ogmem = (*requestlib)(igmem, &len);

	// 結果取得（DLLが応答を返さなかった場合は、従来どおり空の応答として扱う）
	if (ogmem == NULL || len < 0) {
		if (ogmem) {
			GlobalFree(ogmem);
		}
		ostr.erase();
		return 1;
	}
	char	*t_ostr = (char *)malloc((len + 1)*sizeof(char));
	if (t_ostr == NULL) {
		GlobalFree(ogmem);
		ogmem = NULL;
		return 0;
	}
	strncpy(t_ostr, (char *)ogmem, len);
	*(t_ostr + len) = '\0';
	GlobalFree(ogmem);
	ogmem = NULL;

	// 結果をUCS-2へ変換
	wchar_t	*t_ostr2 = Ccct::MbcsToUcs2(t_ostr, charset);
	free(t_ostr);
	t_ostr = NULL;
	if (t_ostr2 == NULL)
		return 0;
	ostr = t_ostr2;
	free(t_ostr2);
	t_ostr2 = NULL;

	return 1;
}
#elif defined(POSIX)
int CLib1::Request(const yaya::string_t &istr, yaya::string_t &ostr) {
	if (isBasic) {
		return RequestBasic(istr, ostr);
	}

	ostr.clear();

    if (hDLL == NULL) {
	return 0;
    }
    
    // アドレス取得
	if (requestlib == NULL) {
        std::string func_name = filename + "_saori_request";
        requestlib = (char*(*)(long, char*, long *))dlsym(hDLL, func_name.c_str());
    }
    if (requestlib == NULL) {
	return 0;
    }
    
    // 文字列をマルチバイト文字コードに変換
    char *t_istr = Ccct::Ucs2ToMbcs(istr, charset);
    if (t_istr == NULL) {
	return 0;
    }

    long len = (long)strlen(t_istr);

    // パス文字列をヒープにコピー
    char* igmem = static_cast<char*>(malloc(len));
    memcpy(igmem, t_istr, len);
    free(t_istr);
	t_istr = NULL;

    // 実行
    char* ogmem = (*requestlib)(id, igmem, &len);

    // 結果取得
	std::string t_ostr(ogmem, len);
	free(ogmem);

    // 結果をUCS-2へ変換
    wchar_t *t_ostr2 = Ccct::MbcsToUcs2(t_ostr, charset);
    if (t_ostr2 == NULL) {
	return 0;
    }
    ostr = t_ostr2;
    free(t_ostr2);
	t_ostr2 = NULL;

    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  SAORI-basic（実行ファイル）
 *
 *  DLL 以外のファイルを LOADLIB したら SAORI-basic として扱い、proxy.dll などを介さずに
 *  本体で実行する。REQUESTLIB に渡された SAORI/1.0 の要求の Argument0, Argument1, ... を
 *  1つずつの引数にして実行ファイルを起動し、標準出力を応答に変換して返す。
 *  GET Version には本体が応答する。
 *
 *  - 作業ディレクトリは実行ファイルのあるディレクトリ
 *  - 引数と標準出力の文字コードは CHARSETLIB / CHARSETLIBEX の設定に従う
 *    （Windows の引数は Unicode のまま CreateProcessW に渡す）
 *  - BASIC_TIMEOUT_MS で終わらないか、出力が BASIC_OUTPUT_LIMIT に達したら
 *    子プロセスを強制終了して 500 を返す
 *  - 標準出力は末尾の改行を削り、Result には行を文字列の \r\n でつないだもの、
 *    Value0, Value1, ... には1行ずつを入れる（応答ヘッダの値に改行は入れられないため）
 * -----------------------------------------------------------------------
 */
static const unsigned long BASIC_TIMEOUT_MS = 10000;
static const size_t BASIC_OUTPUT_LIMIT = 16 * 1024 * 1024;

/* -----------------------------------------------------------------------
 *  読んだ分を out に足す
 *  VC6 の std::string は足りない分しか確保し直さないため、容量は倍々で確保する
 * -----------------------------------------------------------------------
 */
static void	AppendOutput(std::string &out, const char *buf, size_t len)
{
	if ( out.capacity() < out.size() + len ) {
		out.reserve((out.size() + len) * 2);
	}
	out.append(buf, len);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::IsBasicName
 *  機能概要：  ファイル名が SAORI-basic（実行ファイル）を指すかを拡張子で判定します
 *
 *  Windows は .dll と拡張子なし（LoadLibrary が .dll を補う）以外、
 *  POSIX は .dll / .so / .dylib / .bundle 以外を SAORI-basic とします
 * -----------------------------------------------------------------------
 */
bool	CLib1::IsBasicName(const yaya::string_t &n)
{
	yaya::string_t	drive, dir, fname, ext;
	SplitPathParts(n, drive, dir, fname, ext);

	for ( size_t i = 0; i < ext.size(); ++i ) {
		if ( ext[i] >= L'A' && ext[i] <= L'Z' ) {
			ext[i] = static_cast<yaya::char_t>(ext[i] - L'A' + L'a');
		}
	}

	if ( ext == L".dll" ) {
		return false;
	}
#if defined(WIN32)
	if ( ext.empty() ) {
		return false;
	}
#elif defined(POSIX)
	if ( ext == L".so" || ext == L".dylib" || ext == L".bundle" ) {
		return false;
	}
#endif
	return true;
}

#if defined(WIN32)

#ifndef CREATE_NO_WINDOW
#define CREATE_NO_WINDOW 0x08000000
#endif

/* -----------------------------------------------------------------------
 *  CRT（CommandLineToArgvW と同じ規則）で1つの引数として読まれるように、引数をクォートして足す
 * -----------------------------------------------------------------------
 */
static void	AppendQuotedArgWin32(yaya::string_t &cmd, const yaya::string_t &arg)
{
	if ( arg.size() && arg.find_first_of(L" \t\n\v\"") == yaya::string_t::npos ) {
		cmd += arg;
		return;
	}

	cmd += L'"';
	size_t	backslashes = 0;
	for ( size_t i = 0; i < arg.size(); ++i ) {
		if ( arg[i] == L'\\' ) {
			++backslashes;
			continue;
		}
		// " の前の \ は倍にして、" 自体も \ でエスケープする
		cmd.append(arg[i] == L'"' ? backslashes * 2 + 1 : backslashes, L'\\');
		backslashes = 0;
		cmd += arg[i];
	}
	// 閉じる " の前の \ も倍にする
	cmd.append(backslashes * 2, L'\\');
	cmd += L'"';
}

/* -----------------------------------------------------------------------
 *  パイプに溜まっている分を out に足す（BASIC_OUTPUT_LIMIT まで）
 * -----------------------------------------------------------------------
 */
static void	ReadPipeWin32(HANDLE hRead, std::string &out)
{
	char	buf[16384];
	while ( out.size() < BASIC_OUTPUT_LIMIT ) {
		DWORD	avail = 0;
		if ( ! ::PeekNamedPipe(hRead, NULL, 0, NULL, &avail, NULL) || avail == 0 ) {
			break;
		}
		DWORD	read = 0;
		if ( ! ::ReadFile(hRead, buf, sizeof(buf), &read, NULL) || read == 0 ) {
			break;
		}
		AppendOutput(out, buf, read);
	}
}

/* -----------------------------------------------------------------------
 *  実行ファイルを起動し、終わるまで標準出力を読む
 *
 *  返値　　：　空文字列=成功、それ以外=失敗の理由
 * -----------------------------------------------------------------------
 */
static yaya::string_t	RunBasic(const yaya::string_t &path, const std::vector<yaya::string_t> &args, int /*charset*/, std::string &out)
{
	yaya::string_t	cmdline = L"\"";
	cmdline += path;
	cmdline += L"\"";
	for ( size_t i = 0; i < args.size(); ++i ) {
		cmdline += L' ';
		AppendQuotedArgWin32(cmdline, args[i]);
	}

	yaya::string_t	drive, dir, fname, ext;
	SplitPathParts(path, drive, dir, fname, ext);
	yaya::string_t	workdir = drive + dir;

	SECURITY_ATTRIBUTES	sa;
	memset(&sa, 0, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	HANDLE	hRead = NULL;
	HANDLE	hWrite = NULL;
	if ( ! ::CreatePipe(&hRead, &hWrite, &sa, 65536) ) {
		return L"CreatePipe failed.";
	}
	// 読む側は子プロセスに継承させない
	::SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

	// 標準入力と標準エラー出力は NUL につなぐ
	HANDLE	hNul = ::CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa, OPEN_EXISTING, 0, NULL);

	STARTUPINFOW	si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdInput  = ( hNul != INVALID_HANDLE_VALUE ) ? hNul : NULL;
	si.hStdOutput = hWrite;
	si.hStdError  = ( hNul != INVALID_HANDLE_VALUE ) ? hNul : NULL;

	PROCESS_INFORMATION	pi;
	memset(&pi, 0, sizeof(pi));

	// CreateProcessW はコマンドラインに書き換え可能なバッファを要求する
	std::vector<wchar_t>	cmdbuf(cmdline.begin(), cmdline.end());
	cmdbuf.push_back(L'\0');

	BOOL	created = ::CreateProcessW(path.c_str(), &cmdbuf[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL,
		workdir.size() ? workdir.c_str() : NULL, &si, &pi);

	// 書く側は子プロセスだけが持つようにする
	::CloseHandle(hWrite);
	if ( hNul != INVALID_HANDLE_VALUE ) {
		::CloseHandle(hNul);
	}

	if ( ! created ) {
		::CloseHandle(hRead);
		return L"Failed to start the process.";
	}
	::CloseHandle(pi.hThread);

	// 終了を待つ間も標準出力を読み続ける。読まないとパイプが一杯になって子プロセスが止まる。
	// 読めたときは待たずに次を読む（待つとパイプの容量ずつしか進まない）
	yaya::string_t	error;
	const DWORD	start = ::GetTickCount();
	size_t	last_size = out.size() + 1;
	while ( true ) {
		const DWORD	wait = ::WaitForSingleObject(pi.hProcess, ( out.size() != last_size ) ? 0 : 20);
		last_size = out.size();
		ReadPipeWin32(hRead, out);
		if ( wait != WAIT_TIMEOUT ) {
			break;
		}
		if ( out.size() >= BASIC_OUTPUT_LIMIT ) {
			error = L"Output exceeded the limit.";
			break;
		}
		if ( ::GetTickCount() - start >= BASIC_TIMEOUT_MS ) {
			error = L"Timed out.";
			break;
		}
	}

	if ( error.size() ) {
		::TerminateProcess(pi.hProcess, 1);
		::WaitForSingleObject(pi.hProcess, 1000);
	}
	::CloseHandle(pi.hProcess);
	::CloseHandle(hRead);

	return error;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::LoadBasic
 *  機能概要：  SAORI-basic の実行ファイルがあるかを確かめます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CLib1::LoadBasic(void)
{
	DWORD	attr = ::GetFileAttributesW(name.c_str());
	return ( attr != 0xFFFFFFFF && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0 ) ? 1 : 0;
}

#elif defined(POSIX)

/* -----------------------------------------------------------------------
 *  経過時間を測るためのミリ秒単位の時刻
 * -----------------------------------------------------------------------
 */
static unsigned long	MonotonicMsPosix(void)
{
	struct timespec	ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return static_cast<unsigned long>(ts.tv_sec) * 1000UL + static_cast<unsigned long>(ts.tv_nsec / 1000000);
}

/* -----------------------------------------------------------------------
 *  パイプに溜まっている分を out に足す（BASIC_OUTPUT_LIMIT まで）
 *
 *  返値　　：　true=パイプが閉じられた（もう読むものがない）
 * -----------------------------------------------------------------------
 */
static bool	ReadPipePosix(int fd, std::string &out)
{
	char	buf[4096];
	while ( out.size() < BASIC_OUTPUT_LIMIT ) {
		ssize_t	n = read(fd, buf, sizeof(buf));
		if ( n > 0 ) {
			AppendOutput(out, buf, static_cast<size_t>(n));
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
 *  実行ファイルを起動し、終わるまで標準出力を読む
 *
 *  シェルは通さず、引数は execv に1つずつ渡す。ホストがマルチスレッドの場合に備え、
 *  fork から exec までの間は async-signal-safe な関数しか呼ばない（文字列は fork の前に用意する）。
 *
 *  返値　　：　空文字列=成功、それ以外=失敗の理由
 * -----------------------------------------------------------------------
 */
static yaya::string_t	RunBasic(const yaya::string_t &path, const std::vector<yaya::string_t> &args, int charset, std::string &out)
{
	std::string	s_path = narrow(path);
	fix_filepath(s_path);

	std::string	s_dir;
	std::string::size_type	pos_slash = s_path.rfind('/');
	if ( pos_slash != std::string::npos ) {
		s_dir = s_path.substr(0, pos_slash + 1);
	}

	std::vector<std::string>	s_args;
	for ( size_t i = 0; i < args.size(); ++i ) {
		char	*t_arg = Ccct::Ucs2ToMbcs(args[i], charset);
		if ( t_arg == NULL ) {
			return L"Failed to convert the arguments.";
		}
		s_args.push_back(t_arg);
		free(t_arg);
	}

	std::vector<char*>	argv;
	argv.push_back(const_cast<char*>(s_path.c_str()));
	for ( size_t i = 0; i < s_args.size(); ++i ) {
		argv.push_back(const_cast<char*>(s_args[i].c_str()));
	}
	argv.push_back(NULL);

	int	fds[2];
	if ( pipe(fds) != 0 ) {
		return L"pipe failed.";
	}
	// 標準入力と標準エラー出力は /dev/null につなぐ
	int	devnull = open("/dev/null", O_RDWR);

	pid_t	pid = fork();
	if ( pid < 0 ) {
		close(fds[0]);
		close(fds[1]);
		if ( devnull >= 0 ) {
			close(devnull);
		}
		return L"fork failed.";
	}

	if ( pid == 0 ) {
		close(fds[0]);
		if ( fds[1] != STDOUT_FILENO ) {
			dup2(fds[1], STDOUT_FILENO);
			close(fds[1]);
		}
		if ( devnull >= 0 ) {
			dup2(devnull, STDIN_FILENO);
			dup2(devnull, STDERR_FILENO);
			if ( devnull > STDERR_FILENO ) {
				close(devnull);
			}
		}
		if ( s_dir.size() && chdir(s_dir.c_str()) != 0 ) {
			_exit(127);
		}
		execv(argv[0], &argv[0]);
		_exit(127);
	}

	close(fds[1]);
	if ( devnull >= 0 ) {
		close(devnull);
	}
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);

	// 終了を待つ間も標準出力を読み続ける。読まないとパイプが一杯になって子プロセスが止まる
	yaya::string_t	error;
	bool	exited = false;
	bool	eof = false;
	int		status = 0;
	const unsigned long	start = MonotonicMsPosix();
	while ( true ) {
		struct pollfd	pfd;
		pfd.fd = fds[0];
		pfd.events = POLLIN;
		pfd.revents = 0;
		// パイプが閉じた後は待つだけ（孫プロセスが残って閉じないこともある）
		poll(eof ? NULL : &pfd, eof ? 0 : 1, 50);
		if ( ! eof ) {
			eof = ReadPipePosix(fds[0], out);
		}

		pid_t	r = waitpid(pid, &status, WNOHANG);
		// ホストが SIGCHLD を無視していると子は自動で回収され ECHILD になる
		if ( r == pid || (r < 0 && errno != EINTR) ) {
			exited = true;
			if ( ! eof ) {
				ReadPipePosix(fds[0], out);
			}
			break;
		}
		if ( out.size() >= BASIC_OUTPUT_LIMIT ) {
			error = L"Output exceeded the limit.";
			break;
		}
		if ( MonotonicMsPosix() - start >= BASIC_TIMEOUT_MS ) {
			error = L"Timed out.";
			break;
		}
	}

	if ( ! exited ) {
		kill(pid, SIGKILL);
		pid_t	r;
		do {
			r = waitpid(pid, &status, 0);
		} while ( r < 0 && errno == EINTR );
	}
	close(fds[0]);

	return error;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::LoadBasic
 *  機能概要：  SAORI-basic の実行ファイルがあり、実行できるかを確かめます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CLib1::LoadBasic(void)
{
	std::string	s_path = narrow(name);
	fix_filepath(s_path);

	struct stat	sb;
	if ( stat(s_path.c_str(), &sb) != 0 || ! S_ISREG(sb.st_mode) ) {
		return 0;
	}
	return ( access(s_path.c_str(), X_OK) == 0 ) ? 1 : 0;
}

#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::RequestBasic
 *  機能概要：  SAORI-basic の実行ファイルに要求を渡し、応答を作ります
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CLib1::RequestBasic(const yaya::string_t &istr, yaya::string_t &ostr)
{
	ostr.erase();

	const yaya::string_t	charset_header = yaya::string_t(L"Charset: ") + Ccct::CharsetIDToTextW(charset) + L"\r\n";

	// 要求を行に分ける
	std::vector<yaya::string_t>	lines;
	yaya::string_t::size_type	pos = 0;
	while ( pos < istr.size() ) {
		yaya::string_t::size_type	eol = istr.find(L'\n', pos);
		if ( eol == yaya::string_t::npos ) {
			eol = istr.size();
		}
		yaya::string_t	line = istr.substr(pos, eol - pos);
		if ( line.size() && line[line.size() - 1] == L'\r' ) {
			line.erase(line.size() - 1);
		}
		lines.push_back(line);
		pos = eol + 1;
	}

	if ( lines.empty() ) {
		ostr = L"SAORI/1.0 400 Bad Request\r\n" + charset_header + L"\r\n";
		return 1;
	}

	if ( lines[0].compare(0, 11, L"GET Version") == 0 ) {
		ostr = L"SAORI/1.0 200 OK\r\n" + charset_header + L"\r\n";
		return 1;
	}

	if ( lines[0].compare(0, 7, L"EXECUTE") != 0 ) {
		ostr = L"SAORI/1.0 400 Bad Request\r\n" + charset_header + L"\r\n";
		return 1;
	}

	// ArgumentN を集める。SAORI-basic は外部からの呼び出しを見分けられないので、local 以外は断る
	std::map<int, yaya::string_t>	argmap;
	for ( size_t i = 1; i < lines.size(); ++i ) {
		yaya::string_t::size_type	colon = lines[i].find(L':');
		if ( colon == yaya::string_t::npos ) {
			continue;
		}
		yaya::string_t	key = lines[i].substr(0, colon);
		yaya::string_t::size_type	vpos = colon + 1;
		while ( vpos < lines[i].size() && lines[i][vpos] == L' ' ) {
			++vpos;
		}
		yaya::string_t	value = lines[i].substr(vpos);

		if ( key == L"SecurityLevel" ) {
			if ( value != L"local" && value != L"Local" ) {
				ostr = L"SAORI/1.0 400 Bad Request\r\n" + charset_header + L"\r\n";
				return 1;
			}
		}
		else if ( key.size() > 8 && key.compare(0, 8, L"Argument") == 0 ) {
			yaya::string_t	num = key.substr(8);
			if ( num.find_first_not_of(L"0123456789") == yaya::string_t::npos ) {
				argmap[yaya::ws_atoi(num)] = value;
			}
		}
	}

	std::vector<yaya::string_t>	args;
	for ( int n = 0; ; ++n ) {
		std::map<int, yaya::string_t>::const_iterator	it = argmap.find(n);
		if ( it == argmap.end() ) {
			break;
		}
		args.push_back(it->second);
	}

	// 実行
	std::string	out;
	yaya::string_t	error = RunBasic(name, args, charset, out);
	if ( error.size() ) {
		vm.logger().Write(L"SAORI-basic: " + name + L": " + error + L"\n");
		ostr = L"SAORI/1.0 500 Internal Server Error\r\n" + charset_header + L"\r\n";
		return 1;
	}

	// 標準出力を UCS-2 にして、改行を \n にそろえる
	yaya::string_t	text;
	if ( out.size() ) {
		wchar_t	*t_out = Ccct::MbcsToUcs2(out, charset);
		if ( t_out == NULL ) {
			vm.logger().Write(L"SAORI-basic: " + name + L": Failed to convert the output.\n");
			ostr = L"SAORI/1.0 500 Internal Server Error\r\n" + charset_header + L"\r\n";
			return 1;
		}
		text = t_out;
		free(t_out);
		t_out = NULL;
	}

	// テキストモードで \r\n を書いた \r\r\n も1つの改行にする
	yaya::string_t	normalized;
	normalized.reserve(text.size());
	size_t	linecount = 1;
	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( text[i] == L'\r' ) {
			while ( i + 1 < text.size() && text[i + 1] == L'\r' ) {
				++i;
			}
			if ( i + 1 < text.size() && text[i + 1] == L'\n' ) {
				continue;
			}
			normalized += L'\n';
			++linecount;
		}
		else {
			if ( text[i] == L'\n' ) {
				++linecount;
			}
			normalized += text[i];
		}
	}
	while ( normalized.size() && normalized[normalized.size() - 1] == L'\n' ) {
		normalized.erase(normalized.size() - 1);
	}

	if ( normalized.empty() ) {
		ostr = L"SAORI/1.0 204 No Content\r\n" + charset_header + L"\r\n";
		return 1;
	}

	// Result は行を文字列の \r\n でつなぎ、ValueN には1行ずつ入れる
	yaya::string_t	result;
	yaya::string_t	values;
	result.reserve(normalized.size() + linecount * 4);
	values.reserve(normalized.size() + linecount * 20);
	int		n = 0;
	pos = 0;
	while ( true ) {
		yaya::string_t::size_type	eol = normalized.find(L'\n', pos);
		yaya::string_t	line = normalized.substr(pos, eol == yaya::string_t::npos ? yaya::string_t::npos : eol - pos);

		if ( n ) {
			result += L"\\r\\n";
		}
		result += line;
		values += L"Value";
		values += yaya::ws_itoa(n);
		values += L": ";
		values += line;
		values += L"\r\n";
		++n;

		if ( eol == yaya::string_t::npos ) {
			break;
		}
		pos = eol + 1;
	}

	ostr = L"SAORI/1.0 200 OK\r\n" + charset_header + L"Result: " + result + L"\r\n" + values + L"\r\n";
	return 1;
}
