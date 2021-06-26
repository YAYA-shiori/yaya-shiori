﻿// 
// AYA version 5
//
// 重複回避制御を行なうクラス　CDuplEvInfo
// - 主処理部
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <functional>

#include "selecter.h"

#include "log.h"
#include "globaldef.h"
#include "sysfunc.h"
#include "ayavm.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::Choice
 *  機能概要：  候補から選択して出力します
 * -----------------------------------------------------------------------
 */
CValue	CDuplEvInfo::Choice(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values, int mode)
{
	// 領域毎の候補数と総数を更新　変化があった場合は巡回順序を初期化する
	if (UpdateNums(areanum, values))
		InitRoundOrder(vm,mode);

	// 値の取得と巡回制御
	CValue	result = GetValue(vm, areanum, values);

	lastroundorder = roundorder[index];

	// 巡回位置を進める　巡回が完了したら巡回順序を初期化する
	index++;
	if (index >= static_cast<int>(roundorder.size()) )
		InitRoundOrder(vm,mode);

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::InitRoundOrder
 *  機能概要：  巡回順序を初期化します
 * -----------------------------------------------------------------------
 */
void	CDuplEvInfo::InitRoundOrder(CAyaVM &vm,int mode)
{
	// 初期化
	index = 0;
	roundorder.clear();
	roundorder.reserve(total);

    if (mode == CHOICETYPE_NONOVERLAP) {
		for(int i = 0; i < total; ++i) {
			if ( i != lastroundorder ) {
				roundorder.push_back(i);
			}
		}

		//緊急時エラー回避用
		if ( ! roundorder.size() ) {
			roundorder.push_back(0);
		}

		int n = roundorder.size();
		for ( int i = 0 ; i < n ; ++i ) {
			int s = vm.genrand_int(n);
			if ( i != s ) {
				int tmp = roundorder[i];
				roundorder[i] = roundorder[s];
				roundorder[s] = tmp;
			}
		}
	}
	else {
		for(int i = 0; i < total; ++i) {
			roundorder.push_back(i);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::UpdateNums
 *  機能概要：  領域毎の候補数と総数を更新します
 *  返値　　　  0/1=変化なし/あり
 * -----------------------------------------------------------------------
 */
char	CDuplEvInfo::UpdateNums(int areanum, const std::vector<CVecValue> &values)
{
	// 元の候補数を保存しておく
	std::vector<int>	bef_num(num.begin(), num.end());
	int	bef_numlenm1 = bef_num.size() - 1;

	// 領域毎の候補数と組み合わせ総数を更新
	// 候補数に変化があった場合はフラグに記録する
	num.clear();
	total = 1;
	char	changed = (areanum != bef_numlenm1) ? 1 : 0;
	for(int i = 0; i <= areanum; i++) {
		int	t_num = values[i].array.size();
		num.push_back(t_num);
		total *= t_num;
		if (i <= bef_numlenm1)
			if (bef_num[i] != t_num)
				changed = 1;
	}

	return changed;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::GetValue
 *  機能概要：  現在の巡回位置から値を取得します
 *
 *  格納領域が一つしかない場合はそれをそのまま出すので値の型が保護されます。
 *  領域が複数ある場合はそれらは文字列として結合されますので、文字列型での出力となります。
 * -----------------------------------------------------------------------
 */
CValue	CDuplEvInfo::GetValue(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values)
{
	int	t_index = roundorder[index];
	
	vm.sysfunction().SetLso(t_index);

	if (areanum) {
		yaya::string_t	result;
		for ( int i = 0; i <= areanum; i++ ) {
			if ( num[i] ) {
				int	next = t_index/num[i];
				result += values[i].array[t_index - next*(num[i])].GetValueString();
				t_index = next;
			}
		}
		return CValue(result);
	}
	else
		return values[0].array[t_index];
}
