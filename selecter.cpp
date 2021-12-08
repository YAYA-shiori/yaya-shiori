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
#include "wsex.h"
#include "messages.h"

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
CSelecter::CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, ptrdiff_t aid) : vm(vmr), duplctl(dc), aindex(aid)
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
	if (duplctl->GetType() & CHOICETYPE_SPECOUT_FILTER)
		switch(duplctl->GetType()){
		case CHOICETYPE_VOID:
			return CValue(F_TAG_NOP, 0/*dmy*/);
		case CHOICETYPE_ALL:
			return StructString();
		case CHOICETYPE_LAST:
			return *values.rbegin()->array.rbegin();
		}

	switch ( duplctl->GetType() & CHOICETYPE_SELECT_FILTER ) {
	case CHOICETYPE_NONOVERLAP_FLAG:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_NONOVERLAP_FLAG);
	case CHOICETYPE_SEQUENTIAL_FLAG:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_SEQUENTIAL_FLAG);
	case CHOICETYPE_ARRAY_FLAG:
		return StructArray();
	case CHOICETYPE_RANDOM_FLAG:
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
		for(size_t i = 0; i <= areanum; i++)
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
CValue	CSelecter::ChoiceRandom1(size_t index)
{
	if ( ! values[index].array.size() ) {
		return CValue();
	}

	size_t choice = vm.genrand_uint(values[index].array.size());

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
		for(size_t i = 0; i <= areanum; i++)
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
CValue	CSelecter::ChoiceByIndex1(size_t index)
{
	size_t	num = values[index].array.size();

	if (!num)
		return CValue();

	return (aindex >= 0 && (size_t)aindex < num) ? values[index].array[aindex] : CValue();
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
		for(size_t i = 0; i <= areanum; i++) {
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
CValue CSelecter::StructArray1(size_t index)
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

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::StructString
 *  機能概要：  各領域の値を結合した文字列を作成し返します
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructString()
{
	if (areanum) {
		CValue	result(F_TAG_STRING, 0/*dmy*/);
		for(size_t i = 0; i <= areanum; i++) {
			result.s_value += StructString1(i).s_value;
		}
		return result;
	}
	else {
		return StructString1(0);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::StructString1
 *  機能概要：  指定された領域の値を結合した文字列を作成します
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructString1(size_t index)
{
	CValue	result(F_TAG_STRING, 0/*dmy*/);

    for(size_t i = 0; i < values[index].array.size(); ++i) {
		const CValue &target = values[index].array[i];
		int	valtype = target.GetType();
		
		if (valtype == F_TAG_STRING) {
			result.s_value += target.s_value;
		}
		else {
			result.s_value += target.GetValueString();
		}
	}

    return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::GetDefaultBlockChoicetype
 *  機能概要：  { } の出力タイプ未指定の時の標準タイプ
 * -----------------------------------------------------------------------
 */
choicetype_t CSelecter::GetDefaultBlockChoicetype(choicetype_t nowtype)
{
	unsigned int choicetype = 0;

	if (nowtype & CHOICETYPE_SPECOUT_FILTER) {
		return nowtype;
	}
	else {
		if (nowtype & CHOICETYPE_ARRAY_FLAG) {
			choicetype = CHOICETYPE_ARRAY_FLAG;
		}
		else {
			choicetype = CHOICETYPE_RANDOM_FLAG;
		}
	}

	unsigned int outtype = CHOICETYPE_PICKONE_FLAG;
	if (nowtype & CHOICETYPE_POOL_FLAG) {
		outtype = CHOICETYPE_POOL_FLAG;
	}
	else if (nowtype & CHOICETYPE_PICKONE_FLAG) {
		outtype = CHOICETYPE_PICKONE_FLAG;
	}
	else if (nowtype & CHOICETYPE_MELT_FLAG) {
		outtype = CHOICETYPE_MELT_FLAG;
	}

	return static_cast<choicetype_t>(outtype | choicetype);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::StringToChoiceType
 *  機能概要：  文字列->choicetype_t
 * -----------------------------------------------------------------------
 */
choicetype_t CSelecter::StringToChoiceType(const yaya::string_t& ctypestr, CAyaVM &vm, const yaya::string_t& dicfilename, size_t linecount)
{
	unsigned int outtype = CHOICETYPE_PICKONE_FLAG;

	yaya::string_t checkstr = ctypestr;

	if ( checkstr.find(L"pool") != yaya::string_t::npos ) {
		outtype = CHOICETYPE_POOL_FLAG;
		yaya::ws_replace(checkstr,L"pool",L"");
	}
	else if ( checkstr.find(L"melt") != yaya::string_t::npos ) {
		outtype = CHOICETYPE_MELT_FLAG;
		yaya::ws_replace(checkstr,L"melt",L"");
	}
	else if ( checkstr.find(L"void") != yaya::string_t::npos ) {
		outtype = CHOICETYPE_VOID_FLAG;
		yaya::ws_replace(checkstr,L"void",L"");
	}
	else if ( checkstr.find(L"all") != yaya::string_t::npos ) {
		outtype = CHOICETYPE_ALL_FLAG;
		yaya::ws_replace(checkstr,L"all",L"");
	}
	else if ( checkstr.find(L"last") != yaya::string_t::npos ) {
		outtype = CHOICETYPE_LAST_FLAG;
		yaya::ws_replace(checkstr,L"last",L"");
	}

	unsigned int choicetype = 0;

	if (! (outtype & CHOICETYPE_SPECOUT_FILTER) ) {
		choicetype = CHOICETYPE_RANDOM_FLAG;

		if ( checkstr.find(L"sequential") != yaya::string_t::npos ) {
			choicetype = CHOICETYPE_SEQUENTIAL_FLAG;
			yaya::ws_replace(checkstr,L"sequential",L"");
		}
		else if ( checkstr.find(L"array") != yaya::string_t::npos ) {
			choicetype = CHOICETYPE_ARRAY_FLAG;
			yaya::ws_replace(checkstr,L"array",L"");
		}
		else if ( checkstr.find(L"nonoverlap") != yaya::string_t::npos ) {
			choicetype = CHOICETYPE_NONOVERLAP_FLAG;
			yaya::ws_replace(checkstr,L"nonoverlap",L"");
		}
		else {
			yaya::ws_replace(checkstr,L"random",L"");
		}
	}

	yaya::ws_replace(checkstr,L"_",L"");

	if ( checkstr.size() > 0 ) {
		//なにか余分なものがあったぞ
		vm.logger().Error(E_E, 30, ctypestr, dicfilename, linecount);
	}

	return static_cast<choicetype_t>(outtype | choicetype);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSelecter::ChoiceTypeToString
 *  機能概要：  choicetype_t->文字列
 * -----------------------------------------------------------------------
 */
yaya::string_t CSelecter::ChoiceTypeToString(choicetype_t ctype)
{
	yaya::string_t aret;

	switch (ctype & CHOICETYPE_OUTPUT_FILTER)
	{
	case CHOICETYPE_POOL_FLAG:
		break;
	case CHOICETYPE_MELT_FLAG:
		aret += L"melt";
		break;
	case CHOICETYPE_VOID_FLAG:
		return L"void";
	case CHOICETYPE_ALL_FLAG:
		return L"all";
	case CHOICETYPE_LAST_FLAG:
		return L"last";
	case CHOICETYPE_PICKONE_FLAG:
		break;
	default:
		return L"unknown";
	}

	aret += L'_';

	switch (ctype & CHOICETYPE_SELECT_FILTER)
	{
	case CHOICETYPE_SEQUENTIAL_FLAG:
		aret += L"sequential";
		break;
	case CHOICETYPE_ARRAY_FLAG:
		aret += L"array";
		break;
	case CHOICETYPE_NONOVERLAP_FLAG:
		aret += L"nonoverlap";
		break;
	case CHOICETYPE_RANDOM_FLAG:
		aret += L"random";
		break;
	default:
		return L"unknown";
	}

	aret += L'_';

	switch (ctype & CHOICETYPE_OUTPUT_FILTER)
	{
	case CHOICETYPE_POOL_FLAG:
		aret += L"pool";
		break;
	}

	size_t beg = aret.find_first_not_of(L'_');
	size_t end = aret.find_last_not_of(L'_');
	size_t siz = end - beg + 1;
	aret = aret.substr(beg, siz);

	return aret;
}

