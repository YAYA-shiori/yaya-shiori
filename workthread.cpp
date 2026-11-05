//
// AYA version 5
//
// ワーカースレッド（THREADSTART）とインタプリタ全体のロック（GIL）
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <new>
#include <string.h>

#include "workthread.h"

#include "ayavm.h"
#include "function.h"
#include "log.h"
#include "messages.h"
#include "misc.h"
#include "sysfunc.h"
#include "variable.h"

#if defined(POSIX)
# include <pthread.h>
# include <time.h>
# include <errno.h>
# include <sys/time.h>
#elif defined(WIN32)
# include <process.h>
#endif

#if defined(__EMSCRIPTEN__)
# define YAYA_NO_THREADS
#endif

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

bool g_yaya_detaching = false;

#define THR_INFINITE			0xFFFFFFFFUL
#define WORKER_MAX_LIVE			32			// 同時に動かせるスレッドの数
#define WORKER_MAX_RECORDS		256			// 終了した記録を残しておく数
#define WORKER_TIMESLICE_MS		20			// 複数のワーカーが待っているとき、同じワーカーが続けて実行する時間
#define SHUTDOWN_WAIT_MS		5000		// unload時、ワーカーが自分で終わるのを待つ時間
#define UNLOAD_GIL_WAIT_MS		10000		// unload時、GILが空くのを待つ時間

/* -----------------------------------------------------------------------
 *  時刻・スレッドID
 * -----------------------------------------------------------------------
 */
static unsigned long ThrTickMs(void)
{
#if defined(POSIX)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return static_cast<unsigned long>(tv.tv_sec) * 1000UL + static_cast<unsigned long>(tv.tv_usec / 1000);
#elif defined(WIN32)
	return ::GetTickCount();
#else
	return 0;
#endif
}

static size_t ThrCurrentId(void)
{
#if defined(YAYA_NO_THREADS)
	return 1;
#elif defined(POSIX)
	pthread_t t = pthread_self();
	size_t v = 0;
	memcpy(&v, &t, sizeof(t) < sizeof(v) ? sizeof(t) : sizeof(v));
	return v;
#elif defined(WIN32)
	return static_cast<size_t>(::GetCurrentThreadId());
#else
	return 1;
#endif
}

/* -----------------------------------------------------------------------
 *  CThrMonitor : ロックと条件変数（全員を起こす通知だけ持つ）
 *
 *  Waitはロックを持って呼び、待っている間ロックを手放す。起こされたとき・時間切れのとき
 *  ロックを持って戻る。条件は呼び出し側がwhileで調べ直すこと
 * -----------------------------------------------------------------------
 */
#if defined(YAYA_NO_THREADS)

class CThrMonitor
{
public:
	void Lock(void) { }
	void Unlock(void) { }
	void Wait(unsigned long ms) { (void)ms; }
	void NotifyAll(void) { }
};

#elif defined(POSIX)

class CThrMonitor
{
private:
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
public:
	CThrMonitor(void) {
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&cond, NULL);
	}
	~CThrMonitor(void) {
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
	}
	void Lock(void) { pthread_mutex_lock(&mutex); }
	void Unlock(void) { pthread_mutex_unlock(&mutex); }
	void Wait(unsigned long ms) {
		if (ms == THR_INFINITE) {
			pthread_cond_wait(&cond, &mutex);
			return;
		}
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct timespec ts;
		ts.tv_sec = tv.tv_sec + static_cast<time_t>(ms / 1000);
		long nsec = static_cast<long>(tv.tv_usec) * 1000L + static_cast<long>(ms % 1000) * 1000000L;
		if (nsec >= 1000000000L) {
			ts.tv_sec += 1;
			nsec -= 1000000000L;
		}
		ts.tv_nsec = nsec;
		pthread_cond_timedwait(&cond, &mutex, &ts);
	}
	void NotifyAll(void) { pthread_cond_broadcast(&cond); }
};

#else // WIN32

