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
#include <string>
#include <vector>

#include "manifest.h"
#include "misc.h"
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
		dstr1 = L"";
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
		dstr1 = L"";
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
		dstr1 = L"";
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
	yaya::string_t::size_type it = yaya::string_t::npos;
	yaya::string_t::size_type found;

	while ( (found = str.find(findstr,it)) != yaya::string_t::npos ) {
		it = found;
	}

	return it;
}

yaya::string_t::size_type find_last_str(const yaya::string_t &str, const yaya::string_t &findstr)
{
	return find_last_str(str,findstr.c_str());
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
		dstr1 = L"";
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
int	SplitToMultiString(const yaya::string_t &str, std::vector<yaya::string_t> *array, const yaya::string_t &delimiter)
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
				array->push_back(yaya::string_t(str.begin()+beforepoint,str.end()));
			}
			break;
		}
		// 取り出しとvectorへの追加
		if ( array ) {
			array->push_back(yaya::string_t(str.begin()+beforepoint,str.begin()+seppoint));
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
 *              CParser0::LoadDictionary1 も参照してください
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
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(buf, 127, "%Y/%m/%d %H:%M:%S", tm);

#if !defined(POSIX) && !defined(__MINGW32__)
	yaya::char_t wbuf[64];
	for ( size_t i = 0 ; i < 64 ; ++i ) {
		wbuf[i] = buf[i];
		if ( wbuf[i] == 0 ) { break; }
	}
	return wbuf;
#else
    return widen(std::string(buf));
#endif
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

	//32bit
	//2147483647
	if ( (len-i) > 10 ) { return 0; }

	for( ; i < len; i++) {
//		if (!::iswdigit((int)str[i]))
		if (str[i] < L'0' || str[i] > L'9') {
			return 0;
		}
	}

	if ( (len-advance) == 10 ) {
		if ( wcscmp(str.c_str(),L"2147483647") > 0 ) {
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

	//32bit
	if ( (len-i) > 32 ) { return 0; }
	
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

	//32bit
	//7fffffff
	if ( (len-i) > 8 ) { return 0; }

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

	int sysidx = CSystemFunction::FindIndex(str);
	if ( sysidx >= 0 ) { return 5; }

	for(size_t i= 0; i < FLOWCOM_NUM; i++) {
		if (!str.compare(flowcom[i])) {
			return 6;
		}
	}
	for(size_t i= 0; i < FORMULATAG_NUM; i++) {
//		if (!str.compare(formulatag[i]))
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

	int sysidx = CSystemFunction::FindIndex(str);
	if ( sysidx >= 0 ) { return 5; }

	for(size_t i= 0; i < FLOWCOM_NUM; i++) {
		if (!str.compare(flowcom[i])) {
			return 6;
		}
	}
	for(size_t i= 0; i < FORMULATAG_NUM; i++) {
//		if (!str.compare(formulatag[i]))
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
	if (len > 2) {
		int	lenm1 = len - 1;
		for(int i = 1; i < lenm1; i++)
			if (str[i] == L'\"') {
				flg = 4;
				break;
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
		for(int i = 1; i < lenm1; i++)
			if (str[i] == L'\'') {
				flg = 4;
				break;
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
 *  関数名  ：  IsNt
 *  機能概要：  0/1=9x系/NT系　を返します
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
char	IsNt(void)
{
	OSVERSIONINFO	osi = { sizeof(OSVERSIONINFO) };

	GetVersionEx(&osi);
	return (osi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? 1 : 0;
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
 *  関数名  ：  EncodeURL
 * -----------------------------------------------------------------------
 */

void EncodeURL(yaya::string_t &out,const char *in,size_t in_len)
{
	yaya::char_t chr[4] = L"%00";
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in);

	for ( size_t i = 0 ; i < in_len ; ++i ) {
		int current = static_cast<unsigned char>(p[i]);
		if ( (current >= 'a' && current <= 'z') || (current >= 'A' && current <= 'Z') || (current >= '0' && current <= '9') || current == '.' || current == '_' || current == '-' ) {
			out.append(1,current);
		}
		else if ( current == L' ' ) {
			out.append(1,L'+');
		}
		else {
			yaya::snprintf(chr+1,4,L"%02X",current);
			out.append(chr);
		}
	}
}
