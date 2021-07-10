// 
// AYA version 5
//
// 定数
// written by umeici. 2004
// 

#ifndef	MANIFESTH
#define	MANIFESTH

#include "globaldef.h"

//----

extern const yaya::char_t *aya_name;
extern const yaya::char_t *aya_version;
extern const yaya::char_t *aya_author;

//----

// 汎用
#define	STRMAX					1024			/* 汎用の文字列バッファサイズ */

// 文字コードセット
#define	CHARSET_SJIS			0				/* デフォルト */
#define	CHARSET_UTF8			1
#define	CHARSET_EUCJP			2
#define	CHARSET_BIG5			3
#define	CHARSET_GB2312			4
#define	CHARSET_EUCKR			5
#define	CHARSET_JIS				6
#define	CHARSET_BINARY	126						/* 変換しない（一部でのみ利用） */
#define	CHARSET_DEFAULT	127						/* OSデフォルトの言語を使用する */

// 基底プレフィックス文字列
#define	PREFIX_BASE_LEN	2
#define	PREFIX_BIN	L"0b"
#define	PREFIX_HEX	L"0x"

// 構文解析結果デバッグ出力時のインデント
#define	OUTDIC_INDENT			L"    "
#define	OUTDIC_INDENT_SIZE		4

// case文の評価値を格納するローカル変数のプリフィックス
#define	PREFIX_CASE_VAR			L"_CaSe_ExPr_PrEfIx_"
#define	PREFIX_CASE_VAR_SIZE	18

// 標準の簡易配列デリミタ
#define	VAR_DELIMITER			L","

// ファイル保存/復元時にダブルクォート/制御文字(0x0000～0x001f)をエスケープする文字列
#define	ESC_DQ					L"_EsC_DoUbLe_QuOtE_"
#define	ESC_CTRL				L"_EsC_CtRl_CoDe_"

#define	CTRL_CH_START			'@'
#define	END_OF_CTRL_CH			0x1f

// ファイル保存/復元時における汎用配列/ハッシュの空要素
#define	ESC_IARRAY				L"IARRAY"
#define ESC_IHASH				L"IHASH"
#define ESC_IVOID				L"IVOID"

// ステートメントの種別
#define	ST_UNKNOWN				-1
#define	ST_NOP					0				/* no operation */

#define	ST_OPEN					1				/* { */
#define	ST_CLOSE				2				/* } */
#define	ST_COMBINE				3				/* -- */

#define	ST_BREAK				16				/* break */
#define	ST_CONTINUE				17				/* continue */
#define	ST_RETURN				18				/* return */
#define	ST_ELSE					19				/* else */

#define	ST_FORMULA				32				/* 数式（この値は構文解析時のみフラグとして使われます) */
								// 33、34欠番
#define	ST_FORMULA_OUT_FORMULA	35				/* 出力（数式。配列、引数つき関数も含まれます） */
#define	ST_FORMULA_SUBST		36				/* 代入 */
#define	ST_IF					37				/* if */
#define	ST_ELSEIF				38				/* elseif */
#define	ST_WHILE				39				/* while */
#define	ST_SWITCH				40				/* switch */
#define	ST_FOR					41				/* for */
#define	ST_FOREACH				42				/* foreach */
#define	ST_WHEN					44				/* when（この値は構文解析時のみフラグとして使われます) */
												/* AYA5ではcase-when-others構文は内部でif-elseif-else構文に変換されます */
#define	ST_PARALLEL				45				/* parallel */
#define	ST_VOID					46				/* void */

// 数式の項の種別
#define	F_TAG_UNKNOWN			-2
#define	F_TAG_NOP				-1
#define F_TAG_VOID				-1				/* 未定義変数 */

#define	F_TAG_ORIGIN			0				/* 演算子の定義開始位置 */

#define	F_TAG_COMMA				0				/* , */
#define	F_TAG_PLUS				1				/* + */
#define	F_TAG_MINUS				2				/* - */
#define	F_TAG_MUL				3				/* * */
#define	F_TAG_DIV				4				/* / */
#define	F_TAG_SURP				5				/* % */
#define	F_TAG_IFEQUAL			6				/* == */
#define	F_TAG_IFDIFFER			7				/* != */
#define	F_TAG_IFGTEQUAL			8				/* >= */
#define	F_TAG_IFLTEQUAL			9				/* <= */
#define	F_TAG_IFGT				10				/* > */
#define	F_TAG_IFLT				11				/* < */
#define	F_TAG_IFIN				12				/* _in_ */
#define	F_TAG_IFNOTIN			13				/* !_in_ */
#define	F_TAG_OR				14				/* || */
#define	F_TAG_AND				15				/* && */
#define	F_TAG_FEEDBACK			16				/* & */
#define	F_TAG_EXC				17				/* ! */
#define	F_TAG_INCREMENT			18				/* ++（内部で"+= 1"に変換されるので実行時には存在しない） */
#define	F_TAG_DECREMENT			19				/* --（内部で"-= 1"に変換されるので実行時には存在しない） */