// VC6の古いSDKでも使えるよう、待つ人ごとのイベントで作る（CONDITION_VARIABLEはVista以降）
class CThrMonitor
{
private:
	CRITICAL_SECTION		cs;
	std::vector<HANDLE>		waiters;
public:
	CThrMonitor(void) { ::InitializeCriticalSection(&cs); }
	~CThrMonitor(void) {
		::DeleteCriticalSection(&cs);
	}
	void Lock(void) { ::EnterCriticalSection(&cs); }
	void Unlock(void) { ::LeaveCriticalSection(&cs); }
	void Wait(unsigned long ms) {
		HANDLE ev = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!ev) {
			// イベントを作れないときは短く眠って条件を調べ直させる
			::LeaveCriticalSection(&cs);
			::Sleep(1);
			::EnterCriticalSection(&cs);
			return;
		}
		waiters.push_back(ev);
		::LeaveCriticalSection(&cs);
		::WaitForSingleObject(ev, ms == THR_INFINITE ? INFINITE : static_cast<DWORD>(ms));
		::EnterCriticalSection(&cs);
		for (std::vector<HANDLE>::iterator it = waiters.begin(); it != waiters.end(); ++it) {
			if (*it == ev) {
				waiters.erase(it);
				break;
			}
		}
		::CloseHandle(ev);
	}
	void NotifyAll(void) {
		for (size_t i = 0; i < waiters.size(); ++i) {
			::SetEvent(waiters[i]);
		}
		waiters.clear();
	}
};

#endif

/* -----------------------------------------------------------------------
 *  OSスレッドの作成・待ち合わせ・強制停止
 * -----------------------------------------------------------------------
 */
#if defined(YAYA_NO_THREADS)

struct CThrHandle { int dummy; };
static bool ThrCreate(CWorkerContext *) { return false; }
static void ThrJoinClose(CWorkerContext *) { }
static void ThrKill(CWorkerContext *) { }
static void ThrParkForever(void) { }

#elif defined(POSIX)

struct CThrHandle { pthread_t t; };

static void* WorkerThreadProc(void *p)
{
	CWorkerContext *c = static_cast<CWorkerContext*>(p);
	c->mgr->RunWorker(c);
	return NULL;
}

static bool ThrCreate(CWorkerContext *c)
{
	CThrHandle *h = new CThrHandle;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 2 * 1024 * 1024);
	int r = pthread_create(&h->t, &attr, WorkerThreadProc, c);
	pthread_attr_destroy(&attr);
	if (r != 0) {
		delete h;
		return false;
	}
	c->handle = h;
	return true;
}

static void ThrJoinClose(CWorkerContext *c)
{
	if (c->handle) {
		if (!c->abandoned) {
			pthread_join(c->handle->t, NULL);
		}
		delete c->handle;
		c->handle = NULL;
	}
}

// POSIXではスレッドを強制停止しない。pthread_cancelは強制的なunwindで、catch(...)に飲まれて異常終了したり、
// デストラクタが取り戻せないGILを待ち続けたりする。見捨てて（detach）、戻ってきたら眠らせる
static void ThrKill(CWorkerContext *c)
{
	if (c->handle && !c->abandoned) {
		c->abandoned = true;
		pthread_detach(c->handle->t);
	}
}

// 見捨てたスレッドが、GILを取り戻そうとしてここへ戻ってきたとき
static void ThrParkForever(void)
{
	for (;;) {
		struct timespec ts;
		ts.tv_sec = 3600;
		ts.tv_nsec = 0;
		nanosleep(&ts, NULL);
	}
}

#else // WIN32

struct CThrHandle { HANDLE h; };

static unsigned __stdcall WorkerThreadProc(void *p)
{
	CWorkerContext *c = static_cast<CWorkerContext*>(p);
	c->mgr->RunWorker(c);
	return 0;
}

static bool ThrCreate(CWorkerContext *c)
{
	unsigned tid = 0;
	HANDLE th = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadProc, c, 0, &tid);
	if (th == NULL) {
		return false;
	}
	CThrHandle *h = new CThrHandle;
	h->h = th;
	c->handle = h;
	return true;
}

static void ThrJoinClose(CWorkerContext *c)
{
	if (c->handle) {
		::WaitForSingleObject(c->handle->h, 5000);
		::CloseHandle(c->handle->h);
		delete c->handle;
		c->handle = NULL;
	}
}

static void ThrKill(CWorkerContext *c)
{
	if (c->handle) {
		::TerminateThread(c->handle->h, 1);
	}
}

static void ThrParkForever(void) { }	// Windowsは強制停止できるので見捨てない

#endif

