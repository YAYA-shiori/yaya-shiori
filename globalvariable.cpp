// 
// AYA version 5
//
// グローバル変数を扱うクラス　CGlobalVariable
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "variable.h"

#include "ayavm.h"
#include "basis.h"
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
 *  関数名  ：  CGlobalVariable::Make
 *  機能概要：  変数を作成します
 *  引数　　：  erased 0/1=有効状態/消去状態
 *
 *  返値　　：  追加した位置
 *
 *  指定された名前の変数が既に存在していた場合は追加は行わず、既存の位置を返します
 * -----------------------------------------------------------------------
 */
int	CGlobalVariable::Make(const yaya::string_t &name, char erased)
{
	// 既に存在するか調べ、存在していたらそれを返す
	yaya::indexmap::const_iterator it = varmap.find(name);
	if ( it != varmap.end() ) {
		int idx = it->second;
		if (!vm.basis().IsRun()) {
			if (erased)
				var[idx].Erase();
			else
				var[idx].Enable();
		}
		return idx;
	}

	// 作成
	CVariable	addvariable(name);
	if (erased)
		addvariable.Erase();
	else
		addvariable.Enable();
	var.push_back(addvariable);

	int idx = var.size() - 1;
	varmap.insert(yaya::indexmap::value_type(name,idx));
	return idx;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CGlobalVariable::GetMacthedLongestNameLength
 *  機能概要：  指定された文字列にマッチする名前を持つ変数を探し、マッチした長さを返します
 *
 *  複数見つかった場合は最長のものを返します。見つからなかった場合は0を返します
 * -----------------------------------------------------------------------
 */
size_t	CGlobalVariable::GetMacthedLongestNameLength(const yaya::string_t &name)
{
	size_t	max_len = 0;

	for(std::vector<CVariable>::iterator it = var.begin(); it != var.end(); it++) {
		size_t	len = it->name.size();
		if (!it->IsErased() && max_len < len && !name.compare(0,len,it->name,0,len))
			max_len = len;
	}

	return max_len;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CGlobalVariable::GetIndex
 *  機能概要：  指定された名前の変数の位置を返します
 *
 *  見つからなかった場合は-1を返します
 * -----------------------------------------------------------------------
 */
int		CGlobalVariable::GetIndex(const yaya::string_t &name)
{
	yaya::indexmap::const_iterator it = varmap.find(name);
	if ( it != varmap.end() ) {
		int idx = it->second;
		return var[idx].IsErased() ? -1 : idx;
	}
	return -1;
}
