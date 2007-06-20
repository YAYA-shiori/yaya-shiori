// 
// AYA version 5
//
// システム関数を実行するクラス　CSystemFunction
// written by umeici. 2004
// 
// 正規表現系の関数ではboost::regexを使用しています。
// http://www.boost.org/
// http://www.boost.org/libs/regex/doc/index.html
// 邦訳
// http://boost.cppll.jp/HEAD/libs/regex/index.htm
// boost::regexにおける正規表現構文については以下を参照
// http://boost.cppll.jp/HEAD/libs/regex/syntax.htm
//

#ifndef	SYSFUNCH
#define	SYSFUNCH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include <boost/regex.hpp>	/* boost::regexを使用します */
#define	BOOST_REGEX_STATIC_LINK

#include "globaldef.h"
#include "value.h"

class CAyaVM;
class CCell;
class CLocalVariable;
class CFunction;

#define	I_PERF_DATA_SIZE			32768
#define	D_PERF_DATA_SIZE			1024

#define	SYSFUNC_NUM					118
#define	SYSFUNC_HIS					61

const wchar_t	sysfunc[SYSFUNC_NUM][32] = {
	// 型取得/変換
	L"TOINT",
	L"TOREAL",
	L"TOSTR",
	L"GETTYPE",
	L"ISFUNC",
	L"ISVAR",
	// デバッグ
	L"LOGGING",
	L"GETLASTERROR",
	// 外部ライブラリ
	L"LOADLIB",
	L"UNLOADLIB",
	L"REQUESTLIB",
	L"CHARSETLIB",
	// 数値
	L"RAND",
	L"FLOOR",
	L"CEIL",
	L"ROUND",
	L"SIN",
	L"COS",
	L"TAN",
	L"LOG",
	L"LOG10",
	L"POW",
	L"SQRT",
	// 文字列操作
	L"STRSTR",
	L"STRLEN",
	L"REPLACE",
	L"SUBSTR",
	L"ERASE",
	L"INSERT",
	L"TOUPPER",
	L"TOLOWER",
	L"CUTSPACE",
	L"TOBINSTR",
	L"TOHEXSTR",
	L"BINSTRTOI",
	L"HEXSTRTOI",
	L"CHR",
	// ファイル操作
	L"FOPEN",
	L"FCLOSE",
	L"FREAD",
	L"FWRITE",
	L"FWRITE2",
	L"FCOPY",
	L"FMOVE",
	L"MKDIR",
	L"RMDIR",
	L"FDEL",
	L"FRENAME",
	L"FSIZE",
	L"FENUM",
	L"FCHARSET",
	// 配列
	L"ARRAYSIZE",
	L"SETDELIM",
	// 特殊
	L"EVAL",
	L"ERASEVAR",
	// システム時刻/メモリ情報
	L"GETTIME",
	L"GETTICKCOUNT",
	L"GETMEMINFO",
	// 正規表現
	L"RE_SEARCH",
	L"RE_MATCH",
	L"RE_GREP",
	// システムで使用
	L"EmBeD_HiStOrY",	// %[n]（置換済の値の再利用）処理用
	// デバッグ用(2)
	L"SETLASTERROR",
	// 正規表現(2)
	L"RE_REPLACE",
	L"RE_SPLIT",
	L"RE_GETSTR",
	L"RE_GETPOS",
	L"RE_GETLEN",
	// 文字列操作(2)
	L"CHRCODE",
	L"ISINTSTR",
	L"ISREALSTR",
	// 配列(2)
	L"IARRAY",
	// 文字列操作(3)
	L"SPLITPATH",
	// 型取得/変換(2)
	L"CVINT",
	L"CVSTR",
	L"CVREAL",
	// 特殊(2)
	L"LETTONAME",
	L"LSO",
	// 文字列操作(4)
	L"STRFORM",
	L"ANY",
	// 特殊(3)
	L"SAVEVAR",
	// 文字列操作(5)
	L"GETSTRBYTES",
	// 配列(3)
	L"ASEARCH",
	L"ASEARCHEX",
	// 配列(2)
	L"GETDELIM",
	// 特殊(4)
	L"GETSETTING",
	// 数値(2)
	L"ASIN",
	L"ACOS",
	L"ATAN",
	// 文字列操作(6)
	L"SPLIT",
	// ファイル操作(2)
	L"FATTRIB",
	// 型取得/変換(3)
	L"GETFUNCLIST",
	L"GETVARLIST",
	// 正規表現(3)
	L"RE_REPLACEEX",
	// 外部ライブラリ(2)
	L"CHARSETLIBEX",
	// 文字コード
	L"CHARSETTEXTTOID",
	L"CHARSETIDTOTEXT",
	// ビット演算
	L"BITWISE_AND",
	L"BITWISE_OR",
	L"BITWISE_XOR",
	L"BITWISE_NOT",
	L"BITWISE_SHIFT",
	// 半角<->全角
	L"ZEN2HAN",
	L"HAN2ZEN",
	// 型取得/変換(3)
	L"CVAUTO",
	L"TOAUTO",
	// ファイル操作(3)
	L"FREADBIN",
	L"FWRITEBIN",
	// 特殊(5)
	L"RESTOREVAR",
	L"GETCALLSTACK",
	// 文字列操作(7)
	L"GETSTRURLENCODE",
	L"GETSTRURLDECODE",
	// 数値(3)
	L"SINH",
	L"COSH",
	L"TANH",
	// システム時刻/メモリ情報(2)
	L"GETSECCOUNT",
	// FMO(1)
	L"READFMO",
	// ファイル操作(4)
	L"FREADXML",
};