/* -----------------------------------------------------------------------
 *  CWorkerContext
 * -----------------------------------------------------------------------
 */
CWorkerContext::CWorkerContext(void)
{
	id = 0;
	state = WORKER_RUNNING;
	cancel = 0;
	discard = false;
	abandoned = false;
	handle = NULL;
	mgr = NULL;
}

CWorkerContext::~CWorkerContext(void)
{
	if (handle) {
		delete handle;
		handle = NULL;
	}
}

/* -----------------------------------------------------------------------
 *  CThreadManager
 * -----------------------------------------------------------------------
 */
CThreadManager::CThreadManager(CAyaVM &vmr) : vm(vmr)
{
	mon = new CThrMonitor;
	m_host_sfstate = std_make_shared<CSysFuncState>();
	m_locked = false;
	m_owner = 0;
	m_depth = 0;
	m_prio_waiters = 0;
	m_host_parked = false;
	m_host_parked_depth = 0;
	m_active = NULL;
	m_slice_start = 0;
	m_terminating = false;
	m_next_id = 1;
	m_live = 0;
	m_wait_count = 0;
	m_killed = 0;
}

CThreadManager::~CThreadManager(void)
{
	// 通常はShutdownで片付いている。残っているものは待たずに捨てる
	for (size_t i = 0; i < m_workers.size(); ++i) {
		if (m_workers[i]->state != WORKER_RUNNING) {
			ThrJoinClose(m_workers[i].get());
		}
		LeakIfAbandoned(m_workers[i]);
	}
	m_workers.clear();
	delete mon;
	mon = NULL;
}

// ---- GILの出入り（monを持って呼ぶ） ----

void CThreadManager::UpdateAttentionLocked(void)
{
	bool need = (m_active != NULL) && (m_active->cancel || m_prio_waiters > 0 || m_wait_count > 0);
	vm.SetAttention(need);
}

void CThreadManager::AcquireLocked(CWorkerContext *c)
{
	// 入るスレッドの、システム関数の状態を戻す
	vm.sysfunction().SwapState(c ? *c->sfstate : *m_host_sfstate);
	m_locked = true;
	m_owner = ThrCurrentId();
	m_depth = 1;
	m_active = c;
	// ワーカーが動いている間のログは溜めておく（コールバックはホストのスレッドで呼ぶ）
	vm.logger().SetDeferCallbacks(c != NULL);
	m_slice_start = ThrTickMs();
	vm.SetActiveContext(c ? c->call_limit.get() : NULL);
	UpdateAttentionLocked();
}

void CThreadManager::ReleaseLocked(void)
{
	// 出るスレッドの、システム関数の状態を預かる
	if (m_locked) {
		vm.sysfunction().SwapState(m_active ? *m_active->sfstate : *m_host_sfstate);
	}
	m_locked = false;
	m_owner = 0;
	m_depth = 0;
	m_active = NULL;
	vm.logger().SetDeferCallbacks(false);
	vm.SetActiveContext(NULL);
	vm.SetAttention(false);
	mon->NotifyAll();
}

// GILが空いていて、cの番が来ているか（c==NULLはホスト）
bool CThreadManager::EligibleLocked(CWorkerContext *c) const
{
	if (m_locked) {
		return false;
	}
	if (!c) {
		return true;
	}
	return m_prio_waiters == 0 && !m_workq.empty() && m_workq.front() == c;
}

void CThreadManager::WaitAcquireWorkerLocked(CWorkerContext *c)
{
	m_workq.push_back(c);
	m_wait_count = static_cast<int>(m_workq.size());
	UpdateAttentionLocked();
	while (!EligibleLocked(c)) {
		mon->Wait(THR_INFINITE);
	}
	m_workq.erase(m_workq.begin());
	m_wait_count = static_cast<int>(m_workq.size());
	AcquireLocked(c);
}

void CThreadManager::WaitAcquireHostLocked(void)
{
	++m_prio_waiters;
	UpdateAttentionLocked();
	while (m_locked || m_host_parked) {
		mon->Wait(THR_INFINITE);
	}
	--m_prio_waiters;
	AcquireLocked(NULL);
}

