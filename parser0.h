// 
// AYA version 5
//
// 構文解析/中間コードの生成を行うクラス　CParser0
// written by umeici. 2004
// 
// 構文解析時にCBasisから一度だけCParser0::Parseが実行されます。
// CParser0::ParseEmbedStringはeval系の処理で使用されます。
//

#ifndef	PARSER0H
#define	PARSER0H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include "globaldef.h"

class	CDefine
{
public:
	aya::string_t	before;
	aya::string_t	after;
	aya::string_t	dicfilename;
public:
	CDefine(const aya::string_t &bef, const aya::string_t &aft, const aya::string_t &df) : 
	  before(bef) , after(aft) , dicfilename(df)
	{
	}

	CDefine(void) {}
	~CDefine(void) {}
};

//----

class CAyaVM;
class CStatement;
class CCell;
class CDic1;

class	CParser0
{
private:
	CAyaVM &vm;

	CParser0(void);

public:
	CParser0(CAyaVM &vmr) : vm(vmr) {
		; //NOOP
	}
	char	Parse(int charset, const std::vector<CDic1>& dics);
	char	ParseEmbedString(aya::string_t& str, CStatement &st, const aya::string_t &dicfilename, int linecount);
	int		DynamicLoadDictionary(const aya::string_t& filename, int charset);

	int		GetFunctionIndexFromName(const aya::string_t& str);

protected:
	bool	ParseAfterLoad(const aya::string_t &dicfilename);
	char	LoadDictionary1(const aya::string_t& filename, std::vector<CDefine>& gdefines, int charset);
	char	GetPreProcess(aya::string_t& str, std::vector<CDefine>& defines, std::vector<CDefine>& gdefines, const aya::string_t& dicfilename,
					int linecount);

	void	ExecDefinePreProcess(aya::string_t &str, const std::vector<CDefine>& defines);
	void	ExecInternalPreProcess(aya::string_t &str,const aya::string_t &file,int line);

	char	IsCipheredDic(const aya::string_t& filename);
	void	SeparateFactor(std::vector<aya::string_t> &s, aya::string_t &line);
	char	DefineFunctions(std::vector<aya::string_t> &s, const aya::string_t& dicfilename, int linecount, int &depth, int &targetfunction);
	int		MakeFunction(const aya::string_t& name, int chtype, const aya::string_t& dicfilename);
	char	StoreInternalStatement(int targetfunc, aya::string_t &str, int& depth, const aya::string_t& dicfilename, int linecount);
	char	MakeStatement(int type, int targetfunc, aya::string_t &str, const aya::string_t& dicfilename, int linecount);
	char	StructWhen(aya::string_t &str, std::vector<CCell> &cells, const aya::string_t& dicfilename, int linecount);
	char	StructFormula(aya::string_t &str, std::vector<CCell> &cells, const aya::string_t& dicfilename, int linecount);
	void	StructFormulaCell(aya::string_t &str, std::vector<CCell> &cells);

	char	AddSimpleIfBrace(const aya::string_t &dicfilename);

	char	SetCellType(const aya::string_t &dicfilename);
	char	SetCellType1(CCell& scell, char emb, const aya::string_t& dicfilename, int linecount);

	char	MakeCompleteFormula(const aya::string_t &dicfilename);
	char	ParseEmbeddedFactor(const aya::string_t& dicfilename);
	char	ParseEmbeddedFactor1(CStatement& st, const aya::string_t& dicfilename);
	void	ConvertPlainString(const aya::string_t& dicfilename);
	void	ConvertPlainString1(CStatement& st, const aya::string_t& dicfilename);
	char	ConvertEmbedStringToFormula(aya::string_t& str, const aya::string_t& dicfilename, int linecount);
	char	CheckDepthAndSerialize(const aya::string_t& dicfilename);
	char	CheckDepth1(CStatement& st, const aya::string_t& dicfilename);
	char	CheckDepthAndSerialize1(CStatement& st, const aya::string_t& dicfilename);
	char	MakeCompleteConvertionWhenToIf(const aya::string_t& dicfilename);

	char	IsDicFileAlreadyExist(const aya::string_t& dicfilename);

	void	RemoveFunctionAndDefineByName(const aya::string_t& dicfilename);
};

//----

#endif
