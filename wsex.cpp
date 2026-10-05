// 
// AYA version 5
//
// stl::yaya::string_tをchar*風に使うための関数など
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif
#ifdef _MSC_VER
#if (_MSC_VER >= 1900)
#include <corecrt.h>
#endif
#endif // _MSC_AVR


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
#define wcsnicmp(s1, s2, n) wcsncasecmp(s1, s2, n)
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
*  関数名  ：  yaya::ws_atoi / ws_atoll
*  機能概要：  yaya::string_tをintへ変換
* -----------------------------------------------------------------------
*/
int	yaya::ws_atoi(const yaya::string_t &str, int base)
{
	if (!str.size())
		return 0;

	return wcstol(str.c_str(), NULL, base);
}

yaya::int_t yaya::ws_atoll(const yaya::string_t &str, int rdx_arg)
{
	yaya::int_t num = 0;
	yaya::int_t rdx = rdx_arg;
	
	if ( rdx < 2 ) { rdx = 2; }
	if ( rdx > 36 ) { rdx = 36; }

	const yaya::char_t *ptr = str.c_str();
	
	bool minus = false;
	if ( *ptr == L'-' ) {
		minus = true;
		ptr += 1;
	}
	else if ( *ptr == L'+' ) {
		ptr += 1;
	}
	else if ( wcsnicmp(ptr,L"0x",2) == 0 ) {
		ptr += 2;
		rdx = 16;
	}
	else if ( wcsnicmp(ptr,L"0b",2) == 0 ) {
		ptr += 2;
		rdx = 2;
	}

	while ( *ptr ) {
		yaya::int_t add = -1;

		if ( *ptr >= L'0' && *ptr <= L'9' ) {
			add = *ptr - L'0';
		}
		else if ( *ptr >= L'A' && *ptr <= L'Z' ) {
			add = *ptr - L'A' + 10;
		}
		else if ( *ptr >= L'a' && *ptr <= L'z' ) {
			add = *ptr - L'a' + 10;
		}

		if ( add < 0 || add >= rdx ) {
			break;
		}
		num *= rdx;
		num += add;

		ptr += 1;
	}
	
	if ( minus ) {
		return 0-num;
	}
	else {
		return num;
	}
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

	return wcstod(str.c_str(), NULL);
}

/* -----------------------------------------------------------------------
*  関数名  ：  yaya::ws_itoa
*  機能概要：  intをyaya::string_tへ変換
* -----------------------------------------------------------------------
*/
yaya::string_t yaya::ws_itoa(int num, int rdx)
{
	return ws_lltoa(static_cast<yaya::int_t>(num), rdx);
}

