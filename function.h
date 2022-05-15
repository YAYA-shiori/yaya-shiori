// 
// AYA version 5
//
// 関数を扱うクラス　CFunction/CStatement
// written by umeici. 2004
// 
// CFunctionが関数、CStatementが関数内のステートメントです。
// CStatementは値の保持のみで、操作はインスタンスを持つCFunctionで行います。
//

#ifndef	FUNCTIONH
#define	FUNCTIONH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <memory>

#include "cell.h"
#include "selecter.h"
#include "globaldef.h"
#include "variable.h"
#include "value.h"

class CAyaVM;
class CCell;
class CSerial;
class CSelecter;

class	CStatement
{
public:
	int				type;			// ステートメントの種別
	ptrdiff_t		jumpto;			// 飛び先行番号 break/continue/return/if/elseif/else/for/foreachで使用します
									// 該当単位終端の"}"の位置が格納されています
	ptrdiff_t linecount;			// 辞書ファイル中の行番号

	mutable std_shared_ptr < CDuplEvInfo >	dupl_block;		// pool:{ //...

private:
	mutable std_shared_ptr<std::vector<CCell> >		m_cell;				// 数式の項の群　
	mutable std_shared_ptr<std::vector<CSerial> >	m_serial;			// 数式の演算順序

public:
	CStatement(int t, ptrdiff_t l, std_shared_ptr<CDuplEvInfo> dupl = std_shared_ptr<CDuplEvInfo>() )
	{
		type = t;
		linecount = l;
		jumpto = 0;
		dupl_block=dupl;
	}
	CStatement(void) {
		type = ST_NOP;
		linecount = 0;
		jumpto = 0;
		dupl_block.reset();
	}
	~CStatement(void) {}

	void deep_copy(CStatement &from) {
		yaya::shared_ptr_deep_copy(from.m_cell,this->m_cell);
		yaya::shared_ptr_deep_copy(from.m_serial,this->m_serial);
	}

	//////////////////////////////////////////////
	std::vector<CCell>::size_type cell_size(void) const {
		if ( ! m_cell.get() ) {
			return 0;
		}
		else {
			return m_cell->size();
		}
	}
	const std::vector<CCell>& cell(void) const {
		if ( ! m_cell.get() ) {
			m_cell=std_make_shared<std::vector<CCell> >();
		}
		return *m_cell;
	}
	std::vector<CCell>& cell(void) {
		if ( ! m_cell.get() ) { 
			m_cell=std_make_shared<std::vector<CCell> >();
		}
		return *m_cell;
	}
	//////////////////////////////////////////////
	std::vector<CSerial>::size_type serial_size(void) const {
		if ( ! m_serial.get() ) {
			return 0;
		}
		else {
			return m_serial->size();
		}
	}
	const std::vector<CSerial>& serial(void) const {
		if ( ! m_serial.get() ) {
			m_serial=std_make_shared<std::vector<CSerial> >();
		}
		return *m_serial;
	}
	std::vector<CSerial>& serial(void) {
		if ( ! m_serial.get() ) {
			m_serial=std_make_shared<std::vector<CSerial> >();
		}
		return *m_serial;
	}
	//////////////////////////////////////////////
	void cell_cleanup(void) const {
		const std::vector<CCell>& c = cell();

		for ( size_t i = 0 ; i < c.size() ; ++i ) {
			c[i].tmpdata_cleanup();
		}
	}
};

//----

class	CFunction
{
private:
	CAyaVM *pvm;
	
public:
	yaya::string_t				name;			// 名前
	yaya::string_t::size_type	namelen;		// 名前の長さ
	std::vector<CStatement>		statement;		// 命令郡
	CDuplEvInfo					dupl_func;		// 重複回避制御
	yaya::string_t				dicfilename;	// 対応する辞書ファイル名
	yaya::string_t				dicfilename_fullpath;

protected:
	size_t					statelenm1;		// statementの長さ-1（1を減じているのは終端の"}"を処理しないためです）
	size_t					linecount;		// 定義された行

private:
	CFunction(void);

public:
	CFunction(CAyaVM& vmr, const yaya::string_t& n, choicetype_t ct, const yaya::string_t& df, int lc);

	~CFunction(void) {}

	void    deep_copy_statement(CFunction &from);

	class ExecutionResult {
	public:
		CSelecter PossibleResults;

		ExecutionResult(CAyaVM* pvm) :PossibleResults(pvm, NULL, BRACE_DEFAULT) {}
		ExecutionResult(CSelecter& a) :PossibleResults(a) {}

		virtual ~ExecutionResult() { }
		
		CValue Output() { return PossibleResults.Output(); }
		size_t OutputNum() { return PossibleResults.OutputNum(); }
		
		operator CValue() { return Output(); }
	};

	void	CompleteSetting(void);
	ExecutionResult	Execute();
	ExecutionResult	Execute(const CValue& arg);
	ExecutionResult	Execute(const CValue &arg, CLocalVariable &lvar);
private:
	void Execute_SEHhelper(ExecutionResult& aret, CLocalVariable& lvar, int& exitcode);
	void Execute_SEHbody(ExecutionResult& retas, CLocalVariable& lvar, int& exitcode);
public:
	const CValue& GetFormulaAnswer(CLocalVariable &lvar, CStatement &st);

	int     ReindexUserFunctions(void);

	const yaya::string_t&	GetFileName() const {return dicfilename;}
	size_t	GetLineNumBegin() const { return linecount;}
	size_t	GetLineNumEnd() const   { return statement.empty() ? 0 : statement[statement.size()-1].linecount;}

protected:
	
	class ExecutionInBraceResult : public ExecutionResult {
	public:
		size_t linenum;

		ExecutionInBraceResult(CSelecter& a, size_t b) : ExecutionResult(a), linenum(b) {}
		virtual ~ExecutionInBraceResult() { }
	};

	ExecutionInBraceResult	ExecuteInBrace(size_t line, CLocalVariable& lvar, yaya::int_t type, int& exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool);

	void	Foreach(CLocalVariable& lvar, CSelecter& output, size_t line, int& exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool);

	const	CValue& GetValueRefForCalc(CCell &cell, CStatement &st, CLocalVariable &lvar);
	
	void	SolveEmbedCell(CCell &cell, CStatement &st, CLocalVariable &lvar);

	char	Comma(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	CommaAdd(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	Subst(int type, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	SubstToArray(CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar);
	char	Array(CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	bool	_in_(const CValue &src, const CValue &dst);
	bool	not_in_(const CValue &src, const CValue &dst);
	char	ExecFunctionWithArgs(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	ExecSystemFunctionWithArgs(CCell& cell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	void	ExecHistoryP1(size_t start_index, CCell& cell, const CValue &arg, CStatement &st);
	void	ExecHistoryP2(CCell &cell, CStatement &st);
	char	Feedback(CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	void	EncodeArrayOrder(CCell &vcell, const CValue &order, CLocalVariable &lvar, CValue &result);
	void	FeedLineToTail(size_t&line);
};

//----

class CFunctionDef
{
private:
	yaya::indexmap map;

public:
	std::vector<CFunction> func;
	
	ptrdiff_t GetFunctionIndexFromName(const yaya::string_t& name);
	void AddFunctionIndex(const yaya::string_t& name,size_t index);
	void ClearFunctionIndex(void);
	void RebuildFunctionMap(void);
	void deep_copy_func(CFunctionDef &from);
};

//----

#endif
