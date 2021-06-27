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
	std::string name;
	bool isdir = false;

	while ( true ) {
		if ( ! is_init ) {
#if defined(WIN32)

			WIN32_FIND_DATA w32FindData;

			yaya::string_t tmp_str = enumpath + L"\\*.*";
			char *s_filestr = Ccct::Ucs2ToMbcs(tmp_str, CHARSET_DEFAULT);
			if ( ! s_filestr ) { return false; }

			dh = ::FindFirstFile(s_filestr,&w32FindData);
			free(s_filestr);
			s_filestr = NULL;

			if ( dh == INVALID_HANDLE_VALUE ) { return false; }

			is_init = true;

			name = w32FindData.cFileName;
			isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#elif defined(POSIX)

			std::string path = narrow(enumpath);
			fix_filepath(path);

			dh = opendir(path.c_str());
			if ( ! dh ) { return false; }

			struct dirent* ent = readdir(dh);
			if ( ! ent ) { closedir(dh); return false; }

			is_init = true;
	
			name = ent->d_name;
			isdir = ent->d_type == DT_DIR;

#endif
		}
		else {
#if defined(WIN32)

			WIN32_FIND_DATA w32FindData;
			if ( ::FindNextFile(dh,&w32FindData) == 0 ) { return false; }

			name = w32FindData.cFileName;
			isdir = (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#elif defined(POSIX)

			struct dirent* ent = readdir(dh);
			if ( ! ent ) { return false; }

			name = ent->d_name;
			isdir = ent->d_type == DT_DIR;

#endif
		}

		if (name != "." && name != "..") {
			break;
		}
	}

#if defined(WIN32)
	yaya::char_t *t_wfile = Ccct::MbcsToUcs2(name, CHARSET_DEFAULT);
	if (! t_wfile ) { return false; }

	entry.name = t_wfile;
	free(t_wfile);
#elif defined(POSIX)
	entry.name = widen(name);
#endif

	entry.isdir = isdir;

	return true;
}


