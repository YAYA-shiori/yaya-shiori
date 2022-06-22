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

#include <clocale>
#include <string>

#include "ccct.h"
#include "manifest.h"
#include "globaldef.h"
//#include "babel/babel.h"

#ifdef POSIX
#  include <ctype.h>
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


#ifdef POSIX
namespace {
    int wcsicmp(const wchar_t* a, const wchar_t* b) {
        size_t lenA = wcslen(a);
        size_t lenB = wcslen(b);

        if (lenA != lenB) {
            return lenA - lenB;
        }
        else {
            for (size_t i = 0; i < lenA; i++) {
                wchar_t A = tolower(a[i]);
                wchar_t B = tolower(b[i]);

                if (A != B) {
                    return A - B;
                }
            }

            return 0;
        }
    }

    int stricmp(const char* a, const char* b) {
        size_t lenA = strlen(a);
        size_t lenB = strlen(b);

        if (lenA != lenB) {
            return lenA - lenB;
        }
        else {
            for (size_t i = 0; i < lenA; i++) {
                wchar_t A = tolower(a[i]);
                wchar_t B = tolower(b[i]);

                if (A != B) {
                    return A - B;
                }
            }

            return 0;
        }
    }
}
#endif


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
 *  関数名  ：  Ccct::CharsetTextToID
 *  機能概要：  Charset 文字列->Charset ID
 * -----------------------------------------------------------------------
 */
int      Ccct::CharsetTextToID(const wchar_t *ctxt)
{
	if (!wcsicmp(L"UTF-8",ctxt) || !wcsicmp(L"UTF8",ctxt))
		return CHARSET_UTF8;
	else if (!wcsicmp(L"default",ctxt) || !wcsicmp(L"OSNative",ctxt))
		return CHARSET_DEFAULT;
	else if (!wcsicmp(L"Shift_JIS",ctxt) || !wcsicmp(L"ShiftJIS",ctxt) || !wcsicmp(L"SJIS",ctxt))
		return CHARSET_SJIS;
	else if (!wcsicmp(L"EUC_JP",ctxt) || !wcsicmp(L"EUC-JP",ctxt) || !wcsicmp(L"EUCJP",ctxt))
		return CHARSET_EUCJP;
	else if (!wcsicmp(L"ISO-2022-JP",ctxt) || !wcsicmp(L"JIS",ctxt))
		return CHARSET_JIS;
	else if (!wcsicmp(L"BIG5",ctxt) || !wcsicmp(L"BIG-5",ctxt))
		return CHARSET_BIG5;
	else if (!wcsicmp(L"GB2312",ctxt) || !wcsicmp(L"GB-2312",ctxt))
		return CHARSET_GB2312;
	else if (!wcsicmp(L"EUC_KR",ctxt) || !wcsicmp(L"EUC-KR",ctxt) || !wcsicmp(L"EUCKR",ctxt))
		return CHARSET_EUCKR;
	else if (!wcsicmp(L"binary",ctxt))
		return CHARSET_BINARY;

	return CHARSET_DEFAULT;
}

