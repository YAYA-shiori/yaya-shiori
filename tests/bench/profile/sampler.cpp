// 作業コピー専用: 本体スレッドを 1ms ごとに止めてスタックを採る簡易サンプリングプロファイラ
//   停止中はコンテキストとスタックのコピーだけ（ヒープを触らない）。巻き戻しは再開後に StackWalk64 で行う
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

static HANDLE g_main = NULL;
static volatile LONG g_on = 0;
static volatile LONG g_quit = 0;
static HANDLE g_thread = NULL;

struct Sample { std::vector<DWORD64> frames; };
static std::vector<Sample> g_samples;

static const size_t STACK_COPY = 65536;
static unsigned char g_stackcopy[STACK_COPY];
static DWORD64 g_stack_base = 0;
static size_t g_stack_len = 0;

static BOOL CALLBACK ReadMem(HANDLE, DWORD64 addr, PVOID buf, DWORD size, LPDWORD got)
{
	if (addr >= g_stack_base && addr + size <= g_stack_base + g_stack_len) {
		memcpy(buf, g_stackcopy + (addr - g_stack_base), size);
		if (got) *got = size;
		return TRUE;
	}
	if (IsBadReadPtr((const void*)addr, size)) return FALSE;
	memcpy(buf, (const void*)addr, size);
	if (got) *got = size;
	return TRUE;
}

static DWORD WINAPI SamplerMain(LPVOID)
{
	HANDLE proc = GetCurrentProcess();
	timeBeginPeriod(1);
	while (!g_quit) {
		Sleep(1);
		if (!g_on) continue;
		CONTEXT ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_FULL;
		if (SuspendThread(g_main) == (DWORD)-1) continue;
		bool ok = GetThreadContext(g_main, &ctx) != 0;
		if (ok) {
			g_stack_base = ctx.Rsp;
			g_stack_len = STACK_COPY;
			// スタックの終端を越えないよう、読めるところまで
			while (g_stack_len > 0 && IsBadReadPtr((const void*)g_stack_base, g_stack_len)) g_stack_len /= 2;
			memcpy(g_stackcopy, (const void*)g_stack_base, g_stack_len);
		}
		ResumeThread(g_main);
		if (!ok) continue;

		STACKFRAME64 f;
		memset(&f, 0, sizeof(f));
		f.AddrPC.Offset = ctx.Rip;    f.AddrPC.Mode = AddrModeFlat;
		f.AddrFrame.Offset = ctx.Rsp; f.AddrFrame.Mode = AddrModeFlat;
		f.AddrStack.Offset = ctx.Rsp; f.AddrStack.Mode = AddrModeFlat;
		Sample s;
		for (int i = 0; i < 48; i++) {
			s.frames.push_back(f.AddrPC.Offset);
			if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, g_main, &f, &ctx, ReadMem,
				SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
			if (f.AddrPC.Offset == 0) break;
		}
		g_samples.push_back(s);
	}
	return 0;
}

extern "C" void sampler_start(void)
{
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &g_main, 0, FALSE, DUPLICATE_SAME_ACCESS);
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
	SymInitialize(GetCurrentProcess(), NULL, TRUE);
	g_samples.reserve(200000);
	g_thread = CreateThread(NULL, 0, SamplerMain, NULL, 0, NULL);
}

extern "C" void sampler_enable(int on)
{
	g_on = on;
}

// 各サンプルを「関数名 <- 呼び出し元 <- ...」の 1 行にして書き出す（先頭が葉）
extern "C" void sampler_dump(const char* path)
{
	g_quit = 1;
	if (g_thread) WaitForSingleObject(g_thread, 2000);
	HANDLE proc = GetCurrentProcess();
	std::map<DWORD64, std::string> cache;
	FILE* fp = fopen(path, "wb");
	if (!fp) return;
	char symbuf[sizeof(SYMBOL_INFO) + 512];
	for (size_t i = 0; i < g_samples.size(); i++) {
		const std::vector<DWORD64>& fr = g_samples[i].frames;
		for (size_t j = 0; j < fr.size(); j++) {
			DWORD64 a = fr[j];
			// 戻りアドレスは呼び出し命令の次なので、1 引いて呼び出し側の関数に寄せる（葉は除く）
			DWORD64 q = (j == 0) ? a : a - 1;
			std::map<DWORD64, std::string>::iterator it = cache.find(q);
			if (it == cache.end()) {
				SYMBOL_INFO* sym = (SYMBOL_INFO*)symbuf;
				memset(sym, 0, sizeof(SYMBOL_INFO));
				sym->SizeOfStruct = sizeof(SYMBOL_INFO);
				sym->MaxNameLen = 500;
				DWORD64 disp = 0;
				std::string name;
				if (SymFromAddr(proc, q, &disp, sym)) {
					name = sym->Name;
				} else {
					char tmp[48];
					DWORD64 base = (DWORD64)GetModuleHandle(NULL);
					if (q >= base && q < base + 0x10000000) { sprintf(tmp, "@%llx", (unsigned long long)(q - base)); } else { sprintf(tmp, "0x%llx", (unsigned long long)q); }
					name = tmp;
				}
				IMAGEHLP_MODULE64 mi;
				memset(&mi, 0, sizeof(mi));
				mi.SizeOfStruct = sizeof(mi);
				if (SymGetModuleInfo64(proc, q, &mi)) {
					name = std::string(mi.ModuleName) + "!" + name;
				}
				it = cache.insert(std::make_pair(q, name)).first;
			}
			fprintf(fp, j ? " <- %s" : "%s", it->second.c_str());
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}
