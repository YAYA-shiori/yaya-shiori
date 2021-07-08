// 
// AYA version 5
//
// 中間コードをログに出力するクラス　CLogExCode
// written by umeici. 2004
// 
// デバッグ用の機能です。実動作には関係しません。
// これらの関数が吐き出す出力は、ソースコードを理解していないと読み難いものです。
//

#ifndef	LOGEXCODEH
#define	LOGEXCODEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "function.h"
#include "globaldef.h"

class CAyaVM;

class	CLogExCode
{
private:
	CAyaVM &vm;

	CLogExCode(void);
public:
	CLogExCode(CAyaVM &vmr) : vm(vmr) {
	}

	void	OutExecutionCodeForCheck(void);
	void	OutVariableInfoForCheck(void);
protected:
	void	StructCellString(std::vector<CCell> *cellvector, aya::string_t &formula);
	void	StructSerialString(CStatement *st, aya::string_t &formula);
	void	StructArrayString(const CValueArray &vs, aya::string_t &enlist);
};

//----

#endif