// 動かなくなったワーカーを強制停止する（monを持って呼ぶ）。最後の手段
void CThreadManager::KillWorkerLocked(CWorkerContext *c)
{
	if (c->state != WORKER_RUNNING) {
		return;
	}
#if defined(POSIX)
	// GILを持って辞書を実行しているスレッドは、POSIXでは止められない（見捨てると実行を続けてしまう）
	if (m_active == c) {
		return;
	}
#endif
	ThrKill(c);
	c->state = WORKER_FAILED;
	c->cancel = 1;
	for (size_t i = 0; i < m_workq.size(); ++i) {
		if (m_workq[i] == c) {
			m_workq.erase(m_workq.begin() + i);
			break;
		}
	}
	m_wait_count = static_cast<int>(m_workq.size());
	--m_live;
	m_killed++;
	if (m_active == c) {
		// 強制停止したスレッドが持っていたGILを返す
		ReleaseLocked();
	}
	mon->NotifyAll();
}

// ---- ホスト ----

void CThreadManager::HostEnter(void)
{
	if (g_yaya_detaching) {
		return;
	}
	mon->Lock();
	if (m_locked && m_owner == ThrCurrentId() && m_active == NULL) {
		// 同じスレッドからの再入（処理の途中でSSPが同じスレッドからrequestを呼んだとき）
		++m_depth;
		mon->Unlock();
		return;
	}
	WaitAcquireHostLocked();
	mon->Unlock();

	// ワーカーが溜めたログを、このスレッド（ホスト）で送る。ロックは持たない（コールバックがrequestを呼びうる）
	vm.logger().FlushDeferred();
}

void CThreadManager::HostLeave(void)
{
	if (g_yaya_detaching) {
		return;
	}
	mon->Lock();
	bool last = (m_locked && m_active == NULL && m_depth <= 1);
	mon->Unlock();

	if (last) {
		vm.logger().FlushDeferred();
	}

	mon->Lock();
	if (m_locked && m_active == NULL) {
		if (m_depth > 1) {
			--m_depth;
		}
		else {
			ReleaseLocked();
		}
	}
	mon->Unlock();
}

void CThreadManager::HostEnterForUnload(void)
{
	if (g_yaya_detaching) {
		return;
	}
	mon->Lock();
	if (m_locked && m_owner == ThrCurrentId() && m_active == NULL) {
		++m_depth;
		mon->Unlock();
		return;
	}
	++m_prio_waiters;
	UpdateAttentionLocked();
	unsigned long start = ThrTickMs();
	while (m_locked || m_host_parked) {
		unsigned long elapsed = ThrTickMs() - start;
		if (elapsed >= UNLOAD_GIL_WAIT_MS) {
#if defined(POSIX)
			// POSIXではGILを持つスレッドを止められないので、終わるまで待つ
			mon->Wait(1000);
			continue;
#else
			// GILの持ち主が応答しない。unloadを進めるため強制的に取り上げる
			if (m_active) {
				KillWorkerLocked(m_active);
			}
			m_locked = false;
			m_host_parked = false;
			break;
#endif
		}
		mon->Wait(UNLOAD_GIL_WAIT_MS - elapsed);
	}
	--m_prio_waiters;
	AcquireLocked(NULL);
	mon->Unlock();

	vm.logger().FlushDeferred();
}

// ---- ブロックする処理の前後 ----

CGilSaved CThreadManager::BlockingRelease(void)
{
	CGilSaved s;
	s.valid = false;
	s.ctx = NULL;
	s.depth = 0;
	if (g_yaya_detaching) {
		return s;
	}
	mon->Lock();
	if (m_locked && m_owner == ThrCurrentId()) {
		s.valid = true;
		s.ctx = m_active;
		s.depth = m_depth;
		if (!m_active) {
			// ホストが待つ間は、ほかのホスト（別スレッドからのrequest）を入れない
			m_host_parked = true;
		}
		ReleaseLocked();
	}
	mon->Unlock();
	return s;
}

