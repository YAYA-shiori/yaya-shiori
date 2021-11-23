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

#define CHOICETYPE_RANDOM_FLAG       0x0001U
#define CHOICETYPE_SEQUENTIAL_FLAG   0x0002U
#define CHOICETYPE_NONOVERLAP_FLAG   0x0004U
#define CHOICETYPE_ARRAY_FLAG        0x0008U
#define CHOICETYPE_PICKONE_FLAG      0x0100U
#define CHOICETYPE_POOL_FLAG         0x0200U
#define CHOICETYPE_VOID_FLAG         0x0400U

#define CHOICETYPE_SELECT_FILTER     0x00FFU
#define CHOICETYPE_OUTPUT_FILTER     0xFF00U

typedef enum choicetype_t {
	CHOICETYPE_VOID             = CHOICETYPE_VOID_FLAG,									/* 出力なし */
	CHOICETYPE_RANDOM           = CHOICETYPE_PICKONE_FLAG | CHOICETYPE_RANDOM_FLAG,		/* 常に無作為にランダム（デフォルト）*/
	CHOICETYPE_NONOVERLAP       = CHOICETYPE_PICKONE_FLAG | CHOICETYPE_NONOVERLAP_FLAG,	/* ランダムだが一巡するまで重複選択しない */
	CHOICETYPE_SEQUENTIAL       = CHOICETYPE_PICKONE_FLAG | CHOICETYPE_SEQUENTIAL_FLAG,	/* 順番に選択する */
	CHOICETYPE_ARRAY            = CHOICETYPE_PICKONE_FLAG | CHOICETYPE_ARRAY_FLAG,		/* 簡易配列編成 */
	CHOICETYPE_POOL             = CHOICETYPE_POOL_FLAG    | CHOICETYPE_RANDOM_FLAG,		/* randomのスコープ無視版 */
	CHOICETYPE_POOL_ARRAY       = CHOICETYPE_POOL_FLAG    | CHOICETYPE_ARRAY_FLAG,		/* arrayのスコープ無視版 : 全選択候補を配列として返す */
	CHOICETYPE_NONOVERLAP_POOL  = CHOICETYPE_POOL_FLAG    | CHOICETYPE_NONOVERLAP_FLAG,	/* nonoverlapのスコープ無視版 */
	CHOICETYPE_SEQUENTIAL_POOL  = CHOICETYPE_POOL_FLAG    | CHOICETYPE_SEQUENTIAL_FLAG,	/* sequentialのスコープ無視版 */
} choicetype_t;

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
	choicetype_t	type;			// 選択種別

	std::vector<size_t>	num;			// --で区切られた領域毎の候補数
	std::vector<size_t>	roundorder;		// 巡回順序

	size_t				lastroundorder; // 直前の巡回順序値
	size_t				total;			// 出力候補値の総数
	size_t				index;			// 現在の巡回位置

private:
	CDuplEvInfo(void);

public:
	CDuplEvInfo(choicetype_t tp)
	{
		type = tp;
		total = 0;
		index = 0;
		lastroundorder = -1;
	}

	choicetype_t	GetType(void) { return type; }

	CValue	Choice(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values, int mode);

protected:
	void	InitRoundOrder(CAyaVM &vm,int mode);
	bool	UpdateNums(int areanum, const std::vector<CVecValue> &values);
	bool	UpdateNums(const CValue& value);
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

	friend class CFunction;//for pool
private:
	CSelecter(void);
public:
	CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, int aid);

	void	AddArea(void);
	void	Append(const CValue &value);
	CValue	Output(void);

	static choicetype_t			GetDefaultBlockChoicetype(choicetype_t nowtype);
	static choicetype_t			StringToChoiceType(const yaya::string_t& ctypestr, CAyaVM &vm, const yaya::string_t& dicfilename, int linecount);
	static const yaya::char_t*	ChoiceTypeToString(choicetype_t ctype);

protected:
	CValue	StructArray1(int index);
	CValue	StructArray(void);
	CValue	ChoiceRandom(void);
	CValue	ChoiceRandom1(size_t index);
	CValue	ChoiceByIndex(void);
	CValue	ChoiceByIndex1(size_t index);
};

//----

#endif