yaya::string_t yaya::ws_lltoa(yaya::int_t num, int rdx)
{
	int idx;

	//                     123456789012345678901234567890123456789012345678901234567890123456 //64bitmax = 64chars + (+/-) = 65
	yaya::char_t buf[] = L"                                                                  ";
	int offset = (sizeof(buf) / sizeof(buf[0])) - 2;
	
	const yaya::char_t convchars[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	if ( rdx < 2 ) { rdx = 2; }
	if ( rdx > 36 ) { rdx = 36; }
	
	// 最小値(-2^63)は符号を反転できないので、符号なしで桁を求める
	bool minus = false;
	std::uint64_t unum = static_cast<std::uint64_t>(num);
	if ( num < 0 ) {
		minus = true;
		unum = 0 - unum;
	}
	
	if ( unum == 0 ) {
		buf[offset] = L'0';
		--offset;
	}
	else {
		while ( unum ) {
			idx = static_cast<int>(unum % static_cast<std::uint64_t>(rdx));
			buf[offset] = convchars[idx];
			unum /= static_cast<std::uint64_t>(rdx);
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
	// %fはDBL_MAXで300文字を超えるので余裕を持たせる（VC6の_vsnwprintfは溢れると終端しない）
	yaya::char_t numtxt[1024];
	yaya::snprintf(numtxt,512,L"%f",num);
	numtxt[511] = 0;
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
void	yaya::ws_replace(yaya::string_t &str, const wchar_t *before, const wchar_t *after, yaya::int_t count)
{
	if ( ! after ) { after = L""; }

	size_t sz_bef = wcslen(before);
	size_t sz_aft = wcslen(after);

	// 空文字列はどこにでも見つかるので、置換すると終わらなくなる
	if ( sz_bef == 0 ) {
		return;
	}

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
FILE	*yaya::w_fopen(const yaya::char_t *fname, const yaya::char_t *mode)
{
	FILE *fp;
	if ( IsUnicodeAware() ) {
		fp = _wfopen(fname,mode);
	}
	else {
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
		fp = fopen(mfname, mmode);
		free(mfname);
		mfname = NULL;
		free(mmode);
		mmode = NULL;
	}
	
	return fp;
}
#else
FILE* yaya::w_fopen(const yaya::char_t* fname, const yaya::char_t* mode) {
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

// VC6のbasic_stringは容量を32文字ずつしか増やさず、1文字ずつ足すと長い行で2乗の時間がかかるので倍々に確保する
static void ws_fgets_append(std::string &buf, char c)
{
	if (buf.size() >= buf.capacity()) {
		buf.reserve(buf.capacity() * 2 + 32);
	}
	buf += c;
}

/* -----------------------------------------------------------------------
*  関数名  ：  ws_fgets
*  機能概要：  yaya::string_tに取り出せる簡易版fgets、暗号復号とUCS-2 BOM削除も行なう
* -----------------------------------------------------------------------
*/
int yaya::ws_fgets(std::string &buf, yaya::string_t &str, FILE *stream, int charset, int ayc, int lc, int cutspace)
{
	//ayc = 1 復号化
	//lc = 1 BOM削除
	//cutspace = 1 先頭の空白削除

	str.erase();
	buf.erase();
	int c = 1;
	
	if (ayc) {
		while (true) {
			c = fgetc(stream);
			if (c == EOF) {
				break;
			}
			c = decodecipher(c);
			ws_fgets_append(buf, static_cast<char>(c));
			if (c == '\x0a') {
				// 行の終わり
				break;
			}
		}
	}
	else {
		// 1バイトずつfgetcすると、CRTのロックを毎回取るので遅い（/MTのVC6では1行400バイトで約8μs）。
		// fgetsで塊ごとに読む。NULを含む行も壊さないよう、読む前にバッファを0xFFで埋めておき、
		// fgetsが書いた終端NULの位置（末尾から見て最初に0xFFでない所）から読めたバイト数を割り出す。
		// 0x0aで終わらないのは、バッファが満杯のとき（続きを読む）か、最後の行に改行が無いとき。
		const size_t chunk_size = 512;
		char chunk[chunk_size];
		while (true) {
			memset(chunk, 0xFF, chunk_size);
			if (fgets(chunk, static_cast<int>(chunk_size), stream) == NULL) {
				c = EOF;
				break;
			}
			size_t n = chunk_size - 1;
			while (n > 0 && static_cast<unsigned char>(chunk[n]) == 0xFFU) {
				--n;
			}
			// chunk[n] が終端NUL。読めたのは chunk[0..n)
			buf.append(chunk, n);
			if (n > 0 && chunk[n - 1] == '\x0a') {
				// 行の終わり
				c = '\x0a';
				break;
			}
		}
	}

	if ( lc == 1 && buf.length() >= 3 ) {
		if ( static_cast<unsigned char>(buf[0]) == 0xEFU &&
			 static_cast<unsigned char>(buf[1]) == 0xBBU &&
			 static_cast<unsigned char>(buf[2]) == 0xBFU ) { //UTF-8 bom
			buf.erase(0,3);
		}
	}
	
	if ( ! Ccct::MbcsToUcs2Buf(str, buf, charset) ) { return 0; }

	const wchar_t *cstr = str.c_str();
	if (cutspace) {
		while (IsSpace(*cstr)) { ++cstr; }
	}
	ptrdiff_t diff = cstr - str.c_str();
	if ( diff > 0 ) {
		str.erase(0,diff);
	}
	
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
*  関数名  ：  snprintf / format
*  機能概要：  snprintf互換処理
* -----------------------------------------------------------------------
*/
#if defined(__GNUC__)
// in g++ 12.2.0 (Debian 12.2.0-14)
//wsex.h:46:131: error: ‘format’ attribute argument 2 value ‘3’ refers to parameter type ‘const yaya::char_t*’ {aka ‘const wchar_t*’}
//int yaya::snprintf(yaya::char_t* buf, size_t count, const yaya::char_t* format, ...)__attribute__((format(printf, 3, 4)))
int yaya::snprintf(yaya::char_t* buf, size_t count, const yaya::char_t* format, ...)
#elif defined(_MSC_VER)
int yaya::snprintf(_Pre_notnull_ yaya::char_t *buf,size_t count, _Printf_format_string_ const yaya::char_t *format,...)
#else
int yaya::snprintf(yaya::char_t* buf, size_t count, const yaya::char_t* format, ...)
#endif
{
	va_list list;
	va_start( list, format );

	int result;

#ifdef _MSC_VER
#if _MSC_VER <= 1300
	//標準非互換
	result = _vsnwprintf(buf,count,format,list);
#else
	result = vswprintf(buf,count,format,list);
#endif
#else
	result = vswprintf(buf,count,format,list);
#endif

	va_end (list);

	// 入りきらなかったときも必ず終端する（入りきったときはvswprintfが終端済み）
	if (count && (result < 0 || (size_t)result >= count)) {
		buf[count - 1] = L'\0';
	}
	return result;
}
