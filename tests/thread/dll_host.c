/*
 * yaya.dll をSSPと同じようにLoadLibraryで読み込み、ワーカースレッドが動いたまま
 * unload / FreeLibrary / プロセス終了をしても止まらず落ちないことを確かめる試験用ホスト
 *
 * 32bitのyaya.dll（VC6）に合わせて、VC6のclでビルドする:
 *   cl /nologo /MT dll_host.c
 * 使い方: dll_host <yaya.dllのパス> <辞書のディレクトリ> <unload|free|exit|log>
 *   unload : request → unload → FreeLibrary（SSPの通常の終わり方）
 *   free   : request → unloadを呼ばずにFreeLibrary
 *   exit   : request → DLLを読み込んだままExitProcess
 *   log    : Set_loghandlerのコールバックが、ワーカーが警告を出し続けていても、
 *            必ずrequestを呼んだスレッドで呼ばれること（SSPのUIスレッドを待たないこと）を確かめる
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef BOOL    (__cdecl *LOAD_T)(HGLOBAL, long);
typedef BOOL    (__cdecl *UNLOAD_T)(void);
typedef HGLOBAL (__cdecl *REQUEST_T)(HGLOBAL, long*);
typedef void    (__cdecl *LOGHANDLER_T)(const wchar_t *, int, int);
typedef void    (__cdecl *SETLOGHANDLER_T)(LOGHANDLER_T);

static DWORD g_main_tid;
static HWND g_wnd;
static volatile LONG g_calls;
static volatile LONG g_foreign;
static volatile LONG g_stuck;

/* SSPのログ窓と同じように、メインスレッドの窓へSendMessageする。
   ほかのスレッドから呼ばれると、メインがrequestの中で窓のメッセージを処理しないため止まる */
static void __cdecl log_cb(const wchar_t *str, int mode, int id)
{
	DWORD_PTR res = 0;
	(void)str; (void)mode; (void)id;
	InterlockedIncrement(&g_calls);
	if (GetCurrentThreadId() != g_main_tid) {
		InterlockedIncrement(&g_foreign);
	}
	if (g_wnd && !SendMessageTimeout(g_wnd, WM_APP, 0, 0, SMTO_NORMAL, 2000, &res)) {
		InterlockedIncrement(&g_stuck);
	}
}

static HGLOBAL make(const char *s, long *len)
{
	HGLOBAL h;
	*len = (long)strlen(s);
	h = GlobalAlloc(GMEM_FIXED, *len + 1);
	memcpy(h, s, *len + 1);
	return h;
}

static void request_q(REQUEST_T req, const char *text, int quiet)
{
	long len;
	HGLOBAL h;
	HGLOBAL r;
	h = make(text, &len);
	r = req(h, &len);
	if (!quiet) {
		printf("%s => ", text);
		if (r) {
			fwrite(r, 1, len, stdout);
		}
		printf("\n");
		fflush(stdout);
	}
	if (r) {
		GlobalFree(r);
	}
}

static void request(REQUEST_T req, const char *text)
{
	request_q(req, text, 0);
}

int main(int argc, char **argv)
{
	HMODULE dll;
	LOAD_T load;
	UNLOAD_T unload;
	REQUEST_T req;
	SETLOGHANDLER_T setlog;
	long len;
	HGLOBAL h;
	DWORD t0;
	int i;
	int logmode;

	if (argc < 4) {
		printf("usage: dll_host dll dir mode\n");
		return 2;
	}
	logmode = !strcmp(argv[3], "log");
	g_main_tid = GetCurrentThreadId();

	dll = LoadLibraryA(argv[1]);
	if (!dll) {
		printf("LoadLibrary failed %lu\n", GetLastError());
		return 2;
	}
	load = (LOAD_T)GetProcAddress(dll, "load");
	unload = (UNLOAD_T)GetProcAddress(dll, "unload");
	req = (REQUEST_T)GetProcAddress(dll, "request");
	setlog = (SETLOGHANDLER_T)GetProcAddress(dll, "Set_loghandler");
	if (!load || !unload || !req || !setlog) {
		printf("GetProcAddress failed\n");
		return 2;
	}

	if (logmode) {
		g_wnd = CreateWindowA("STATIC", "yaya_dll_host", 0, 0, 0, 10, 10, NULL, NULL, GetModuleHandle(NULL), NULL);
		setlog(log_cb);
	}

	{
		char dir[MAX_PATH + 4];
		strcpy(dir, argv[2]);
		if (dir[strlen(dir) - 1] != '\\') strcat(dir, "\\");
		h = make(dir, &len);
		load(h, len);
		GlobalFree(h);
	}

	if (logmode) {
		long before;
		request(req, "?? THREADSTART(\"TH.WarnLoop\", 100000)");
		before = g_calls;
		for (i = 0; i < 300; i++) {
			request_q(req, "?? 1+1", 1);
			Sleep(2);
		}
		request(req, "?? THREADSTOP(1)");
		request(req, "?? THREADJOIN(1, 5000)");
		request(req, "?? 1+1");
		printf("log callbacks: total=%ld during-loop=%ld foreign-thread=%ld stuck=%ld\n",
			(long)g_calls, (long)g_calls - before, (long)g_foreign, (long)g_stuck);
		t0 = GetTickCount();
		unload();
		printf("unload took %lu ms\n", GetTickCount() - t0);
		FreeLibrary(dll);
		if (g_foreign || g_stuck || !g_calls) {
			printf("FAIL\n");
			return 1;
		}
		printf("OK\n");
		return 0;
	}

	request(req, "?? THREADSTART(\"TH.Spin\")");
	request(req, "?? THREADSTART(\"TH.Sleeper\", 60000)");
	request(req, "?? THREADSTART(\"TH.SpinDeep\")");
	request(req, "?? THREADSTART(\"TH.Shared\", 100000)");
	Sleep(300);
	request(req, "?? 1+1");
	request(req, "?? THREADSTATE(1)");

	t0 = GetTickCount();
	if (!strcmp(argv[3], "unload")) {
		unload();
		printf("unload took %lu ms\n", GetTickCount() - t0);
		FreeLibrary(dll);
		printf("FreeLibrary took %lu ms (total)\n", GetTickCount() - t0);
	}
	else if (!strcmp(argv[3], "free")) {
		FreeLibrary(dll);
		printf("FreeLibrary without unload took %lu ms\n", GetTickCount() - t0);
	}
	else {
		printf("ExitProcess with the DLL loaded\n");
		fflush(stdout);
		ExitProcess(0);
	}
	Sleep(300);
	printf("done\n");
	return 0;
}
