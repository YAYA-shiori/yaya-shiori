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
	}
	dlclose(handle);
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
    return (hDLL != NULL) ? 1 : 0;
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
	if (hDLL == NULL)
		return 0;

	// アドレス取得
	if ( ! isAlreadyLoaded ) {
		if (loadlib == NULL)
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "load");
		if (loadlib == NULL)
			loadlib = (bool (*)(HGLOBAL h, long len))GetProcAddress(hDLL, "_load");
		if (loadlib == NULL)
			return 0;

		// DLLパス文字列作成
		wchar_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
		_wsplitpath(name.c_str(), drive, dir, fname, ext);
		yaya::string_t	dllpath = drive;
		dllpath += dir;

		// パス文字列をMBCSに変換
		char	*t_dllpath = Ccct::Ucs2ToMbcs(dllpath, CHARSET_DEFAULT);
		if (t_dllpath == NULL)
			return 0;

		long	len = (long)strlen(t_dllpath);

		// パス文字列をヒープにコピー
		HGLOBAL	gmem = ::GlobalAlloc(GMEM_FIXED, len);
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
    if (hDLL == NULL) {
	return 0;
    }
    
    // アドレス取得
	if (loadlib == NULL)
	 loadlib = (int(*)(char*,long))dlsym(hDLL, "load");
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
    (*loadlib)(gmem, len);
    
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
	if (hDLL == NULL)
		return 2;

	// アドレス取得
	if ( ! isAlreadyLoaded ) {
		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "unload");
		if (unloadlib == NULL)
			unloadlib = (bool (*)(void))GetProcAddress(hDLL, "_unload");
		if (unloadlib == NULL)
			return 0;

		// 実行
		(*unloadlib)();
	}

	return 1;
}
#elif defined(POSIX)
int CLib1::Unload(void) {
    if (hDLL == NULL) {
	return 2;
    }

    // アドレス取得
	if (unloadlib == NULL)
     unloadlib = (int(*)(void))dlsym(hDLL, "unload");
    if (unloadlib == NULL) {
	 return 0;
    }

    // 実行
    (*unloadlib)();
    
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib1::Release
 *  機能概要：  DLLをリリースします
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
 *  関数名  ：  CLib1::Request
 *  機能概要：  requestを実行します
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
int	CLib1::Request(const yaya::string_t &istr, yaya::string_t &ostr)
{
	ostr = L"";

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
	memcpy(igmem, t_istr, len);
	free(t_istr);
	t_istr = NULL;

	// 実行
	HGLOBAL	ogmem = (*requestlib)(igmem, &len);

	// 結果取得
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
    ostr = L"";

    if (hDLL == NULL) {
	return 0;
    }
    
    // アドレス取得
	if (requestlib == NULL)
    requestlib = (char*(*)(char*, long *))dlsym(hDLL, "request");
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
    char* ogmem = (*requestlib)(igmem, &len);

    // 結果取得
	std::string t_ostr(ogmem, len);

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
