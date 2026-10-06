// 
// AYA version 5
//
// 文字コード変換クラス　Ccct
//
// 変換部分のコードは以下のサイトで公開されているものを利用しております。
// class CUnicodeF
// kamoland
// http://kamoland.com/comp/unicode.html
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string.h>

#include <stdlib.h>
#include <wctype.h>
#include <clocale>
#include <string>

#include "ccct.h"
#include "manifest.h"
#include "globaldef.h"
//#include "babel/babel.h"

#ifdef POSIX
#  include <ctype.h>
#  include <errno.h>
#  include <iconv.h>
#  include <locale.h>
#  if defined(__APPLE__) || defined(__FreeBSD__)
#    include <xlocale.h>
#  endif
//https://learn.microsoft.com/ja-jp/windows/win32/winprog/windows-data-types
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#endif

/*
#define PRIMARYLANGID(lgid)    ((WORD)(lgid) & 0x3ff)
*/

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////




/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::CheckCharset
 *  機能概要：  Charset IDのチェック
 * -----------------------------------------------------------------------
 */
bool     Ccct::CheckInvalidCharset(int charset)
{
	if (charset != CHARSET_SJIS &&
		charset != CHARSET_UTF8 &&
		charset != CHARSET_EUCJP &&
		charset != CHARSET_BIG5 &&
		charset != CHARSET_GB2312 &&
		charset != CHARSET_EUCKR &&
		charset != CHARSET_JIS &&
		charset != CHARSET_BINARY &&
		charset != CHARSET_DEFAULT) {
		return true;
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  文字コードの名前の対照表
 *
 *  名前は英小文字にし、'-' '_' '.' 空白を除いてから比べます（"Shift-JIS" と "shift_jis" は同じ）。
 *  表の名前もその形で書きます。空の名前は従来どおりOSデフォルトとします。
 * -----------------------------------------------------------------------
 */
namespace {
	struct CharsetNameEntry {
		const char *name;
		int charset;
	};

	const CharsetNameEntry charset_name_table[] = {
		{ "utf8",           CHARSET_UTF8 },
		{ "utf8n",          CHARSET_UTF8 },
		{ "cp65001",        CHARSET_UTF8 },
		{ "csutf8",         CHARSET_UTF8 },

		{ "",               CHARSET_DEFAULT },
		{ "default",        CHARSET_DEFAULT },
		{ "osnative",       CHARSET_DEFAULT },
		{ "ansi",           CHARSET_DEFAULT },

		{ "shiftjis",       CHARSET_SJIS },
		{ "sjis",           CHARSET_SJIS },
		{ "xsjis",          CHARSET_SJIS },
		{ "mskanji",        CHARSET_SJIS },
		{ "csshiftjis",     CHARSET_SJIS },
		{ "cp932",          CHARSET_SJIS },
		{ "ms932",          CHARSET_SJIS },
		{ "windows932",     CHARSET_SJIS },
		{ "windows31j",     CHARSET_SJIS },
		{ "cswindows31j",   CHARSET_SJIS },
		{ "xmscp932",       CHARSET_SJIS },

		{ "eucjp",          CHARSET_EUCJP },
		{ "xeucjp",         CHARSET_EUCJP },
		{ "ujis",           CHARSET_EUCJP },
		{ "cseucpkdfmtjapanese", CHARSET_EUCJP },
		{ "cp20932",        CHARSET_EUCJP },
		{ "cp51932",        CHARSET_EUCJP },
		{ "eucjpms",        CHARSET_EUCJP },

		{ "iso2022jp",      CHARSET_JIS },
		{ "jis",            CHARSET_JIS },
		{ "csiso2022jp",    CHARSET_JIS },
		{ "cp50220",        CHARSET_JIS },
		{ "cp50221",        CHARSET_JIS },
		{ "cp50222",        CHARSET_JIS },

		{ "big5",           CHARSET_BIG5 },
		{ "csbig5",         CHARSET_BIG5 },
		{ "cp950",          CHARSET_BIG5 },
		{ "ms950",          CHARSET_BIG5 },
		{ "windows950",     CHARSET_BIG5 },

		{ "gb2312",         CHARSET_GB2312 },
		{ "csgb2312",       CHARSET_GB2312 },
		{ "euccn",          CHARSET_GB2312 },
		{ "xeuccn",         CHARSET_GB2312 },
		{ "gbk",            CHARSET_GB2312 },
		{ "cp936",          CHARSET_GB2312 },
		{ "ms936",          CHARSET_GB2312 },
		{ "windows936",     CHARSET_GB2312 },

		{ "euckr",          CHARSET_EUCKR },
		{ "cseuckr",        CHARSET_EUCKR },
		{ "ksc5601",        CHARSET_EUCKR },
		{ "ksc56011987",    CHARSET_EUCKR },
		{ "uhc",            CHARSET_EUCKR },
		{ "cp949",          CHARSET_EUCKR },
		{ "ms949",          CHARSET_EUCKR },
		{ "windows949",     CHARSET_EUCKR },

		{ "binary",         CHARSET_BINARY },
	};

	template<class C>
	int CharsetNameToID(const C *ctxt)
	{
		std::string name;
		for ( ; *ctxt; ++ctxt) {
			unsigned long c = static_cast<unsigned long>(*ctxt);
			if (c >= 0x80) {
				return -1;
			}
			if (c == '-' || c == '_' || c == '.' || c == ' ' || c == '\t') {
				continue;
			}
			if (c >= 'A' && c <= 'Z') {
				c = c - 'A' + 'a';
			}
			name += static_cast<char>(c);
		}

		for (size_t i = 0; i < sizeof(charset_name_table) / sizeof(charset_name_table[0]); ++i) {
			if (name == charset_name_table[i].name) {
				return charset_name_table[i].charset;
			}
		}
		return -1;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::CharsetTextToID
 *  機能概要：  Charset 文字列->Charset ID
 *
 *  不明な名前はCHARSET_DEFAULTになります（設定ファイルなど、エラーにできない所で使います）
 * -----------------------------------------------------------------------
 */
int      Ccct::CharsetTextToID(const wchar_t *ctxt)
{
	int charset = CharsetNameToID(ctxt);
	return (charset < 0) ? CHARSET_DEFAULT : charset;
}

int      Ccct::CharsetTextToID(const char *ctxt)
{
	int charset = CharsetNameToID(ctxt);
	return (charset < 0) ? CHARSET_DEFAULT : charset;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::CharsetTextToIDStrict
 *  機能概要：  Charset 文字列->Charset ID　不明な名前は-1を返します
 * -----------------------------------------------------------------------
 */
int      Ccct::CharsetTextToIDStrict(const wchar_t *ctxt)
{
	return CharsetNameToID(ctxt);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::CharsetIDToText(A/W)
 *  機能概要：  Charset 文字列->Charset ID
 * -----------------------------------------------------------------------
 */
const wchar_t *Ccct::CharsetIDToTextW(const int charset)
{
	if ( charset == CHARSET_UTF8 ) {
		return L"UTF-8";
	}
	if ( charset == CHARSET_SJIS ) {
		return L"Shift_JIS";
	}
	if ( charset == CHARSET_EUCJP ) {
		return L"EUC_JP";
	}
	if ( charset == CHARSET_JIS ) {
		return L"ISO-2022-JP";
	}
	if ( charset == CHARSET_BIG5 ) {
		return L"BIG5";
	}
	if ( charset == CHARSET_GB2312 ) {
		return L"GB2312";
	}
	if ( charset == CHARSET_EUCKR ) {
		return L"EUC_KR";
	}
	if ( charset == CHARSET_BINARY ) {
		return L"binary";
	}
	return L"default";
}
const char *Ccct::CharsetIDToTextA(const int charset)
{
	if ( charset == CHARSET_UTF8 ) {
		return "UTF-8";
	}
	if ( charset == CHARSET_SJIS ) {
		return "Shift_JIS";
	}
	if ( charset == CHARSET_EUCJP ) {
		return "EUC_JP";
	}
	if ( charset == CHARSET_JIS ) {
		return "ISO-2022-JP";
	}
	if ( charset == CHARSET_BIG5 ) {
		return "BIG5";
	}
	if ( charset == CHARSET_GB2312 ) {
		return "GB2312";
	}
	if ( charset == CHARSET_EUCKR ) {
		return "EUC_KR";
	}
	if ( charset == CHARSET_BINARY ) {
		return "binary";
	}
	return "default";
}

/* -----------------------------------------------------------------------
 *  UTF-8変換用先行宣言
 * -----------------------------------------------------------------------
 */
size_t Ccct_ConvUTF8ToUnicode(yaya::string_t &buf,const char* pStrIn);
size_t Ccct_ConvUnicodeToUTF8(std::string &buf,const yaya::char_t *pStrw);

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::Ucs2ToMbcs
 *  機能概要：  UTF-16BE -> MBCS へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
static char* string_to_malloc(const std::string &str)
{
	char* pch = (char*)malloc(str.length()+1);
	memcpy(pch,str.c_str(),str.length()+1);
	return pch;
}

char	*Ccct::Ucs2ToMbcs(const yaya::char_t *wstr, int charset)
{
	return Ucs2ToMbcs(yaya::string_t(wstr), charset);
}

//----

char	*Ccct::Ucs2ToMbcs(const yaya::string_t &wstr, int charset)
{
	/*if ( charset == CHARSET_UTF8 ) {
		return string_to_malloc(babel::unicode_to_utf8(wstr));
	}
	else if ( charset == CHARSET_SJIS ) {
		return string_to_malloc(babel::unicode_to_sjis(wstr));
	}
	else if ( charset == CHARSET_EUCJP ) {
		return string_to_malloc(babel::unicode_to_euc(wstr));
	}
	else if ( charset == CHARSET_JIS ) {
		return string_to_malloc(babel::unicode_to_jis(wstr));
	}*/
	if ( charset == CHARSET_UTF8 ) {
		std::string buf;
		Ccct_ConvUnicodeToUTF8(buf,wstr.c_str());
		return string_to_malloc(buf);
	}
	else {
		return utf16be_to_mbcs(wstr.c_str(),charset);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::Ucs2ToPlainASCII
 *  機能概要：  UTF-16BEからASCII std::string へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
std::string Ccct::Ucs2ToPlainASCII(const yaya::string_t &wstr)
{
	std::string str;
	for ( size_t i = 0 ; i < wstr.length() ; ++i ) {
		char c = (char)(wstr[i] & 0x7FU);
		if ( c >= 0x20U && c <= 0x7EU ) {
			str += c;
		}
	}
	return str;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::MbcsToUcs2
 *  機能概要：  MBCS -> UTF-16BE へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
static yaya::char_t* wstring_to_malloc(const yaya::string_t &str)
{
	size_t sz = (str.length()+1) * sizeof(yaya::char_t);
	yaya::char_t* pch = (yaya::char_t*)malloc(sz);
	memcpy(pch,str.c_str(),sz);
	return pch;
}

yaya::char_t	*Ccct::MbcsToUcs2(const char *mstr, int charset)
{
	if ( charset == CHARSET_UTF8 ) {
		yaya::string_t buf;
		buf.reserve(1000);
		Ccct_ConvUTF8ToUnicode(buf,mstr);
		return wstring_to_malloc(buf);
	}
	else {
		return mbcs_to_utf16be(mstr,charset);
	}
}

//----

yaya::char_t	*Ccct::MbcsToUcs2(const std::string &mstr, int charset)
{
	return MbcsToUcs2(mstr.c_str(), charset);
}

//----

bool Ccct::MbcsToUcs2Buf(yaya::string_t &out, const char *mstr, int charset)
{
	if ( charset == CHARSET_UTF8 ) {
		out.erase();
		Ccct_ConvUTF8ToUnicode(out,mstr);
		return true;
	}
	else {
		yaya::char_t *p = mbcs_to_utf16be(mstr,charset);
		if ( p ) {
			out = p;
			free(p);
			return true;
		}
		else {
			out.erase();
			return false;
		}
	}
}

//----

bool Ccct::MbcsToUcs2Buf(yaya::string_t &out, const std::string &mstr, int charset)
{
	return MbcsToUcs2Buf(out, mstr.c_str(), charset);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::sys_setlocale
 *  機能概要：  OSデフォルトの言語IDでロケール設定する
 * -----------------------------------------------------------------------
 */
char *Ccct::sys_setlocale(int category)
{
	return setlocale(category,"");
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::ccct_getcodepage
 *  機能概要：  言語ID->Windows CP
 * -----------------------------------------------------------------------
 */
unsigned int Ccct::ccct_getcodepage(int charset)
{
	if (charset == CHARSET_SJIS) {
		return 932;
	}
	else if (charset == CHARSET_EUCJP) {
		return 20932;
	}
	else if (charset == CHARSET_BIG5) {
		return 950;
	}
	else if (charset == CHARSET_GB2312) {
		return 936;
	}
	else if (charset == CHARSET_EUCKR) {
		return 949;
	}
	else if (charset == CHARSET_JIS) {
		return 50222;
	}
	else {
#if defined(WIN32) || defined(_WIN32_WCE)
		return ::AreFileApisANSI() ? ::GetACP() : ::GetOEMCP();
#else
		return 0;
#endif
	}
}

/* -----------------------------------------------------------------------
 *  ロケール名の整形
 *
 *  "ja_JP.UTF-8" "ja-JP@euro" のような名前から、言語タグ（"ja-JP"）にあたる部分だけを取り出す。
 *  ASCII以外が混ざっていたらfalse。空文字列は空のタグ（OSのユーザー既定）になる。
 * -----------------------------------------------------------------------
 */
namespace {
	bool NormalizeLocaleTag(const char *locale, std::string &tag)
	{
		tag.erase();
		if ( ! locale ) {
			return true;
		}
		for ( ; *locale && *locale != '.' && *locale != '@'; ++locale ) {
			unsigned char c = static_cast<unsigned char>(*locale);
			if ( c >= 0x80 ) {
				return false;
			}
			tag += (c == '_') ? '-' : static_cast<char>(c);
		}
		return true;
	}

	yaya::string_t AsciiToString(const std::string &s)
	{
		yaya::string_t r;
		for ( std::string::size_type i = 0; i < s.size(); ++i ) {
			r += static_cast<yaya::char_t>(static_cast<unsigned char>(s[i]));
		}
		return r;
	}
}

#if defined(WIN32) || defined(_WIN32_WCE)

#ifndef LOCALE_SISO639LANGNAME
# define LOCALE_SISO639LANGNAME 0x00000059
#endif
#ifndef LOCALE_SISO3166CTRYNAME
# define LOCALE_SISO3166CTRYNAME 0x0000005A
#endif

namespace {
	// 古いSDKにも無いものは、実行時にkernel32から探す（Vista以降）
	typedef int (WINAPI *LCMapStringExFn)(LPCWSTR, DWORD, LPCWSTR, int, LPWSTR, int, void *, void *, LPARAM);
	typedef LCID (WINAPI *LocaleNameToLCIDFn)(LPCWSTR, DWORD);
	typedef int (WINAPI *GetUserDefaultLocaleNameFn)(LPWSTR, int);
	typedef int (WINAPI *LCIDToLocaleNameFn)(LCID, LPWSTR, int, DWORD);
	typedef LANGID (WINAPI *GetUserDefaultUILanguageFn)(void);

	const DWORD YAYA_LCMAP_LOWERCASE = 0x00000100;
	const DWORD YAYA_LCMAP_UPPERCASE = 0x00000200;
	const DWORD YAYA_LCMAP_LINGUISTIC_CASING = 0x01000000;

	FARPROC Kernel32Proc(const char *name)
	{
		HMODULE k32 = ::GetModuleHandleA("kernel32");
		return k32 ? ::GetProcAddress(k32, name) : NULL;
	}

	// ロケールIDから"ja-JP"のようなタグを得る
	yaya::string_t LcidToTag(LCID lcid)
	{
		static const LCIDToLocaleNameFn pLCIDToLocaleName = reinterpret_cast<LCIDToLocaleNameFn>(Kernel32Proc("LCIDToLocaleName"));

		wchar_t buf[96];
		if ( pLCIDToLocaleName ) {
			int n = pLCIDToLocaleName(lcid, buf, 96, 0);
			if ( n > 1 ) {
				return yaya::string_t(buf);
			}
		}

		// XPなど：ISO 639の言語名とISO 3166の国名を"-"でつなぐ
		wchar_t lang[16] = L"";
		wchar_t ctry[16] = L"";
		if ( ::GetLocaleInfoW(lcid, LOCALE_SISO639LANGNAME, lang, 16) <= 1 ) {
			return yaya::string_t();
		}
		yaya::string_t tag(lang);
		if ( ::GetLocaleInfoW(lcid, LOCALE_SISO3166CTRYNAME, ctry, 16) > 1 ) {
			tag += L'-';
			tag += ctry;
		}
		return tag;
	}

	// OSが知っているロケール名か。構文が正しければ"xx-ZZ"のような名前も有効扱いになるが、
	// 知らない名前はLOCALE_CUSTOM_UNSPECIFIED(0x1000)のロケールIDになるので、それで見分ける
	bool IsKnownLocaleName(const wchar_t *name)
	{
		static const LocaleNameToLCIDFn pLocaleNameToLCID = reinterpret_cast<LocaleNameToLCIDFn>(Kernel32Proc("LocaleNameToLCID"));
		if ( ! pLocaleNameToLCID ) {
			return true; // 調べられないときは、LCMapStringExの結果に任せる
		}
		LCID lcid = pLocaleNameToLCID(name, 0);
		return lcid != 0 && lcid != 0x1000;
	}

	// CRTのロケール名（"Japanese_Japan.932"など）でも通すための、setlocale経由の変換。
	// このDLLのCRTのロケールを一時的に変えるだけで、ホストには影響しない（静的リンク時）
	bool MapCaseByCrt(yaya::string_t &str, bool upper, const char *locale)
	{
		const char *old = setlocale(LC_CTYPE, NULL);
		std::string old_locale(old ? old : "C");

		if ( ! setlocale(LC_CTYPE, locale) ) {
			return false;
		}
		for ( size_t i = 0; i < str.size(); ++i ) {
			str[i] = static_cast<yaya::char_t>(upper ? towupper(str[i]) : towlower(str[i]));
		}
		setlocale(LC_CTYPE, old_locale.c_str());
		return true;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::MapCase
 *  機能概要：  ロケールに従って大文字／小文字に変換する
 *
 *  setlocaleでプロセスのロケールを切り替えず、ロケールを引数に取るAPI（LCMapStringEx）で変換する。
 *  言語固有の規則（トルコ語のiなど）も反映される。
 *  localeは"ja-JP" "ja_JP" "tr_TR.UTF-8"のような名前で、空文字列ならOSのユーザー既定。
 *  LCMapStringExが無い、または名前が通らないときは、CRTのロケール名として扱う。
 *  使えないロケールならfalseを返し、strは変えない。
 * -----------------------------------------------------------------------
 */
bool Ccct::MapCase(yaya::string_t &str, bool upper, const char *locale)
{
	if ( str.empty() ) {
		return true;
	}

	static const LCMapStringExFn pLCMapStringEx = reinterpret_cast<LCMapStringExFn>(Kernel32Proc("LCMapStringEx"));

	std::string tag;
	if ( pLCMapStringEx && NormalizeLocaleTag(locale, tag) ) {
		const wchar_t *name = NULL; // LOCALE_NAME_USER_DEFAULT
		yaya::string_t wtag;
		if ( ! tag.empty() ) {
			wtag = AsciiToString(tag);
			name = wtag.c_str();
		}

		if ( name && ! IsKnownLocaleName(name) ) {
			return MapCaseByCrt(str, upper, locale ? locale : "");
		}

		const DWORD base = upper ? YAYA_LCMAP_UPPERCASE : YAYA_LCMAP_LOWERCASE;
		const DWORD flags[2] = { base | YAYA_LCMAP_LINGUISTIC_CASING, base };
		const int src_len = static_cast<int>(str.size());

		for ( int i = 0; i < 2; ++i ) {
			int n = pLCMapStringEx(name, flags[i], str.c_str(), src_len, NULL, 0, NULL, NULL, 0);
			if ( n > 0 ) {
				yaya::string_t out(static_cast<size_t>(n), L'\0');
				int m = pLCMapStringEx(name, flags[i], str.c_str(), src_len, &out[0], n, NULL, NULL, 0);
				if ( m > 0 ) {
					out.resize(static_cast<size_t>(m));
					str = out;
					return true;
				}
				break;
			}
			if ( ::GetLastError() != ERROR_INVALID_FLAGS ) {
				break;
			}
		}
	}

	return MapCaseByCrt(str, upper, locale ? locale : "");
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::GetOsLocaleName
 *  機能概要：  OSのユーザー設定のロケールを"ja-JP"のような名前で返す
 *              uiがtrueなら表示言語、falseなら地域の書式のロケール。取得できなければ空文字列
 * -----------------------------------------------------------------------
 */
yaya::string_t Ccct::GetOsLocaleName(bool ui)
{
	if ( ui ) {
		static const GetUserDefaultUILanguageFn pGetUILang = reinterpret_cast<GetUserDefaultUILanguageFn>(Kernel32Proc("GetUserDefaultUILanguage"));
		if ( pGetUILang ) {
			return LcidToTag(MAKELCID(pGetUILang(), SORT_DEFAULT));
		}
		return LcidToTag(::GetUserDefaultLCID());
	}

	static const GetUserDefaultLocaleNameFn pGetUserLocaleName = reinterpret_cast<GetUserDefaultLocaleNameFn>(Kernel32Proc("GetUserDefaultLocaleName"));
	if ( pGetUserLocaleName ) {
		wchar_t buf[96];
		if ( pGetUserLocaleName(buf, 96) > 1 ) {
			return yaya::string_t(buf);
		}
	}
	return LcidToTag(::GetUserDefaultLCID());
}

#else // POSIX

namespace {
	// "ja-JP" → "ja_JP"。UTF-8のロケールを優先して開く（wchar_tをUnicodeとして扱うため）
	locale_t OpenCtypeLocale(const char *locale)
	{
		std::string name(locale ? locale : "");
		for ( std::string::size_type i = 0; i < name.size() && name[i] != '.' && name[i] != '@'; ++i ) {
			if ( name[i] == '-' ) {
				name[i] = '_';
			}
		}

		std::string cand[3];
		size_t count = 0;
		if ( name.empty() || name.find('.') != std::string::npos || name.find('@') != std::string::npos ) {
			cand[count++] = name; // 空文字列は環境変数（LC_ALL/LC_CTYPE/LANG）の指定
		}
		else {
			cand[count++] = name + ".UTF-8";
			cand[count++] = name + ".utf8";
			cand[count++] = name;
		}

		for ( size_t i = 0; i < count; ++i ) {
			locale_t loc = newlocale(LC_CTYPE_MASK, cand[i].c_str(), static_cast<locale_t>(0));
			if ( loc ) {
				return loc;
			}
		}
		return static_cast<locale_t>(0);
	}

	bool IsCLocaleTag(const std::string &tag)
	{
		return tag == "C" || tag == "POSIX";
	}

	// 環境変数から取り出してロケール名（"ja-JP"）にする。"C" "POSIX"は設定なしと同じ
	yaya::string_t LocaleFromEnv(const char *const *names)
	{
		for ( ; *names; ++names ) {
			const char *v = getenv(*names);
			if ( ! v || ! *v ) {
				continue;
			}

			// LANGUAGEは"ja:en"のようにコロン区切りの優先順
			std::string first;
			for ( ; *v && *v != ':'; ++v ) {
				first += *v;
			}

			std::string tag;
			if ( ! NormalizeLocaleTag(first.c_str(), tag) || tag.empty() || IsCLocaleTag(tag) ) {
				return yaya::string_t();
			}
			return AsciiToString(tag);
		}
		return yaya::string_t();
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::MapCase
 *  機能概要：  ロケールに従って大文字／小文字に変換する
 *
 *  setlocaleはプロセス全体（ホストを含む）のロケールを変えてしまうので使わず、
 *  newlocaleで作ったロケールをtowupper_l / towlower_lに渡す。
 *  localeは"ja_JP" "ja-JP" "tr_TR.UTF-8"のような名前で、空文字列なら環境変数の指定。
 *  OSにそのロケールが入っていないときはfalseを返し、strは変えない。
 * -----------------------------------------------------------------------
 */
bool Ccct::MapCase(yaya::string_t &str, bool upper, const char *locale)
{
	locale_t loc = OpenCtypeLocale(locale);
	if ( ! loc ) {
		return false;
	}

	for ( size_t i = 0; i < str.size(); ++i ) {
		wint_t c = static_cast<wint_t>(str[i]);
		str[i] = static_cast<yaya::char_t>(upper ? towupper_l(c, loc) : towlower_l(c, loc));
	}

	freelocale(loc);
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::GetOsLocaleName
 *  機能概要：  環境変数のロケールを"ja-JP"のような名前で返す
 *              uiがtrueなら表示言語（LANGUAGE/LC_ALL/LC_MESSAGES/LANG）、
 *              falseなら書式のロケール（LC_ALL/LC_NUMERIC/LANG）。設定が無ければ空文字列
 * -----------------------------------------------------------------------
 */
yaya::string_t Ccct::GetOsLocaleName(bool ui)
{
	static const char *const ui_names[]  = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG", NULL };
	static const char *const fmt_names[] = { "LC_ALL", "LC_NUMERIC", "LANG", NULL };

	return LocaleFromEnv(ui ? ui_names : fmt_names);
}

#endif

#ifdef POSIX
/* -----------------------------------------------------------------------
 *  iconv による変換（POSIX）
 *
 *  ロケールには頼らない（SJIS などのロケールは入っていないことが多く、setlocale は
 *  プロセス全体に効くため）。いったん UTF-8 を介して変換する。
 *  OSデフォルトと、iconv が対応していない文字コードは UTF-8 として扱う。
 * -----------------------------------------------------------------------
 */
namespace {
	// Windows のコードページに近いものから順に試す
	const char *const *IconvNames(int charset)
	{
		static const char *const sjis[]  = { "CP932", "WINDOWS-31J", "SHIFT_JIS", "SJIS", NULL };
		static const char *const eucjp[] = { "EUC-JP-MS", "EUC-JP", "EUCJP", NULL };
		static const char *const jis[]   = { "ISO-2022-JP", "CSISO2022JP", NULL };
		static const char *const big5[]  = { "CP950", "BIG5", NULL };
		static const char *const gb[]    = { "CP936", "GBK", "GB2312", NULL };
		static const char *const kr[]    = { "CP949", "UHC", "EUC-KR", NULL };

		switch ( charset ) {
		case CHARSET_SJIS:   return sjis;
		case CHARSET_EUCJP:  return eucjp;
		case CHARSET_JIS:    return jis;
		case CHARSET_BIG5:   return big5;
		case CHARSET_GB2312: return gb;
		case CHARSET_EUCKR:  return kr;
		default:             return NULL;
		}
	}

	iconv_t OpenIconvName(const char *name, bool to_mbcs)
	{
		return to_mbcs ? iconv_open(name, "UTF-8") : iconv_open("UTF-8", name);
	}

	iconv_t OpenIconv(int charset, bool to_mbcs)
	{
		const char *const *names = IconvNames(charset);
		if ( ! names ) {
			return (iconv_t)-1;
		}

		// 使えた名前を覚えておき、次からは先に試す（読み込み中は1行ごとに呼ばれるため）
		static const char *last_name[2][CHARSET_JIS + 1];
		const char **cache = NULL;
		if ( charset >= 0 && charset <= CHARSET_JIS ) {
			cache = &last_name[to_mbcs ? 1 : 0][charset];
			if ( *cache ) {
				iconv_t cd = OpenIconvName(*cache, to_mbcs);
				if ( cd != (iconv_t)-1 ) {
					return cd;
				}
			}
		}

		for ( ; *names; ++names ) {
			iconv_t cd = OpenIconvName(*names, to_mbcs);
			if ( cd != (iconv_t)-1 ) {
				if ( cache ) {
					*cache = *names;
				}
				return cd;
			}
		}
		return (iconv_t)-1;
	}

	// 入力の引数が char** の実装と const char** の実装（古い macOS / FreeBSD）の両方に合わせる
	template<class T>
	size_t CallIconv(size_t (*func)(iconv_t, T, size_t *, char **, size_t *), iconv_t cd, char **in, size_t *inleft, char **out, size_t *outleft)
	{
		return func(cd, (T)in, inleft, out, outleft);
	}

	size_t Utf8CharLen(unsigned char c)
	{
		if ( (c & 0xe0) == 0xc0 ) { return 2; }
		if ( (c & 0xf0) == 0xe0 ) { return 3; }
		if ( (c & 0xf8) == 0xf0 ) { return 4; }
		return 1;
	}

	void IconvAppend(iconv_t cd, char **in, size_t *inleft, std::string &out)
	{
		char tmp[4096];
		while ( true ) {
			char *op = tmp;
			size_t ol = sizeof(tmp);
			size_t r = CallIconv(&iconv, cd, in, inleft, &op, &ol);
			out.append(tmp, static_cast<size_t>(op - tmp));
			if ( r == (size_t)-1 && errno == E2BIG ) {
				continue;
			}
			return;
		}
	}

	// 変換できない文字は '?' にする（Windows の既定の置換文字と同じ）
	void IconvConvert(iconv_t cd, const char *in, size_t len, std::string &out, bool to_mbcs)
	{
		char *ip = const_cast<char*>(in);
		size_t il = len;

		while ( il > 0 ) {
			IconvAppend(cd, &ip, &il, out);
			if ( il == 0 ) {
				break;
			}
			// EILSEQ（変換できない）/ EINVAL（途中で切れている）: 1文字飛ばす
			size_t skip = to_mbcs ? Utf8CharLen(static_cast<unsigned char>(*ip)) : 1;
			if ( skip > il ) {
				skip = il;
			}
			ip += skip;
			il -= skip;

			if ( to_mbcs ) {
				// ISO-2022-JP のシフト状態に合わせるため、置換文字も iconv を通す
				char q[] = "?";
				char *qp = q;
				size_t ql = 1;
				IconvAppend(cd, &qp, &ql, out);
			}
			else {
				out += '?';
			}
		}

		// シフト状態を初期状態に戻す（ISO-2022-JP）
		IconvAppend(cd, NULL, NULL, out);
	}
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf16be_to_mbcs
 *  機能概要：  UTF-16BE -> MBCS へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
char *Ccct::utf16be_to_mbcs(const yaya::char_t *pUcsStr, int charset)
{
    char *pAnsiStr = NULL;

    if (!pUcsStr) {
		return NULL;
	}
	if (!*pUcsStr) {
		char *p = (char*)malloc(1);
		p[0] = 0;
		return p;
	}

#if defined(WIN32) || defined(_WIN32_WCE)

	int cp = ccct_getcodepage(charset);

	int alen = ::WideCharToMultiByte(cp,0,pUcsStr,-1,NULL,0,NULL,NULL);

	if ( alen <= 0 ) { return NULL; }

	pAnsiStr = (char*)malloc(alen+1+5); //add +5 for safety

	alen = ::WideCharToMultiByte(cp,0,pUcsStr,-1,pAnsiStr,alen+1,NULL,NULL);

	if ( alen <= 0 ) { return NULL; }

	pAnsiStr[alen] = 0;

#else
    size_t nLen = wcslen( pUcsStr);

	if (charset == CHARSET_BINARY) {
		pAnsiStr = (char *)malloc(nLen+1);
		if (!pAnsiStr) {
			return NULL;
		}
		for (size_t i = 0; i < nLen; i++) {
			pAnsiStr[i] = (char)(0x00ff & pUcsStr[i]);
		}
		pAnsiStr[nLen] = 0;
		return pAnsiStr;
	}

	if (pUcsStr[0] == static_cast<yaya::char_t>(0xfeff) ||
			pUcsStr[0] == static_cast<yaya::char_t>(0xfffe)) {
		pUcsStr++; // 先頭にBOM(byte Order Mark)があれば，スキップする
	}

	std::string utf8;
	Ccct_ConvUnicodeToUTF8(utf8, pUcsStr);

	iconv_t cd = OpenIconv(charset, true);
	if (cd == (iconv_t)-1) {
		return string_to_malloc(utf8);
	}

	std::string out;
	IconvConvert(cd, utf8.data(), utf8.size(), out, true);
	iconv_close(cd);

	pAnsiStr = string_to_malloc(out);
#endif

    return pAnsiStr;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::mbcs_to_utf16be
 *  機能概要：  MBCS -> UTF-16 へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
yaya::char_t *Ccct::mbcs_to_utf16be(const char *pAnsiStr, int charset)
{
    if (!pAnsiStr) {
		return NULL;
	}
	if (!*pAnsiStr) {
		yaya::char_t* p = (yaya::char_t*)malloc(sizeof(yaya::char_t));
		p[0] = 0;
		return p;
	}

#if defined(WIN32) || defined(_WIN32_WCE)

    size_t nLen = strlen(pAnsiStr);
	int cp = ccct_getcodepage(charset);

	int wlen = ::MultiByteToWideChar(cp,0,pAnsiStr,nLen,NULL,0);

	if ( wlen <= 0 ) { return NULL; }

	yaya::char_t* pUcsStr = (yaya::char_t*)malloc((wlen + 1 + 5) * sizeof(yaya::char_t)); //add +5 for safety

	wlen = ::MultiByteToWideChar(cp,0,pAnsiStr,nLen,pUcsStr,wlen+1);

	if ( wlen <= 0 ) { return NULL; }

	pUcsStr[wlen] = 0;

#else
    size_t nLen = strlen(pAnsiStr);

	if (charset == CHARSET_BINARY) {
		yaya::char_t *pUcsStr = (yaya::char_t *)malloc(sizeof(yaya::char_t)*(nLen+1));
		if (!pUcsStr) {
			return NULL;
		}
		for (size_t i = 0; i < nLen; i++) {
			pUcsStr[i] = static_cast<yaya::char_t>(static_cast<unsigned char>(pAnsiStr[i]));
		}
		pUcsStr[nLen] = 0;
		return pUcsStr;
	}

	yaya::string_t wstr;

	iconv_t cd = OpenIconv(charset, false);
	if (cd == (iconv_t)-1) {
		Ccct_ConvUTF8ToUnicode(wstr, pAnsiStr);
	}
	else {
		std::string utf8;
		IconvConvert(cd, pAnsiStr, nLen, utf8, false);
		iconv_close(cd);
		Ccct_ConvUTF8ToUnicode(wstr, utf8.c_str());
	}

	yaya::char_t *pUcsStr = wstring_to_malloc(wstr);
#endif

    return pUcsStr;
}

/*--------------------------------------------
	UTF-9をUTF-16に
--------------------------------------------*/
size_t Ccct_ConvUTF8ToUnicode(yaya::string_t &buf,const char* pStrIn)
{
	unsigned char *pStr = (unsigned char*)pStrIn;
	unsigned char *pStrLast = pStr + strlen(pStrIn);

	unsigned char c;
	unsigned long tmp;

	// UTF-16の文字数はUTF-8のバイト数を超えない。1文字ずつappendすると遅い（VC6では1文字あたり十数ns）ので、
	// 先にバイト数ぶん確保して直接書き込み、最後に実際の長さへ詰める
	const size_t base_len = buf.length();
	const size_t in_len = static_cast<size_t>(pStrLast - pStr);
	if ( in_len == 0 ) {
		return base_len;
	}
	buf.resize(base_len + in_len);
	yaya::char_t *po = &buf[base_len];

	while( pStr < pStrLast ){
		c = *(pStr++);
		if( (c & 0x80) == 0 ){ //1Byte - 0???????
			*(po++) = static_cast<yaya::char_t>(static_cast<WORD>(c));
		}
		/*else if( (c & 0xc0) == 0x80 ){ //1Byte - 10?????? -> 必ず2バイト目以降のため、単体で出たら不正 
			m_Str.Add() = (WORD)c;
		}*/
		else if( (c & 0xe0) == 0xc0 ){ //2Byte - 110????? 
			if( pStrLast - pStr < 1 ){ break; } //末尾で途切れている
			tmp  = static_cast<DWORD>(c & 0x1f) << 6; //下5bit - 10-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			*(po++) = static_cast<yaya::char_t>(static_cast<WORD>(tmp));
		}
		else if( (c & 0xf0) == 0xe0 ){ //3Byte - 1110????
			if( pStrLast - pStr < 2 ){ break; } //末尾で途切れている
			tmp  = static_cast<DWORD>(c & 0x0f) << 12; //下4bit - 15-12
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 6;  //下6bit - 11-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			if ( tmp != 0xfeff && tmp != 0xfffe ) { //BOMでない
				*(po++) = static_cast<yaya::char_t>(static_cast<WORD>(tmp));
			}
		}
		else if( (c & 0xf8) == 0xf0 ){ //4Byte - 11110??? UTF-16 Surrogate
			if( pStrLast - pStr < 3 ){ break; } //末尾で途切れている
			tmp  = static_cast<DWORD>(c & 0x07) << 18; //下3bit -> 20-18
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 12; //下6bit - 17-12
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 6; //下6bit - 11-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			tmp -= 0x10000;
			*(po++) = static_cast<yaya::char_t>((WORD)(0xD800U | ((tmp >> 10) & 0x3FF))); //上位サロゲート
			*(po++) = static_cast<yaya::char_t>((WORD)(0xDC00U | (tmp & 0x3FF))); //下位サロゲート
		}
		else if( (c & 0xfc) == 0xf8 ){ //5Byte - 111110?? -- UCS-4
			if( pStrLast - pStr < 4 ){ break; } //末尾で途切れている
			pStr += 4; //無視
		}
		else if( (c & 0xfe) == 0xfc ){ //6Byte - 1111110? -- UCS-4
			if( pStrLast - pStr < 5 ){ break; } //末尾で途切れている
			pStr += 5; //無視
		}
		/*else { // - 11111110 , 11111111 (0xfe,0xff) - そんな文字あるかい！
			m_Str.Add() = (WORD)c;
		}*/
	}

	buf.resize(base_len + static_cast<size_t>(po - &buf[base_len]));

	return buf.length();
}

/*--------------------------------------------
	UTF-16をUTF-8に
--------------------------------------------*/
size_t Ccct_ConvUnicodeToUTF8(std::string &buf,const yaya::char_t *pStrw)
{
	yaya::char_t w;
	unsigned long surrogateTemp;
	size_t length = wcslen(pStrw);
	size_t i = 0;

	buf.reserve(length*4+1); //4倍まで (UTF-8 5-6byte領域はUCS-2からの変換では存在しない)

	while(i < length){
		w = pStrw[i++];

		if (w < 0x80) { //1byte
			buf.append(1,(char)(BYTE)w); //5-0
		}
		else if ( w < 0x0800 ) { //2byte
			buf.append(1,(char)(BYTE)((w >> 6) & 0x001f) | 0xc0); //10-6
			buf.append(1,(char)(BYTE)(w & 0x3f) | 0x80); //5-0
		}
		else {
			if ( (w & 0xF800) == 0xD800 ) { //4byte サロゲートページ D800->DFFF
				surrogateTemp = ( ( (w & 0x3FF) << 10 ) | (pStrw[i++] & 0x3FF) ) + 0x10000;

				buf.append(1,(char)(BYTE)((surrogateTemp >> 18) & 0x07) | 0xf0); //20-18
				buf.append(1,(char)(BYTE)((surrogateTemp >> 12) & 0x3f) | 0x80); //17-12
				buf.append(1,(char)(BYTE)((surrogateTemp >> 6 ) & 0x3f) | 0x80); //11-6
				buf.append(1,(char)(BYTE)(surrogateTemp & 0x3f) | 0x80); //5-0
			}
			else { //3byte
				buf.append(1,(char)(BYTE)((w >> 12) & 0x0f) | 0xe0); //15-12
				buf.append(1,(char)(BYTE)((w >> 6)  & 0x3f) | 0x80); //11-6
				buf.append(1,(char)(BYTE)(w & 0x3f) | 0x80); //5-0
			}
		}
	}

	return buf.length();
}
