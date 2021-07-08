// 
// AYA version 5
//
// stl::wstringをchar*風に使うための関数など
// written by umeici. 2004
// 

#ifndef	WSEXH
#define	WSEXH

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <cstdio>

#include "globaldef.h"

//----

namespace aya {
	static const int WS_EOF = -1;

	int		ws_atoi(const aya::string_t &str, int base = 10);
	double	ws_atof(const aya::string_t &str);

	aya::string_t	ws_itoa(int num, int base = 10);
	aya::string_t	ws_ftoa(double num);

	void	ws_eraseend(aya::string_t &str, wchar_t c);
	void	ws_replace(aya::string_t &str, const wchar_t *before, const wchar_t *after, int count = 0);

	FILE	*w_fopen(const wchar_t *fname, const wchar_t *mode); 
	//void	write_utf8bom(FILE *fp);

	int		ws_fgets(aya::string_t &str, FILE *stream, int charset, int ayc, int lc, int cutspace = true);
	
	int		ws_fputs(const aya::char_t *str, FILE *stream, int charset, int ayc);
	inline int ws_fputs(const aya::string_t &str, FILE *stream, int charset, int ayc) {
		return ws_fputs(str.c_str(),stream,charset,ayc);
	}

	int snprintf(aya::char_t *buf,size_t count,const aya::char_t *format,...);
};

//----

#endif
