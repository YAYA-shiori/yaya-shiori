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

enum {
	CHOICETYPE_RANDOM = 0,		/* 常に無作為にランダム（デフォルト）*/
	CHOICETYPE_NONOVERLAP,		/* ランダムだが一巡するまで重複選択しない */
	CHOICETYPE_SEQUENTIAL,		/* 順番に選択する */
	CHOICETYPE_VOID,			/* 出力なし */
	CHOICETYPE_ARRAY,			/* 簡易配列編成 */
	CHOICETYPE_POOL,			/* randomのスコープ無視版 */
	CHOICETYPE_POOL_ARRAY,		/* arrayのスコープ無視版 : 全選択候補を配列として返す */
	CHOICETYPE_NONOVERLAP_POOL,	/* nonoverlapのスコープ無視版 */
	CHOICETYPE_SEQUENTIAL_POOL,	/* sequentialのスコープ無視版 */
};

const struct { yaya::char_t *name; int type; } choicetype[] = {
	{ L"random", CHOICETYPE_RANDOM } ,
	{ L"nonoverlap", CHOICETYPE_NONOVERLAP } ,
	{ L"sequential", CHOICETYPE_SEQUENTIAL } ,
	{ L"void", CHOICETYPE_VOID } ,
	{ L"array", CHOICETYPE_ARRAY } ,
	{ L"pool", CHOICETYPE_POOL } ,
	{ L"pool_array", CHOICETYPE_POOL_ARRAY } , //human-error safety!
	{ L"array_pool", CHOICETYPE_POOL_ARRAY } ,
	{ L"pool_nonoverlap", CHOICETYPE_NONOVERLAP_POOL } , //human-error safety!
	{ L"nonoverlap_pool", CHOICETYPE_NONOVERLAP_POOL } ,
	{ L"pool_sequential",CHOICETYPE_SEQUENTIAL_POOL } , //human-error safety!
	{ L"sequential_pool",CHOICETYPE_SEQUENTIAL_POOL } ,
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
	CValue	StructArray(void);
	CValue	ChoiceRandom(void);
	CValue	ChoiceRandom1(int index);
	CValue	ChoiceByIndex(void);
	CValue	ChoiceByIndex1(int index);
};

//----

#endif
