// 
// AYA version 5
//
// 出力の選択を行なうクラス　CSelecter
// - 主処理部
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "selecter.h"

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
 * CSelecterコンストラクタ
 * -----------------------------------------------------------------------
 */
CSelecter::CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, int aid) : vm(vmr), duplctl(dc), aindex(aid)
{
	areanum = 0;

	CVecValue	addvv;
	values.emplace_back(addvv);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::AddArea
 *  機能概要：  新しい出力格納用の領域を用意します
 * -----------------------------------------------------------------------
 */
void	CSelecter::AddArea(void)
{
	// 追加前の領域が空だった場合はダミーの空文字列を追加
	if (!values[areanum].array.size())
		Append(CValue());

	// 領域を追加
	CVecValue	addvv;
	values.emplace_back(addvv);
	areanum++;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::Append
 *  機能概要：  候補値を追加します
 * -----------------------------------------------------------------------
 */
void	CSelecter::Append(const CValue &value)
{
	//空文字列はやっぱり追加しないとまずい
	//if (value.GetType() == F_TAG_STRING && !value.s_value.size())
	//	return;
	if (value.GetType() == F_TAG_VOID)
		return;

	values[areanum].array.emplace_back(value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::Output
 *  機能概要：  各領域から値を抽出して出力を作成し返します
 *  引数　　：  duplctl 重複回避情報へのポインタ
 *  　　　　　          NULLに場合はランダム選択で動作します
 *
 *  ランダム選択はこのクラスで処理します。
 *  nonoverlao/sequential選択はCDuplEvInfoクラスに任せます。
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::Output()
{
	// switch選択
	if (aindex >= BRACE_SWITCH_OUT_OF_RANGE) {
		vm.sysfunction().SetLso(aindex);
		return ChoiceByIndex();
	}

	// 領域が1つしかなく、かつ候補が存在しない場合は出力なし
	if (!areanum && !values[0].array.size()) {
		vm.sysfunction().SetLso(-1);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	// 最後の領域が空だった場合はダミーの空文字列を追加
	if (!values[areanum].array.size())
		Append(CValue());

	// ランダム選択
	if (duplctl == NULL)
		return ChoiceRandom();

	// 重複回避制御付き選択
	switch(duplctl->GetType()) {
	case CHOICETYPE_NONOVERLAP:
	case CHOICETYPE_NONOVERLAP_POOL:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_NONOVERLAP);
	case CHOICETYPE_SEQUENTIAL:
	case CHOICETYPE_SEQUENTIAL_POOL:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_SEQUENTIAL);
	case CHOICETYPE_VOID:
		return CValue(F_TAG_NOP, 0/*dmy*/);
	case CHOICETYPE_ARRAY:
	case CHOICETYPE_POOL_ARRAY:
		return StructArray();
	case CHOICETYPE_RANDOM:
	case CHOICETYPE_POOL:
	default:
		return ChoiceRandom();
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::ChoiceRandom
 *  機能概要：  各領域からランダムに値を抽出して出力を作成し返します
 *
 *  格納領域が一つしかない場合はそれをそのまま出すので値の型が保護されます。
 *  領域が複数ある場合はそれらは文字列として結合されますので、文字列型での出力となります。
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceRandom(void)
{
	if (areanum) {
		yaya::string_t	result;
		for(int i = 0; i <= areanum; i++)
			result += ChoiceRandom1(i).GetValueString();
		return CValue(result);
	}
	else
		return ChoiceRandom1(0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::ChoiceRandom1
 *  機能概要：  指定された領域からランダムに値を抽出します
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceRandom1(int index)
{
	if ( ! values[index].array.size() ) {
		return CValue();
	}

    int choice = vm.genrand_int(static_cast<int>(values[index].array.size()));

    vm.sysfunction().SetLso(choice);

    return values[index].array[choice];
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::ChoiceByIndex
 *  機能概要：  各領域から指定位置の値を抽出して出力を作成し返します
 *
 *  格納領域が一つしかない場合はそれをそのまま出すので値の型が保護されます。
 *  領域が複数ある場合はそれらは文字列として結合されますので、文字列型での出力となります。
 *
 *  指定より候補数が少ない場合は空文字列が出力されます。
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceByIndex()
{
	// 最後の領域が空だった場合はダミーの空文字列を追加
	if (!values[areanum].array.size())
		Append(CValue());

	// 主処理
	if (areanum) {
		yaya::string_t	result;
		for(int i = 0; i <= areanum; i++)
			result += ChoiceByIndex1(i).GetValueString();
		return CValue(result);
	}
	else
		return ChoiceByIndex1(0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::ChoiceByIndex
 *  機能概要：  指定された領域から指定位置の値を抽出します
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceByIndex1(int index)
{
	int	num = values[index].array.size();

	if (!num)
		return CValue();

	return (aindex >= 0 && aindex < num) ? values[index].array[aindex] : CValue();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::StructArray
 *  機能概要：  各領域の値を結合した汎用配列を作成し返します
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructArray()
{
	if (areanum) {
		CValue	result(F_TAG_ARRAY, 0/*dmy*/);
		for(int i = 0; i <= areanum; i++) {
			result = result + StructArray1(i);
//			result = result + StructArray1(i).GetValueString();
		}
		return result;
	}
	else {
		return StructArray1(0);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::StructArray1
 *  機能概要：  指定された領域の値を結合した汎用配列を作成します
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructArray1(int index)
{
	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

    for(size_t i = 0; i < values[index].array.size(); ++i) {
		const CValue &target = values[index].array[i];
		int	valtype = target.GetType();
		
		if (valtype == F_TAG_ARRAY) {
			result.array().insert(result.array().end(), target.array().begin(), target.array().end());
		}
		else {
			result.array().emplace_back(CValueSub(target));
		}
	}

    return result;
}

choicetype_t GetDefaultBlockChoicetype(choicetype_t nowtype) {
		switch (nowtype) {
		case CHOICETYPE_RANDOM:
		case CHOICETYPE_NONOVERLAP:
		case CHOICETYPE_SEQUENTIAL:
				return CHOICETYPE_RANDOM;
		case CHOICETYPE_VOID:
				return CHOICETYPE_VOID;
		case CHOICETYPE_ARRAY:
		case CHOICETYPE_POOL:
		case CHOICETYPE_POOL_ARRAY:
		case CHOICETYPE_NONOVERLAP_POOL:
		case CHOICETYPE_SEQUENTIAL_POOL:
		default:
				return CHOICETYPE_ARRAY;
		}
}
