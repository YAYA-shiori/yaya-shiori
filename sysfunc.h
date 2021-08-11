﻿// 
// AYA version 5
//
// システム関数を実行するクラス　CSystemFunction
// written by umeici. 2004
// 

#ifndef	SYSFUNCH
#define	SYSFUNCH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "deelx.h"

#include "globaldef.h"
#include "value.h"

class CAyaVM;
class CCell;
class CLocalVariable;
class CFunction;

typedef const std::vector<CValue> CValueArgArray;

//----

class	CSystemFunction
{
private:
	CAyaVM &vm;

	protected:
	int		lasterror;					// 最期に発生したエラーの番号
	int		lso;						// LSOが返す値

	CValue	re_str;						// 正規表現処理結果の詳細情報（一致した文字列）
	CValue	re_pos;						// 正規表現処理結果の詳細情報（一致した位置）
	CValue	re_len;						// 正規表現処理結果の詳細情報（一致した長さ）

	int re_option; //正規表現オプション

private:
	CSystemFunction(void);

public:
	CSystemFunction(CAyaVM &vmr);

	static int FindIndex(const yaya::string_t &str);
	static int FindIndexLongestMatch(const yaya::string_t &str,int max_len = 0);

	static const yaya::char_t* GetNameFromIndex(int idx);
	static int GetMaxNameLength(void);

	static int HistoryIndex(void);
	static const yaya::char_t* HistoryFunctionName(void);

	void	SetLso(int order) { lso = order; }

	CValue	Execute(int index, const CValue &arg, const std::vector<CCell *> &pcellarg,
				CValueArgArray &valuearg, CLocalVariable &lvar, int l, CFunction *thisfunc);

private:
	CValue	*GetReStrPtr(void) { return &re_str; }
	CValue	*GetRePosPtr(void) { return &re_pos; }
	CValue	*GetReLenPtr(void) { return &re_len; }

	bool ProcessTranslateSyntax(std::vector<yaya::char_t> &array,yaya::string_t &str, yaya::string_t &d, int &l);

protected:
	CValue	TOINT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOREAL(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TOSTR(CValueArgArray &valuearg, yaya::string_t &d, int &l);
	CValue	TOAUTO(const CValue &args, yaya::string_t &d, int &l);

	CValue	GETTYPE(CValueArgArray &valuearg, yaya::string_t &d, int &l);
	CValue	GETTYPEEX(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);

	CValue	ISFUNC(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	LOGGING(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETERRORLOG(const CValue &arg, yaya::string_t &d, int &l);
	CValue	LOADLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	UNLOADLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	REQUESTLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	CHARSETLIB(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RAND(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SRAND(const CValue &arg, yaya::string_t &d, int &l);
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
	CValue	FREADENCODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITE2(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITEBIN(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FWRITEDECODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FCOPY(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FMOVE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	MKDIR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RMDIR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FDEL(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FRENAME(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FDIGEST(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FSIZE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FENUM(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FCHARSET(const CValue &arg, yaya::string_t &d, int &l);
	// ARRAYSYZE winnt.h とマクロが被った。減点２。
	CValue	ArraySize(CValueArgArray &valuearg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	SETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	EVAL(const CValue &arg, yaya::string_t &d, int &l, CLocalVariable &lvar,
				CFunction *thisfunc);
	CValue	ERASEVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETTIME(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETSECCOUNT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETTICKCOUNT(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue	GETMEMINFO(void);

	CValue	SETLASTERROR(const CValue &arg, yaya::string_t &d, int &l);

	CValue	RE_SEARCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_MATCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_GREP(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_REPLACE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_SPLIT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_OPTION(const CValue &arg, yaya::string_t &d, int &l);

	CValue	RE_ASEARCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RE_ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l);

	CValue	CHRCODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISINTSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ISREALSTR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SPLITPATH(const CValue &arg, yaya::string_t &d, int &l);

	CValue	CVINT(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVSTR(CValueArgArray &valuearg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVREAL(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	CValue	CVAUTO(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);
	
	CValue	LETTONAME(CValueArgArray &valuearg, yaya::string_t &d, int &l, CLocalVariable &lvar,
				CFunction *thisfunc);
	CValue	STRFORM(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ANY(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
				yaya::string_t &d, int &l);

	CValue	SAVEVAR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	RESTOREVAR(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETSTRBYTES(const CValue &arg, yaya::string_t &d, int &l);

	CValue	ASEARCH(const CValue &arg, yaya::string_t &d, int &l);
	CValue	ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l);

	CValue	ASORT(const CValue &arg, yaya::string_t &d, int &l);

	CValue	GETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l);

	CValue	GETSETTING(const CValue &arg, yaya::string_t &d, int &l);
	CValue	SETSETTING(const CValue &arg, yaya::string_t &d, int &l);

	CValue	SPLIT(const CValue &arg, yaya::string_t &d, int &l);

	CValue	FATTRIB(const CValue &arg, yaya::string_t &d, int &l);

	CValue GETFUNCLIST(const CValue &arg, yaya::string_t &d, int &l);
	CValue GETSYSTEMFUNCLIST(const CValue &arg, yaya::string_t &, int &);
	CValue	GETVARLIST(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l);
	CValue UNDEFFUNC(const CValue &arg, yaya::string_t &, int &);
	CValue UNDEFGLOBALDEFINE(const CValue &arg, yaya::string_t &, int &);
	CValue UNLOADDIC(const CValue &arg, yaya::string_t &, int &);
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

	CValue	STRENCODE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	STRDECODE(const CValue &arg, yaya::string_t &d, int &l);

	CValue	READFMO(const CValue &arg,yaya::string_t &d,int &l);

	CValue	EXECUTE(const CValue &arg, yaya::string_t &d, int &l);
	CValue	EXECUTE_WAIT(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETENV(const CValue &arg, yaya::string_t &d, int &l);

	CValue	DUMPVAR(const CValue &arg, yaya::string_t &d, int &l);
	CValue	TRANSLATE(const CValue &arg, yaya::string_t &d, int &l);

	CValue	LICENSE(void);

	CValue	FSEEK(const CValue &arg, yaya::string_t &d, int &l);
	CValue	FTELL(const CValue &arg, yaya::string_t &d, int &l);

	CValue	DICLOAD(const CValue &arg, yaya::string_t &d, int &l);
	CValue	GETFUNCINFO(const CValue &arg, yaya::string_t &d, int &l);
	CValue	PROCESSGLOBALDEFINE(const CValue &arg, yaya::string_t &d, int &l);

	CValue	RE_SPLIT_CORE(const CValue &arg, yaya::string_t &d, int &l, const yaya::char_t *fncname, size_t num);

	void	StoreReResultDetails(const yaya::string_t &str,MatchResult &result);
	void	ClearReResultDetails(void);
	void	AppendReResultDetail(const yaya::string_t &str, int pos, int len);
	void	SetError(int code);

	int		GetCharset(const CValueSub &var,const wchar_t *fname, yaya::string_t &d, int &l);
	yaya::string_t	ToFullPath(const yaya::string_t &str);
};

//----

#endif
