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
// 利用制限等はなさそうだったのでそのまま使わせていただきました。
//
// 細かい点はわたしのほうで弄りましたが、肝心の読み替え部分はオリジナルのままです。
//
// 変更点
// ・オリジナルはロケールJapaneseのみ対応だったので、OSデフォルト言語を調べてsetlocaleする機能を追加。
// ・MBC to UCS2、及びその逆を行う関数を差し替えました。
// ・高速化のための修正を若干。
// 　1.オリジナルでは変換前に必要な領域のサイズをしっかり調べていましたが、その処理は省き、どのように
// 　  変換されても収まるだけの領域を用意するようにしました。
// 　2.オリジナルはcallocでしたが、極力mallocへ置き換えました。若干速い（はず）。
//

#ifndef CCCTH
#define CCCTH

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <cstdlib>

#include "globaldef.h"

//----

class Ccct {
public:
	static char		*Ucs2ToMbcs(const wchar_t *wstr, int charset);
	static char		*Ucs2ToMbcs(const yaya::string_t &wstr, int charset);
	static wchar_t	*MbcsToUcs2(const char *mstr, int charset);
	static wchar_t	*MbcsToUcs2(const std::string &mstr, int charset);

	static bool     CheckInvalidCharset(int charset);
	static int      CharsetTextToID(const wchar_t *ctxt);
	static int      CharsetTextToID(const char *ctxt);
	static const wchar_t *CharsetIDToTextW(const int charset);
	static const char    *CharsetIDToTextA(const int charset);

	static char		*sys_setlocale(int category);
	static char		*ccct_setlocale(int category, int charset);

private:
	static char *utf16be_to_mbcs(const yaya::char_t *pUcsStr, int charset);
	static yaya::char_t *mbcs_to_utf16be(const char *pAnsiStr, int charset);

};

//----

#endif

