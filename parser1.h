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
	char	CheckExecutionCode1(CStatement& st, const yaya::string_t& dicfilename);

protected:
	char	CheckNomialCount(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckSubstSyntax(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckFeedbackOperatorPos(CStatement& st, const yaya::string_t& dicfilename);
	char	SetFormulaType(CStatement& st, const yaya::string_t& dicfilename);
	char	SetBreakJumpNo(const yaya::string_t& dicfilename);
	char	CheckCaseSyntax(const yaya::string_t& dicfilename);
	char	CheckIfSyntax(const yaya::string_t& dicfilename);
	char	CheckElseSyntax(const yaya::string_t& dicfilename);
	char	CheckForSyntax(const yaya::string_t& dicfilename);
	char	CheckForeachSyntax(const yaya::string_t& dicfilename);
	char	SetIfJumpNo(const yaya::string_t& dicfilename);
	char	CheckFunctionArgument(CStatement& st, const yaya::string_t& dicfilename);

	void	CompleteSetting(void);
};

//----

#endif
