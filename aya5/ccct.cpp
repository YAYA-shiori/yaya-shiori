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

#include <clocale>
#include <string>

#include "ccct.h"
#include "manifest.h"
#include "globaldef.h"

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
 *  関数名  ：  Ccct::Ucs2ToMbcs
 *  機能概要：  UTF-16BE -> MBCS へ文字列のコード変換
 *
 * (written by umeici)
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
char	*Ccct::Ucs2ToMbcs(const yaya::char_t *wstr, int charset)
{
	if (charset == CHARSET_UTF8)
		return Ccct::utf16be_to_utf8(wstr);
	else
		return Ccct::utf16be_to_mbcs(wstr, charset);
}

//----

char	*Ccct::Ucs2ToMbcs(const yaya::string_t &wstr, int charset)
{
	return Ucs2ToMbcs(wstr.c_str(), charset);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::MbcsToUcs2
 *  機能概要：  MBCS -> UTF-16BE へ文字列のコード変換
 *
 * (written by umeici)
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
yaya::char_t	*Ccct::MbcsToUcs2(const char *mstr, int charset)
{
	if (charset == CHARSET_UTF8)
		return Ccct::utf8_to_utf16be(mstr);
	else
		return Ccct::mbcs_to_utf16be(mstr, charset);
}

//----

yaya::char_t	*Ccct::MbcsToUcs2(const std::string &mstr, int charset)
{
	return MbcsToUcs2(mstr.c_str(), charset);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf16be_to_mbcs
 *  機能概要：  UTF-16BE -> MBCS へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
char *Ccct::utf16be_to_mbcs(const yaya::char_t *pUcsStr, int charset)
{
    char *pAnsiStr = NULL;
    size_t nLen;

    if (!pUcsStr)
		return NULL;

	ccct_setlocale(LC_ALL, charset);

    nLen = wcslen( pUcsStr);

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
		if (charset != CHARSET_UTF8 && charset != CHARSET_BINARY) ccct_setlocale(LC_ALL, charset);
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

	if (charset != CHARSET_UTF8 && charset != CHARSET_BINARY) ccct_setlocale(LC_ALL, charset);
    return pAnsiStr;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::mbcs_to_utf16be
 *  機能概要：  MBCS -> UTF-16 へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
yaya::char_t *Ccct::mbcs_to_utf16be(const char *pAnsiStr, int charset)
{
    if (!pAnsiStr)
		return NULL;

	ccct_setlocale(LC_ALL, charset);

    size_t nLen = strlen(pAnsiStr);

    yaya::char_t *pUcsStr = (yaya::char_t *)malloc(sizeof(yaya::char_t)*(nLen+1));
    if (!pUcsStr) {
		if (charset != CHARSET_UTF8 && charset != CHARSET_BINARY) ccct_setlocale(LC_ALL, charset);
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

	if (charset != CHARSET_UTF8 && charset != CHARSET_BINARY) ccct_setlocale(LC_ALL, charset);
    return pUcsStr;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf16be_to_utf8
 *  機能概要：  UTF-16 -> UTF-8 へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
char *Ccct::utf16be_to_utf8(const yaya::char_t *pUcsStr)
{
    size_t nUcsNum = wcslen(pUcsStr);

    char *pUtf8Str = (char *)malloc((nUcsNum + 1)*3*sizeof(char));
    utf16be_to_utf8_sub( pUtf8Str, pUcsStr, nUcsNum);

    return pUtf8Str;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf16be_to_utf8_sub
 *  機能概要：  UTF-16 -> UTF-8変換（utf16be_to_utf8で使用します）
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
size_t Ccct::utf16be_to_utf8_sub( char *pUtf8, const yaya::char_t *pUcs2, size_t nUcsNum)
{
    size_t nUtf8 = 0;

    for (size_t nUcs2 = 0; nUcs2 < nUcsNum; nUcs2++) {
        if ( (unsigned short)pUcs2[nUcs2] <= 0x007f) {
            pUtf8[nUtf8] = (pUcs2[nUcs2] & 0x007f);
            nUtf8 += 1;
        } else if ( (unsigned short)pUcs2[nUcs2] <= 0x07ff) {
            pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0x07C0) >> 6 ) | 0xc0; // 2002.08.17 修正
            pUtf8[nUtf8+1] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            nUtf8 += 2;
        } else {
            pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0xf000) >> 12) | 0xe0; // 2002.08.04 修正
            pUtf8[nUtf8+1] = ((pUcs2[nUcs2] & 0x0fc0) >> 6) | 0x80;
            pUtf8[nUtf8+2] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            nUtf8 += 3;
        }
    }

    pUtf8[nUtf8] = '\0';

    return nUtf8;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf8_to_utf16be
 *  機能概要：  UTF-8 -> UTF-16BE へ文字列のコード変換
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
yaya::char_t *Ccct::utf8_to_utf16be(const char *pUtf8Str)
{
    size_t nUtf8Num = strlen(pUtf8Str); // UTF-8文字列には，'\0' がない

	yaya::char_t *pUcsStr = (yaya::char_t *)malloc((nUtf8Num + 1)*sizeof(yaya::char_t));
	utf8_to_utf16be_sub( pUcsStr, pUtf8Str, nUtf8Num);

    return pUcsStr;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::utf8_to_utf16be_sub
 *  機能概要：  UTF-8 -> UTF-16BE変換（utf16be_to_utf8で使用します）
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
size_t Ccct::utf8_to_utf16be_sub( yaya::char_t *pUcs2, const char *pUtf8, size_t nUtf8Num)
{
    size_t	nUcs2 = 0;

    for (size_t nUtf8 = 0; nUtf8 < nUtf8Num; ) {
        if ( ( pUtf8[nUtf8] & 0x80) == 0x00) { // 最上位ビット = 0
            pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x7f);
            nUtf8 += 1;
        } else if ((pUtf8[nUtf8] & 0xe0) == 0xc0) { // 上位3ビット = 110
            pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x1f) << 6;
            pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f);
            nUtf8 += 2;
        } else {
            pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x0f) << 12;
            pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f) << 6;
            pUcs2[nUcs2] |= ( pUtf8[nUtf8+2] & 0x3f);
            nUtf8 += 3;
        }

        nUcs2 += 1;
    }

    pUcs2[nUcs2] = L'\0';

    return nUcs2;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::sys_setlocale
 *  機能概要：  OSデフォルトの言語IDでロケール設定する
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
char *Ccct::sys_setlocale(int category)
{
	return setlocale(category,".ACP");

	/*switch(PRIMARYLANGID(GetSystemDefaultLangID())) {
		case LANG_JAPANESE:
			return setlocale(category, "Japanese");
		case LANG_KOREAN:
			return setlocale(category, "Korean");
		case LANG_CHINESE:
			return setlocale(category, "Chinese");
		case LANG_RUSSIAN:
			return setlocale(category, "Russian");
		case LANG_FRENCH:
			return setlocale(category, "French");
		case LANG_GERMAN:
			return setlocale(category, "German");
		case LANG_SPANISH:
			return setlocale(category, "Spanish");
		case LANG_ITALIAN:
			return setlocale(category, "Italian");
		default:
			return setlocale(category, "English");
	};*/
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  Ccct::ccct_setlocale
 *  機能概要：  言語IDでロケール設定する
 * -----------------------------------------------------------------------
 */
#ifndef POSIX
char *Ccct::ccct_setlocale(int category, int charset)
{
	if (charset == CHARSET_SJIS)
		return setlocale(LC_ALL, ".932");
	else if (charset == CHARSET_EUCJP)
		return setlocale(LC_ALL, ".20932");
	else if (charset == CHARSET_BIG5)
		return setlocale(LC_ALL, ".950");
	else if (charset == CHARSET_GB2312)
		return setlocale(LC_ALL, ".936");
	else if (charset == CHARSET_EUCKR)
		return setlocale(LC_ALL, ".949");
	else
		return sys_setlocale(LC_ALL);
}
#endif

