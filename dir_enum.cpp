// 
// AYA version 5
//
// ディレクトリ内列挙　CDirEnum
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#elif defined(POSIX)
# include "posix_utils.h"
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
#if defined(WIN32)
	yaya::string_t name_w;
#elif defined(POSIX)
	std::string name_a;
#endif
	bool isdir = false;

	while ( true ) {
		if ( ! is_init ) {
#if defined(WIN32)
			yaya::string_t tmp_str = enumpath + L"\\*.*";

			WIN32_FIND_DATAW w32FindData;

			dh = ::FindFirstFileW(tmp_str.c_str(),&w32FindData);

			if ( dh == INVALID_HANDLE_VALUE ) { return false; }

			is_init = true;

			name_w = w32FindData.cFileName;
			isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

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

			WIN32_FIND_DATAW w32FindData;
			if ( ::FindNextFileW(dh,&w32FindData) == 0 ) { return false; }

			name_w = w32FindData.cFileName;
			isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#elif defined(POSIX)

			struct dirent* ent = readdir(dh);
			if ( ! ent ) { return false; }

			name_a = ent->d_name;
			isdir = ent->d_type == DT_DIR;

#endif
		}

#if defined(WIN32)
		if (name_w != L"." && name_w != L"..") {
			break;
		}
#elif defined(POSIX)
		if (name_a != "." && name_a != "..") {
			break;
		}
#endif
	}

#if defined(WIN32)
	entry.name = name_w;
#elif defined(POSIX)
	entry.name = widen(name_a);
#endif

	entry.isdir = isdir;

	return true;
}


