// 
// AYA version 5
//
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <ctime>

#include "aya5.h"
#include "basis.h"
#include "ayavm.h"
#include "ccct.h"

static CAyaVM  vm;

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  DllMain
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
extern "C" BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		vm.basis().SetModuleHandle(hModule);
	}

	return TRUE;
}
#endif

/* -----------------------------------------------------------------------
 *  load
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB load(yaya::global_t h, long len)
{
#if defined(WIN32) || defined(_WIN32_WCE)
	Ccct::sys_setlocale(LC_ALL);
#endif
	vm.Load();

	vm.basis().SetPath(h, len);
	vm.basis().Configure();

    return 1;
}

/* -----------------------------------------------------------------------
 *  unload
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB unload()
{
	vm.basis().Termination();

	vm.Unload();

    return 1;
}

/* -----------------------------------------------------------------------
 *  request
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT yaya::global_t FUNCATTRIB request(yaya::global_t h, long *len)
{
	return vm.basis().ExecuteRequest(h, len);
}

/* -----------------------------------------------------------------------
 *  logsend（AYA固有　チェックツールから使用）
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB logsend(long hwnd)
{
	vm.basis().SetLogRcvWnd(hwnd);

	return TRUE;
}
#endif