void CThreadManager::BlockingReacquire(const CGilSaved &s)
{
	if (!s.valid) {
		return;
	}
	// 見捨てられた（unloadで強制停止の対象になった）スレッド。VMやロックはもう解放されているかもしれないので、
	// 何にも触れずに眠る。見捨てた記録は解放せずに残す（ReapFinished、デストラクタ）
	if (s.ctx && s.ctx->abandoned) {
		ThrParkForever();
		return;
	}
	mon->Lock();
	if (s.ctx) {
		WaitAcquireWorkerLocked(s.ctx);
	}
	else {
		++m_prio_waiters;
		UpdateAttentionLocked();
		while (m_locked) {
			mon->Wait(THR_INFINITE);
		}
		--m_prio_waiters;
		AcquireLocked(NULL);
		m_depth = s.depth;
		m_host_parked = false;
		mon->NotifyAll();
	}
	mon->Unlock();

	if (!s.ctx) {
		// ホストに戻ったので、ワーカーが溜めたログを送る
		vm.logger().FlushDeferred();
	}
}

bool CThreadManager::LockLib(const void *key, const CWorkerContext *me)
{
	mon->Lock();
	for (;;) {
		bool busy = false;
		for (size_t i = 0; i < m_busy_libs.size(); ++i) {
			if (m_busy_libs[i] == key) {
				busy = true;
				break;
			}
		}
		if (!busy) {
			break;
		}
		if (me && me->cancel) {
			mon->Unlock();
			return false;
		}
		mon->Wait(THR_INFINITE);
	}
	m_busy_libs.push_back(key);
	mon->Unlock();
	return true;
}

void CThreadManager::UnlockLib(const void *key)
{
	mon->Lock();
	for (size_t i = 0; i < m_busy_libs.size(); ++i) {
		if (m_busy_libs[i] == key) {
			m_busy_libs.erase(m_busy_libs.begin() + i);
			break;
		}
	}
	mon->NotifyAll();
	mon->Unlock();
}

bool CThreadManager::WorkerSleep(unsigned long ms)
{
	CWorkerContext *me = m_active;
	if (!me) {
		return false;
	}
	CGilSaved s = BlockingRelease();
	mon->Lock();
	unsigned long start = ThrTickMs();
	while (!me->cancel) {
		unsigned long elapsed = ThrTickMs() - start;
		if (elapsed >= ms) {
			break;
		}
		mon->Wait(ms - elapsed);
	}
	mon->Unlock();
	BlockingReacquire(s);
	return true;
}

// ---- 文の境界での確認 ----

bool CThreadManager::Attend(void)
{
	CWorkerContext *c = m_active;
	if (!c) {
		vm.SetAttention(false);
		return false;
	}
	if (c->cancel) {
		return true;
	}
	bool yield = false;
	if (m_prio_waiters > 0) {
		yield = true;
	}
	else if (m_wait_count > 0 && (ThrTickMs() - m_slice_start) >= WORKER_TIMESLICE_MS) {
		yield = true;
	}
	if (yield) {
		mon->Lock();
		ReleaseLocked();
		WaitAcquireWorkerLocked(c);
		mon->Unlock();
	}
	return c->cancel != 0;
}

// ---- ワーカーの操作 ----

std_shared_ptr<CWorkerContext> CThreadManager::Find(int id)
{
	for (size_t i = 0; i < m_workers.size(); ++i) {
		if (m_workers[i]->id == id) {
			return m_workers[i];
		}
	}
	return std_shared_ptr<CWorkerContext>();
}

int CThreadManager::LiveCount(void) const
{
	return m_live;
}

// 終了した記録のうち、捨てるよう頼まれたもの（all=trueなら全部）と、残す数を超えた古いものを片付ける
void CThreadManager::ReapFinished(bool all)
{
	std::vector< std_shared_ptr<CWorkerContext> > dead;

	mon->Lock();
	for (size_t i = 0; i < m_workers.size(); ) {
		CWorkerContext *w = m_workers[i].get();
		if (w->state != WORKER_RUNNING && (all || w->discard)) {
			dead.push_back(m_workers[i]);
			m_workers.erase(m_workers.begin() + i);
		}
		else {
			++i;
		}
	}
	for (size_t i = 0; m_workers.size() > WORKER_MAX_RECORDS && i < m_workers.size(); ) {
		if (m_workers[i]->state != WORKER_RUNNING) {
			dead.push_back(m_workers[i]);
			m_workers.erase(m_workers.begin() + i);
		}
		else {
			++i;
		}
	}
	mon->Unlock();

	// スレッドの終わりを待つ（終了済みなのですぐ返る）。monは持たない
	for (size_t i = 0; i < dead.size(); ++i) {
		ThrJoinClose(dead[i].get());
		LeakIfAbandoned(dead[i]);
	}
}

