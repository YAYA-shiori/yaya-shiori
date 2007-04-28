// 
// AYA version 5
//
// stl::wstring‚ğchar*•—‚Ég‚¤‚½‚ß‚ÌŠÖ”‚È‚Ç
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

#define	WS_MAXLEN	1024
#define	WS_EOF		-1

int		ws_atoi(const yaya::string_t &str, int base);
double	ws_atof(const yaya::string_t &str);
void	ws_itoa(yaya::string_t &str, int num, int base);
void	ws_ftoa(yaya::string_t &str, double num);
void	ws_eraseend(yaya::string_t &str, wchar_t c);
void	ws_replace(yaya::string_t &str, const wchar_t *before, const wchar_t *after);

FILE	*w_fopen(const wchar_t *fname, const wchar_t *mode); 
//void	write_utf8bom(FILE *fp);
int		ws_fgets(yaya::string_t &str, FILE *stream, int charset, int ayc, int lc, int cutspace = true);
int		ws_fputs(const yaya::char_t *str, FILE *stream, int charset, int ayc);
inline int ws_fputs(const yaya::string_t &str, FILE *stream, int charset, int ayc) {
	return ws_fputs(str.c_str(),stream,charset,ayc);
}

//----

#endif
