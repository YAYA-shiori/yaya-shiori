// 
// AYA version 5
//
// ディレクトリ内列挙　CDirEnum
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <list>
#include <algorithm>

#include "misc.h"
#include "globaldef.h"
#include "dir_enum.h"
#include "ccct.h"
#include "manifest.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


CDirEnum::CDirEnum(const yaya::string_t &ep)
{
	enumpath = ep;
	is_init = false;
}

CDirEnum::~CDirEnum()
{
	if ( is_init ) {
#if defined(WIN32)
		::FindClose(dh);
#elif defined(POSIX)
		closedir(dh);
#endif
	}
}

bool CDirEnum::next(CDirEnumEntry &entry)
{
	std::string name_a;
	yaya::string_t name_w;
	bool isdir = false;

	bool isUnicode = false;

#if defined(WIN32)
	isUnicode = IsUnicodeAware();
#endif

	while ( true ) {
		if ( ! is_init ) {
#if defined(WIN32)
			yaya::string_t tmp_str = enumpath + L"\\*.*";

			if ( isUnicode ) {
				WIN32_FIND_DATAW w32FindData;

				dh = ::FindFirstFileW(tmp_str.c_str(),&w32FindData);

				if ( dh == INVALID_HANDLE_VALUE ) { return false; }

				is_init = true;

				name_w = w32FindData.cFileName;
				isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}
			else {
				WIN32_FIND_DATAA w32FindData;

				char *s_filestr = Ccct::Ucs2ToMbcs(tmp_str, CHARSET_DEFAULT);
				if ( ! s_filestr ) { return false; }

				dh = ::FindFirstFileA(s_filestr,&w32FindData);
				free(s_filestr);
				s_filestr = NULL;

				if ( dh == INVALID_HANDLE_VALUE ) { return false; }

				is_init = true;

				name_a = w32FindData.cFileName;
				isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}

#elif defined(POSIX)

			std::string path = narrow(enumpath);
			fix_filepath(path);

			dh = opendir(path.c_str());
			if ( ! dh ) { return false; }

			struct dirent* ent = readdir(dh);
			if ( ! ent ) { closedir(dh); return false; }

			is_init = true;
	
			name_a = ent->d_name;
			isdir = ent->d_type == DT_DIR;

#endif
		}
		else {
#if defined(WIN32)

			if ( isUnicode ) {
				WIN32_FIND_DATAW w32FindData;
				if ( ::FindNextFileW(dh,&w32FindData) == 0 ) { return false; }

				name_w = w32FindData.cFileName;
				isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}
			else {
				WIN32_FIND_DATAA w32FindData;
				if ( ::FindNextFileA(dh,&w32FindData) == 0 ) { return false; }

				name_a = w32FindData.cFileName;
				isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}

#elif defined(POSIX)

			struct dirent* ent = readdir(dh);
			if ( ! ent ) { return false; }

			name_a = ent->d_name;
			isdir = ent->d_type == DT_DIR;

#endif
		}

		if ( isUnicode ) {
			if (name_w != L"." && name_w != L"..") {
				break;
			}
		}
		else {
			if (name_a != "." && name_a != "..") {
				break;
			}
		}
	}

#if defined(WIN32)
	if ( isUnicode ) {
		entry.name = name_w;
	}
	else {
		Ccct::MbcsToUcs2Buf(entry.name, name_a, CHARSET_DEFAULT);
	}
#elif defined(POSIX)
	entry.name = widen(name_a);
#endif

	entry.isdir = isdir;

	return true;
}


