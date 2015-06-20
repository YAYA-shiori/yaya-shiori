// 
// AYA version 5
//
// stl::yaya::string_tをchar*風に使うための関数など
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#if defined(POSIX)
# include <iomanip>
# include <sstream>
#endif
#include <string>
#include <stdarg.h>
#include <string.h>

#include "ccct.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "manifest.h"
#include "misc.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_atoi
*  機能概要：  yaya::string_tをintへ変換
* -----------------------------------------------------------------------
*/
int	yaya::ws_atoi(const yaya::string_t &str, int base)
{
	if (!str.size())
		return 0;
	
	wchar_t	*dmy;
	return (int)wcstol(str.c_str(), &dmy, base);
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_atof
*  機能概要：  yaya::string_tをdoubleへ変換
* -----------------------------------------------------------------------
*/
double	yaya::ws_atof(const yaya::string_t &str)
{
	if (!str.size())
		return 0.0;
	
	wchar_t	*dmy;
	return wcstod(str.c_str(), &dmy);
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_itoa
*  機能概要：  intをyaya::string_tへ変換
* -----------------------------------------------------------------------
*/
yaya::string_t yaya::ws_itoa(int num, int rdx)
{
	int idx;

	yaya::char_t buf[] = L"                  ";
	int offset = (sizeof(buf) / sizeof(buf[0])) - 2;
	
	const yaya::char_t convchars[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	if ( rdx < 2 ) { rdx = 2; }
	if ( rdx > 36 ) { rdx = 36; }
	
	bool minus = false;
	if ( num < 0 ) {
		minus = true;
		num = -num;
	}
	
	if ( num == 0 ) {
		buf[offset] = L'0';
		--offset;
	}
	else {
		while ( num ) {
			idx = num % rdx;
			buf[offset] = convchars[idx];
			num -= idx;
			num /= rdx;
			--offset;
		}
	}
	
	if ( minus ) {
		buf[offset] = '-';
		--offset;
	}
	
	return (buf + offset + 1);
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_ftoa
*  機能概要：  doubleをyaya::string_tへ変換
* -----------------------------------------------------------------------
*/
yaya::string_t	yaya::ws_ftoa(double num)
{
	yaya::char_t numtxt[128];
	yaya::snprintf(numtxt,64,L"%f",num);
	return numtxt;
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_eraseend
*  機能概要：  yaya::string_tの終端からcを削る
* -----------------------------------------------------------------------
*/
void	yaya::ws_eraseend(yaya::string_t &str,wchar_t c)
{
	if (!str.size())
		return;
	
	if (str[str.size() - 1] == c)
		str.erase(str.end() - 1);
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_replace
*  機能概要：  str内のbeforeをすべてafterに置換します
* -----------------------------------------------------------------------
*/
void	yaya::ws_replace(yaya::string_t &str, const wchar_t *before, const wchar_t *after, int count)
{
	if ( ! after ) { after = L""; }

	size_t sz_bef = wcslen(before);
	size_t sz_aft = wcslen(after);

	for(size_t rp_pos = 0; ; rp_pos += sz_aft) {
		rp_pos = str.find(before, rp_pos);
		if (rp_pos == yaya::string_t::npos)
			break;
		str.replace(rp_pos, sz_bef, after);
		if ( count > 0 ) {
			count -= 1;
			if ( count <= 0 ) { break; }
		}
	}
}

/* -----------------------------------------------------------------------
*  関数名  ：  w_fopen
*  機能概要：  UCS-2文字列のファイル名でオープンできるfopen
*
*  補足　wchar_t*を直接渡せる_wfopenはWin9x系未サポートのため使えないのです。無念。
* -----------------------------------------------------------------------
*/
#if defined(WIN32) || defined(_WIN32_WCE)
FILE	*yaya::w_fopen(const wchar_t *fname, const wchar_t *mode)
{
	// ファイル名とオープンモードををMBCSへ変換
	char	*mfname = Ccct::Ucs2ToMbcs(fname, CHARSET_DEFAULT);
	if (mfname == NULL)
		return NULL;
	char	*mmode  = Ccct::Ucs2ToMbcs(mode,  CHARSET_DEFAULT);
	if (mmode == NULL) {
		free(mfname);
		mfname = NULL;
		return NULL;
	}
	// オープン
	FILE	*fp = fopen(mfname, mmode);
	free(mfname);
	mfname = NULL;
	free(mmode);
	mmode = NULL;
	
	return fp;
}
#else
FILE* yaya::w_fopen(const wchar_t* fname, const wchar_t* mode) {
	std::string s_fname = narrow(yaya::string_t(fname));
	std::string s_mode = narrow(yaya::string_t(mode));
	
    fix_filepath(s_fname);
	
    return fopen(s_fname.c_str(), s_mode.c_str());
}
#endif

/* -----------------------------------------------------------------------
*  関数名  ：  write_utf8bom
*  機能概要：  UTF-8 BOMを書き込む
* -----------------------------------------------------------------------
*/
/*
void	write_utf8bom(FILE *fp)
{
fputc(0xef, fp);
fputc(0xbb, fp);
fputc(0xbf, fp);
}
*/

/* -----------------------------------------------------------------------
*  関数名  ：  decode/encodecipher
*  機能概要：  AYA暗号化された文字を復号する
*
*  ただのビット反転とかき混ぜです
* -----------------------------------------------------------------------
*/
static int decodecipher(const int c)
{
	return (((c & 0x7) << 5) | ((c & 0xf8) >> 3)) ^ 0x5a;
}

static int encodecipher(const int c)
{
	return (((c^ 0x5a) << 3) & 0xF8) | (((c^ 0x5a) >> 5) & 0x7);
}

/* -----------------------------------------------------------------------
*  関数名  ：  ws_fgets
*  機能概要：  yaya::string_tに取り出せる簡易版fgets、暗号復号とUCS-2 BOM削除も行なう
* -----------------------------------------------------------------------
*/
int yaya::ws_fgets(yaya::string_t &str, FILE *stream, int charset, int ayc, int lc, int cutspace)
{
	//ayc = 1 復号化
	//lc = 1 BOM削除
	//cutspace = 1 先頭の空白削除

	str.erase();
	std::string buf;
	buf.reserve(1000);
	int c = 1;
	
	if (ayc) {
		while (true) {
			c = fgetc(stream);
			if (c == EOF) {
				break;
			}
			c = decodecipher(c);
			buf += static_cast<char>(c);
			if (c == '\x0a') {
				// 行の終わり
				break;
			}
		}
	}
	else {
		while (true) {
			c = fgetc(stream);
			if (c == EOF) {
				break;
			}
			buf += static_cast<char>(c);
			if (c == '\x0a') {
				// 行の終わり
				break;
			}
		}
	}
	
	wchar_t *wstr_result = Ccct::MbcsToUcs2(buf, charset);
	if ( ! wstr_result ) { return 0; }

	wchar_t *wstr = wstr_result;
	if (charset == CHARSET_UTF8 && lc == 1) {
		if (wstr[0] == static_cast<yaya::char_t>(0xfeff) || wstr[0] == static_cast<yaya::char_t>(0xffef)) {
			wstr += 1;
		}
	}
	if (cutspace) {
		while (IsSpace(*wstr)) { ++wstr; }
	}
	str = wstr;

	free(wstr_result);
	wstr_result = NULL;
	
	if (c == EOF && str.empty()) {
		return yaya::WS_EOF;
	}
	else {
		return str.size();
	}
}

/* -----------------------------------------------------------------------
*  関数名  ：  ws_fputs
*  機能概要：  yaya::string_tを書き込む簡易版fputs、暗号化も行なう
* -----------------------------------------------------------------------
*/
int yaya::ws_fputs(const yaya::char_t *str, FILE *stream, int charset, int ayc)
{
	//ayc = 1 復号化
	char *str_result = Ccct::Ucs2ToMbcs(str, charset);
	if ( ! str_result ) { return 0; }

	int len = strlen(str_result);

	if (ayc) {
		char *resulttmp = str_result;
		while ( *resulttmp ) {
			*resulttmp = encodecipher(*resulttmp);
			++resulttmp;
		}
	}

	fwrite(str_result,1,len,stream);

	free(str_result);
	str_result = NULL;

	return len;
}

/* -----------------------------------------------------------------------
*  関数名  ：  snprintf
*  機能概要：  snprintf互換処理
* -----------------------------------------------------------------------
*/
int yaya::snprintf(yaya::char_t *buf,size_t count,const yaya::char_t *format,...)
{
	va_list list;
	va_start( list, format );

	int result;

#ifdef _MSC_VER
#if _MSC_VER <= 1300
	//標準非互換
	result = _vsnwprintf(buf,count,format,list);
#else
	result = vswprintf(buf,count*2,format,list);
#endif
#else
	result = vswprintf(buf,count*2,format,list);
#endif

	va_end (list);
	return result;
}
