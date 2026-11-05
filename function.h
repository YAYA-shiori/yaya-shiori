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

#include <assert.h>
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
	size_t							tmpbase;		// 実行フレーム（CExecFrame）の中での、この文の最初の項の位置（CFunction::CompleteSettingが決める）

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
		tmpbase = 0;
	}
	CStatement(void) {
		type = ST_NOP;
		linecount = 0;
		jumpto = 0;
		dupl_block.reset();
		ismutiarea = false;
		tmpbase = 0;
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
};

//----

/* -----------------------------------------------------------------------
 *  クラス名：  CExecFrame
 *  機能概要：  関数の1回の実行（呼び出し）が、式の評価中に使う一時値の置き場です
 *
 *  CCell（中間コード）には一時値を持たせず、項ごとの一時値（CCellTmp）をここに持たせます。
 *  これで中間コードは実行中に書き換わらない（読み取り専用）ので、同じ関数が再帰したり
 *  複数のスレッドから同時に実行されたりしても、呼び出しごとのフレームが別であれば衝突しません。
 *
 *  一時値の位置は、文の tmpbase と文の中の項の位置の和です（at）。
 *  フレームと文は必ず対応させること。関数のフレーム（CFunction::framesize 個）は
 *  その関数の文にだけ使い、関数に属さない文（EVALの式など）には、その文の項の数だけの
 *  専用のフレームを用意する（tmpbase は0のまま）。
 *  フレームの大きさは作るときに決めて変えない（GetFormulaAnswerが一時値への参照を返すため）。
 * -----------------------------------------------------------------------
 */
class CExecFrame
{
private:
	mutable std::vector<CCellTmp>	m_tmp;

public:
	explicit CExecFrame(size_t n) : m_tmp(n) { }
	size_t size(void) const { return m_tmp.size(); }

	// 文stの項cの一時値。cはst.cell()の要素でなければならない
	CCellTmp& at(const CStatement &st, const CCell &c) const {
		const CCell *pbase = &(st.cell()[0]);
		assert(&c >= pbase && &c < pbase + st.cell_size());
		const size_t i = st.tmpbase + static_cast<size_t>(&c - pbase);
		assert(i < m_tmp.size());
		return m_tmp[i];
	}

	// 大きな値（長い文字列や配列）を持ったままにしないよう手放す
	void cleanup(void) {
		for ( size_t i = 0 ; i < m_tmp.size() ; ++i ) {
			m_tmp[i].tmpdata_cleanup();
		}
	}
};

/* -----------------------------------------------------------------------
 *  クラス名：  CFrameCache
 *  機能概要：  関数が、使い終わった実行フレームを1つだけ取っておく入れ物です
 *
 *  非再帰の呼び出しでは前回のフレームを使い回せる（一時値の確保をやり直さない）。
 *  再帰やスレッドで、取っておいたフレームがすでに使われている間の呼び出しには
 *  新しいフレームを作る。
 *
 *  コピーしても中身は引き継がない。CFunctionDefのコピー（VMの複製）で
 *  同じフレームを複数の関数が持つと、同じフレームを同時に使ってしまうため
 * -----------------------------------------------------------------------
 */
class CFrameCache
{
private:
	CExecFrame	*m_idle;

public:
	CFrameCache(void) : m_idle(NULL) { }
	CFrameCache(const CFrameCache&) : m_idle(NULL) { }
	CFrameCache& operator=(const CFrameCache&) {
		clear();
		return *this;
	}
	~CFrameCache(void) { clear(); }