// 見捨てたスレッドは、あとでこの記録を読みにくる（GILを取り戻そうとして）。解放せず、わざと残す
void CThreadManager::LeakIfAbandoned(const std_shared_ptr<CWorkerContext> &w)
{
	if (w->abandoned) {
		new std_shared_ptr<CWorkerContext>(w);
	}
}

int CThreadManager::Start(const yaya::string_t &funcname, const CValue &args)
{
#if defined(YAYA_NO_THREADS)
	(void)funcname; (void)args;
	return WORKER_START_NOSUPPORT;
#else
	if (m_terminating) {
		return WORKER_START_DENIED;
	}

	ReapFinished(false);

	if (m_live >= WORKER_MAX_LIVE) {
		return WORKER_START_TOOMANY;
	}

	if (vm.function_exec().GetFunctionIndexFromName(funcname) < 0) {
		return WORKER_START_NOFUNC;
	}

	std_shared_ptr<CWorkerContext> c = std_make_shared<CWorkerContext>();
	c->funcname = funcname;
	c->args = args;
	c->mgr = this;

	c->sfstate = std_make_shared<CSysFuncState>();
	c->call_limit = std_make_shared<CCallLimit>();
	c->call_limit->SetMaxDepth(vm.host_call_limit().GetMaxDepth());
	c->call_limit->SetMaxLoop(vm.host_call_limit().GetMaxLoop());

	// 関数表はホストと共有する（式の評価中の一時値は実行ごとのフレーム（CExecFrame）にあり、
	// 関数表は読み取り専用なので、同じ関数を別のスレッドと同時に実行しても衝突しない）

	mon->Lock();
	c->id = m_next_id++;
	m_workers.push_back(c);
	++m_live;
	mon->Unlock();

	if (!ThrCreate(c.get())) {
		mon->Lock();
		--m_live;
		for (size_t i = 0; i < m_workers.size(); ++i) {
			if (m_workers[i].get() == c.get()) {
				m_workers.erase(m_workers.begin() + i);
				break;
			}
		}
		mon->Unlock();
		return WORKER_START_FAILED;
	}
	return c->id;
#endif
}

int CThreadManager::State(int id)
{
	std_shared_ptr<CWorkerContext> c = Find(id);
	if (!c) {
		return -1;
	}
	return c->state;
}

int CThreadManager::Stop(int id)
{
	std_shared_ptr<CWorkerContext> c = Find(id);
	if (!c) {
		return -1;
	}
	if (c->state != WORKER_RUNNING) {
		return 0;
	}
	mon->Lock();
	c->cancel = 1;
	if (c.get() == m_active) {
		vm.SetAttention(true);
	}
	mon->NotifyAll();
	mon->Unlock();
	return 1;
}

int CThreadManager::Join(int id, long timeout_ms)
{
	std_shared_ptr<CWorkerContext> c = Find(id);
	if (!c) {
		return WORKER_JOIN_NOTFOUND;
	}
	if (c.get() == m_active) {
		return WORKER_JOIN_SELF;
	}
	if (c->state != WORKER_RUNNING) {
		return 1;
	}

	CWorkerContext *me = m_active;
	CGilSaved s = BlockingRelease();

	mon->Lock();
	unsigned long start = ThrTickMs();
	while (c->state == WORKER_RUNNING && !(me && me->cancel)) {
		if (timeout_ms < 0) {
			mon->Wait(THR_INFINITE);
		}
		else {
			unsigned long elapsed = ThrTickMs() - start;
			if (elapsed >= static_cast<unsigned long>(timeout_ms)) {
				break;
			}
			mon->Wait(static_cast<unsigned long>(timeout_ms) - elapsed);
		}
	}
	mon->Unlock();

	BlockingReacquire(s);

	return c->state != WORKER_RUNNING ? 1 : 0;
}

bool CThreadManager::GetResult(int id, CValue &result)
{
	std_shared_ptr<CWorkerContext> c = Find(id);
	if (!c || c->state == WORKER_RUNNING) {
		return false;
	}
	result = c->result;
	return true;
}

