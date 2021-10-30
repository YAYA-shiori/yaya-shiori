// 
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

class CSF_FUNCPARAM;

//----

class	CSystemFunction;
class	CSystemFunctionInit;

typedef CValue (CSystemFunction::*SysFuncProc)(CSF_FUNCPARAM &p);

typedef struct CSF_FUNCTABLE
{
	SysFuncProc func;
	const yaya::char_t* name;
} CSF_FUNCTABLE;

class	CSystemFunction
{
private:
	CAyaVM &vm;

	protected:
	int		lasterror;					// 最期に発生したエラーの番号
	size_t	lso;						// LSOが返す値

	CValue	re_str;						// 正規表現処理結果の詳細情報（一致した文字列）
	CValue	re_pos;						// 正規表現処理結果の詳細情報（一致した位置）
	CValue	re_len;						// 正規表現処理結果の詳細情報（一致した長さ）

	int re_option; //正規表現オプション

	static const CSF_FUNCTABLE sysfunc[];

	friend CSystemFunctionInit;

private:
	CSystemFunction(void);

public:
	CSystemFunction(CAyaVM &vmr);

	static ptrdiff_t FindIndex(const yaya::string_t &str);
	static size_t FindIndexLongestMatch(const yaya::string_t &str,size_t max_len = 0);

	static const yaya::char_t* GetNameFromIndex(int idx);
	static size_t GetMaxNameLength(void);

	static size_t HistoryIndex(void);
	static const yaya::char_t* HistoryFunctionName(void);

	void	SetLso(size_t order) { lso = order; }

	CValue	Execute(ptrdiff_t index, const CValue &arg, const std::vector<CCell *> &pcellarg,
				CValueArgArray &valuearg, CLocalVariable &lvar, int l, CFunction *thisfunc);

private:
	CValue	*GetReStrPtr(void) { return &re_str; }
	CValue	*GetRePosPtr(void) { return &re_pos; }
	CValue	*GetReLenPtr(void) { return &re_len; }

	bool ProcessTranslateSyntax(std::vector<yaya::char_t> &array,yaya::string_t &str, const yaya::string_t &d, int l);

	CValue	ToLowerOrUpper(CSF_FUNCPARAM &p,const yaya::char_t *funcname,bool isUpper);

protected:
	CValue	TOINT(CSF_FUNCPARAM &p);
	CValue	TOREAL(CSF_FUNCPARAM &p);
	CValue	TOSTR(CSF_FUNCPARAM &p);
	CValue	TOAUTO(CSF_FUNCPARAM &p);
	CValue	TOAUTOEX(CSF_FUNCPARAM &p);

	CValue	GETTYPE(CSF_FUNCPARAM &p);
	CValue	GETTYPEEX(CSF_FUNCPARAM &p);

	CValue	ISFUNC(CSF_FUNCPARAM &p);
	CValue	ISVAR(CSF_FUNCPARAM &p);
	CValue	LOGGING(CSF_FUNCPARAM &p);
	CValue	GETERRORLOG(CSF_FUNCPARAM &p);
	CValue	CLEARERRORLOG(CSF_FUNCPARAM& p);
	CValue	LOADLIB(CSF_FUNCPARAM &p);
	CValue	UNLOADLIB(CSF_FUNCPARAM &p);
	CValue	REQUESTLIB(CSF_FUNCPARAM &p);
	CValue	CHARSETLIB(CSF_FUNCPARAM &p);
	CValue	RAND(CSF_FUNCPARAM &p);
	CValue	SRAND(CSF_FUNCPARAM &p);
	CValue	FLOOR(CSF_FUNCPARAM &p);
	CValue	CEIL(CSF_FUNCPARAM &p);
	CValue	ROUND(CSF_FUNCPARAM &p);

	CValue	SIN(CSF_FUNCPARAM &p);
	CValue	COS(CSF_FUNCPARAM &p);
	CValue	TAN(CSF_FUNCPARAM &p);
	CValue	ASIN(CSF_FUNCPARAM &p);
	CValue	ACOS(CSF_FUNCPARAM &p);
	CValue	ATAN(CSF_FUNCPARAM &p);
	CValue	SINH(CSF_FUNCPARAM &p);
	CValue	COSH(CSF_FUNCPARAM &p);
	CValue	TANH(CSF_FUNCPARAM &p);

