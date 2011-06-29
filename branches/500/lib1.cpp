// 
// AYA version 5
//
// 1��DLL�������N���X�@CLib1
// written by umeici. 2004
// 

#if defined(WIN32)
# include "stdafx.h"
#endif

#include <string>
#include <vector>

#if defined(POSIX)
# include <dlfcn.h>
# include <sys/stat.h>
#endif
#include <string.h>

#include "lib.h"

#include "ayavm.h"

#include "aya5.h"
#include "ccct.h"
#include "log.h"
#include "manifest.h"
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
// dll�T�[�`�p�X�֘A�B
// POSIX��ł́A�������̏ꏊ��DLL�Ɠ����̃��C�u������u������SAORI�ɑΉ�����B
// DLL�Ɠ����Ƃ͉]���Ă��A����̓V���{���b�N�����N�ł���ׂ��ŁA�Ⴆ�Ύ��̂悤�ɂł���B
// % pwd
// /home/foo/.saori
// % ls -l
// -rwxr-xr-x x foo bar xxxxx 1 1 00:00 libssu.so
// lrwxr-xr-x x foo bar xxxxx 1 1 00:00 ssu.dll -> libssu.so
//
// �p�X�͊��ϐ� SAORI_FALLBACK_PATH ����擾����B����̓R������؂�̐�΃p�X�ł���B
static std::vector<std::string> posix_dll_search_path;
static bool posix_dll_search_path_is_ready = false;
static std::string posix_search_fallback_dll(const std::string& dllfile) {
    // dllfile�͒T�������t�@�C��DLL���B�p�X��؂蕶����/�B
    // ��փ��C�u���������t����΂��̐�΃p�X���A
    // ���t�����Ȃ���΋󕶎����Ԃ��B
    
    if (!posix_dll_search_path_is_ready) {
	// SAORI_FALLBACK_PATH������B
		std::string path = str_getenv("SAORI_FALLBACK_PATH");
        if (path.length() > 0) {
            while (true) {
				std::string::size_type colon_pos = path.find(':');
                if (colon_pos == std::string::npos) {
                    posix_dll_search_path.push_back(path);
                    break;
                }
                else {
                    posix_dll_search_path.push_back(path.substr(0, colon_pos));
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
	    // ��փ��C�u���������݂���悤���B����ȏ�̃`�F�b�N�͏ȗ��B
	    return fpath;
	}
    }
    return std::string();
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib1::LoadLib
 *  �@�\�T�v�F  DLL�����[�h���܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����(���Ƀ��[�h����Ă���܂�)
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

	isAlreadyLoaded = ::GetModuleHandle(dllpathname) != NULL;

	hDLL = ::LoadLibrary(dllpathname);
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

    // ���ϐ� SAORI_FALLBACK_ALWAYS ����`����Ă��āA����
    // ��ł�"0"�ł��Ȃ���΁A����dll�t�@�C�����J���Ă݂鎖��
    // ���߂�����Ȃ��B�����łȂ���΁A������dlopen���Ă݂�B
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
	    // load, unload, request����o���Ă݂�B
	    void* sym_load = dlsym(handle, "load");
	    void* sym_unload = dlsym(handle, "unload");
	    void* sym_request = dlsym(handle, "request");
	    if (sym_load != NULL && sym_unload != NULL && sym_request != NULL) {
		// �Ȃ�Ɛ���ɓǂ߂��B
		do_fallback = false;
	    }
	}
	dlclose(handle);
    }
    if (do_fallback) {
	// ��փ��C�u������T���B
		std::string fallback_lib = posix_search_fallback_dll(get_fname(libfile));
	if (fallback_lib.length() == 0) {
	    // �����B
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
    return (hDLL != NULL) ? 1 : 0;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib1::Load
 *  �@�\�T�v�F  load�����s���܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Load(void)
{
	if (hDLL == NULL)
		return 0;

	// �A�h���X�擾
	if ( ! isAlreadyLoaded ) {
		if (loadlib == NULL)
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "load");
		if (loadlib == NULL)
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "_load");
		if (loadlib == NULL)
			return 0;

		// DLL�p�X������쐬
		wchar_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
		_wsplitpath(name.c_str(), drive, dir, fname, ext);
		yaya::string_t	dllpath = drive;
		dllpath += dir;

		// �p�X�������MBCS�ɕϊ�
		char	*t_dllpath = Ccct::Ucs2ToMbcs(dllpath, CHARSET_DEFAULT);
		if (t_dllpath == NULL)
			return 0;

		long	len = (long)strlen(t_dllpath);

		// �p�X��������q�[�v�ɃR�s�[
		HGLOBAL	gmem = ::GlobalAlloc(GMEM_FIXED, len);
		memcpy(gmem, t_dllpath, len);
		free(t_dllpath);
		t_dllpath = NULL;

		// ���s
		(*loadlib)(gmem, len);
	}
	
	return 1;
}
#elif defined(POSIX)
int CLib1::Load(void) {
    if (hDLL == NULL) {
	return 0;
    }
    
    // �A�h���X�擾
	if (loadlib == NULL)
	 loadlib = (int(*)(char*,long))dlsym(hDLL, "load");
    if (loadlib == NULL) {
	 return 0;
    }
    
    // DLL�p�X������쐬
	yaya::string_t::size_type pos_slash = name.rfind(L'/');
	std::string dllpath;
    if (pos_slash == yaya::string_t::npos) {
		dllpath = ".";
    }
    else {
		dllpath = narrow(name.substr(0, pos_slash+1));
    }

    long len = dllpath.length();

    // �p�X��������q�[�v�ɃR�s�[
    char* gmem = static_cast<char*>(malloc(len));
    memcpy(gmem, dllpath.c_str(), len);

    // ���s
    (*loadlib)(gmem, len);
    
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib1::Unload
 *  �@�\�T�v�F  unload�����s���܂�
 *
 *  �Ԓl�@�@�F�@0/1/2=���s/����/���[�h����Ă��Ȃ��A�������͊���unload����Ă���
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Unload(void)
{
	if (hDLL == NULL)
		return 2;

	// �A�h���X�擾
	if ( ! isAlreadyLoaded ) {
		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "unload");
		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "_unload");
		if (unloadlib == NULL)
			return 0;

		// ���s
		(*unloadlib)();
	}

	return 1;
}
#elif defined(POSIX)
int CLib1::Unload(void) {
    if (hDLL == NULL) {
	return 2;
    }

    // �A�h���X�擾
	if (unloadlib == NULL)
     unloadlib = (int(*)(void))dlsym(hDLL, "unload");
    if (unloadlib == NULL) {
	 return 0;
    }

    // ���s
    (*unloadlib)();
    
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib1::Release
 *  �@�\�T�v�F  DLL�������[�X���܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CLib1::Release(void)
{
	if (hDLL == NULL)
		return;

	loadlib = NULL;
	unloadlib = NULL;
	requestlib = NULL;

	FreeLibrary(hDLL);
	hDLL = NULL;
}
#elif defined(POSIX)
void CLib1::Release(void) {
    if (hDLL == NULL) {
	return;
    }

    dlclose(hDLL);
    hDLL = NULL;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib1::Request
 *  �@�\�T�v�F  request�����s���܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Request(const yaya::string_t &istr, yaya::string_t &ostr)
{
	ostr = L"";

	if (hDLL == NULL)
		return 0;

	// �A�h���X�擾
	if (requestlib == NULL)
		requestlib = (HGLOBAL (*)(HGLOBAL h, long *len))GetProcAddress(hDLL, "request");
	if (requestlib == NULL)
		requestlib = (HGLOBAL (*)(HGLOBAL h, long *len))GetProcAddress(hDLL, "_request");
	if (requestlib == NULL)
		return 0;

	// ��������}���`�o�C�g�����R�[�h�ɕϊ�
	char	*t_istr = Ccct::Ucs2ToMbcs(istr, charset);
	if (t_istr == NULL)
		return 0;

	long	len = (long)strlen(t_istr);

	// �p�X��������q�[�v�ɃR�s�[
	HGLOBAL	igmem = ::GlobalAlloc(GMEM_FIXED, len);
	memcpy(igmem, t_istr, len);
	free(t_istr);
	t_istr = NULL;

	// ���s
	HGLOBAL	ogmem = (*requestlib)(igmem, &len);

	// ���ʎ擾
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

	// ���ʂ�UCS-2�֕ϊ�
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
    ostr = L"";

    if (hDLL == NULL) {
	return 0;
    }
    
    // �A�h���X�擾
	if (requestlib == NULL)
    requestlib = (char*(*)(char*, long *))dlsym(hDLL, "request");
    if (requestlib == NULL) {
	return 0;
    }
    
    // ��������}���`�o�C�g�����R�[�h�ɕϊ�
    char *t_istr = Ccct::Ucs2ToMbcs(istr, charset);
    if (t_istr == NULL) {
	return 0;
    }

    long len = (long)strlen(t_istr);

    // �p�X��������q�[�v�ɃR�s�[
    char* igmem = static_cast<char*>(malloc(len));
    memcpy(igmem, t_istr, len);
    free(t_istr);
	t_istr = NULL;

    // ���s
    char* ogmem = (*requestlib)(igmem, &len);

    // ���ʎ擾
	std::string t_ostr(ogmem, len);

    // ���ʂ�UCS-2�֕ϊ�
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
