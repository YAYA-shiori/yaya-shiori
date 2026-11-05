//
// AYA version 5
//
// ワーカースレッド（THREADSTART）とインタプリタ全体のロック（GIL）を扱うクラス
//
// ・VMごとに1つのGIL（実行権）がある。辞書の関数を実行できるのはGILを持つスレッドだけ
// ・SSPからの load / request / unload（ホスト側）は優先してGILを取る。ワーカーは文の境界で
//   ホストが待っていれば実行権を譲る。ホストの処理はワーカーに割り込まれない
// ・関数表（中間コード）は全スレッドで共有し、実行中は読み取り専用。式の評価中の一時値は呼び出しごとの
//   実行フレーム（CExecFrame）にあるので、同じ関数を複数のスレッドが同時に実行できる。
//   実行権を持つスレッドが切り替わるたびに、VMの「今の呼び出し制限」を差し替える
// ・shared_ptrの参照カウントはスレッドセーフではない。GILを手放している間は
//   CValueなどの共有されうるオブジェクトに触れないこと
// ・中断はキャンセルフラグを文の境界で調べ、exitcodeを使って関数を巻き戻す（強制停止は最後の手段）
//

#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <vector>

#include "globaldef.h"
#include "value.h"

class CAyaVM;
class CFunctionDef;
class CCallLimit;
class CThrMonitor;
struct CThrHandle;
class CThreadManager;
class CSysFuncState;

// ワーカーの状態
#define WORKER_RUNNING		0		// 実行中（実行待ちを含む）
#define WORKER_FINISHED		1		// 正常に終了した
#define WORKER_CANCELED		2		// 中断された
#define WORKER_FAILED		3		// エラーで終了した（関数がない、メモリ異常、強制停止など）

// THREADSTARTの失敗理由
#define WORKER_START_NOFUNC		(-1)	// 関数がない
#define WORKER_START_TOOMANY	(-2)	// 同時に動かせる数を超えた
#define WORKER_START_DENIED		(-3)	// 終了処理中
#define WORKER_START_FAILED		(-4)	// スレッドを作れなかった
#define WORKER_START_NOSUPPORT	(-5)	// このビルドはスレッドに対応していない

// THREADJOINの失敗理由
#define WORKER_JOIN_NOTFOUND	(-1)
#define WORKER_JOIN_SELF		(-2)	// 自分自身は待てない

class CWorkerContext
{
public:
	int					id;
	yaya::string_t		funcname;
	CValue				args;
	CValue				result;
	volatile int		state;			// WORKER_*　GIL（またはmon）を持って読み書きする
	volatile int		cancel;			// 1で中断要求
	bool				discard;		// 終了したら記録を捨てる（THREADCLOSEされた）
	bool				abandoned;		// 強制停止できずに見捨てた（POSIX）。戻ってきても何もせず眠り続ける
	std_shared_ptr<CCallLimit>		call_limit;
	std_shared_ptr<CSysFuncState>	sfstate;	// 実行権を手放している間のシステム関数の状態（GETLASTERRORなど）
	CThrHandle*			handle;
	CThreadManager*		mgr;

	CWorkerContext(void);
	~CWorkerContext(void);
};

// GILを手放す前の状態（BlockingReleaseで保存し、BlockingReacquireで戻す）
struct CGilSaved
{
	bool				valid;
	CWorkerContext*		ctx;			// NULLならホスト
	int					depth;
};

class CThreadManager
{
private:
	CAyaVM&				vm;
	CThrMonitor*		mon;			// GILの状態を守るロックと待ち合わせ

	bool				m_locked;		// GILが取られている
	size_t				m_owner;		// GILを持つOSスレッド
	int					m_depth;		// ホストの再入の深さ
	int					m_prio_waiters;	// GILを待っているホストの数
	bool				m_host_parked;	// ホストがTHREADJOINのために一時的に実行権を譲っている
	int					m_host_parked_depth;
	std::vector<CWorkerContext*>	m_workq;	// GILを待っているワーカー（先着順）
	CWorkerContext*		m_active;		// 今GILを持っているワーカー。NULLならホスト（または誰もいない）
	std_shared_ptr<CSysFuncState>	m_host_sfstate;	// ホストが実行権を手放している間のシステム関数の状態
	unsigned long		m_slice_start;	// 今のスレッドが実行権を取った時刻（ms）
	bool				m_terminating;	// 終了処理中。新しいスレッドを作らない
	int					m_next_id;
	volatile int		m_live;			// 実行中（状態がWORKER_RUNNING）のワーカーの数。monを持って更新する
	volatile int		m_wait_count;	// m_workqの長さ（GILを持つスレッドがロックなしで見るための写し）
	int					m_killed;		// 強制停止したワーカーの数
	std::vector< std_shared_ptr<CWorkerContext> >	m_workers;
	std::vector<const void*>	m_busy_libs;	// 呼び出し中の外部ライブラリ（LockLib）。monを持って読み書きする