	CValue	LOG(CSF_FUNCPARAM &p);
	CValue	LOG10(CSF_FUNCPARAM &p);
	CValue	POW(CSF_FUNCPARAM &p);
	CValue	SQRT(CSF_FUNCPARAM &p);
	CValue	STRSTR(CSF_FUNCPARAM &p);
	CValue	STRLEN(CSF_FUNCPARAM &p);
	CValue	REPLACE(CSF_FUNCPARAM &p);
	CValue	SUBSTR(CSF_FUNCPARAM &p);
	CValue	ERASE(CSF_FUNCPARAM &p);
	CValue	INSERT(CSF_FUNCPARAM &p);
	CValue	TOUPPER(CSF_FUNCPARAM &p);
	CValue	TOLOWER(CSF_FUNCPARAM &p);
	CValue	TOBINSTR(CSF_FUNCPARAM &p);
	CValue	TOHEXSTR(CSF_FUNCPARAM &p);
	CValue	BINSTRTOI(CSF_FUNCPARAM &p);
	CValue	HEXSTRTOI(CSF_FUNCPARAM &p);
	CValue	CUTSPACE(CSF_FUNCPARAM &p);
	CValue	CHR(CSF_FUNCPARAM &p);
	CValue	FOPEN(CSF_FUNCPARAM &p);
	CValue	FCLOSE(CSF_FUNCPARAM &p);
	CValue	FREAD(CSF_FUNCPARAM &p);
	CValue	FREADBIN(CSF_FUNCPARAM &p);
	CValue	FREADENCODE(CSF_FUNCPARAM &p);
	CValue	FWRITE(CSF_FUNCPARAM &p);
	CValue	FWRITE2(CSF_FUNCPARAM &p);
	CValue	FWRITEBIN(CSF_FUNCPARAM &p);
	CValue	FWRITEDECODE(CSF_FUNCPARAM &p);
	CValue	FCOPY(CSF_FUNCPARAM &p);
	CValue	FMOVE(CSF_FUNCPARAM &p);
	CValue	MKDIR(CSF_FUNCPARAM &p);
	CValue	RMDIR(CSF_FUNCPARAM &p);
	CValue	FDEL(CSF_FUNCPARAM &p);
	CValue	FRENAME(CSF_FUNCPARAM &p);
	CValue	FDIGEST(CSF_FUNCPARAM &p);
	CValue	STRDIGEST(CSF_FUNCPARAM& p);
	CValue	FSIZE(CSF_FUNCPARAM &p);
	CValue	FENUM(CSF_FUNCPARAM &p);
	CValue	FCHARSET(CSF_FUNCPARAM &p);
	// ARRAYSYZE winnt.h とマクロが被った。減点２。
	CValue	ArraySize(CSF_FUNCPARAM &p);
	CValue	SETDELIM(CSF_FUNCPARAM &p);
	CValue	ISEVALUABLE(CSF_FUNCPARAM &p);
	CValue	EVAL(CSF_FUNCPARAM &p);
	CValue	ERASEVAR(CSF_FUNCPARAM &p);
	CValue	GETTIME(CSF_FUNCPARAM &p);
	CValue	GETSECCOUNT(CSF_FUNCPARAM &p);
	CValue	GETTICKCOUNT(CSF_FUNCPARAM &p);
	CValue	GETMEMINFO(CSF_FUNCPARAM &p);

	CValue	GETLASTERROR(CSF_FUNCPARAM &p);
	CValue	SETLASTERROR(CSF_FUNCPARAM &p);

	CValue  EmBeD_HiStOrY(CSF_FUNCPARAM &p);

	CValue	RE_SEARCH(CSF_FUNCPARAM &p);
	CValue	RE_MATCH(CSF_FUNCPARAM &p);
	CValue	RE_GREP(CSF_FUNCPARAM &p);
	CValue	RE_REPLACE(CSF_FUNCPARAM &p);
	CValue	RE_SPLIT(CSF_FUNCPARAM &p);
	CValue	RE_OPTION(CSF_FUNCPARAM &p);

	CValue	RE_GETSTR(CSF_FUNCPARAM &p);
	CValue	RE_GETPOS(CSF_FUNCPARAM &p);
	CValue	RE_GETLEN(CSF_FUNCPARAM &p);

	CValue	RE_ASEARCH(CSF_FUNCPARAM &p);
	CValue	RE_ASEARCHEX(CSF_FUNCPARAM &p);

	CValue	CHRCODE(CSF_FUNCPARAM &p);
	CValue	ISINTSTR(CSF_FUNCPARAM &p);
	CValue	ISREALSTR(CSF_FUNCPARAM &p);
	CValue	SPLITPATH(CSF_FUNCPARAM &p);

	CValue  IARRAY(CSF_FUNCPARAM &p);

	CValue	CVINT(CSF_FUNCPARAM &p);
	CValue	CVSTR(CSF_FUNCPARAM &p);
	CValue	CVREAL(CSF_FUNCPARAM &p);
	CValue	CVAUTO(CSF_FUNCPARAM &p);
	CValue	CVAUTOEX(CSF_FUNCPARAM &p);
	
	CValue	LETTONAME(CSF_FUNCPARAM &p);
	CValue	LSO(CSF_FUNCPARAM &p);
	CValue	STRFORM(CSF_FUNCPARAM &p);
	CValue	ANY(CSF_FUNCPARAM &p);

	CValue	SAVEVAR(CSF_FUNCPARAM &p);
	CValue	RESTOREVAR(CSF_FUNCPARAM &p);

	CValue	GETSTRBYTES(CSF_FUNCPARAM &p);

	CValue	ASEARCH(CSF_FUNCPARAM &p);
	CValue	ASEARCHEX(CSF_FUNCPARAM &p);

	CValue	ASORT(CSF_FUNCPARAM &p);

	CValue	GETDELIM(CSF_FUNCPARAM &p);

