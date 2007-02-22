// 
// AYA version 5
//
// コメントアウト処理クラス　CComment
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "comment.h"
#include "misc.h"
#include "globaldef.h"

/* -----------------------------------------------------------------------
 *  関数名  ：  CComment::Process
 *  機能概要：  コメントアウト処理（/＊～＊/）
 * -----------------------------------------------------------------------
 */
void	CComment::Process(yaya::string_t &str)
{
	if (str.empty()) { return; }

	yaya::string_t	outstr;
	while(true) {
		yaya::string_t::size_type found0 = Find_IgnoreDQ(str,L"/*");
		yaya::string_t::size_type found1 = Find_IgnoreDQ(str,L"*/");

		if (found0 == yaya::string_t::npos && found1 == yaya::string_t::npos) {
			if (!flag)
				outstr.append(str);
			break;
		}
		if (found0 == yaya::string_t::npos || (found0 != yaya::string_t::npos && found1 != yaya::string_t::npos && found0 >= found1)) {
			if (!flag)
				outstr.append(str,0,found1);
			flag = 0;
			str.erase(0,found1+2); //+2は */のぶん
		}
		else {
			if (!flag)
				outstr.append(str,0,found0);
			flag = 1;
			str.erase(0,found0+2); //+2は /*のぶん
		}
	}
	str = outstr;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CComment::Process_Top
 *  機能概要：  コメントアウト処理（先頭の//）
 * -----------------------------------------------------------------------
 */
void	CComment::Process_Top(yaya::string_t &str)
{
	if (str.empty()) { return; }
	
	const wchar_t *pt = str.c_str();
	
	while ( *pt == L' ' || *pt == L'\t' ) { ++pt; }
	
	//コメント
	if ( *pt == L'/' && *(pt+1) == L'/' ) { str.erase(); }
	
	//ついでに空白だけの行もなかったことに
	if ( *pt == 0 ) { str.erase(); }
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CComment::Process_Tail
 *  機能概要：  コメントアウト処理（中途の//）
 * -----------------------------------------------------------------------
 */
void	CComment::Process_Tail(yaya::string_t &str)
{
	if (str.empty()) { return; }

	yaya::string_t::size_type found = Find_IgnoreDQ(str, L"//");
	if ( found != yaya::string_t::npos ) {
		str.erase(found, (int)str.size() - found);
		CutSpace(str);
	}
}
