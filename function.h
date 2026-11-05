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
	mutable bool						ismutiarea;

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
		ismutiarea = false;
	}
	CStatement(void) {
		type = ST_NOP;
		linecount = 0;
		jumpto = 0;
		dupl_block.reset();
		ismutiarea = false;
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
	// 各セルの一時値をpからの領域（1セルにつき3つ）と交換し、次の領域を返します
	std_shared_ptr<CValue> *cell_swap_tmpdata(std_shared_ptr<CValue> *p) const {
		if ( ! m_cell.get() ) {
			return p;
		}
		for ( size_t i = 0 ; i < m_cell->size() ; ++i ) {
			(*m_cell)[i].tmpdata_swap(p);
			p += 3;
		}
		return p;
	}
	void cell_cleanup(void) const {
		const std::vector<CCell>& c = cell();

		for ( size_t i = 0 ; i < c.size() ; ++i ) {
			c[i].tmpdata_cleanup();
		}
	}
	void cell_reset_tmpdata(void) const {
		if ( ! m_cell.get() ) {
			return;
		}
		for ( size_t i = 0 ; i < m_cell->size() ; ++i ) {
			(*m_cell)[i].tmpdata_reset();
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
	yaya::string_t				dicfilename;	// 対応する辞書ファイル名
	yaya::string_t				dicfilename_fullpath;

protected:
	size_t					statelenm1;		// statementの長さ-1（1を減じているのは終端の"}"を処理しないためです）
	size_t					linecount;		// 定義された行
	size_t					execdepth;		// 実行中の呼び出しの数（再帰の検出用）
	bool					shared_ast;		// セル（中間コード）を別の関数表と共有している。スレッド用の関数表の関数は、
											// 初めて実行するときにMakePrivateで自分用のコピーにする

	friend class CFunctionReentryGuard;

private:
	CFunction(void);

public:
	CFunction(CAyaVM& vmr, const yaya::string_t& n, const yaya::string_t& df, int lc);

	~CFunction(void) {}

	void    deep_copy_statement(CFunction &from);
	// 実行中の状態（再帰の深さとセルの一時値）を捨てる。スレッド用に関数表をコピーした直後に使う
	void    ResetExecState(void) {
		execdepth = 0;
		for ( size_t i = 0 ; i < statement.size() ; ++i ) {
			statement[i].cell_reset_tmpdata();
		}
	}
	// セルを共有している関数（shared_ast）を、自分用のコピーにする
	void    MakePrivate(void);
	void    SetSharedAst(bool value) { shared_ast = value; }

	class ExecutionResult {
	public:
		CSelecter PossibleResults;

		ExecutionResult(CAyaVM* pvm) :PossibleResults(pvm, NULL, BRACE_DEFAULT) {}
		// 候補を持たない入れ物（swapで受け取る専用。受け取る前にOutputしてはいけない）
		enum EmptyTag { EMPTY };
		ExecutionResult(CAyaVM* pvm, EmptyTag) :PossibleResults(pvm, CSelecter::EMPTY) {}
		// aの候補を奪う（コピーしない）。aは以後破棄しか許されない
		ExecutionResult(CSelecter& a) :PossibleResults(a, CSelecter::STEAL) {}

		virtual ~ExecutionResult() { }
		
		CValue Output() { return PossibleResults.Output(); }
		// Outputと同じ結果をdstに入れる。この結果は以後使えない（コピーを省くため）
		void OutputConsume(CValue &dst) { PossibleResults.OutputConsume(dst); }
		size_t OutputNum() { return PossibleResults.OutputNum(); }
		// 中身を入れ替える（コピーしない）
		void swap(ExecutionResult &rhs) { PossibleResults.swap(rhs.PossibleResults); }
		
		operator CValue() { return Output(); }
	};

	void	CompleteSetting(void);
	ExecutionResult	Execute();
	ExecutionResult	Execute(const CValue& arg);
	ExecutionResult	Execute(const CValue &arg, CLocalVariable &lvar);
	// Execute(arg, lvar).Output()と同じ値をanswerに入れる（出力候補をコピーしない）
	void	ExecuteToValue(CValue &answer, const CValue &arg, CLocalVariable &lvar);
	ExecutionResult	ExecuteEval(CLocalVariable &lvar);
private:
	void	ExecuteInto(ExecutionResult &result, const CValue &arg, CLocalVariable &lvar);
	void	CallLimitExceeded(ExecutionResult &result);
	void Execute_SEHhelper(ExecutionResult& aret, CLocalVariable& lvar, int& exitcode);
	void Execute_SEHbody(ExecutionResult& retas, CLocalVariable& lvar, int& exitcode);
public:
	const CValue& GetFormulaAnswer(CLocalVariable &lvar, CStatement &st);

	int     ReindexUserFunctions(void);

	const yaya::string_t&	GetFileName() const {return dicfilename;}
	size_t	GetLineNumBegin() const { return linecount;}
	size_t	GetLineNumEnd() const   { return statement.empty() ? 0 : statement.back().linecount;}

protected:
	
	class ExecutionInBraceResult : public ExecutionResult {
	public:
		size_t linenum;

		ExecutionInBraceResult(CSelecter& a, size_t b) : ExecutionResult(a), linenum(b) {}
		virtual ~ExecutionInBraceResult() { }
	};

	struct SReturnWithParamExpr {
		CValue value;
		bool   used;
		SReturnWithParamExpr() : used(false) {}
	};

	// ExecuteInBraceの補助。再帰のたびに積まれるExecuteInBraceのフレームを小さく保つため、
	// 再帰の経路に載らない処理や、一時変数の多い処理を分けてある（いずれもインライン展開しない）
	size_t	ExecuteSubBlock(size_t line, CLocalVariable& lvar, yaya::int_t type, int& exitcode, CSelecter& output, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr);
	void	FinishBlock(CSelecter& output, int exitcode, SReturnWithParamExpr* pReturnExpr, CSelecter* pUpperOutput, std::vector<CVecValue>* UpperLvCandidatePool, bool meltblock, bool inpool, bool ispoolbegin, bool notpoolblock, bool inmutiarea);
	void	AppendParallel(CSelecter& output, const CValue& val);
	void	SetForeachElement(CValue& value, CValue& key, const yaya::string_t& elem, int index);
	void	SetForeachKey(CValue& key, int index);
	void	TakeFormulaAnswer(CValue& dst, CStatement& st, const CValue& ans);
	void	AppendFormulaOutput(CSelecter& output, CStatement& st, const CValue& ans);
	void	LoopLimitExceeded(const CStatement& st);
	// ワーカースレッドの確認（ExecuteInBraceのフレームを増やさないよう分けてある）
	// 実行を打ち切るべきなら、exitcodeをST_RETURNにして真を返す
	bool	ThreadCancelStep(size_t& line, const CStatement& st, int& exitcode);
	bool	ThreadCancelLoop(int& exitcode);
	// GetFormulaAnswerの補助（同じ理由で分けてある）
	void	ApplyBinaryOp(CCell& o_cell, int op, const CValue& lv, const CValue& rv, const CStatement& st);
	void	SetIntAnswer(CCell& o_cell, int value);
	void	ExecSystemFunctionNoArg(CCell& cell, CStatement& st, CLocalVariable& lvar);

	ExecutionInBraceResult	ExecuteInBrace(size_t line, CLocalVariable& lvar, yaya::int_t type, int& exitcode, CSelecter* pUpperOutput, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr = NULL);

	void	Foreach(CLocalVariable& lvar, CSelecter& output, size_t line, int& exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr = NULL);

	const	CValue& GetValueRefForCalc(CCell &cell, CStatement &st, CLocalVariable &lvar);
	const	CValue& GetLeftValueRef(CCell &s_cell, const CCell *d_cell, CValue &hold, CStatement &st, CLocalVariable &lvar);
	
	void	SolveEmbedCell(CCell &cell, CStatement &st, CLocalVariable &lvar);

	char	Comma(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	CommaAdd(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	CVariable*	GetSubstVariable(const CCell &vcell, CLocalVariable &lvar);
	char	Subst(int type, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar, bool need_answer = true);
	const	CValue& GetCompoundSubstLeftValue(std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	SubstToArray(CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar, size_t oindex = 0, bool spread = true);
	ptrdiff_t	FindUpperArrayOrder(CStatement &st, size_t oindex);
	ptrdiff_t	FindFeedbackArrayOrder(CStatement &st, size_t findex);
	char	RefreshUpperArrayOrder(CStatement &st, size_t oindex, CLocalVariable &lvar);
	void	WarnHashCalc(const CValue &lv, const CValue &rv, const yaya::char_t *op, const CStatement &st);
	char	Array(CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	bool	_in_(const CValue &src, const CValue &dst);
	bool	not_in_(const CValue &src, const CValue &dst);
	CFunction*	GetUserFunction(const CCell &cell);
	char	ExecFunctionWithArgs(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	void	BuildFunctionArgs(CValue &arg, std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	WriteBackFeedback(CLocalVariable &t_lvar, const std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
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
	// スレッド用に関数表をコピーした直後に呼ぶ。セルはコピーせず共有したままにして、
	// 各関数を初めて実行するとき（CFunction::MakePrivate）に自分用のコピーを作る
	void MarkSharedAst(void) {
		for (size_t i = 0; i < func.size(); ++i) {
			func[i].SetSharedAst(true);
		}
	}
};

//----

#endif
