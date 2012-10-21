// 
// AYA version 5
//
// �\�����/���ԃR�[�h�̐������s���N���X�@CParser0
// written by umeici. 2004
// 
// �\����͎���CBasis�����x����CParser0::Parse�����s����܂��B
// CParser0::ParseEmbedString��eval�n�̏����Ŏg�p����܂��B
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
public:
	CDefine(yaya::string_t &bef, yaya::string_t &aft)
	{
		before = bef;
		after  = aft;
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
	char	Parse(int charset, const std::vector<CDic1>& dics, int &lindex, int &ulindex, int &rindex);
	char	ParseEmbedString(yaya::string_t& str, CStatement &st, const yaya::string_t &dicfilename, int linecount);

	int		GetFunctionIndexFromName(const yaya::string_t& str);

protected:
	char	LoadDictionary1(const yaya::string_t& filename, std::vector<CDefine>& gdefines, int charset);
	char	GetPreProcess(yaya::string_t& str, std::vector<CDefine>& defines, std::vector<CDefine>& gdefines, const yaya::string_t& dicfilename,
					int linecount);

	void	ExecDefinePreProcess(yaya::string_t &str, const std::vector<CDefine>& defines);
	void	ExecInternalPreProcess(yaya::string_t &str,const yaya::string_t &file,int line);

	char	IsCipheredDic(const yaya::string_t& filename);
	void	SeparateFactor(std::vector<yaya::string_t> &s, yaya::string_t &line);
	char	DefineFunctions(std::vector<yaya::string_t> &s, const yaya::string_t& dicfilename, int linecount, int &depth, int &targetfunction);
	int		MakeFunction(const yaya::string_t& name, int chtype, const yaya::string_t& dicfilename);
	char	StoreInternalStatement(int targetfunc, yaya::string_t &str, int& depth, const yaya::string_t& dicfilename, int linecount);
	char	MakeStatement(int type, int targetfunc, yaya::string_t &str, const yaya::string_t& dicfilename, int linecount);
	char	StructWhen(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount);
	char	StructFormula(yaya::string_t &str, std::vector<CCell> &cells, const yaya::string_t& dicfilename, int linecount);
	void	StructFormulaCell(yaya::string_t &str, std::vector<CCell> &cells);

	char	AddSimpleIfBrace(void);

	char	SetCellType(void);
	char	SetCellType1(CCell& scell, char emb, const yaya::string_t& dicfilename, int linecount);

	char	MakeCompleteFormula(void);
	char	ParseEmbeddedFactor(void);
	char	ParseEmbeddedFactor1(CStatement& st, const yaya::string_t& dicfilename);
	void	ConvertPlainString(void);
	void	ConvertPlainString1(CStatement& st, const yaya::string_t& dicfilename);
	char	ConvertEmbedStringToFormula(yaya::string_t& str, const yaya::string_t& dicfilename, int linecount);
	char	CheckDepthAndSerialize(void);
	char	CheckDepth1(CStatement& st, const yaya::string_t& dicfilename);
	char	CheckDepthAndSerialize1(CStatement& st, const yaya::string_t& dicfilename);
	char	MakeCompleteConvertionWhenToIf(void);
};

//----

#endif
