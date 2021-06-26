﻿// 
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

#define	CHOICETYPE_RANDOM		0	/* 常に無作為にランダム（デフォルト）*/
#define	CHOICETYPE_NONOVERLAP	1	/* ランダムだが一巡するまで重複選択しない */
#define	CHOICETYPE_SEQUENTIAL	2	/* 順番に選択する */
#define	CHOICETYPE_VOID			3	/* 出力なし */
#define	CHOICETYPE_ARRAY		4	/* 簡易配列編成 */

#define	CHOICETYPE_NUM			5

const wchar_t	choicetype[CHOICETYPE_NUM][16] = {
	L"random",
	L"nonoverlap",
	L"sequential",
	L"void",
	L"array"
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

protected:
	void	InitRoundOrder(CAyaVM &vm,int mode);
	char	UpdateNums(int areanum, const std::vector<CVecValue> &values);
	CValue	GetValue(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values);
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
	CValue StructArray1(int index);
	CValue StructArray(void);
	CValue	ChoiceRandom(void);
	CValue	ChoiceRandom1(int index);
	CValue	ChoiceByIndex(void);
	CValue	ChoiceByIndex1(int index);
};

//----

#endif