#define	F_TAG_EQUAL				20				/* = */
#define	F_TAG_PLUSEQUAL			21				/* += */
#define	F_TAG_MINUSEQUAL		22				/* -= */
#define	F_TAG_MULEQUAL			23				/* *= */
#define	F_TAG_DIVEQUAL			24				/* /= */
#define	F_TAG_SURPEQUAL			25				/* %= */
#define	F_TAG_EQUAL_D			26				/* := （AYA5においては= と同一） */
#define	F_TAG_PLUSEQUAL_D		27				/* +:=（AYA5においては+=と同一） */
#define	F_TAG_MINUSEQUAL_D		28				/* -:=（AYA5においては-=と同一） */
#define	F_TAG_MULEQUAL_D		29				/* *:=（AYA5においては*=と同一） */
#define	F_TAG_DIVEQUAL_D		30				/* /:=（AYA5においては/=と同一） */
#define	F_TAG_SURPEQUAL_D		31				/* %:=（AYA5においては%=と同一） */
#define	F_TAG_COMMAEQUAL		32				/* ,= */

#define	F_TAG_BRACKETIN			33				/* ( */
#define	F_TAG_BRACKETOUT		34				/* ) */
#define	F_TAG_HOOKBRACKETIN		35				/* [ */
#define	F_TAG_HOOKBRACKETOUT	36				/* ] */

#define	F_TAG_FUNCPARAM			37				/* 引数を関数に渡すための演算子 */
#define	F_TAG_ARRAYORDER		38				/* 配列の序数を変数に渡すための演算子 */
#define	F_TAG_SYSFUNCPARAM		39				/* 引数をシステム関数に渡すための演算子 */

#define	F_TAG_ORIGIN_VALUE		128				/* 項の定義開始位置 */

#define	F_TAG_INT				128				/* 整数項 */
#define	F_TAG_DOUBLE			131				/* 実数項 */
#define	F_TAG_STRING			132				/* 文字列項 */
#define	F_TAG_STRING_EMBED		133				/* 先頭に変数もしくは関数が埋め込まれている可能性がある文字列項 */
#define	F_TAG_STRING_PLAIN		134				/* 埋め込み展開されない文字列項 */

#define	F_TAG_SYSFUNC			256				/* システム関数 */
#define	F_TAG_USERFUNC			512				/* ユーザー関数 */

#define	F_TAG_VARIABLE			1024			/* 変数 */
#define	F_TAG_LOCALVARIABLE		2048			/* ローカル変数 */

#define	F_TAG_ARRAY				4096			/* 汎用配列 */
#define F_TAG_HASH              8192            /* ハッシュ */

// 有効な演算子の数
// 演算子の名前
const yaya::char_t	formulatag[][19] = {
	L",",   L"+",   L"-",    L"*",     L"/",
	L"%",   L"==",  L"!=",   L">=",    L"<=",
	L">",   L"<",   L"_in_", L"!_in_", L"||",
	L"&&",  L"&",   L"!",    L"++",    L"--",

	L"=",   L"+=",  L"-=",   L"*=",    L"/=",
	L"%=",  L":=",  L"+:=",  L"-:=",   L"*:=",
    L"/:=", L"%:=", L",=",

	L"(",   L")",   L"[",    L"]",

	L"_Op_FuNc_PaRaM_", L"_Op_ArArY_OrDeR_", L"_Op_SySfUnC_PaRaM_"
};

#define	FORMULATAG_NUM			(sizeof(formulatag) / sizeof(formulatag[0]))

// 演算子の名前の長さ
const int	formulatag_len[FORMULATAG_NUM] = {
	1, 1, 1, 1, 1,
	1, 2, 2, 2, 2,
	1, 1, 4, 5, 2,
	2, 1, 1, 2, 2,

	1, 2, 2, 2, 2,
	2, 2, 3, 3, 3,
	3, 3, 2,

	1, 1, 1, 1,

	15, 16, 18
};

// 演算子で関係する項の数の最小値：エラー検出用（単項演算子か二項演算子か）
const int	formulatag_params[FORMULATAG_NUM] = {
	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 1, 1, 1,

	2, 2, 2, 2, 2,
	2, 2, 2, 2, 2,
	2, 2, 2,

	1, 1, 1, 1,

	1, 1, 1
};


// 演算子の優先順位
// 数値が大きいほど優先度は高くなります。
// ()と[]では、その中に入る際に優先度を上げ、出る際に戻すようにします。
const int	formulatag_depth[FORMULATAG_NUM] = {
	0, 7, 7, 8,  8,
	8, 5, 5, 5,  5,
	5, 5, 5, 5,  3,
	4, 6, 10, 9, 9,

	2, 1, 1, 1, 1,
	1, 2, 1, 1, 1,
	1, 1, 1,

	20, -20, 20, -20,

	11, 11, 11
};

// 制御構文
#define	FLOWCOM_NUM	(sizeof(flowcom) / sizeof(flowcom[0]))

const yaya::char_t	flowcom[][16] = {
	L"if",
	L"elseif",
	L"else",
	L"case",
	L"when",
	L"others",
	L"switch",
	L"while",
	L"for",
	L"break",
	L"continue",
	L"return",
	L"foreach",
	L"parallel",
	L"void",
};

// {}の種別
										// 0～はswitch構文の候補抽出位置
#define	BRACE_SWITCH_OUT_OF_RANGE	-1	/* switch構文における範囲外の候補抽出位置 */
#define	BRACE_DEFAULT				-2	/* デフォルト */
#define	BRACE_LOOP					-3	/* ループ構文 */

//----

// swap
template <class T>
void exchange(T& a, T& b)
{
	T t_value = a;
	a = b;
	b = t_value;
}

//----

#endif


