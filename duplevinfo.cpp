// 
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
	if ( UpdateNums(areanum, values) ) {
		lastroundorder = -1;
		InitRoundOrder(vm,mode);
	}

	// 値の取得と巡回制御
	CValue	result = GetValue(vm, areanum, values);

	lastroundorder = roundorder[index];

	// 巡回位置を進める　巡回が完了したら巡回順序を初期化する
	index++;
	if ( index >= static_cast<int>(roundorder.size()) ) {
		InitRoundOrder(vm,mode);
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::InitRoundOrder
 *  機能概要：  巡回順序を初期化します
 * -----------------------------------------------------------------------
 */
void	CDuplEvInfo::InitRoundOrder(CAyaVM &vm,int mode_param)
{
	// 初期化
	index = 0;
	roundorder.clear();
	roundorder.reserve(total);

	int mode = mode_param & CHOICETYPE_SELECT_FILTER;

    if ( mode == CHOICETYPE_NONOVERLAP_FLAG ) {
		for(size_t i = 0; i < total; ++i) {
			if ( i != lastroundorder ) {
				roundorder.emplace_back(i);
			}
		}

		//緊急時エラー回避用
		if ( ! roundorder.size() ) {
			roundorder.emplace_back(0);
		}

		//シャッフルする
		size_t n = roundorder.size();
		if ( n >= 2 ) {
			for (size_t i = 0 ; i < n ; ++i ) {
				size_t s = vm.genrand_int(n);
				if ( i != s ) {
					int tmp = roundorder[i];
					roundorder[i] = roundorder[s];
					roundorder[s] = tmp;
				}
			}
		}

		//lastroundorderは i = 1 以降 (2個目以降) のランダムな位置に差し込む
		if ( lastroundorder >= 0 ) {
			if ( n >= 2 ) {
				size_t lrand = vm.genrand_int(n) + 1;
				if ( lrand == n ) {
					roundorder.emplace_back(lastroundorder);
				}
				else {
					roundorder.insert(roundorder.begin() + lrand,lastroundorder);
				}
			}
			else {
				roundorder.emplace_back(lastroundorder);
			}
		}
	}
	else {
		for(size_t i = 0; i < total; ++i) {
			roundorder.emplace_back(i);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CDuplEvInfo::UpdateNums
 *  機能概要：  領域毎の候補数と総数を更新します
 *  返値　　　  0/1=変化なし/あり
 * -----------------------------------------------------------------------
 */
bool	CDuplEvInfo::UpdateNums(int areanum, const std::vector<CVecValue> &values)
{
	// 元の候補数を保存しておく
	int	bef_numlenm1 = num.size() - 1;

	// 領域毎の候補数と組み合わせ総数を更新
	// 候補数に変化があった場合はフラグに記録する
	bool changed = areanum != bef_numlenm1;
	if ( changed ) {
		num.resize(areanum+1);
	}
	total = 1;

	for(int i = 0; i <= areanum; i++) {
		int	t_num = values[i].array.size();

		if (num[i] != t_num) {
			changed = true;
		}

		total *= t_num;
		num[i] = t_num;
	}

	return changed;
}

bool	CDuplEvInfo::UpdateNums(const CValue& value)
{
	bool changed = false;

	if(num.size()!=1) {
		num.resize(1);
		changed = true;
	}

	if (num[0] != value.array().size()) {
		changed = true;
		total = num[0] = value.array().size();
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