//----

class	CSystemFunction
{
protected:
	size_t	sysfunc_len[SYSFUNC_NUM];	// 各関数名の長さ

	int		lasterror;					// 最期に発生したエラーの番号
	int		lso;						// LSOが返す値

	CValue	re_str;						// 正規表現処理結果の詳細情報（一致した文字列）
	CValue	re_pos;						// 正規表現処理結果の詳細情報（一致した位置）
	CValue	re_len;						// 正規表現処理結果の詳細情報（一致した長さ）

private:
	CAyaVM &vm;

	CSystemFunction(void);

public:
	CSystemFunction(CAyaVM &vmr)
		: vm(vmr), re_str(F_TAG_ARRAY, 0/*dmy*/), re_pos(F_TAG_ARRAY, 0/*dmy*/), re_len(F_TAG_ARRAY, 0/*dmy*/)
	{
		lasterror   = 0;
		lso         = -1;

		for(size_t i = 0; i < SYSFUNC_NUM; i++)
			sysfunc_len[i] = ::wcslen(sysfunc[i]);
	}

	size_t	GetNameLen(int index)
	{
		return (index >= 0 && index < SYSFUNC_NUM) ? sysfunc_len[index] : 0;
	}

	void	SetLso(int order) { lso = order; }

	CValue	*GetReStrPtr(void) { return &re_str; }
	CValue	*GetRePosPtr(void) { return &re_pos; }
	CValue	*GetReLenPtr(void) { return &re_len; }