	void clear(void) {
		delete m_idle;
		m_idle = NULL;
	}
	// n個の一時値を持つフレームを取り出す。無ければ作る
	CExecFrame* take(size_t n) {
		CExecFrame *p = m_idle;
		m_idle = NULL;
		if (p && p->size() != n) {
			delete p;
			p = NULL;
		}
		return p ? p : new CExecFrame(n);
	}
	// 使い終わったフレームを返す。すでに1つ持っていれば捨てる
	void give(CExecFrame *p) {
		if (m_idle) {
			delete p;
		}
		else {
			m_idle = p;
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
	size_t					framesize;		// 実行フレームの大きさ（全文の項の数。CompleteSettingで決める）
	CFrameCache				frame_cache;	// 使い終わった実行フレームを1つ取っておく（コピーしても引き継がない）

	friend class CFrameHolder;

private:
	CFunction(void);

	// 実行フレームの貸し出しと返却（CFrameHolderが対で呼ぶ。実行の経路のフレームを増やさないよう別の関数にしてある）
	CExecFrame*	AcquireFrame(void);
	void		ReleaseFrame(CExecFrame *pf);

public:
	CFunction(CAyaVM& vmr, const yaya::string_t& n, const yaya::string_t& df, int lc);

	~CFunction(void) {}

	void    deep_copy_statement(CFunction &from);

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
	void Execute_SEHhelper(CExecFrame &fr, ExecutionResult& aret, CLocalVariable& lvar, int& exitcode);
	void Execute_SEHbody(CExecFrame &fr, ExecutionResult& retas, CLocalVariable& lvar, int& exitcode);
public:
	const CValue& GetFormulaAnswer(CExecFrame &fr, CLocalVariable &lvar, CStatement &st);
	// この関数に属さない一時の文（EVALの式）を実行して結果を返す。文の専用のフレームを用意する
	CValue	EvalTempStatement(CLocalVariable &lvar, CStatement &st);

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
	size_t	ExecuteSubBlock(CExecFrame &fr, size_t line, CLocalVariable& lvar, yaya::int_t type, int& exitcode, CSelecter& output, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr);
	void	FinishBlock(CSelecter& output, int exitcode, SReturnWithParamExpr* pReturnExpr, CSelecter* pUpperOutput, std::vector<CVecValue>* UpperLvCandidatePool, bool meltblock, bool inpool, bool ispoolbegin, bool notpoolblock, bool inmutiarea);
	void	AppendParallel(CSelecter& output, const CValue& val);
	void	SetForeachElement(CValue& value, CValue& key, const yaya::string_t& elem, int index);
	void	SetForeachKey(CValue& key, int index);
	void	TakeFormulaAnswer(CExecFrame &fr, CValue& dst, CStatement& st, const CValue& ans);
	void	AppendFormulaOutput(CExecFrame &fr, CSelecter& output, CStatement& st, const CValue& ans);
	void	LoopLimitExceeded(const CStatement& st);
	// ワーカースレッドの確認（ExecuteInBraceのフレームを増やさないよう分けてある）
	// 実行を打ち切るべきなら、exitcodeをST_RETURNにして真を返す
	bool	ThreadCancelStep(size_t& line, const CStatement& st, int& exitcode);
	bool	ThreadCancelLoop(int& exitcode);
	// GetFormulaAnswerの補助（同じ理由で分けてある）
	void	ApplyBinaryOp(CExecFrame &fr, CCell& o_cell, int op, const CValue& lv, const CValue& rv, const CStatement& st);
	void	SetIntAnswer(CExecFrame &fr, const CStatement& st, CCell& o_cell, int value);
	void	ExecSystemFunctionNoArg(CExecFrame &fr, CCell& cell, CStatement& st, CLocalVariable& lvar);

	ExecutionInBraceResult	ExecuteInBrace(CExecFrame &fr, size_t line, CLocalVariable& lvar, yaya::int_t type, int& exitcode, CSelecter* pUpperOutput, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr = NULL);

	void	Foreach(CExecFrame &fr, CLocalVariable& lvar, CSelecter& output, size_t line, int& exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr = NULL);

	const	CValue& GetValueRefForCalc(CExecFrame &fr, CCell &cell, CStatement &st, CLocalVariable &lvar);
	const	CValue& GetLeftValueRef(CExecFrame &fr, CCell &s_cell, const CCell *d_cell, CValue &hold, CStatement &st, CLocalVariable &lvar);
	
	void	SolveEmbedCell(CExecFrame &fr, CCell &cell, CStatement &st, CLocalVariable &lvar);

	char	Comma(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	CommaAdd(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	CVariable*	GetSubstVariable(const CCell &vcell, CLocalVariable &lvar);
	char	Subst(CExecFrame &fr, int type, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar, bool need_answer = true);
	const	CValue& GetCompoundSubstLeftValue(CExecFrame &fr, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	SubstToArray(CExecFrame &fr, CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar, size_t oindex = 0, bool spread = true);
	ptrdiff_t	FindUpperArrayOrder(CStatement &st, size_t oindex);
	ptrdiff_t	FindFeedbackArrayOrder(CStatement &st, size_t findex);
	char	RefreshUpperArrayOrder(CExecFrame &fr, CStatement &st, size_t oindex, CLocalVariable &lvar);
	void	WarnHashCalc(const CValue &lv, const CValue &rv, const yaya::char_t *op, const CStatement &st);
	char	Array(CExecFrame &fr, CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	bool	_in_(const CValue &src, const CValue &dst);
	bool	not_in_(const CValue &src, const CValue &dst);
	CFunction*	GetUserFunction(const CCell &cell);
	char	ExecFunctionWithArgs(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	void	BuildFunctionArgs(CExecFrame &fr, CValue &arg, std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	WriteBackFeedback(CExecFrame &fr, CLocalVariable &t_lvar, const std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	char	ExecSystemFunctionWithArgs(CExecFrame &fr, CCell& cell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
	void	ExecHistoryP1(CExecFrame &fr, size_t start_index, CCell& cell, const CValue &arg, CStatement &st);
	void	ExecHistoryP2(CExecFrame &fr, CCell &cell, CStatement &st);
	char	Feedback(CExecFrame &fr, CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar);
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
