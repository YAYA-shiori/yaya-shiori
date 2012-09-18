// 
// AYA version 5
//
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#endif

#include <vector>
#include <ctime>
#include <locale>
#include <clocale>
#include <locale>
#include <stdio.h>

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
		vm.push_back(NULL); //0��VM��load�ȂǏ]���֐��Ŏg���W��
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

static CAyaVMPrepare prepare; //����̓R���X�g���N�^�E�f�X�g���N�^�쓮�p

/* -----------------------------------------------------------------------
 *  DllMain
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)

static void AYA_InitModule(HMODULE hModule)
{
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

#endif //win32

#if !defined(AYA_MAKE_EXE)
#if defined(WIN32)

extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
	// ���W���[���̎�t�@�C�������擾
	// NT�n�ł͂����Ȃ�UNICODE�Ŏ擾�ł��邪�A9x�n���l������MBCS�Ŏ擾���Ă���UCS-2�֕ϊ�
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		AYA_InitModule(hModule);
	}

	return TRUE;
}

#endif //win32
#endif //aya_make_exe

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
	for ( long i = 1 ; i < n ; ++i ) { //1���� 0�Ԃ͏]���p
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
	if ( id <= 0 || id > (long)vm.size() || vm[id] == NULL ) { //1���� 0�Ԃ͏]���p
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
	if ( id <= 0 || id > (long)vm.size() || vm[id] == NULL ) { //1���� 0�Ԃ͏]���p
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
 *  logsend�iAYA�ŗL�@�`�F�b�N�c�[������g�p�j
 * -----------------------------------------------------------------------
 */
#if !defined(AYA_MAKE_EXE)
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
#endif //win32
#endif //aya_make_exe


/* -----------------------------------------------------------------------
 *  main (���s�t�@�C���ł̂�)
 * -----------------------------------------------------------------------
 */

#if defined(AYA_MAKE_EXE)

int main( int argc, char *argv[ ], char *envp[ ] )
{
	AYA_InitModule(NULL);

	std::string bufstr;

	_setmode( _fileno( stdin ), _O_BINARY );
	_setmode( _fileno( stdout ), _O_BINARY );

	while ( 1 ) {
		bufstr = "";

		while ( 1 ) {
			char buf[2];
			fread(buf,1,1,stdin);
			bufstr += static_cast<char>(buf[0]);

			if ( bufstr.size() >= 2 ) {
				if ( strcmp(bufstr.c_str() + bufstr.size() - 2,"\r\n") == 0 ) { //���s���o
					break;
				}
			}
		}

		const char* bufptr = bufstr.c_str();

		if ( strncmp(bufptr,"load:",5) == 0 ) {
			bufptr += 5;
			long size = atoi(bufptr);
			if ( size > 0 ) {
				char *read_ptr = (char*)::GlobalAlloc(GMEM_FIXED,size+1);
				fread(read_ptr,1,size,stdin);
				read_ptr[size] = 0;

				char *p = strstr(read_ptr,"\r\n");
				if ( p ) { *p = 0; size -= 2; }
				
				load(read_ptr,size);
			}

			const char* result = "load:3\r\n1\r\n";
			fwrite(result,1,strlen(result),stdout);
			fflush(stdout);
		}
		else if ( strncmp(bufptr,"unload:",7) == 0 ) {
			bufptr += 7;
			long size = atoi(bufptr);
			if ( size > 0 ) {
				char *read_ptr = (char*)malloc(size);
				fread(read_ptr,1,size,stdin);
				free(read_ptr); //�f�[�^�܂Ƃ߂Ĕj��
			}

			unload();

			const char* result = "unload:3\r\n1\r\n";
			fwrite(result,1,strlen(result),stdout);
			fflush(stdout);
			break;
		}
		else if ( strncmp(bufptr,"request:",8) == 0 ) {
			bufptr += 8;
			long size = atoi(bufptr);
			if ( size > 0 ) {
				char *read_ptr = (char*)::GlobalAlloc(GMEM_FIXED,size+1);
				fread(read_ptr,1,size,stdin);
				read_ptr[size] = 0;
				
				yaya::global_t res = request(read_ptr,&size);

				char write_header[64];
				sprintf(write_header,"request:%d\r\n",size);
				fwrite(write_header,1,strlen(write_header),stdout);

				fwrite(res,1,size,stdout);
				fflush(stdout);

				::GlobalFree(res);
			}
			else {
				const char* w = "request:0\r\n";
				fwrite(w,1,strlen(w),stdout);
			}
		}
	}

	return 0;
}

#endif //aya_make_exe
