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
#include <locale>
#include <clocale>

#include "aya5.h"
#include "basis.h"
#include "ayavm.h"
#include "ccct.h"
#include "manifest.h"

static std::vector<CAyaVM*> vm;
static yaya::string_t modulename;

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

class CAyaVMPrepare {
public:
	CAyaVMPrepare(void) {
		vm.clear();
		vm.push_back(NULL); //0番VM＝loadなど従来関数で使う標準
	}
	~CAyaVMPrepare(void) {
		size_t n = vm.size();
		for ( size_t i = 0 ; i < n ; ++i ) {
			if ( vm[i] ) {
				delete vm[i];
			}
		}
	}
};

static CAyaVMPrepare prepare; //これはコンストラクタ・デストラクタ作動用

/* -----------------------------------------------------------------------
 *  DllMain
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
	// モジュールの主ファイル名を取得
	// NT系ではいきなりUNICODEで取得できるが、9x系を考慮してMBCSで取得してからUCS-2へ変換
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		char path[MAX_PATH];
		GetModuleFileName(hModule, path, sizeof(path));
		char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(path, drive, dir, fname, ext);
		std::string	mbmodulename = fname;

		wchar_t	*wcmodulename = Ccct::MbcsToUcs2(mbmodulename, CHARSET_DEFAULT);
		modulename = wcmodulename;

		free(wcmodulename);
		wcmodulename = NULL;

		Ccct::sys_setlocale(LC_ALL);
	}

	return TRUE;
}
#endif

/* -----------------------------------------------------------------------
 *  load
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB load(yaya::global_t h, long len)
{
	vm[0] = new CAyaVM;

	vm[0]->basis().SetModuleName(modulename);

	vm[0]->Load();

	vm[0]->basis().SetPath(h, len);
	vm[0]->basis().Configure();

    return 1;
}

extern "C" DLLEXPORT long FUNCATTRIB multi_load(yaya::global_t h, long len)
{
	long id = 0;
	
	long n = (long)vm.size();
	for ( long i = 1 ; i < n ; ++i ) { //1から 0番は従来用
		if ( vm[i] == NULL ) {
			id = i;
		}
	}

	if ( id <= 0 ) {
		vm.push_back(NULL);
		id = (long)vm.size() - 1;
	}

	vm[id] = new CAyaVM;

	vm[id]->basis().SetModuleName(modulename);

	vm[id]->Load();

	vm[id]->basis().SetPath(h, len);
	vm[id]->basis().Configure();

    return id;
}

/* -----------------------------------------------------------------------
 *  unload
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB unload()
{
	vm[0]->basis().Termination();

	vm[0]->Unload();

	delete vm[0];
	vm[0] = NULL;

    return 1;
}

extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB multi_unload(long id)
{
	if ( id <= 0 || id > (long)vm.size() || vm[id] == NULL ) { //1から 0番は従来用
		return 0;
	}

	vm[id]->basis().Termination();

	vm[id]->Unload();

	delete vm[id];
	vm[id] = NULL;

	return 1;
}

/* -----------------------------------------------------------------------
 *  request
 * -----------------------------------------------------------------------
 */
extern "C" DLLEXPORT yaya::global_t FUNCATTRIB request(yaya::global_t h, long *len)
{
	if ( vm[0] ) {
		return vm[0]->basis().ExecuteRequest(h, len);
	}
	else {
		return NULL;
	}
}

extern "C" DLLEXPORT yaya::global_t FUNCATTRIB multi_request(long id, yaya::global_t h, long *len)
{
	if ( id <= 0 || id > (long)vm.size() || vm[id] == NULL ) { //1から 0番は従来用
		return 0;
	}

	if ( vm[id] ) {
		return vm[id]->basis().ExecuteRequest(h, len);
	}
	else {
		return NULL;
	}
}

/* -----------------------------------------------------------------------
 *  logsend（AYA固有　チェックツールから使用）
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB logsend(long hwnd)
{
	if ( vm[0] ) {
		vm[0]->basis().SetLogRcvWnd(hwnd);
	}
	else if ( vm.size() >= 2 && vm[1] ) {
		vm[1]->basis().SetLogRcvWnd(hwnd);
	}

	return TRUE;
}
#endif