	CValue	Execute(int index, const CValue &arg, const std::vector<CCell *> &pcellarg,
				CLocalVariable &lvar, int l, CFunction *thisfunc);

protected:
	CValue	TOINT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOREAL(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOSTR(const CValue &args, yaya::string_t &d, int &l);
	CValue	TOAUTO(const CValue &args, yaya::string_t &d, int &l);

	CValue	GETTYPE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISFUNC(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	LOGGING(const CValue &arg, yaya::string_t &d, int &l);
	CValue	LOADLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	UNLOADLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	REQUESTLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CHARSETLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RAND(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FLOOR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CEIL(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ROUND(const CValue &arg, yaya::string_t &d, int &l);

	CValue	SIN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	COS(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TAN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ASIN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ACOS(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ATAN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SINH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	COSH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TANH(const CValue &arg, yaya::string_t &d, int &l);

	CValue	LOG(const CValue &arg, yaya::string_t &d, int &l);
	CValue	LOG10(const CValue &arg, yaya::string_t &d, int &l);
	CValue	POW(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SQRT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	STRSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	STRLEN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	REPLACE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SUBSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ERASE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	INSERT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOUPPER(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOLOWER(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOBINSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOHEXSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	BINSTRTOI(const CValue &arg, yaya::string_t &d, int &l);
	CValue	HEXSTRTOI(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CUTSPACE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CHR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FOPEN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FCLOSE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FREAD(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FREADBIN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FREADXML(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITE2(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITEBIN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FCOPY(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FMOVE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	MKDIR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RMDIR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FDEL(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FRENAME(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FSIZE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FENUM(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FCHARSET(const CValue &arg, yaya::string_t &d, int &l);
	// ARRAYSYZE winnt.h とマクロが被った。減点２。
	CValue	ArraySize(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	SETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	EVAL(const CValue &arg, yaya::string_t &d, int &l, CLocalVariable &lvar,
				CFunction *thisfunc);
	CValue	ERASEVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETTIME(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETSECCOUNT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETTICKCOUNT(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETMEMINFO(void);
	CValue	RE_SEARCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_MATCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_GREP(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SETLASTERROR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_REPLACE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_SPLIT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CHRCODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISINTSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISREALSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SPLITPATH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CVINT(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVSTR(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVREAL(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVAUTO(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	LETTONAME(const CValue &arg, yaya::string_t &d, int &l, CLocalVariable &lvar,
				CFunction *thisfunc);
	CValue	STRFORM(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ANY(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);

	CValue	SAVEVAR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RESTOREVAR(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETSTRBYTES(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ASEARCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETSETTING(const CValue &arg, yaya::string_t &d, int &l);

	CValue	SPLIT(const CValue &arg, yaya::string_t &d, int &l);

	CValue	FATTRIB(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETFUNCLIST(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETVARLIST(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETCALLSTACK(const CValue &arg, yaya::string_t &d, int &l);

	CValue	RE_REPLACEEX(const CValue &arg, yaya::string_t &d, int &l);

	CValue	CHARSETLIBEX(const CValue &arg, yaya::string_t &d, int &l);

	CValue	CHARSETTEXTTOID(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CHARSETIDTOTEXT(const CValue &arg, yaya::string_t &d, int &l);

	CValue  BITWISE_AND(const CValue &arg, yaya::string_t &d, int &l);
	CValue  BITWISE_OR(const CValue &arg, yaya::string_t &d, int &l);
	CValue  BITWISE_XOR(const CValue &arg, yaya::string_t &d, int &l);
	CValue  BITWISE_NOT(const CValue &arg, yaya::string_t &d, int &l);
	CValue  BITWISE_SHIFT(const CValue &arg, yaya::string_t &d, int &l);

	CValue  ZEN2HAN(const CValue &arg, yaya::string_t &d, int &l);
	CValue  HAN2ZEN(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETSTRURLENCODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETSTRURLDECODE(const CValue &arg, yaya::string_t &d, int &l);

	CValue	READFMO(const CValue &arg,yaya::string_t &d,int &l);

	CValue	RE_SPLIT_CORE(const CValue &arg, yaya::string_t &d, int &l, const yaya::char_t *fncname, std::vector<yaya::string_t> *replace_array);

	void	StoreReResultDetails(boost::match_results<yaya::string_t::const_iterator> &result);
	void	ClearReResultDetails(void);
	void	AppendReResultDetail(const yaya::string_t &str, int pos, int len);
	void	SetError(int code);

	int     GetCharset(const CValueSub &var,const wchar_t *fname, yaya::string_t &d, int &l);
	yaya::string_t	ToFullPath(const yaya::string_t &str);
};

//----

#endif
