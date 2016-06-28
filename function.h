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
#include "fix_old_compiler.h"

class CAyaVM;
class CCell;
class CSerial;
class CSelecter;

class	CStatement
{
public:
	int				type;			// ステートメントの種別
	int				jumpto;			// 飛び先行番号 break/continue/return/if/elseif/else/for/foreachで使用します
									// 該当単位終端の"}"の位置が格納されています
	int	linecount;					// 辞書ファイル中の行番号

private:
	mutable std_shared_ptr<std::vector<CCell> >   m_cell;			// 数式の項の群　
	mutable std_shared_ptr<std::vector<CSerial> > m_serial;			// 数式の演算順序

public:
	CStatement(int t, int l)
	{
		type      = t;
		linecount = l;
	}
	CStatement(void) {}
	~CStatement(void) {}

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
			m_cell.reset(new std::vector<CCell>);
		}
		return *m_cell;
	}
	std::vector<CCell>& cell(void) {
		if ( ! m_cell.get() ) {
			m_cell.reset(new std::vector<CCell>);
		}
		else if ( m_cell.use_count() >= 2 ) {
			std::vector<CCell> *pV = m_cell.get();
			m_cell.reset(new std::vector<CCell>(*pV));
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
			m_serial.reset(new std::vector<CSerial>);
		}
		return *m_serial;
	}
	std::vector<CSerial>& serial(void) {
		if ( ! m_serial.get() ) {
			m_serial.reset(new std::vector<CSerial>);
		}
		else if ( m_serial.use_count() >= 2 ) {
			std::vector<CSerial> *pV = m_serial.get();
			m_serial.reset(new std::vector<CSerial>(*pV));
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
	CDuplEvInfo					dupl;			// 重複回避制御
	yaya::string_t				dicfilename;	// 対応する辞書ファイル名

protected:
	int					statelenm1;		// statementの長さ-1（1を減じているのは終端の"}"を処理しないためです）

private:
	CFunction(void);

public:
	CFunction(CAyaVM &vmr, const yaya::string_t& n, int ct, const yaya::string_t& df) : pvm(&vmr) , dupl(ct)
	{
		name        = n;
		namelen     = name.size();
		dicfilename = df;
	}

	~CFunction(void) {}

	void	CompleteSetting(void);
	int		Execute(CValue &result, const CValue &arg, CLocalVariable &lvar);

	const CValue& GetFormulaAnswer(CLocalVariable &lvar, CStatement &st);

protected:
	int		ExecuteInBrace(int line, CValue &result, CLocalVariable &lvar, int type, int &exitcode);

	void	Foreach(CLocalVariable &lvar, CSelecter &output, int line, int &exitcode);

	const CValue& GetValueRefForCalc(CCell &cell, CStatement &st, CLocalVariable &lvar);
	
	void	SolveEmbedCell(CCell &cell, CStatement &st, CLocalVariable &lvar);

	char	Comma(CValue &answer, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	char	CommaAdd(CValue &answer, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	char	Subst(int type, CValue &answer, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	char	SubstToArray(CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar);
	char	Array(CCell &anscell, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	int		_in_(const CValue &src, const CValue &dst);
	int		not_in_(const CValue &src, const CValue &dst);
	char	ExecFunctionWithArgs(CValue &answer, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	char	ExecSystemFunctionWithArgs(CCell& cell, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	void	ExecHistoryP1(int start_index, CCell& cell, const CValue &arg, CStatement &st);
	void	ExecHistoryP2(CCell &cell, CStatement &st);
	char	Feedback(CCell &anscell, std::vector<int> &sid, CStatement &st, CLocalVariable &lvar);
	void	EncodeArrayOrder(CCell &vcell, const CValue &order, CLocalVariable &lvar, CValue &result);
	void	FeedLineToTail(int &line);
};

//----

#endif
