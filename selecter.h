// 
// AYA version 5
//
// 出力の選択を行なうクラス　CSelecter/CDuplEvInfo
// written by umeici. 2004
// 
// CSelecterは出力の選択を行ないます。
// CDuplEvInfoは重複回避を行ないます。
//

#ifndef	SELECTERH
#define	SELECTERH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "cell.h"
#include "value.h"
#include "variable.h"

#define	CHOICETYPE_RANDOM				0	/* 常に無作為にランダム（デフォルト）*/
#define	CHOICETYPE_NONOVERLAP			1	/* ランダムだが一巡するまで重複選択しない */
#define	CHOICETYPE_SEQUENTIAL			2	/* 順番に選択する */
#define	CHOICETYPE_VOID					3	/* 出力なし */
#define	CHOICETYPE_ARRAY				4	/* 簡易配列編成 */
#define	CHOICETYPE_POSSIBILITY_LIST		5	/* 全選択候補を配列として返す */
#define	CHOICETYPE_POOL					6	/* randomのスコープ無視版 */
#define	CHOICETYPE_POOL_ARRAY			7	/* arrayのスコープ無視版 */
#define	CHOICETYPE_NONOVERLAP_POOL		8	/* nonoverlapのスコープ無視版 */
#define	CHOICETYPE_SEQUENTIAL_POOL		9	/* sequentialのスコープ無視版 */

#define	CHOICETYPE_NUM			10

const wchar_t* const choicetype[CHOICETYPE_NUM] = {
	L"random",
	L"nonoverlap",
	L"sequential",
	L"void",
	L"array",
	L"possibility_list",
	L"pool",
	L"pool_array",
	L"nonoverlap_pool",
	L"sequential_pool",
};

class CAyaVM;

//----

class CVecValue
{
public:
	std::vector<CValue> array;
};

//----

class	CDuplEvInfo
{
protected:
	int				type;			// 選択種別

	std::vector<int>	num;			// --で区切られた領域毎の候補数
	std::vector<int>	roundorder;		// 巡回順序

	int				lastroundorder; // 直前の巡回順序値
	int				total;			// 出力候補値の総数
	int				index;			// 現在の巡回位置

private:
	CDuplEvInfo(void);

public:
	CDuplEvInfo(int tp)
	{
		type = tp;
		total = 0;
		index = 0;
		lastroundorder = -1;
	}

	int		GetType(void) { return type; }

	CValue	Choice(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values, int mode);
	CValue	ChoiceValue(CAyaVM& vm, CValue& value, int mode);

protected:
	void	InitRoundOrder(CAyaVM &vm,int mode);
	bool	UpdateNums(int areanum, const std::vector<CVecValue> &values);
	bool	UpdateNums(const CValue& value);
	CValue	GetValue(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values);
	CValue	GetValue(CAyaVM& vm, const CValue& value);
};

//----

class CSelecter
{
protected:
	CAyaVM &vm;
	std::vector<CVecValue>	values;			// 出力候補値
	int					areanum;		// 出力候補を蓄積する領域の数
	CDuplEvInfo			*duplctl;		// 対応する重複回避情報へのポインタ
	int					aindex;			// switch構文で使用

private:
	CSelecter(void);
public:
	CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, int aid);

	void	AddArea(void);
	void	Append(const CValue &value);
	CValue	Output(void);

protected:
	CValue	StructArray1(int index);
	CValue	StructPool(void);
	CValue	StructPossibilityList(void);
	CValue	StructArray(void);
	CValue	ChoiceRandom(void);
	CValue	ChoiceRandom1(int index);
	CValue	ChoiceByIndex(void);
	CValue	ChoiceByIndex1(int index);
};

//----

#endif