int CThreadManager::Close(int id)
{
	std_shared_ptr<CWorkerContext> c = Find(id);
	if (!c) {
		return -1;
	}
	if (c->state == WORKER_RUNNING) {
		// 止めてから、終わったときに記録を捨てる
		mon->Lock();
		c->cancel = 1;
		c->discard = true;
		if (c.get() == m_active) {
			vm.SetAttention(true);
		}
		mon->NotifyAll();
		mon->Unlock();
		return 1;
	}
	c->discard = true;
	ReapFinished(false);
	return 0;
}

// ---- ワーカーの実行 ----

// GILを持って呼ばれる。関数を実行して結果を返す
int CThreadManager::ExecuteWorker(CWorkerContext *c)
{
	int st = WORKER_FINISHED;

	try {
		c->call_limit->InitCall();

		CFunctionDef &fd = vm.function_exec();
		ptrdiff_t idx = fd.GetFunctionIndexFromName(c->funcname);
		if (idx < 0) {
			return WORKER_FAILED;
		}

		CLocalVariable lvar(vm);
		CFunction::ExecutionResult r = fd.func[idx].Execute(c->args, lvar);
		if (c->cancel) {
			st = WORKER_CANCELED;
		}
		else {
			c->result = r.Output();
		}
	}
	catch (const yaya::memory_error&) {
		vm.logger().Error(E_W, 33, c->funcname);
		st = WORKER_FAILED;
	}
	catch (const std::bad_alloc&) {
		vm.logger().Error(E_W, 33, c->funcname);
		st = WORKER_FAILED;
	}
	catch (...) {
		vm.logger().Error(E_W, 33, c->funcname);
		st = WORKER_FAILED;
	}

	return st;
}

void CThreadManager::RunWorker(CWorkerContext *c)
{
	mon->Lock();
	WaitAcquireWorkerLocked(c);
	mon->Unlock();

	int st = WORKER_CANCELED;
	if (!c->cancel) {
		st = ExecuteWorker(c);
	}

	// 終わったワーカーの引数・呼び出し制限は、すぐ手放す（GILを持っている間に）。
	// 記録はTHREADCLOSEされるまで残るので、持ち続けると使い捨てのスレッドを繰り返したときにメモリが尽きる。
	// 呼び出し制限を捨てる前に、VMが指さないようにしておく。monは持たない（捨てるのに時間がかかる）
	vm.SetActiveContext(NULL);
	c->call_limit.reset();
	{
		CValue	empty;
		c->args = empty;
	}

	// 結果などの後片付けはGILを持ったまま。状態を書いたらGILを返して終わる
	mon->Lock();
	if (c->state == WORKER_RUNNING) {
		c->state = st;
		--m_live;
	}
	if (m_active == c) {
		ReleaseLocked();
	}
	mon->NotifyAll();
	mon->Unlock();
}

// ---- 終了処理 ----

void CThreadManager::Shutdown(void)
{
	mon->Lock();
	m_terminating = true;
	for (size_t i = 0; i < m_workers.size(); ++i) {
		if (m_workers[i]->state == WORKER_RUNNING) {
			m_workers[i]->cancel = 1;
		}
	}
	mon->NotifyAll();
	mon->Unlock();

	if (m_live <= 0) {
		ReapFinished(true);
		return;
	}

	if (g_yaya_detaching) {
		// DLLの解放中は待てない（ローダーロックを持っている）。強制停止だけして戻る
		mon->Lock();
		for (size_t i = 0; i < m_workers.size(); ++i) {
			KillWorkerLocked(m_workers[i].get());
		}
		m_locked = false;
		mon->Unlock();
		return;
	}

	// ホストのGILを手放して、ワーカーが中断を見つけて終わるのを待つ
	CGilSaved s = BlockingRelease();

	mon->Lock();
	unsigned long start = ThrTickMs();
	while (m_live > 0) {
		unsigned long elapsed = ThrTickMs() - start;
		if (elapsed >= SHUTDOWN_WAIT_MS) {
			break;
		}
		mon->Wait(SHUTDOWN_WAIT_MS - elapsed);
	}
	if (m_live > 0) {
		// 自分では終わらない（ブロックする処理の中など）。最後の手段として強制停止する
		for (size_t i = 0; i < m_workers.size(); ++i) {
			KillWorkerLocked(m_workers[i].get());
		}
	}
	mon->Unlock();

	BlockingReacquire(s);

	if (m_killed > 0) {
		vm.logger().Error(E_W, 34);
	}

	ReapFinished(true);
}
