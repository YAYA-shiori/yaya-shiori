//
// AYA version 5
//
// 外部プログラムを起動し、標準入力を渡して、標準出力・標準エラー出力を取り込む（EXECUTE_CAPTURE）
//
// ・CValue など yaya のオブジェクトに触れない。呼び出し側が文字列を作って渡し、GIL を手放した
//   まま ProcCaptureRun を呼べる
// ・パイプを作ってから子プロセスを起動するまでの間は、プロセス全体で1つのロック（CProcSpawnLock）を持つ。
//   別のスレッドの起動が、こちらのパイプの書き込み側を継承して EOF が来なくなるのを防ぐ。
//   SAORI-basic の RunBasic も同じロックを使う
// ・SAORI-basic（lib1.cpp の RunBasic）もこの ProcCaptureRun で実行する（起動・読み取り・時間切れの処理は共通）。
//   シェルを通さず直接実行する指定（Windows は app、POSIX は argv）だけが EXECUTE_CAPTURE と異なる
//

#ifndef PROCCAPTURE_H
#define PROCCAPTURE_H

#include <string>
#include <vector>

#include "globaldef.h"

// 標準エラー出力の扱い
#define PCAP_STDERR_SEPARATE	0		// 標準出力とは別に取り込む
#define PCAP_STDERR_MERGE		1		// 標準出力と同じパイプに混ぜる
#define PCAP_STDERR_DISCARD		2		// 捨てる

// 結果の状態
#define PCAP_OK			0		// 子プロセスが終了した
#define PCAP_TIMEOUT	1		// 時間切れで終了させた
#define PCAP_CANCELED	2		// スレッドの中断で終了させた
#define PCAP_TRUNCATED	3		// 出力が上限を超えたので終了させた
#define PCAP_FAILED		4		// 起動できなかった（作業ディレクトリがない、実行できないなど）

// パイプを作る～子プロセスを起動する区間を、プロセス全体で直列にするロック
class CProcSpawnLock
{
public:
	CProcSpawnLock(void);
	~CProcSpawnLock(void);
private:
	CProcSpawnLock(const CProcSpawnLock&);
	CProcSpawnLock& operator=(const CProcSpawnLock&);
};

struct CProcCapture
{
	// ----- 入力 -----
#if defined(WIN32)
	yaya::string_t	cmdline;		// コマンドライン全体（実行ファイルは " で囲んでおく）
	yaya::string_t	app;			// 実行ファイルのパス（CreateProcessW の lpApplicationName）。空なら cmdline から決める
	yaya::string_t	dir;			// 作業ディレクトリ。空なら指定しない
#else
	std::string		cmd;			// /bin/sh -c に渡すコマンド（argv が空のとき）
	std::vector<std::string>	argv;	// 空でなければ、シェルを通さず argv[0] を execv で実行する（SAORI-basic）
	std::string		dir;			// 作業ディレクトリ（UTF-8 のパス）。空なら指定しない
#endif
	bool			has_input;		// false なら標準入力は空（すぐ EOF になる）
	std::string		input;			// 標準入力に渡すバイト列
	int				stderr_mode;	// PCAP_STDERR_*
	unsigned long	timeout_ms;		// 0 なら無制限
	size_t			maxsize;		// 標準出力・標準エラー出力それぞれの上限（バイト）
	volatile int	*cancel;		// 1 になったら中断する。NULL なら見ない

	// ----- 出力 -----
	int				status;			// PCAP_*
	yaya::int_t		exitcode;		// 終了コード。終了を確認できなければ -1
	std::string		out;			// 標準出力
	std::string		err;			// 標準エラー出力（PCAP_STDERR_SEPARATE のときだけ）

	CProcCapture(void)
		: has_input(false), stderr_mode(PCAP_STDERR_SEPARATE), timeout_ms(0), maxsize(0),
		  cancel(NULL), status(PCAP_FAILED), exitcode(-1) {}
};

// 起動して、終わるまで（または時間切れ・中断・上限超過まで）取り込む。ブロックする
void ProcCaptureRun(CProcCapture &c);

#endif //PROCCAPTURE_H