int      Ccct::CharsetTextToID(const char *ctxt)
{
	if (!stricmp("UTF-8",ctxt) || !stricmp("UTF8",ctxt))
		return CHARSET_UTF8;
	else if (!stricmp("default",ctxt) || !stricmp("OSNative",ctxt))
		return CHARSET_DEFAULT;
	else if (!stricmp("Shift_JIS",ctxt) || !stricmp("ShiftJIS",ctxt) || !stricmp("SJIS",ctxt))
		return CHARSET_SJIS;
	else if (!stricmp("EUC_JP",ctxt) || !stricmp("EUC-JP",ctxt) || !stricmp("EUCJP",ctxt))
		return CHARSET_EUCJP;
	else if (!stricmp("ISO-2022-JP",ctxt) || !stricmp("JIS",ctxt))
		return CHARSET_JIS;
	else if (!stricmp("BIG5",ctxt) || !stricmp("BIG-5",ctxt))
		return CHARSET_BIG5;
	else if (!stricmp("GB2312",ctxt) || !stricmp("GB-2312",ctxt))
		return CHARSET_GB2312;
	else if (!stricmp("EUC_KR",ctxt) || !stricmp("EUC-KR",ctxt) || !stricmp("EUCKR",ctxt))
		return CHARSET_EUCKR;
	else if (!stricmp("binary",ctxt))
		return CHARSET_BINARY;

	return CHARSET_DEFAULT;
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
 *  関数名  ：  Ccct::ccct_setlocale
 *  機能概要：  言語IDでロケール設定する
 * -----------------------------------------------------------------------
 */
char *Ccct::ccct_setlocale(int category, int charset)
{
#ifdef POSIX
	if (charset == CHARSET_SJIS) {
		return setlocale(category, "ja_JP.SJIS");
	}
	else if (charset == CHARSET_EUCJP) {
		return setlocale(category, "ja_JP.eucJP");
	}
	else if (charset == CHARSET_BIG5) {
		return setlocale(category, "zh_TW.Big5");
	}
	else if (charset == CHARSET_GB2312) {
		return setlocale(category, "zh_CN.GB2312");
	}
	else if (charset == CHARSET_EUCKR) {
		return setlocale(category, "ko_KR.eucKR");
	}
	else if (charset == CHARSET_JIS) {
		return setlocale(category, "ja_JP.SJIS");
	}
#else
	if (charset == CHARSET_SJIS) {
		return setlocale(category, ".932");
	}
	else if (charset == CHARSET_EUCJP) {
		return setlocale(category, ".20932");
	}
	else if (charset == CHARSET_BIG5) {
		return setlocale(category, ".950");
	}
	else if (charset == CHARSET_GB2312) {
		return setlocale(category, ".936");
	}
	else if (charset == CHARSET_EUCKR) {
		return setlocale(category, ".949");
	}
	else if (charset == CHARSET_JIS) {
		return setlocale(category, ".50222");
	}
#endif
	else {
		return sys_setlocale(category);
	}
}

/* -----------------------------------------------------------------------
 *  setlocaleバリア
 * -----------------------------------------------------------------------
 */

class CcctSetLocaleSwitcher {
private:
	const char *m_oldLocale;
	int m_category;
public:
	CcctSetLocaleSwitcher(int category,int charset) {
		m_category = category;
		m_oldLocale = setlocale(category,NULL);
		Ccct::ccct_setlocale(category,charset);
	}
	~CcctSetLocaleSwitcher() {
		if ( m_oldLocale ) {
			setlocale(m_category,m_oldLocale);
		}
	}
};

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
	CcctSetLocaleSwitcher loc(LC_CTYPE, charset);

    size_t nLen = wcslen( pUcsStr);

	if (charset != CHARSET_BINARY) {
	    if (pUcsStr[0] == static_cast<yaya::char_t>(0xfeff) ||
				pUcsStr[0] == static_cast<yaya::char_t>(0xfffe)) {
			pUcsStr++; // 先頭にBOM(byte Order Mark)があれば，スキップする
	        nLen--;
		}
	}

	//文字長×マルチバイト最大長＋ゼロ終端
    pAnsiStr = (char *)malloc((nLen*MB_CUR_MAX)+1);
    if (!pAnsiStr) {
		return NULL;
	}

    // 1文字ずつ変換する。
    // まとめて変換すると、変換不能文字への対応が困難なので
    size_t i, nMbpos = 0;
	int nRet;

    for (i = 0; i < nLen; i++) {
		if (charset != CHARSET_BINARY) {
			nRet = wctomb(pAnsiStr+nMbpos, pUcsStr[i]);
		}
		else {
			pAnsiStr[nMbpos] = (char)(0x00ff & pUcsStr[i]);
			nRet = 1;
		}
		if ( nRet <= 0 ) { // can not conversion
            pAnsiStr[nMbpos++] = ' ';
        }
		else {
			nMbpos += nRet;
		}
    }

	pAnsiStr[nMbpos] = 0;
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
		yaya::char_t* p = (yaya::char_t*)malloc(2);
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
	CcctSetLocaleSwitcher loc(LC_CTYPE, charset);

    size_t nLen = strlen(pAnsiStr);

    yaya::char_t *pUcsStr = (yaya::char_t *)malloc(sizeof(yaya::char_t)*(nLen+7));
    if (!pUcsStr) {
		return NULL;
	}

    // 1文字ずつ変換する。
    // まとめて変換すると、変換不能文字への対応が困難なので
    size_t i, nMbpos = 0;
	int nRet;

    for (i = 0; i < nLen; ) {
		if (charset != CHARSET_BINARY) {
	        nRet = mbtowc(pUcsStr+nMbpos, pAnsiStr+i, nLen-i);
		}
		else {
			pUcsStr[i]=static_cast<yaya::char_t>(pAnsiStr[i]);
			nRet = 1;
		}
		if ( nRet <= 0 ) { // can not conversion
            pUcsStr[nMbpos++] = L' ';
			i += 1;
        }
		else {
			++nMbpos;
			i += nRet;
		}
    }

	pUcsStr[nMbpos] = 0;
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

	while( pStr < pStrLast ){
		c = *(pStr++);
		if( (c & 0x80) == 0 ){ //1Byte - 0???????
			buf.append(1,(WORD)c);
		}
		/*else if( (c & 0xc0) == 0x80 ){ //1Byte - 10?????? -> 必ず2バイト目以降のため、単体で出たら不正 
			m_Str.Add() = (WORD)c;
		}*/
		else if( (c & 0xe0) == 0xc0 ){ //2Byte - 110????? 
			tmp  = static_cast<DWORD>(c & 0x1f) << 6; //下5bit - 10-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			buf.append(1,static_cast<WORD>(tmp));
		}
		else if( (c & 0xf0) == 0xe0 ){ //3Byte - 1110????
			tmp  = static_cast<DWORD>(c & 0x0f) << 12; //下4bit - 15-12
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 6;  //下6bit - 11-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			if ( tmp != 0xfeff && tmp != 0xfffe ) { //BOMでない
				buf.append(1,static_cast<WORD>(tmp));
			}
		}
		else if( (c & 0xf8) == 0xf0 ){ //4Byte - 11110??? UTF-16 Surrogate
			tmp  = static_cast<DWORD>(c & 0x07) << 18; //下3bit -> 20-18
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 12; //下6bit - 17-12
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f) << 6; //下6bit - 11-6
			tmp |= static_cast<DWORD>(*(pStr++) & 0x3f); //下6bit - 5-0
			tmp -= 0x10000;
			buf.append(1,(WORD)(0xD800U | ((tmp >> 10) & 0x3FF))); //上位サロゲート
			buf.append(1,(WORD)(0xDC00U | (tmp & 0x3FF))); //下位サロゲート
		}
		else if( (c & 0xfc) == 0xf8 ){ //5Byte - 111110?? -- UCS-4
			pStr += 4; //無視
		}
		else if( (c & 0xfe) == 0xfc ){ //6Byte - 1111110? -- UCS-4
			pStr += 5; //無視
		}
		/*else { // - 11111110 , 11111111 (0xfe,0xff) - そんな文字あるかい！
			m_Str.Add() = (WORD)c;
		}*/
	}

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
