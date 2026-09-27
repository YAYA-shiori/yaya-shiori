// 
// AYA version 5
//
// 構文解析/中間コードの生成を行うクラス　CParser1
// written by umeici. 2004
// 
// 構文解析時にCParser0から一度だけCParser1::CheckExecutionCodeが実行されます。
//

#ifndef	PARSER1H
#define	PARSER1H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "globaldef.h"

class	CVecint
{
public:
	std::vector<int> i_array;
};

class CAyaVM;
class CStatement;
class CFunction;

//----

class	CParser1
{
private:
	CAyaVM &vm;

	CParser1(void);

public:
	CParser1(CAyaVM &vmr) : vm(vmr) {
		; //NOOP
	}

	char	CheckExecutionCode(const yaya::string_t& dicfilename);
	char	CheckExecutionCode(CFunction& func);
	char	CheckExecutionCode1(CStatement& st, const yaya::string_t& dicfilename);

protected:
	char	CheckNomialCount(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckSubstSyntax(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckFeedbackOperatorPos(CStatement& st, const yaya::string_t& dicfilename);
	char	SetFormulaType(CStatement& st, const yaya::string_t& dicfilename);
	char	SetBreakJumpNo(const yaya::string_t& dicfilename);
	char	SetBreakJumpNo(CFunction& func);
	char	CheckCaseSyntax(const yaya::string_t& dicfilename);
	char	CheckCaseSyntax(CFunction& func);
	char	CheckIfSyntax(const yaya::string_t& dicfilename);
	char	CheckIfSyntax(CFunction& func);
	char	CheckElseSyntax(const yaya::string_t& dicfilename);
	char	CheckElseSyntax(CFunction& func);
	char	CheckForSyntax(const yaya::string_t& dicfilename);
	char	CheckForSyntax(CFunction& func);
	char	CheckForeachSyntax(const yaya::string_t& dicfilename);
	char	CheckForeachSyntax(CFunction& func);
	char	SetIfJumpNo(const yaya::string_t& dicfilename);
	char	SetIfJumpNo(CFunction& func);
	char	CheckFunctionArgument(CStatement& st, const yaya::string_t& dicfilename);

	void	CompleteSetting(void);
};

//----

#endif