	CValue	GETSETTING(CSF_FUNCPARAM &p);
	CValue	SETSETTING(CSF_FUNCPARAM &p);

	CValue	ARRAYDEDUP(CSF_FUNCPARAM& p);
	CValue	SPLIT(CSF_FUNCPARAM &p);

	CValue	SETTAMAHWND(CSF_FUNCPARAM &p);

	CValue	FATTRIB(CSF_FUNCPARAM &p);

	CValue	GETFUNCLIST(CSF_FUNCPARAM &p);
	CValue	GETSYSTEMFUNCLIST(CSF_FUNCPARAM &p);
	CValue	GETVARLIST(CSF_FUNCPARAM &p);
	CValue	GETCALLSTACK(CSF_FUNCPARAM &p);

	CValue	RE_REPLACEEX(CSF_FUNCPARAM &p);

	CValue	CHARSETLIBEX(CSF_FUNCPARAM &p);

	CValue	CHARSETTEXTTOID(CSF_FUNCPARAM &p);
	CValue	CHARSETIDTOTEXT(CSF_FUNCPARAM &p);

	CValue  BITWISE_AND(CSF_FUNCPARAM &p);
	CValue  BITWISE_OR(CSF_FUNCPARAM &p);
	CValue  BITWISE_XOR(CSF_FUNCPARAM &p);
	CValue  BITWISE_NOT(CSF_FUNCPARAM &p);
	CValue  BITWISE_SHIFT(CSF_FUNCPARAM &p);

	CValue  ZEN2HAN(CSF_FUNCPARAM &p);
	CValue  HAN2ZEN(CSF_FUNCPARAM &p);

	CValue	STRENCODE(CSF_FUNCPARAM &p);
	CValue	STRDECODE(CSF_FUNCPARAM &p);

	CValue	READFMO(CSF_FUNCPARAM &p);

	CValue	EXECUTE(CSF_FUNCPARAM &p);
	CValue	EXECUTE_WAIT(CSF_FUNCPARAM &p);
	CValue	SLEEP(CSF_FUNCPARAM &p);
	CValue	GETENV(CSF_FUNCPARAM &p);

	CValue	DUMPVAR(CSF_FUNCPARAM &p);
	CValue	TRANSLATE(CSF_FUNCPARAM &p);

	CValue	LICENSE(CSF_FUNCPARAM &p);

	CValue	FSEEK(CSF_FUNCPARAM &p);
	CValue	FTELL(CSF_FUNCPARAM &p);

	CValue	DICLOAD(CSF_FUNCPARAM &p);
	CValue	DICUNLOAD(CSF_FUNCPARAM &p);

	CValue UNDEFFUNC(CSF_FUNCPARAM &p);
	CValue APPEND_RUNTIME_DIC(CSF_FUNCPARAM &p);
	CValue UNDEFGLOBALDEFINE(CSF_FUNCPARAM &p);

	CValue ISGLOBALDEFINE(CSF_FUNCPARAM &p);
	CValue SETGLOBALDEFINE(CSF_FUNCPARAM &p);

	CValue	GETFUNCINFO(CSF_FUNCPARAM &p);
	CValue	PROCESSGLOBALDEFINE(CSF_FUNCPARAM &p);

	CValue	FUNCDECL_READ(CSF_FUNCPARAM& p);
	CValue	FUNCDECL_WRITE(CSF_FUNCPARAM& p);
	CValue	FUNCDECL_ERASE(CSF_FUNCPARAM& p);

	CValue	OUTPUTNUM(CSF_FUNCPARAM& p);

	//LINT
	CValue	LINT_GetFuncUsedBy(CSF_FUNCPARAM& p);
	CValue	LINT_GetUserDefFuncUsedBy(CSF_FUNCPARAM& p);
	CValue	LINT_GetGlobalVarUsedBy(CSF_FUNCPARAM& p);
	CValue	LINT_GetLocalVarUsedBy(CSF_FUNCPARAM& p);
	CValue	LINT_GetGlobalVarLetted(CSF_FUNCPARAM& p);
	CValue	LINT_GetLocalVarLetted(CSF_FUNCPARAM& p);

	CValue	RE_SPLIT_CORE(const CValue &arg, const yaya::string_t &d, int l, const yaya::char_t *fncname, size_t num);

	CValue	IHASH(CSF_FUNCPARAM &p);
	CValue	HASH_KEYS(CSF_FUNCPARAM &p);
	CValue	HASH_VALUES(CSF_FUNCPARAM &p);
	CValue	HASH_SPLIT(CSF_FUNCPARAM &p);
	CValue	HASH_EXIST(CSF_FUNCPARAM &p);
	CValue	HASH_SIZE(CSF_FUNCPARAM &p);

	void	StoreReResultDetails(const yaya::string_t &str,MatchResult &result);
	void	ClearReResultDetails(void);
	void	AppendReResultDetail(const yaya::string_t &str, int pos, int len);
	void	SetError(int code);

	int		GetCharset(const CValueSub &var,const wchar_t *fname, const yaya::string_t &d, int l);
};

//----

#endif
