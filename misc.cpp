// 
// AYA version 5
//
// 雑用関数
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <ctime>
#include <string.h>
#include <time.h>
#include <string>
#include <vector>
#if defined(POSIX)
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
#endif

#include "manifest.h"
#include "misc.h"
#include "ccct.h"
#if defined(POSIX) || defined(__MINGW32__)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "wsex.h"
#include "function.h"
#include "sysfunc.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  Split
 *  機能概要：  文字列を分割して余分な空白を削除します
 *
 *  返値　　：  0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1.erase();
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += ::wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

//----

char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1.erase();
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += sepstr.size();
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SplitOnly
 *  機能概要：  文字列を分割します
 *
 *  返値　　：  0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
char	SplitOnly(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1.erase();
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += ::wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Find_IgnoreDQ
 *  機能概要：  ダブル/シングルクォート内を無視して文字列を検索
 *
 *  返値　　：  負=失敗   0・正=成功
 * -----------------------------------------------------------------------
 */
yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::char_t *findstr)
{
	yaya::string_t::size_type findpoint = 0;
	yaya::string_t::size_type nextdq = 0;

	while(true){
		findpoint = str.find(findstr, findpoint);
		if (findpoint == yaya::string_t::npos)
			return yaya::string_t::npos;

		nextdq = IsInDQ(str, nextdq, findpoint);
		if (nextdq >= IsInDQ_npos) {
			if (nextdq == IsInDQ_runaway) { //クオートが終わらないまま終了
				return yaya::string_t::npos;
			}
			break; //みつかった
		}
		else { //クオート内部。無視して次へ
			findpoint = nextdq;
		}
	}

	return findpoint;
}

yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::string_t &findstr)
{
	return Find_IgnoreDQ(str,findstr.c_str());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  find_last_str
 *  機能概要：  一番最後に見つかった文字列の位置を返す
 *
 *  返値　　：  npos=失敗   0・正=成功
 * -----------------------------------------------------------------------
 */
yaya::string_t::size_type find_last_str(const yaya::string_t &str, const yaya::char_t *findstr)
{
	return str.rfind(findstr);
}

yaya::string_t::size_type find_last_str(const yaya::string_t &str, const yaya::string_t &findstr)
{
	return find_last_str(str,findstr.c_str());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SplitPathParts
 *  機能概要：  パスをドライブ・ディレクトリ・ファイル名・拡張子に分割します
 *  　　　　　  _wsplitpathと同じ規則ですが、長さに制限がありません
 *  　　　　　  （ドライブは2文字目が":"のとき先頭2文字、ディレクトリは最後の"\"か"/"まで、
 *  　　　　　  拡張子はファイル名の最後の"."から）
 * -----------------------------------------------------------------------
 */
void	SplitPathParts(const yaya::string_t &path, yaya::string_t &drive, yaya::string_t &dir, yaya::string_t &fname, yaya::string_t &ext)
{
	yaya::string_t::size_type pos = 0;

	if ( path.size() >= 2 && path[1] == L':' ) {
		drive = path.substr(0, 2);
		pos = 2;
	}
	else {
		drive.erase();
	}

	yaya::string_t::size_type sep = path.find_last_of(L"\\/");
	if ( sep != yaya::string_t::npos && sep >= pos ) {
		dir = path.substr(pos, sep + 1 - pos);
		pos = sep + 1;
	}
	else {
		dir.erase();
	}

	yaya::string_t::size_type dot = path.rfind(L'.');
	if ( dot != yaya::string_t::npos && dot >= pos ) {
		fname = path.substr(pos, dot - pos);
		ext = path.substr(dot);
	}
	else {
		fname = path.substr(pos);
		ext.erase();
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  Split_IgnoreDQ
 *  機能概要：  文字列を分割して余分な空白を削除します
 *  　　　　　  ただしダブル/シングルクォート内では分割しません
 *
 *  返値　　：  0/1=失敗/成功
 * -----------------------------------------------------------------------
 */

char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = Find_IgnoreDQ(str,sepstr);
	if ( seppoint == yaya::string_t::npos ) {
		dstr0 = str;
		dstr1.erase();
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

//----

char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr)
{
	return Split_IgnoreDQ(str,dstr0,dstr1,sepstr.c_str());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  SplitToMultiString
 *  機能概要：  文字列を分割してvectorに格納します
 *
 *　返値　　：　分割数(array.size())
 * -----------------------------------------------------------------------
 */
size_t	SplitToMultiString(const yaya::string_t &str, std::vector<yaya::string_t> *array, const yaya::string_t &delimiter)
{
	if (!str.size())
		return 0;

	const yaya::string_t::size_type dlmlen = delimiter.size();
	yaya::string_t::size_type beforepoint = 0,seppoint;
	size_t count = 1;

	for( ; ; ) {
		// デリミタの発見
		seppoint = str.find(delimiter,beforepoint);
		if (seppoint == yaya::string_t::npos) {
			if ( array ) {
				array->emplace_back(yaya::string_t(str.begin()+beforepoint,str.end()));
			}
			break;
		}
		// 取り出しとvectorへの追加
		if ( array ) {
			array->emplace_back(yaya::string_t(str.begin()+beforepoint,str.begin()+seppoint));
		}
		// 取り出した分を削除
		beforepoint = seppoint + dlmlen;
		++count;
	}

	return count;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CutSpace
 *  機能概要：  与えられた文字列の前後に半角空白かタブがあった場合、すべて削除します
 * -----------------------------------------------------------------------
 */
void	CutSpace(yaya::string_t &str)
{
	CutEndSpace(str);
	CutStartSpace(str);
}

void	CutStartSpace(yaya::string_t &str)
{
	int	len = str.size();
	// 前方
	int	erasenum = 0;
	for(int i = 0; i < len; i++) {
		if (IsSpace(str[i])) {
			erasenum++;
		}
		else {
			break;
		}
	}
	if (erasenum) {
		str.erase(0, erasenum);
	}
}

void	CutEndSpace(yaya::string_t &str)
{
	int	len = str.size();
	// 後方
	int erasenum = 0;
	for(int i = len - 1; i >= 0; i--) {
		if (IsSpace(str[i])) {
			erasenum++;
		}
		else {
			break;
		}
	}
	if (erasenum) {
		str.erase(len - erasenum, erasenum);
	}
}


/* -----------------------------------------------------------------------
 *  関数名  ：  UnescapeSpecialString
 *  機能概要：  (ヒアドキュメント仕様用の)有害文字エスケープを戻します
 *              parser0.cpp の CHereDocument も参照してください
 * -----------------------------------------------------------------------
 */
void	UnescapeSpecialString(yaya::string_t &str)
{
	if ( str.size() <= 1 ) {
		return;
	}

	size_t len = str.size()-1; //1文字手前まで
	for ( size_t i = 0 ; i < len ; ++i ) {
		if ( str[i] == 0xFFFFU ) {
			if ( str[i+1] == 0x0001U ) {
				str[i]   = L'\r';
				str[i+1] = L'\n';
			}
			else if ( str[i+1] == 0x0002U ) {
				str.erase(i, 1);
				str[i] = L'"';
				len -= 1;
			}
			else if ( str[i+1] == 0x0003U ) {
				str.erase(i, 1);
				str[i] = L'\'';
				len -= 1;
			}
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CutDoubleQuote
 *  機能概要：  与えられた文字列の前後にダブルクォートがあった場合削除します
 * -----------------------------------------------------------------------
 */
void	CutDoubleQuote(yaya::string_t &str)
{
	size_t len = str.size();
	if (!len)
		return;
	// 前方
	if (str[0] == L'\"') {
		str.erase(0, 1);
		len--;
		if (!len)
			return;
	}
	// 後方
	if (str[len - 1] == L'\"')
		str.erase(len - 1, 1);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CutSingleQuote
 *  機能概要：  与えられた文字列の前後にシングルクォートがあった場合削除します
 * -----------------------------------------------------------------------
 */
void	CutSingleQuote(yaya::string_t &str)
{
	size_t len = str.size();
	if (!len)
		return;
	// 前方
	if (str[0] == L'\'') {
		str.erase(0, 1);
		len--;
		if (!len)
			return;
	}
	// 後方
	if (str[len - 1] == L'\'')
		str.erase(len - 1, 1);
}

void EscapingInsideDoubleDoubleQuote(yaya::string_t &str) {
	yaya::ws_replace(str, L"\"\"", L"\"");
}
void EscapingInsideDoubleSingleQuote(yaya::string_t &str) {
	yaya::ws_replace(str, L"\'\'", L"\'");
}

/* -----------------------------------------------------------------------
 *  関数名  ：  AddDoubleQuote
 *  機能概要：  与えられた文字列をダブルクォートで囲みます
 * -----------------------------------------------------------------------
 */
void	AddDoubleQuote(yaya::string_t &str)
{
	str = L"\"" + str + L"\"";
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CutCrLf
 *  機能概要：  与えられた文字列の後端に改行(CRLF)があった場合削除します
 * -----------------------------------------------------------------------
 */
void	CutCrLf(yaya::string_t &str)
{
	yaya::ws_eraseend(str, L'\n');
	yaya::ws_eraseend(str, L'\r');
}

/* -----------------------------------------------------------------------
 *  関数名  ：  GetDateString
 *  機能概要：  年月日/時分秒の文字列を作成して返します
 * -----------------------------------------------------------------------
 */

yaya::string_t GetDateString()
{
    char buf[128];
    struct tm tm;
    if (!EpochTimeToTM(GetEpochTime(), CTimeZone(), tm)) {
        memset(&tm, 0, sizeof(tm));
        tm.tm_mday = 1;
        tm.tm_year = 70;
        tm.tm_wday = 4;
    }
    strftime(buf, 127, "%Y/%m/%d %H:%M:%S", &tm);

	yaya::char_t wbuf[64];
	for ( size_t i = 0 ; i < 64 ; ++i ) {
		wbuf[i] = buf[i];
		if ( wbuf[i] == 0 ) { break; }
	}
	return wbuf;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsInDQ
 *  機能概要：  文字列内の指定位置がダブル/シングルクォート範囲内かをチェックします
 *
 *  返値　　：  -1   = ダブル/シングルクォートの外部
 *             !=-1 = ダブル/シングルクォートの内部
 *                -2  = ダブル/シングルクォートの内部のままテキスト終了
 *                >=0 = 次に外部になる位置
 * -----------------------------------------------------------------------
 */
const yaya::string_t::size_type IsInDQ_notindq = static_cast<yaya::string_t::size_type>(-1);
const yaya::string_t::size_type IsInDQ_runaway = static_cast<yaya::string_t::size_type>(-2);
const yaya::string_t::size_type IsInDQ_npos    = static_cast<yaya::string_t::size_type>(-2);

yaya::string_t::size_type IsInDQ(const yaya::string_t &str, yaya::string_t::size_type startpoint, yaya::string_t::size_type checkpoint)
{
	bool dq    = false;
	bool quote = false;

	yaya::string_t::size_type len    = str.size();
	yaya::string_t::size_type found  = startpoint;

	while(true) {
		if (found >= len) {
			found = IsInDQ_runaway;
			break;
		}
		
		found = str.find_first_of(L"'\"",found);
		if (found == yaya::string_t::npos) {
			found = IsInDQ_runaway;
			break;
		}
		else {
			if (found >= checkpoint) {
				if ( (dq && str[found] == L'\"') || (quote && str[found] == L'\'') ) {
					found += 1;
					break;
				}
				if ( ! dq && ! quote ) {
					break;
				}
			}

			if (str[found] == L'\"') {
				if (!quote) {
					dq = !dq;
				}
			}
			else if (str[found] == L'\'') {
				if (!dq ) {
					quote = !quote;
				}
			}

			found += 1;
		}
	}

	if ( dq || quote ) {
		return found;
	}
	else {
		return IsInDQ_notindq;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsDoubleButNotIntString
 *  機能概要：  文字列がIntを除く実数数値として正当かを検査します
 *  注意　　：　整数値もDoubleとして正当な値なので実装時はIsIntStringとあわせること
 *
 *  返値　　：  0/1=×/○
 * -----------------------------------------------------------------------
 */
char	IsDoubleButNotIntString(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	int	dotcount = 0;
	for( ; i < len; i++) {
//		if (!::iswdigit((int)str[i])) {
		if (str[i] < L'0' || str[i] > L'9') {
			if (str[i] == L'.') {
				dotcount++;
			}
			else {
				return 0;
			}
		}
	}

	return dotcount == 1 && (len-advance) > 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsIntString
 *  機能概要：  文字列が10進整数数値として正当かを検査します
 *
 *  返値　　：  0/1=×/○
 * -----------------------------------------------------------------------
 */
char	IsIntString(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	//64bit
	//9223372036854775807
	if ( (len-i) > 19 ) { return 0; }

	for( ; i < len; i++) {
//		if (!::iswdigit((int)str[i]))
		if (str[i] < L'0' || str[i] > L'9') {
			return 0;
		}
	}

	if ( (len-advance) == 19 ) {
		// 符号を除いた桁だけを比べる　負の数は-9223372036854775808まで許す
		const yaya::char_t *limit = (str[0] == L'-') ? L"9223372036854775808" : L"9223372036854775807";
		if ( wcscmp(str.c_str() + advance,limit) > 0 ) {
			return 0; //Overflow
		}
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsIntBinString
 *  機能概要：  文字列が2進整数数値として正当かを検査します
 *  引数　　：  header 0/1=先頭"0x"なし/あり
 *
 *  返値　　：  0/1=×/○
 * -----------------------------------------------------------------------
 */
char	IsIntBinString(const yaya::string_t &str, char header)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	if (header) {
		if (::wcsncmp(PREFIX_BIN, str.c_str() + i,PREFIX_BASE_LEN))
			return 0;
		i += PREFIX_BASE_LEN;
	}

	//64bit
	if ( (len-i) > 64 ) { return 0; }
	
	for( ; i < len; i++) {
		yaya::char_t	j = str[i];
		if (j != L'0' && j != L'1')
			return 0;
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsIntHexString
 *  機能概要：  文字列が16進整数数値として正当かを検査します
 *
 *  返値　　：  0/1=×/○
 * -----------------------------------------------------------------------
 */
char	IsIntHexString(const yaya::string_t &str, char header)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	if (header) {
		if (::wcsncmp(PREFIX_HEX, str.c_str() + i,PREFIX_BASE_LEN))
			return 0;
		i += PREFIX_BASE_LEN;
	}

	//64bit
	//7fffffffffffffff
	if ( (len-i) > 16 ) { return 0; }

	for( ; i < len; i++) {
		yaya::char_t	j = str[i];
		if (j >= L'0' && j <= L'9')
			continue;
		else if (j >= L'a' && j <= L'f')
			continue;
		else if (j >= L'A' && j <= L'F')
			continue;

		return 0;
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsLegalFunctionName
 *  機能概要：  文字列が関数名として適正かを判定します
 *
 *  返値　　：  0/非0=○/×
 *
 *  　　　　　  1/2/3/4/5/6=空文字列/数値のみで構成/先頭が数値もしくは"_"/使えない文字を含んでいる
 *  　　　　　  　システム関数と同名/制御文もしくは演算子と同名
 * -----------------------------------------------------------------------
 */
char	IsLegalFunctionName(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 1;

	if (IsIntString(str))
		return 2;

//	if (::iswdigit(str[0]) || str[0] == L'_')
//	if ((str[0] >= L'0' && str[0] <= L'9') || str[0] == L'_') //チェックする必要はなさそう
//		return 3;
	if (str[0] == L'_') //頭がアンダースコアは蹴らないとローカル変数とカブる
		return 3;

	for(int i = 0; i < len; i++) {
		yaya::char_t	c = str[i];
		if ((c >= (yaya::char_t)0x0000 && c <= (yaya::char_t)0x0026) ||
			(c >= (yaya::char_t)0x0028 && c <= (yaya::char_t)0x002d) ||
			 c == (yaya::char_t)0x002f ||
			(c >= (yaya::char_t)0x003a && c <= (yaya::char_t)0x0040) ||
			 c == (yaya::char_t)0x005b ||
			(c >= (yaya::char_t)0x005d && c <= (yaya::char_t)0x005e) ||
			 c == (yaya::char_t)0x0060 ||
			(c >= (yaya::char_t)0x007b && c <= (yaya::char_t)0x007f))
			return 4;
	}

	ptrdiff_t sysidx = CSystemFunction::FindIndex(str);
	if( sysidx >= 0 ) { return 5; }

	for(size_t i= 0; i < FLOWCOM_NUM; i++) {
		if (str == flowcom[i]) {
			return 6;
		}
	}
	for(size_t i= 0; i < FORMULATAG_NUM; i++) {
//		if (str == formulatag[i])
		if (str.find(formulatag[i]) != yaya::string_t::npos) {
			return 6;
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsLegalVariableName
 *  機能概要：  文字列が変数名として適正かを判定します
 *
 *  返値　　：  0/1～6/16非0=○(グローバル変数)/×/○(ローカル変数)
 *
 *  　　　　　  1/2/3/4/5/6=空文字列/数値のみで構成/先頭が数値/使えない文字を含んでいる
 *  　　　　　  　システム関数と同名/制御文もしくは演算子と同名
 * -----------------------------------------------------------------------
 */
char	IsLegalVariableName(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 1;

	if (IsIntString(str))
		return 2;

//	if (::iswdigit((int)str[0]))
//	if (str[0] >= L'0' && str[0] <= L'9') //チェックする必要はなさそう
//		return 3;

	for(int i = 0; i < len; i++) {
		yaya::char_t	c = str[i];
		if ((c >= (yaya::char_t)0x0000  && c <= (yaya::char_t)0x0026) ||
			(c >= (yaya::char_t)0x0028  && c <= (yaya::char_t)0x002d) ||
			 c == (yaya::char_t)0x002f ||
			(c >= (yaya::char_t)0x003a && c <= (yaya::char_t)0x0040) ||
			 c == (yaya::char_t)0x005b ||
			(c >= (yaya::char_t)0x005d && c <= (yaya::char_t)0x005e) ||
			 c == (yaya::char_t)0x0060 ||
			(c >= (yaya::char_t)0x007b && c <= (yaya::char_t)0x007f))
			return 4;
	}

	ptrdiff_t sysidx = CSystemFunction::FindIndex(str);
	if( sysidx >= 0 ) { return 5; }

	for(size_t i= 0; i < FLOWCOM_NUM; i++) {
		if (str == flowcom[i]) {
			return 6;
		}
	}
	for(size_t i= 0; i < FORMULATAG_NUM; i++) {
//		if (str == formulatag[i])
		if (str.find(formulatag[i]) != yaya::string_t::npos) {
			return 6;
		}
	}

	return (str[0] == L'_') ? 16 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsLegalStrLiteral
 *  機能概要：  ダブルクォートで囲まれているべき文字列の正当性を検査します
 *
 *  返値　　：  0/1/2/3=正常/ダブルクォートが閉じていない/
 *  　　　　　  　ダブルクォートで囲まれているがその中にダブルクォートが包含されている/
 *  　　　　　  　ダブルクォートで囲まれていない
 * -----------------------------------------------------------------------
 */
char	IsLegalStrLiteral(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 3;

	// 先頭のダブルクォートチェック
	int	flg = (str[0] == L'\"') ? 1 : 0;
	// 後端のダブルクォートチェック
	if (len > 1)
		if (str[len - 1] == L'\"')
			flg += 2;
	// 内包されているダブルクォートの探索
	if(len > 2) {
		int lenm1 = len - 1;
		int i	  = 1;
		while(i < lenm1) {
			if(str[i] == L'\"') {
				if(str[i + 1] != L'\"') {
					flg = 4;
					break;
				}
				else
					i++;
			}
			i++;
		}
	}

	// 結果を返します
	switch(flg) {
	case 3:
		return 0;
	case 1:
	case 2:
	case 5:
	case 6:
		return 1;
	case 7:
		return 2;
	default:
		return 3;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsLegalPlainStrLiteral
 *  機能概要：  シングルクォートで囲まれているべき文字列の正当性を検査します
 *
 *  返値　　：  0/1/2/3=正常/ダブルクォートが閉じていない/
 *  　　　　　  　ダブルクォートで囲まれているがその中にダブルクォートが包含されている/
 *  　　　　　  　ダブルクォートで囲まれていない
 * -----------------------------------------------------------------------
 */
char	IsLegalPlainStrLiteral(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 3;

	// 先頭のシングルクォートチェック
	int	flg = (str[0] == L'\'') ? 1 : 0;
	// 後端のシングルクォートチェック
	if (len > 1)
		if (str[len - 1] == L'\'')
			flg += 2;
	// 内包されているシングルクォートの探索
	if (len > 2) {
		int	lenm1 = len - 1;
		int i	  = 1;
		while(i < lenm1) {
			if(str[i] == L'\'') {
				if(str[i + 1] != L'\'') {
					flg = 4;
					break;
				}
				else
					i++;
			}
			i++;
		}
	}

	// 結果を返します
	switch(flg) {
	case 3:
		return 0;
	case 1:
	case 2:
	case 5:
	case 6:
		return 1;
	case 7:
		return 2;
	default:
		return 3;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  GetEpochTime
 *  機能概要：  64bit対応の time() 相当
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
yaya::time_t FileTimeToEpochTime(const FILETIME &ft)
{
	ULARGE_INTEGER ul;
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;

	yaya::time_t tv = ul.QuadPart;
	tv -= LL_DEF(116444736000000000);
	tv /= LL_DEF(10000000);

	return tv;
}

static FILETIME EpochTimeToFileTime(yaya::time_t &tv)
{
	union {
		ULARGE_INTEGER ul;
		FILETIME ft;
	} tc;

	tc.ul.QuadPart = tv;
	tc.ul.QuadPart *= LL_DEF(10000000);
	tc.ul.QuadPart += LL_DEF(116444736000000000);

	return tc.ft;
}

#endif

yaya::time_t GetEpochTime()
{
#if defined(WIN32) || defined(_WIN32_WCE)
	FILETIME ft;
	::GetSystemTimeAsFileTime(&ft);

	return FileTimeToEpochTime(ft);
#else
	time_t tv;
	time(&tv);
	return (yaya::time_t)tv;
#endif
}

/* -----------------------------------------------------------------------
 *  タイムゾーンと暦の計算
 *
 *  localtime / mktime / Win32 の時刻変換APIは、扱える年の範囲やタイムゾーンの
 *  解釈が環境ごとに違い、範囲外の値で失敗する（localtime は NULL を返す）。
 *  そのためEPOCH秒と年月日時分秒の相互変換は64bitの暦計算で自前で行い、
 *  OSからは「あるUTC時刻でのローカルタイムのUTCオフセット」だけを借りる。
 * -----------------------------------------------------------------------
 */
static yaya::time_t FloorDiv(yaya::time_t a, yaya::time_t b)
{
	yaya::time_t q = a / b;
	if ( (a % b != 0) && ((a < 0) != (b < 0)) ) {
		--q;
	}
	return q;
}

// 1970-01-01 からの通算日数（グレゴリオ暦、month は 1-12）
static yaya::time_t DaysFromCivil(yaya::time_t y, int m, int d)
{
	if ( m <= 2 ) { y -= 1; }
	yaya::time_t era = FloorDiv(y, 400);
	yaya::time_t yoe = y - era * 400;
	yaya::time_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	yaya::time_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

static void CivilFromDays(yaya::time_t z, yaya::time_t &y, int &m, int &d)
{
	z += 719468;
	yaya::time_t era = FloorDiv(z, 146097);
	yaya::time_t doe = z - era * 146097;
	yaya::time_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	yaya::time_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	yaya::time_t mp = (5 * doy + 2) / 153;
	d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
	m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
	y = yoe + era * 400 + (m <= 2 ? 1 : 0);
}

// 扱うEPOCH秒の範囲（約±9億5千万年）。tm_year が int に収まり、オフセットを足してもあふれない
static const yaya::time_t EPOCH_LIMIT = LL_DEF(30000000000000000);

#if defined(WIN32) || defined(_WIN32_WCE)

typedef BOOL (WINAPI *DefSystemTimeToTzSpecificLocalTimeEx)(const void *pDynamicTzInfo, const SYSTEMTIME *pUtc, SYSTEMTIME *pLocal);
typedef BOOL (WINAPI *DefSystemTimeToTzSpecificLocalTime)(const TIME_ZONE_INFORMATION *pTzInfo, const SYSTEMTIME *pUtc, SYSTEMTIME *pLocal);

// UTC の SYSTEMTIME を、現在のタイムゾーンのローカルタイムにする
// その時点の夏時間の規則を使う（Windows 7 以降は年ごとの規則も反映される）
static bool SystemTimeUtcToLocal(const SYSTEMTIME &utc, SYSTEMTIME &local)
{
	static bool inited = false;
	static DefSystemTimeToTzSpecificLocalTimeEx pEx = NULL;
	static DefSystemTimeToTzSpecificLocalTime pOld = NULL;

	if ( ! inited ) {
		HMODULE hKernel = ::GetModuleHandleA("kernel32");
		if ( hKernel ) {
			pEx = (DefSystemTimeToTzSpecificLocalTimeEx)::GetProcAddress(hKernel,"SystemTimeToTzSpecificLocalTimeEx");
			pOld = (DefSystemTimeToTzSpecificLocalTime)::GetProcAddress(hKernel,"SystemTimeToTzSpecificLocalTime");
		}
		inited = true;
	}

	if ( pEx && pEx(NULL,&utc,&local) ) {
		return true;
	}
	if ( pOld && pOld(NULL,&utc,&local) ) {
		return true;
	}
	return false;
}

// 変換APIが使えないときの代用。今のバイアスだけを見る
static int GetLocalOffsetByBias()
{
	TIME_ZONE_INFORMATION tzinfo;
	DWORD r = ::GetTimeZoneInformation(&tzinfo);

	LONG bias = tzinfo.Bias;
	if ( r == TIME_ZONE_ID_DAYLIGHT ) {
		bias += tzinfo.DaylightBias;
	}
	else if ( r != TIME_ZONE_ID_INVALID ) {
		bias += tzinfo.StandardBias;
	}
	return static_cast<int>(-bias * 60);
}

static int GetLocalOffsetRaw(yaya::time_t utc)
{
	// FILETIME / SYSTEMTIME で表せる範囲に収める
	static const yaya::time_t lo = DaysFromCivil(1602,1,1) * 86400;
	static const yaya::time_t hi = DaysFromCivil(30000,1,1) * 86400;

	yaya::time_t t = utc;
	if ( t < lo ) { t = lo; }
	if ( t > hi ) { t = hi; }

	FILETIME ft = EpochTimeToFileTime(t);
	SYSTEMTIME su;
	SYSTEMTIME sl;
	if ( ::FileTimeToSystemTime(&ft,&su) && SystemTimeUtcToLocal(su,sl) ) {
		yaya::time_t lt = DaysFromCivil(sl.wYear,sl.wMonth,sl.wDay) * 86400
			+ sl.wHour * 3600 + sl.wMinute * 60 + sl.wSecond;
		return static_cast<int>(lt - t);
	}
	return GetLocalOffsetByBias();
}

// utc の時点でのローカルタイムの UTC オフセット（秒、東が正）。isdst は夏時間なら 1
static int GetLocalOffset(yaya::time_t utc, int *isdst)
{
	int offset = GetLocalOffsetRaw(utc);

	if ( isdst ) {
		// 夏時間かどうかはAPIから直接分からないので、その年の1月と7月のうち
		// オフセットの小さいほうを標準時とみなす
		yaya::time_t y;
		int m, d;
		CivilFromDays(FloorDiv(utc + offset,86400),y,m,d);

		int o1 = GetLocalOffsetRaw(DaysFromCivil(y,1,1) * 86400);
		int o2 = GetLocalOffsetRaw(DaysFromCivil(y,7,1) * 86400);
		int std_offset = (o1 < o2) ? o1 : o2;

		*isdst = (offset > std_offset) ? 1 : 0;
	}
	return offset;
}

#else

static int GetLocalOffset(yaya::time_t utc, int *isdst)
{
	if ( isdst ) { *isdst = 0; }

	time_t t = static_cast<time_t>(utc);
	if ( static_cast<yaya::time_t>(t) != utc ) {
		return 0;
	}

	// 環境変数 TZ の変更に追従する
	tzset();

	struct tm lt;
	if ( ! localtime_r(&t,&lt) ) {
		return 0;
	}
	if ( isdst ) { *isdst = (lt.tm_isdst > 0) ? 1 : 0; }
	return static_cast<int>(lt.tm_gmtoff);
}

#endif

bool EpochTimeToTM(yaya::time_t tv, const CTimeZone &tz, struct tm &out)
{
	memset(&out,0,sizeof(out));

	if ( tv > EPOCH_LIMIT || tv < -EPOCH_LIMIT ) {
		return false;
	}

	int isdst = 0;
	int offset = tz.is_local ? GetLocalOffset(tv,&isdst) : tz.offset_sec;

	yaya::time_t lt = tv + offset;
	yaya::time_t days = FloorDiv(lt,86400);
	int rem = static_cast<int>(lt - days * 86400);

	yaya::time_t y;
	int m, d;
	CivilFromDays(days,y,m,d);

	out.tm_sec = rem % 60;
	out.tm_min = (rem / 60) % 60;
	out.tm_hour = rem / 3600;
	out.tm_mday = d;
	out.tm_mon = m - 1;
	out.tm_year = static_cast<int>(y - 1900);
	out.tm_wday = static_cast<int>((days % 7 + 11) % 7); // 1970-01-01 は木曜
	out.tm_yday = static_cast<int>(days - DaysFromCivil(y,1,1)); // 0始まり（localtime と同じ）
	out.tm_isdst = isdst;

	return true;
}

bool TMToEpochTime(const struct tm &in, const CTimeZone &tz, yaya::time_t &out)
{
	// 月が1-12の外でも、年をずらして正規化する
	yaya::time_t y = static_cast<yaya::time_t>(in.tm_year) + 1900;
	yaya::time_t carry = FloorDiv(in.tm_mon,12);
	y += carry;
	int mon = static_cast<int>(in.tm_mon - carry * 12);

	// 日以下の桁あふれは線形に足すだけで正規化される
	yaya::time_t t = (DaysFromCivil(y,mon + 1,1) + in.tm_mday - 1) * 86400
		+ static_cast<yaya::time_t>(in.tm_hour) * 3600
		+ static_cast<yaya::time_t>(in.tm_min) * 60
		+ in.tm_sec;

	if ( t > EPOCH_LIMIT || t < -EPOCH_LIMIT ) {
		return false;
	}

	if ( tz.is_local ) {
		// ローカルタイムを UTC に直すには、その時刻でのオフセットが要る。
		// 夏時間の境目でずれないよう、求めたオフセットで引いた時刻のオフセットを使い直す
		int o1 = GetLocalOffset(t,NULL);
		int o2 = GetLocalOffset(t - o1,NULL);
		out = t - o2;
	}
	else {
		out = t - tz.offset_sec;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  ローカルタイムゾーンの情報（GETTIMEZONE / GETSETTING）
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)

// DYNAMIC_TIME_ZONE_INFORMATION（VC6のSDKには無い）
struct DynamicTzInfo {
	LONG Bias;
	WCHAR StandardName[32];
	SYSTEMTIME StandardDate;
	LONG StandardBias;
	WCHAR DaylightName[32];
	SYSTEMTIME DaylightDate;
	LONG DaylightBias;
	WCHAR TimeZoneKeyName[128];
	BOOLEAN DynamicDaylightTimeDisabled;
};

typedef DWORD (WINAPI *DefGetDynamicTimeZoneInformation)(DynamicTzInfo *pInfo);
// ICU (Windows 10 1903 以降の icu.dll)。UChar は 16bit
typedef int (__cdecl *DefUcalGetTimeZoneIDForWindowsID)(const unsigned short *winid, int len, const char *region, unsigned short *id, int capacity, int *status);

bool GetLocalTimeZoneInfo(yaya::time_t utc, int &offset, int &isdst, yaya::string_t &name)
{
	if ( utc > EPOCH_LIMIT || utc < -EPOCH_LIMIT ) {
		return false;
	}

	offset = GetLocalOffset(utc,&isdst);

	// 名前はOSの表示名（日本語環境なら「東京 (標準時)」）
	TIME_ZONE_INFORMATION tzinfo;
	::GetTimeZoneInformation(&tzinfo);
	name = isdst ? tzinfo.DaylightName : tzinfo.StandardName;

	return true;
}

yaya::string_t GetLocalTimeZoneId()
{
	static bool inited = false;
	static DefGetDynamicTimeZoneInformation pGetDynamic = NULL;
	static DefUcalGetTimeZoneIDForWindowsID pUcalConv = NULL;

	if ( ! inited ) {
		HMODULE hKernel = ::GetModuleHandleA("kernel32");
		if ( hKernel ) {
			pGetDynamic = (DefGetDynamicTimeZoneInformation)::GetProcAddress(hKernel,"GetDynamicTimeZoneInformation");
		}
		// システムフォルダのDLLだけを読む（0x800 = LOAD_LIBRARY_SEARCH_SYSTEM32）
		HMODULE hIcu = ::LoadLibraryExW(L"icu.dll",NULL,0x00000800);
		if ( hIcu ) {
			pUcalConv = (DefUcalGetTimeZoneIDForWindowsID)::GetProcAddress(hIcu,"ucal_getTimeZoneIDForWindowsID");
		}
		inited = true;
	}

	// Windows のタイムゾーン名（"Tokyo Standard Time"）を、ICU で IANA の名前（"Asia/Tokyo"）にする。
	// ICU の既定のタイムゾーン（ucal_getDefaultTimeZone）は最初に決めた値を覚えてしまうので、
	// 実行中の設定変更に追従するよう、毎回OSから名前を取って変換する
	if ( ! pGetDynamic || ! pUcalConv ) {
		return yaya::string_t();
	}

	DynamicTzInfo dtzi;
	memset(&dtzi,0,sizeof(dtzi));
	if ( pGetDynamic(&dtzi) == TIME_ZONE_ID_INVALID || ! dtzi.TimeZoneKeyName[0] ) {
		return yaya::string_t();
	}

	unsigned short buf[128];
	int status = 0;
	int len = pUcalConv(reinterpret_cast<const unsigned short*>(dtzi.TimeZoneKeyName),-1,NULL,buf,128,&status);
	if ( status > 0 || len <= 0 || len >= 128 ) { // 正の値がエラー（負は警告）
		return yaya::string_t();
	}

	yaya::string_t result;
	for ( int i = 0 ; i < len ; ++i ) {
		result.append(1,static_cast<yaya::char_t>(buf[i]));
	}
	return result;
}

#else

bool GetLocalTimeZoneInfo(yaya::time_t utc, int &offset, int &isdst, yaya::string_t &name)
{
	if ( utc > EPOCH_LIMIT || utc < -EPOCH_LIMIT ) {
		return false;
	}

	offset = GetLocalOffset(utc,&isdst);

	// 略称（"JST"）
	name.erase();
	time_t t = static_cast<time_t>(utc);
	struct tm lt;
	if ( static_cast<yaya::time_t>(t) == utc && localtime_r(&t,&lt) && lt.tm_zone ) {
		Ccct::MbcsToUcs2Buf(name,lt.tm_zone,CHARSET_UTF8);
	}

	return true;
}

yaya::string_t GetLocalTimeZoneId()
{
	std::string id;

	// 環境変数 TZ があればそれ。無ければ /etc/localtime のリンク先（.../zoneinfo/Asia/Tokyo）、
	// それも無ければ /etc/timezone
	const char *tz = getenv("TZ");
	if ( tz && *tz ) {
		id = tz;
		if ( id[0] == ':' ) {
			id.erase(0,1);
		}
	}
	else {
		char buf[1024];
		ssize_t n = readlink("/etc/localtime",buf,sizeof(buf) - 1);
		if ( n > 0 ) {
			buf[n] = 0;
			id = buf;
		}
		else {
			FILE *fp = fopen("/etc/timezone","r");
			if ( fp ) {
				if ( fgets(buf,sizeof(buf),fp) ) {
					id = buf;
				}
				fclose(fp);
			}
		}
	}

	// 絶対パスなら zoneinfo/ より後ろだけにする（posix/ right/ の下も同じ名前）
	std::string::size_type pos = id.find("zoneinfo/");
	if ( pos != std::string::npos ) {
		id.erase(0,pos + 9);
		if ( id.compare(0,6,"posix/") == 0 ) { id.erase(0,6); }
		else if ( id.compare(0,6,"right/") == 0 ) { id.erase(0,6); }
	}

	while ( ! id.empty() && (id[id.size() - 1] == '\n' || id[id.size() - 1] == '\r' || id[id.size() - 1] == ' ') ) {
		id.erase(id.size() - 1);
	}

	yaya::string_t result;
	Ccct::MbcsToUcs2Buf(result,id,CHARSET_UTF8);
	return result;
}

#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  EscapeString
 *  機能概要：  セーブファイル保存の際に有害な文字を置き換えます
 * -----------------------------------------------------------------------
 */

void	EscapeString(yaya::string_t &wstr)
{
	yaya::ws_replace(wstr, L"\"", ESC_DQ);

	for ( size_t i = 0 ; i < wstr.length() ; ++i ) {
		if ( wstr[i] <= END_OF_CTRL_CH ) {
			yaya::string_t replace_text(ESC_CTRL);
			replace_text += (yaya::char_t)(wstr[i] + CTRL_CH_START);

			wstr.replace(i,1,replace_text);
			i += replace_text.length() - 1; //置き換え処理した後に移動
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  UnescapeString
 *  機能概要：  セーブファイル読み込みの際に有害な文字を戻します
 * -----------------------------------------------------------------------
 */
void	UnescapeString(yaya::string_t &wstr)
{
	yaya::ws_replace(wstr, ESC_DQ, L"\"");

	yaya::string_t::size_type found = 0;
	const size_t len = ::wcslen(ESC_CTRL);
	yaya::char_t ch;
	yaya::char_t str[2] = L"x"; //置き換え用ダミー

	while(true) {
		found = wstr.find(ESC_CTRL,found);
		if ( found == yaya::string_t::npos ) {
			break;
		}

		ch = wstr[found + len];
		if ( ch > CTRL_CH_START && ch <= (CTRL_CH_START + END_OF_CTRL_CH) ) {
			str[0] = ch - CTRL_CH_START;
			wstr.replace(found,len + 1,str);
			found += 1;
		}
		else { //範囲外だったので無視
			found += len;
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  EncodeBase64
 * -----------------------------------------------------------------------
 */

void EncodeBase64(yaya::string_t &out,const char *in,size_t in_len)
{
	int len = in_len;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in);
	static const yaya::char_t table[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	while (len > 0)
	{
		// 1文字目 1-6bit  xxxxxx--:--------:--------
		out.append(1,table[static_cast<int>(*p)>>2]);
		
		// 2文字目 7-12bit ------xx:xxxx----:--------
		if ( len-1 > 0 )
			out.append(1,table[((static_cast<int>(*p) << 4)&0x30) | ((static_cast<int>(*(p+1)) >> 4)&0x0f)]);
		else
			out.append(1,table[((static_cast<int>(*p) << 4)&0x30) ]);
		
		--len;
		++p;
		
		// 3文字目 13-18bit --------:----xxxx:xx------
		if ( len > 0 ) {
			if ( len-1 > 0 ) {
				out.append(1,table[((static_cast<int>(*p) << 2)&0x3C) | ((static_cast<int>(*(p+1)) >> 6)&0x03)]);
			}
			else {
				out.append(1,table[((static_cast<int>(*p) << 2)&0x3C) ]);
			}
			++p;
		}
		else {
			out.append(1,L'=');
		}
		
		// 4文字目 19-24bit --------:--------:--xxxxxx
		out.append(1,(--len>0? table[static_cast<int>(*p) & 0x3F]: L'='));
		
		if(--len>0) p++;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  DecodeBase64
 * -----------------------------------------------------------------------
 */

void DecodeBase64(std::string &out,const yaya::char_t *in,size_t in_len)
{
	static const unsigned char reverse_64[] = {
		//0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00 - 0x0F
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10 - 0x1F
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,   // 0x20 - 0x2F
		 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,   // 0x30 - 0x3F
		  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   // 0x40 - 0x4F
		 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,   // 0x50 - 0x5F
		  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   // 0x60 - 0x6F
		 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0    // 0x70 - 0x7F
	};

	const yaya::char_t* p = in;

	while (*p!='=')
	{
		//11111122:22223333:33444444
		if ( (*p=='\0') || (*(p+1)=='=') ) break;
		out.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<2) & 0xFC | (reverse_64[*(p+1)&0x7f] >>4) & 0x03));
		++p;

		if ( (*p=='\0') || (*(p+1)=='=') ) break;
		out.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<4) & 0xF0 | (reverse_64[*(p+1)&0x7f] >>2) & 0x0F));
		++p;

		if ( (*p=='\0') || (*(p+1)=='=') ) break;
		out.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<6) & 0xC0 | reverse_64[*(p+1)&0x7f] & 0x3f ));
		++p;

		if ( (*p=='\0') || (*(p+1)=='=') ) break;
		++p;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  EncodeURL
 * -----------------------------------------------------------------------
 */

void EncodeURL(yaya::string_t &out,const char *in,size_t in_len,bool isPlusPercent)
{
	yaya::char_t chr[4] = L"%00";
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in);

	for ( size_t i = 0 ; i < in_len ; ++i ) {
		int current = static_cast<unsigned char>(p[i]);
		if ( (current >= 'a' && current <= 'z') || (current >= 'A' && current <= 'Z') || (current >= '0' && current <= '9') || current == '.' || current == '_' || current == '-' ) {
			out.append(1,current);
		}
		else if ( (current == L' ') && isPlusPercent ) {
			out.append(1,L'+');
		}
		else {
			yaya::snprintf(chr+1,4,L"%02X",current);
			out.append(chr);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  DecodeURL
 * -----------------------------------------------------------------------
 */

void DecodeURL(std::string &out,const yaya::char_t *in,size_t in_len,bool isPlusPercent)
{
	char ch[3] = {0,0,0};

	for ( size_t pos = 0 ; pos < in_len ; ++pos ) {

		if ( in[pos] == L'%' && (in_len - pos) >= 3) {
			ch[0] = static_cast<char>(in[pos+1]);
			ch[1] = static_cast<char>(in[pos+2]);

			out.append(1,static_cast<char>(strtol(ch,NULL,16)));

			pos += 2;
		}
		else if ( isPlusPercent && in[pos] == L'+' ) {
			out.append(1,' ');
		}
		else {
			out.append(1,static_cast<char>(in[pos]));
		}
	}
}

