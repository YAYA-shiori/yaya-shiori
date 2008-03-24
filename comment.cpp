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

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CComment::Process
 *  機能概要：  コメントアウト処理（/＊～＊/）
 * -----------------------------------------------------------------------
 */
void	CComment::Process(yaya::string_t &str)
{
	Process_Top(str);
	Process_Tail(str);

	if (str.empty()) { return; }

	yaya::string_t::size_type found0;
	yaya::string_t::size_type found1;

	while ( true ) {
		//前行からコメントが続いてる
		if ( flag ) {
			found1 = str.find(L"*/"); //コメント終了位置を探索 これはクォートに影響されない

			if ( found1 != yaya::string_t::npos ) {
				str.erase(0,found1+2); //+2は */ の分
				flag = 0;
			}
			else {
				str.erase(); //全部なかったことに
				break;
			}
		}
		//続いてない
		else {
			found0 = Find_IgnoreDQ(str,L"/*");//スタートのみクォートを考慮

			if ( found0 != yaya::string_t::npos ) { //スタート位置が同じ行内で見つかった

				found1 = str.find(L"*/",found0+2);

				if ( found1 != yaya::string_t::npos ) { //ストップ位置も同じ行内で見つかった
					str.erase(found0,(found1+2-found0)); //+2は */ の分
				}
				else {
					str.erase(found0,str.size()-found0); //残り全部ばっさり
					flag = 1;
					break; //とりあえずこの行は処理終了
				}
			}
			else { //もうコメントはない
				break;
			}
		}
	}
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
