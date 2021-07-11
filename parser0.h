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
	yaya::string_t	before;
	yaya::string_t	after;
	yaya::string_t	dicfilename;
public:
	CDefine(const yaya::string_t &bef, const yaya::string_t &aft, const yaya::string_t &df) : 
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
	char	ParseEmbedString(yaya::string_t& str, CStatement &st, const yaya::string_t &dicfilename, int linecount);
	int		DynamicLoadDictionary(const yaya::string_t& filename, int charset);

	int		GetFunctionIndexFromName(const yaya::string_t& str);

protected:
	bool	ParseAfterLoad(const yaya::string_t &dicfilename);
	char	LoadDictionary1(const yaya::string_t& filename, std::vector<CDefine>& gdefines, int charset);
	char	GetPreProcess(yaya::string_t& str, std::vector<CDefine>& defines, std::vector<CDefine>& gdefines, const yaya::string_t& dicfilename,
					int linecount);

	void	ExecDefinePreProcess(yaya::string_t &str, const std::vector<CDefine>& defines);
	void	ExecInternalPreProcess(yaya::string_t &str,const yaya::string_t &file,int line);

	char	IsCipheredDic(const yaya::string_t& filename);
	void	SeparateFactor(std::vector<yaya::string_t> &s, yaya::string_t &line);
	char	DefineFunctions(std::vector<yaya::string_t> &s, const yaya::string_t& dicfilename, int linecount, int &depth, int &targetfunction);
	int		MakeFunction(const yaya::string_t& name, int chtype, const yaya::string_t& dicfilename, int linecount);
	char	StoreInternalStatement(int targetfunc, yaya::string_t &str, int& depth, const yaya::string_t& dicfilename, int linecount);
	char	MakeStatement(int type, int targetfunc, yaya::string_t &str, const yaya::string_t& dicfilename, int linecount);
	char	StructWhen(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount);
	char	StructFormula(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount);
	void	StructFormulaCell(yaya::string_t &str, std::vector<CCell> &cells);

	char	AddSimpleIfBrace(const yaya::string_t &dicfilename);

	char	SetCellType(const yaya::string_t &dicfilename);
	char	SetCellType1(CCell& scell, char emb, const yaya::string_t& dicfilename, int linecount);

	char	MakeCompleteFormula(const yaya::string_t &dicfilename);
	char	ParseEmbeddedFactor(const yaya::string_t& dicfilename);
	char	ParseEmbeddedFactor1(CStatement& st, const yaya::string_t& dicfilename);
	void	ConvertPlainString(const yaya::string_t& dicfilename);
	void	ConvertPlainString1(CStatement& st, const yaya::string_t& dicfilename);
	char	ConvertEmbedStringToFormula(yaya::string_t& str, const yaya::string_t& dicfilename, int linecount);
	char	CheckDepthAndSerialize(const yaya::string_t& dicfilename);
	char	CheckDepth1(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckDepthAndSerialize1(CStatement& st, const yaya::string_t& dicfilename);
	char	MakeCompleteConvertionWhenToIf(const yaya::string_t& dicfilename);

	char	IsDicFileAlreadyExist(const yaya::string_t& dicfilename);

	void	RemoveFunctionAndDefineByName(const yaya::string_t& dicfilename);
};

//----

#endif
