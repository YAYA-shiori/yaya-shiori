// 
// AYA version 5
//
// �֐��������N���X�@CFunction/CStatement
// written by umeici. 2004
// 
// CFunction���֐��ACStatement���֐����̃X�e�[�g�����g�ł��B
// CStatement�͒l�̕ێ��݂̂ŁA����̓C���X�^���X������CFunction�ōs���܂��B
//

#ifndef	FUNCTIONH
#define	FUNCTIONH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <boost/shared_ptr.hpp>

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
	int				type;			// �X�e�[�g�����g�̎��
	int				jumpto;			// ��ѐ�s�ԍ� break/continue/return/if/elseif/else/for/foreach�Ŏg�p���܂�
									// �Y���P�ʏI�[��"}"�̈ʒu���i�[����Ă��܂�
	int	linecount;					// �����t�@�C�����̍s�ԍ�

private:
	mutable boost::shared_ptr<std::vector<CCell> >   m_cell;			// �����̍��̌Q�@
	mutable boost::shared_ptr<std::vector<CSerial> > m_serial;			// �����̉��Z����

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
};

//----

class	CFunction
{
public:
	yaya::string_t				name;			// ���O
	yaya::string_t::size_type	namelen;		// ���O�̒���
	std::vector<CStatement>		statement;		// ���ߌS
	CDuplEvInfo					dupl;			// �d����𐧌�
	yaya::string_t				dicfilename;	// �Ή����鎫���t�@�C����

protected:
	int					statelenm1;		// statement�̒���-1�i1�������Ă���̂͏I�[��"}"���������Ȃ����߂ł��j

private:
	CAyaVM *pvm;

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