	// monを持った状態で呼ぶ
	void	AcquireLocked(CWorkerContext *c);
	void	ReleaseLocked(void);
	void	UpdateAttentionLocked(void);
	bool	EligibleLocked(CWorkerContext *c) const;
	void	WaitAcquireWorkerLocked(CWorkerContext *c);
	void	WaitAcquireHostLocked(void);
	void	KillWorkerLocked(CWorkerContext *c);

	std_shared_ptr<CWorkerContext>	Find(int id);
	int		LiveCount(void) const;
	void	ReapFinished(bool all);
	static void	LeakIfAbandoned(const std_shared_ptr<CWorkerContext> &w);
	int		ExecuteWorker(CWorkerContext *c);

public:
	CThreadManager(CAyaVM &vmr);
	~CThreadManager(void);

	// ホスト（load/request/unload）の出入り。再入できる
	void	HostEnter(void);
	void	HostLeave(void);
	// unload用。待ち続けるGILの持ち主を一定時間で強制停止する
	void	HostEnterForUnload(void);

	// ワーカーの操作（GILを持って呼ぶ）
	int		Start(const yaya::string_t &funcname, const CValue &args);
	int		State(int id);
	int		Stop(int id);
	int		Join(int id, long timeout_ms);
	bool	GetResult(int id, CValue &result);
	int		Close(int id);

	bool	IsWorker(void) const { return m_active != NULL; }
	bool	HasLiveWorkers(void) const { return LiveCount() > 0; }
	// 今実行しているワーカー。ホストならNULL。GILを手放す前に控えておく（手放した後は呼ぶ側のスレッドを指さない）
	CWorkerContext*	Current(void) const { return m_active; }

	// SAORIなど、同時に1つのスレッドからしか呼べない外部ライブラリの呼び出しの排他（keyごと）。
	// GILを手放した状態で呼ぶこと（GILを持ったまま待つと、持ち主がGILを待って止まる）。
	// 待っている間にmeが中断されたらfalse（ロックは取れていない）
	bool	LockLib(const void *key, const CWorkerContext *me);
	void	UnlockLib(const void *key);

	// 文の境界で呼ぶ。trueなら今のワーカーは中断しなければならない
	bool	Attend(void);

	// ブロックする処理の前後でGILを手放す・取り戻す
	CGilSaved	BlockingRelease(void);
	void		BlockingReacquire(const CGilSaved &saved);
	// ワーカーなら実行権を手放して待ち、trueを返す（中断で早く戻ることがある）。ホストならfalse
	bool	WorkerSleep(unsigned long ms);

	// ワーカーの実行本体（OSスレッドから呼ばれる）
	void	RunWorker(CWorkerContext *c);

	// unload時：全ワーカーを中断して終わるのを待つ。止まらないものは強制停止する
	void	Shutdown(void);
};

// ブロックする処理（DIRECTSSTPなど）の間だけGILを手放す
class CGilRelease
{
private:
	CThreadManager&	mgr;
	CGilSaved		saved;
	bool			active;

public:
	// alsoHost: falseならワーカーのときだけ手放す（ホストは今までどおり持ち続ける）。
	// trueなら、ホストも、動いているワーカーがあるときは手放す（ワーカーを止めずに待つため。
	// ホストが手放している間は、別のホスト（別スレッドのrequest）は入れない）
	CGilRelease(CThreadManager &m, bool alsoHost = false) : mgr(m), active(false) {
		saved.valid = false; saved.ctx = NULL; saved.depth = 0;
		if (mgr.IsWorker() || (alsoHost && mgr.HasLiveWorkers())) {
			saved = mgr.BlockingRelease();
			active = saved.valid;
		}
	}
	~CGilRelease(void) {
		if (active) {
			mgr.BlockingReacquire(saved);
		}
	}
};

// DLLのプロセス終了・解放中か（このとき待ち合わせはしない）
extern bool g_yaya_detaching;

#endif //WORKTHREAD_H
