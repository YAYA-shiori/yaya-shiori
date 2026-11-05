// 
// AYA version 5
//
// 関数を扱うクラス　CFunction
// - 主処理部
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <assert.h>

#include <vector>

#include "function.h"
#include "ayavm.h"

#include "log.h"
#include "messages.h"
#include "misc.h"
#include "parser0.h"
#include "globaldef.h"
#include "sysfunc.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
#include "basis.h"
////////////////////////////////////////

// 再帰の経路に載らない補助関数が呼び出し元へインライン展開されると、呼び出し元のフレーム
// （再帰のたびに積まれる）が元に戻ってしまう。VC6は同じ型の一時変数でも領域を共有しない
// ので、展開を禁じる。VC6は関数単位の指定が無いため #pragma auto_inline で囲む
#if defined(__GNUC__)
# define YAYA_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER) && _MSC_VER >= 1300
# define YAYA_NOINLINE __declspec(noinline)
#else
# define YAYA_NOINLINE
#endif

// 関数の呼び出しのたびに作る _argv/_argc/_FUNC_NAME_ の名前（毎回文字列を作らないよう使い回す）
static const yaya::string_t& ArgvName(void) {
	static const yaya::string_t s(L"_argv");
	return s;
}
static const yaya::string_t& ArgcName(void) {
	static const yaya::string_t s(L"_argc");
	return s;
}
static const yaya::string_t& FuncNameName(void) {
	static const yaya::string_t s(L"_FUNC_NAME_");
	return s;
}

CFunction::CFunction(CAyaVM& vmr, const yaya::string_t& n, const yaya::string_t& df, int lc) : pvm(&vmr), name(n), dicfilename(df), linecount(lc), dicfilename_fullpath(vmr.basis().ToFullPath(df)), framesize(0)
{
	statelenm1 = 0;
	namelen = name.size();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::deep_copy_statement
 *  機能概要：  CStatement配列をディープコピーします
 * -----------------------------------------------------------------------
 */
void    CFunction::deep_copy_statement(CFunction &from)
{
	for ( size_t i = 0 ; i < from.statement.size() ; ++i ) {
		statement[i].deep_copy(from.statement[i]);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::CompleteSetting
 *  機能概要：  関数の構築が完了した（≒辞書の読み込みが完了した）際に呼ばれます
 *  　　　　　  実行の際に必要な最後の初期化処理を行ないます
 * -----------------------------------------------------------------------
 */
void	CFunction::CompleteSetting(void)
{
	statelenm1 = statement.size() - 1;

	// 実行フレーム（CExecFrame）の中での、各文の一時値の位置を決める
	// （中間コードを作り終えてから呼ぶこと。項の数が変わったらもう一度呼ぶ）
	size_t base = 0;
	for ( size_t i = 0 ; i < statement.size() ; ++i ) {
		statement[i].tmpbase = base;
		base += statement[i].cell_size();
	}
	framesize = base;
	frame_cache.clear();
}

/* -----------------------------------------------------------------------
 *  クラス名：  CFrameHolder
 *  機能概要：  関数の1回の実行の間、実行フレームを借りておきます
 *
 *  式の評価中の一時値（CCellTmp）は、関数の中間コード（CCell）ではなく、呼び出しごとの
 *  実行フレームに持ちます。再帰や別のスレッドからの同時実行でも、呼び出しごとに
 *  別のフレームを使うので、呼び出し元が評価の途中で持っている値を壊しません。
 *  例外（memory_error）で抜けるときも返却されます。
 * -----------------------------------------------------------------------
 */
class CFrameHolder
{
private:
	CFunction &func;

public:
	CExecFrame *p;

	CFrameHolder(CFunction &f) : func(f), p(f.AcquireFrame()) { }
	~CFrameHolder() {
		func.ReleaseFrame(p);
	}
};

CFunction::ExecutionResult	CFunction::Execute() {
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	return Execute(arg);
}
CFunction::ExecutionResult	CFunction::Execute(const CValue& arg) {
	CLocalVariable	lvar(*pvm);
	return Execute(arg, lvar);
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Execute
 *  機能概要：  関数を実行します
 *
 *  引数CValue argは必ず配列型です。arrayが空であれば引数の無いコールとなります
 * -----------------------------------------------------------------------
 */
CFunction::ExecutionResult	CFunction::Execute(const CValue &arg, CLocalVariable &lvar)
{
	ExecutionResult result(pvm);
	ExecuteInto(result, arg, lvar);
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecuteToValue
 *  機能概要：  関数を実行し、出力として選ばれた値をanswerに入れます
 *
 *  Execute(arg, lvar).Output()と同じ結果になりますが、出力候補の一覧や
 *  選んだ値をコピーせずに受け渡します。辞書の中のユーザー関数呼び出しは
 *  すべてここを通るので、再帰のたびのコピーが省けます
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecuteToValue(CValue &answer, const CValue &arg, CLocalVariable &lvar)
{
	ExecutionResult result(pvm, ExecutionResult::EMPTY);
	ExecuteInto(result, arg, lvar);
	result.OutputConsume(answer);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecuteInto
 *  機能概要：  関数を実行し、出力候補をresultと入れ替えて返します
 *
 *  resultにはEMPTYかExecutionResult(pvm)で作ったものを渡します。
 *  呼び出し制限に掛かったときは、候補の無いExecutionResult(pvm)と同じ状態になります
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecuteInto(ExecutionResult &result, const CValue &arg, CLocalVariable &lvar)
{
	int exitcode = ST_NOP;

	// _argv/_argc/_FUNC_NAME_を作成
	// （名前は呼び出しのたびに作らず使い回す。_argvは配列の中身を共有するだけでコピーしない）
	lvar.SetValue(ArgvName(), arg);
	lvar.SetValue(ArgcName(), CValue((yaya::int_t)arg.array_size()));
	lvar.SetValue(FuncNameName(), CValue(this->name));

	// 実行
	if (!pvm->call_limit().AddCall(name)) {
		// 滅多に通らない経路なので、フレームを広げないよう別の関数に出している
		CallLimitExceeded(result);
		return;
	}

	// この呼び出し専用の実行フレームを借りる（式の評価中の一時値の置き場。抜けるときに返す）
	CFrameHolder frame(*this);

	Execute_SEHbody(*frame.p, result,lvar, exitcode);
	
	pvm->call_limit().DeleteCall();
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline(off)
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::AcquireFrame / ReleaseFrame
 *  機能概要：  実行フレームの貸し出しと返却です（CFrameHolderが対で呼びます）
 *
 *  使い終わったフレームは、大きな一時値を手放してから関数が1つだけ取っておきます
 *  （次の非再帰の呼び出しで使い回す）。ExecuteIntoのフレームを増やさないよう、ここに分けてあります
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE CExecFrame*	CFunction::AcquireFrame(void)
{
	return frame_cache.take(framesize);
}

YAYA_NOINLINE void	CFunction::ReleaseFrame(CExecFrame *pf)
{
	pf->cleanup();
	frame_cache.give(pf);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::CallLimitExceeded
 *  機能概要：  呼び出しの深さの上限を超えたときの処理です（shiori.OnCallLimitを呼びます）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::CallLimitExceeded(ExecutionResult &result)
{
	// 深さの上限（fncdepth）ではなく、C++のスタックの残りが少ないために止めたのか
	const bool by_stack = pvm->call_limit().StackFailed();

	if (by_stack && pvm->call_limit().InStackHandler()) {
		// shiori.OnCallLimitの実行中にさらにスタックが足りなくなった。ハンドラをまた呼ぶと
		// 際限なく再帰してしまうので、ログだけ出して打ち切る
		pvm->logger().Error(E_E, 97, dicfilename, linecount);
		pvm->call_limit().DeleteCall();

		ExecutionResult	empty(pvm);
		result.swap(empty);
		return;
	}

	CBasisFuncPos shiori_OnCallLimit;
	ptrdiff_t funcpos = shiori_OnCallLimit.Find(*pvm, L"shiori.OnCallLimit");
	size_t lock = pvm->call_limit().temp_unlock();

	if(funcpos >= 0) {
		//_argv[0] = dicname _argv[1] = linenum
		CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(CValue(dicfilename));
		arg.array().emplace_back(CValue((yaya::int_t)linecount));

		if (by_stack)
			pvm->call_limit().SetInStackHandler(true);
		pvm->function_exec().func[funcpos].Execute(arg);
		if (by_stack)
			pvm->call_limit().SetInStackHandler(false);
	}
	else {
		pvm->logger().Error(E_E, 97, dicfilename, linecount);
	}

	pvm->call_limit().reset_lock(lock);
	pvm->call_limit().DeleteCall();

	// 候補の無い結果にする
	ExecutionResult	empty(pvm);
	result.swap(empty);
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline()
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecuteEval
 *  機能概要：  EVALの一時関数を、呼び出し元のローカル変数のもとで実行します
 *
 *  呼び出し元の{}の中にこの関数の本体を書いたのと同じように振る舞います。
 *  呼び出し元のローカル変数は読み書きでき、中で作ったローカル変数は終了時に消えます。
 *  _argv/_argc/_FUNC_NAME_ は呼び出し元のものをそのまま使います
 * -----------------------------------------------------------------------
 */
CFunction::ExecutionResult	CFunction::ExecuteEval(CLocalVariable &lvar)
{
	int exitcode = ST_NOP;

	CFrameHolder frame(*this);

	ExecutionResult result(NULL);
	Execute_SEHbody(*frame.p, result,lvar, exitcode);

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::EvalTempStatement
 *  機能概要：  この関数に属さない一時の文（EVALの式）を実行して、結果を返します
 *
 *  一時の文の項は関数の実行フレームの外にあるので、文の項の数だけの専用のフレームを使います
 * -----------------------------------------------------------------------
 */
CValue	CFunction::EvalTempStatement(CLocalVariable &lvar, CStatement &st)
{
	CExecFrame	tmpframe(st.cell_size());
	return GetFormulaAnswer(tmpframe, lvar, st);
}

void CFunction::Execute_SEHhelper(CExecFrame &fr, CFunction::ExecutionResult& aret, CLocalVariable& lvar, int& exitcode)
{
	SReturnWithParamExpr returnExpr;
	ExecutionInBraceResult r = ExecuteInBrace(fr, 1, lvar, BRACE_DEFAULT, exitcode, NULL, NULL, 0, &returnExpr);
	// 代入だと出力候補が丸ごとコピーされるので、入れ替えて渡す
	aret.swap(r);
}

void CFunction::Execute_SEHbody(CExecFrame &fr, ExecutionResult& retas, CLocalVariable& lvar, int& exitcode)
{
#ifdef _MSC_VER
	__try
#else
	try
#endif
	{
		Execute_SEHhelper(fr, retas, lvar, exitcode);
	}
#ifdef _MSC_VER
	__except (EXCEPTION_EXECUTE_HANDLER)
#else
	catch(...)
#endif
	{
		throw yaya::memory_error();
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecuteInBrace
 *  機能概要：  {}を実行し、結果をひとつ返します
 *  引数　　　  type     この{}の種別。ただし0～の場合はswitch構文の際の候補抽出位置
 *  　　　　　  exitcode 終了コード。ST_NOP/ST_BREAK/ST_RETURN/ST_CONTINUE=通常/break/return/continue
 *  　　　　　  pUpperOutput すぐ外側の{}の出力候補。meltの溶かし先。関数の一番外側の{}ではNULL
 *
 *  "{}"内の各ステートメントを実行します。引数lineで指定される位置から実行を開始し、"}"に突き当たるまで
 *  順次実行していきます。
 *  返値は実行を終了した"}"の位置です。
 * -----------------------------------------------------------------------
 */
CFunction::ExecutionInBraceResult	CFunction::ExecuteInBrace(CExecFrame &fr, size_t line, CLocalVariable &lvar, yaya::int_t type, int &exitcode, CSelecter* pUpperOutput, std::vector<CVecValue>* UpperLvCandidatePool,bool inpool, SReturnWithParamExpr* pReturnExpr)
{
	// 開始時の処理
	lvar.AddDepth();
	CDuplEvInfo* pdupl = statement[line-1].dupl_block.get();
	const bool	 inmutiarea = statement[line-1].ismutiarea;		// pool用

	// 実行
	CSelecter	output(pvm, pdupl, (ptrdiff_t)type);
	bool		exec_end	 = 0;		// この{}の実行を終了するためのフラグ 1で終了
	bool		ifflg		 = 0;		// if-elseif-else制御用。1でそのブロックを処理したことを示す
	bool		notpoolblock = 0;		// pool用
	const bool	ispoolbegin	 = !inpool;	// pool用
	bool		meltblock	 = 0;
	if(pdupl){
		if ( pdupl->GetType() & CHOICETYPE_POOL_FLAG ) {
			if(!inpool) {
				UpperLvCandidatePool = &output.values;
				inpool = 1;
			}
		}
		else {
			notpoolblock = true;
		}
		if ( pdupl->GetType() & CHOICETYPE_MELT_FLAG ) {
			meltblock = 1;
		}
	}
	if(!UpperLvCandidatePool){
		UpperLvCandidatePool = &output.values;
	}

	const bool inpool_to_next = (!inmutiarea ? !notpoolblock : false);

	size_t t_statelenm1 = statelenm1;

	size_t i = 0;
	for(i = line; i < t_statelenm1; i++) {
		CStatement& st = statement[i];

		// ワーカースレッド用の確認。ホストの処理ではpvm->thread_attention()が常に偽で、読むだけ
		if (pvm->thread_attention() && ThreadCancelStep(i, st, exitcode))
			continue;

		switch(st.type) {
		case ST_OPEN: {					// "{"
			i = ExecuteSubBlock(fr, i + 1, lvar, BRACE_DEFAULT, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);
			break;
		}
		case ST_CLOSE:					// "}"　注　関数終端の"}"はここを通らない
			exec_end = 1;
			break;
		case ST_COMBINE:				// "--"
			output.AddArea();
			break;
		case ST_FORMULA_OUT_FORMULA:	// 出力（数式。配列、引数つき関数も含まれる）
			{
				// 評価（再帰の経路）と値の受け渡し（AppendFormulaOutput）を分けて、フレームを増やさない
				const CValue& ans = GetFormulaAnswer(fr, lvar, st);
				AppendFormulaOutput(fr, output, st, ans);
			}
			break;
		case ST_FORMULA_SUBST:			// 代入
			GetFormulaAnswer(fr, lvar, st);
			// 結果は使わないので、代入先の変数と共有している配列・ハッシュを手放す
			// 持ったままだと、次にその変数を書き換えるときに配列が丸ごと複製される
			if ( st.serial_size() ) {
				std_shared_ptr<CValue> &t_ansv = fr.at(st, st.cell()[st.serial().back().tindex]).ansv_shared();
				if ( t_ansv.get() && (t_ansv->IsArray() || t_ansv->IsHash()) ) {
					t_ansv.reset();
				}
			}
			break;
		case ST_IF:						// if
			ifflg = 0;
			if (GetFormulaAnswer(fr, lvar, st).GetTruth()) {
				i = ExecuteSubBlock(fr, i + 2, lvar, BRACE_DEFAULT, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);
				ifflg = 1;
			}
			else
				i = st.jumpto;
			break;
		case ST_ELSEIF:					// elseif
			if (ifflg)
				i = st.jumpto;
			else if (GetFormulaAnswer(fr, lvar, st).GetTruth()) {
				i = ExecuteSubBlock(fr, i + 2, lvar, BRACE_DEFAULT, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);
				ifflg = 1;
			}
			else
				i = st.jumpto;
			break;
		case ST_ELSE:					// else
			if (ifflg)
				i = st.jumpto;
			else {
				i = ExecuteSubBlock(fr, i + 2, lvar, BRACE_DEFAULT, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);
			}
			break;
		case ST_PARALLEL:				// parallel
			AppendParallel(output, GetFormulaAnswer(fr, lvar, st));
			break;
		case ST_VOID:				// void
			{
				//実行だけして捨てる
		        GetFormulaAnswer(fr, lvar, st);
			}
			break;
		case ST_WHILE:					// while
			{
				size_t loop_max = pvm->call_limit().GetMaxLoop();
				size_t loop_cur = 0;
				bool loop_limited = false; //上限に達して打ち切ったときだけ真（自然終了やbreakでは偽）

				while ( true ) {
					if (!GetFormulaAnswer(fr, lvar, st).GetTruth())
						break;
					if ( loop_max != 0 && loop_max <= loop_cur ) {
						loop_limited = true;
						break;
					}
					++loop_cur;
					if (pvm->thread_attention() && ThreadCancelLoop(exitcode))
						break;
					ExecuteSubBlock(fr, i + 2, lvar, BRACE_LOOP, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);

					if (exitcode == ST_BREAK) {
						exitcode = ST_NOP;
						break;
					}
					else if (exitcode == ST_RETURN)
						break;
					else if (exitcode == ST_RETURN_PARAM)
						break;
					else if (exitcode == ST_CONTINUE)
						exitcode = ST_NOP;
				}

				if (loop_limited)
					LoopLimitExceeded(st);

				i = st.jumpto;
				break;
			}
		case ST_FOR:					// for
			{
				GetFormulaAnswer(fr, lvar, st); //for第一パラメータ

				size_t loop_max = pvm->call_limit().GetMaxLoop();
				size_t loop_cur = 0;
				bool loop_limited = false; //上限に達して打ち切ったときだけ真（自然終了やbreakでは偽）

				while ( true ) {
					if (!GetFormulaAnswer(fr, lvar, statement[i + 1]).GetTruth()) //for第二パラメータ
						break;
					if ( loop_max != 0 && loop_max <= loop_cur ) {
						loop_limited = true;
						break;
					}
					++loop_cur;
					if (pvm->thread_attention() && ThreadCancelLoop(exitcode))
						break;
					ExecuteSubBlock(fr, i + 4, lvar, BRACE_LOOP, exitcode, output, UpperLvCandidatePool, inpool_to_next, pReturnExpr);

					if (exitcode == ST_BREAK) {
						exitcode = ST_NOP;
						break;
					}
					else if (exitcode == ST_RETURN)
						break;
					else if (exitcode == ST_RETURN_PARAM)
						break;
					else if (exitcode == ST_CONTINUE)
						exitcode = ST_NOP;

					GetFormulaAnswer(fr, lvar, statement[i + 2]); //for第三パラメータ
				}

				if (loop_limited)
					LoopLimitExceeded(st);

				i = st.jumpto;
				break;
			}
		case ST_SWITCH: {				// switch
				yaya::int_t sw_index = GetFormulaAnswer(fr, lvar, st).GetValueInt();
				if (sw_index < 0)
					sw_index = BRACE_SWITCH_OUT_OF_RANGE;
				i = ExecuteSubBlock(fr, i + 2, lvar, sw_index, exitcode, output, NULL, 0, pReturnExpr);
			}
			break;
		case ST_FOREACH:				// foreach
			Foreach(fr, lvar, output, i, exitcode, UpperLvCandidatePool, inpool_to_next, pReturnExpr);
			i  = st.jumpto;
			break;
		case ST_BREAK:					// break
			exitcode = ST_BREAK;
			break;
		case ST_CONTINUE:				// continue
			exitcode = ST_CONTINUE;
			break;
		case ST_RETURN:					// return
			exitcode = ST_RETURN;
			break;
		case ST_RETURN_PARAM:			// 引数つきreturn
			if (st.cell_size() > 0 && pReturnExpr) {
				const CValue& ans = GetFormulaAnswer(fr, lvar, st);
				TakeFormulaAnswer(fr, pReturnExpr->value, st, ans);
				pReturnExpr->used  = true;
			}
			exitcode = ST_RETURN_PARAM;
			break;
		default:						// 未知のステートメント
			pvm->logger().Error(E_E, 82, dicfilename, st.linecount);
			break;
		};
		if (exec_end)
			break;

		if (exitcode != ST_NOP)
			FeedLineToTail(i);
	}
	#undef POOL_TO_NEXT

	// return式による候補の上書き、melt、poolの処理
	FinishBlock(output, exitcode, pReturnExpr, pUpperOutput, UpperLvCandidatePool, meltblock, inpool, ispoolbegin, notpoolblock, inmutiarea);

	// 終了時の処理
	lvar.DelDepth();

	// outputの候補はコピーせずに結果へ移す（outputは以後使わない）
	return CFunction::ExecutionInBraceResult( output,i );
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline(off)
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::FinishBlock
 *  機能概要：  {}の実行を終えたときの、return式・melt・poolの後処理です
 *
 *  ExecuteInBraceのフレームを小さく保つため（再帰のたびに積まれる）、ここに分けてあります。
 *  内側の{}を呼ぶことはないので、再帰の経路には載りません
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::FinishBlock(CSelecter &output, int exitcode, SReturnWithParamExpr *pReturnExpr, CSelecter *pUpperOutput, std::vector<CVecValue> *UpperLvCandidatePool, bool meltblock, bool inpool, bool ispoolbegin, bool notpoolblock, bool inmutiarea)
{
	// return式による候補の上書き
	const bool returned_with_expr = (exitcode == ST_RETURN_PARAM && pReturnExpr && pReturnExpr->used);
	if (returned_with_expr) {
		output.clear();
		if (pReturnExpr->value.GetType() != F_TAG_NOP)
			output.Append(pReturnExpr->value);
	}

	if (meltblock) {
		if (pUpperOutput) {
			// 選んだ出力が配列ならばらして、すぐ外側の{}の候補に加える
			CValue result;
			output.OutputConsume(result);

			if (result.GetType() == F_TAG_ARRAY) {
				for(size_t j = 0; j < result.array_size(); ++j) {
					pUpperOutput->Append(result.array()[j]);
				}
			}
			else
				pUpperOutput->AppendTake(result);
			output.clear();
		}
		else if (!returned_with_expr) {
			// 関数の一番外側の{}は溶かす先が無いので、配列の候補をばらしてから選ぶ
			output.MeltArray();
		}
	}
	// 候補から出力を選び出す　入れ子の深さが0なら重複回避が働く
	else if (inpool&&!ispoolbegin) {
		std::vector<CValue>& thepool = UpperLvCandidatePool->rbegin()->array;
		std::vector<CValue>& thispool = output.values[0].array;

		if(notpoolblock || inmutiarea)
			thepool.insert(thepool.end(), output.Output());
		else
			thepool.insert(thepool.end(), thispool.begin(), thispool.end());
		output.clear();
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecuteSubBlock
 *  機能概要：  内側の{}を実行し、選んだ出力をoutputの候補へ加えます
 *  返値　　：  実行を終了した"}"の位置
 *
 *  結果の受け渡しにコピーを使わないよう、Outputではなくコピーしない取り出しを使います
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE size_t	CFunction::ExecuteSubBlock(CExecFrame &fr, size_t line, CLocalVariable &lvar, yaya::int_t type, int &exitcode, CSelecter &output, std::vector<CVecValue> *UpperLvCandidatePool, bool inpool, SReturnWithParamExpr *pReturnExpr)
{
	ExecutionInBraceResult info = ExecuteInBrace(fr, line, lvar, type, exitcode, &output, UpperLvCandidatePool, inpool, pReturnExpr);
	CValue	v;
	info.OutputConsume(v);
	output.AppendTake(v);
	return info.linenum;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::SetForeachElement / SetForeachKey
 *  機能概要：  foreachの要素値・キーを代入します（Foreachのフレームに一時変数を作らないため分けてある）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::SetForeachElement(CValue &value, CValue &key, const yaya::string_t &elem, int index)
{
	value = elem;
	key = CValue(static_cast<yaya::int_t>(index));
}

YAYA_NOINLINE void	CFunction::SetForeachKey(CValue &key, int index)
{
	key = CValue(static_cast<yaya::int_t>(index));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::TakeFormulaAnswer
 *  機能概要：  GetFormulaAnswerの結果ansを、コピーせずにdstへ移します（移せないときはコピー）
 *
 *  ansが、この文の最後の演算の項が持つ一時値そのもので、他と共有されていないときだけ
 *  中身を入れ替えて渡します。変数やリテラルの値を指しているときや、emptyvalueのときは
 *  壊してはいけないので、コピーします。この文の一時値は次に実行するときに計算し直されます
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::TakeFormulaAnswer(CExecFrame &fr, CValue &dst, CStatement &st, const CValue &ans)
{
	if (st.serial_size()) {
		std_shared_ptr<CValue> &sp = fr.at(st, st.cell()[st.serial().back().tindex]).ansv_shared();
		if (sp.get() == &ans && sp.use_count() == 1) {
			dst.swap(*sp);
			return;
		}
	}
	dst = ans;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::AppendFormulaOutput
 *  機能概要：  出力の文の結果ansを、出力候補にコピーせずに加えます（加えられないときはコピー）
 *  　　　　　  条件はTakeFormulaAnswerと同じです
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::AppendFormulaOutput(CExecFrame &fr, CSelecter &output, CStatement &st, const CValue &ans)
{
	if (st.serial_size()) {
		std_shared_ptr<CValue> &sp = fr.at(st, st.cell()[st.serial().back().tindex]).ansv_shared();
		if (sp.get() == &ans && sp.use_count() == 1) {
			output.AppendTake(*sp);
			return;
		}
	}
	output.Append(ans);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::AppendParallel
 *  機能概要：  parallel文の値を出力候補に加えます（配列なら要素ごとに）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::AppendParallel(CSelecter &output, const CValue &val)
{
	if (val.GetType() == F_TAG_ARRAY) {
		for(size_t j = 0; j < val.array_size(); ++j)
			output.Append(val.array()[j]);
	}
	else
		output.Append(val);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ThreadCancelStep / ThreadCancelLoop
 *  機能概要：  ワーカースレッドの確認です。ホストのリクエストが待っていれば実行権を譲り、
 *  　　　　　  中断が求められていれば、return文と同じ形でこの関数の実行を打ち切ります
 *
 *  pvm->thread_attention()が真のときだけ呼ばれます（ホストの処理では常に偽）。
 *  ExecuteInBraceのフレームを増やさないよう、ここに分けてあります
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE bool	CFunction::ThreadCancelStep(size_t &line, const CStatement &st, int &exitcode)
{
	// {}の出入りの行や、すでに抜け出している途中では確認しない（FeedLineToTailの前提を保つため）
	if (exitcode != ST_NOP || st.type == ST_OPEN || st.type == ST_CLOSE) {
		return false;
	}
	if (!pvm->thread_attend()) {
		return false;
	}
	exitcode = ST_RETURN;
	FeedLineToTail(line);
	return true;
}

YAYA_NOINLINE bool	CFunction::ThreadCancelLoop(int &exitcode)
{
	if (!pvm->thread_attend()) {
		return false;
	}
	exitcode = ST_RETURN;
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::LoopLimitExceeded
 *  機能概要：  while/forがループ回数の上限で打ち切られたときの処理です（shiori.OnLoopLimitを呼びます）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::LoopLimitExceeded(const CStatement &st)
{
	CBasisFuncPos shiori_OnLoopLimit;
	ptrdiff_t funcpos = shiori_OnLoopLimit.Find(*pvm, L"shiori.OnLoopLimit");

	if (funcpos >= 0) {
		//_argv[0] = dicname _argv[1] = linenum
		CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
		arg.array().emplace_back(CValue(dicfilename));
		arg.array().emplace_back(CValue(static_cast<yaya::int_t>(st.linecount)));

		pvm->function_exec().func[funcpos].Execute(arg);
	}
	else {
		pvm->logger().Error(E_E, 98, dicfilename, st.linecount);
	}
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline()
#endif
/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Foreach
 *  機能概要：  foreach処理を行います
 *  返値　　：  0/1=ループ脱出/ループ実行
 *
 *  実際に送るのは"}"の1つ手前の行の位置です
 * -----------------------------------------------------------------------
 */
void	CFunction::Foreach(CExecFrame &fr, CLocalVariable &lvar, CSelecter &output,size_t line,int &exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool, SReturnWithParamExpr* pReturnExpr)
{
	CStatement &st0 = statement[line];
	CStatement &st1 = statement[line + 1];

	// 代入値を求める
	// 注意：foreach中の副作用を回避するため必ず参照にはしないこと
	const CValue value = GetFormulaAnswer(fr, lvar, st0);

	// 代入値の要素数を求める
	// 簡易配列かつ変数からの取得の場合、その変数に設定されているデリミタを取得する
	bool isPseudoarray = false;

	ptrdiff_t sz;
	std::vector<yaya::string_t>	s_array;
	if (value.IsString()) {
		isPseudoarray = true;

		yaya::string_t delimiter = VAR_DELIMITER;
		if (st0.cell_size() == 1) {
			if (st0.cell()[0].value_GetType() == F_TAG_VARIABLE) {
				delimiter = pvm->variable().GetDelimiter(st0.cell()[0].index);
			}
			else if (st0.cell()[0].value_GetType() == F_TAG_LOCALVARIABLE)
				delimiter = lvar.GetDelimiter(st0.cell()[0].name);
		}
		else {
			CCell *l_cell = &(st0.cell()[st0.serial()[st0.serial_size() - 1].tindex]);
			if (l_cell->value_GetType() == F_TAG_VARIABLE)
				delimiter = pvm->variable().GetDelimiter(l_cell->index);
			else if (l_cell->value_GetType() == F_TAG_LOCALVARIABLE)
				delimiter = lvar.GetDelimiter(l_cell->name);
			else if (l_cell->value_GetType() == F_TAG_ARRAYORDER) {
				l_cell = &(st0.cell()[st0.serial()[st0.serial_size() - 1].tindex - 1]);
				if (l_cell->value_GetType() == F_TAG_VARIABLE)
					delimiter = pvm->variable().GetDelimiter(l_cell->index);
				else if (l_cell->value_GetType() == F_TAG_LOCALVARIABLE)
					delimiter = lvar.GetDelimiter(l_cell->name);
			}
		}
		sz = SplitToMultiString(value.GetValueString(), &s_array, delimiter);
	}
	else if (value.IsArray()) {
		sz = value.array_size();
	}
    else if (value.IsHash()) {
        sz = value.hash_size();
	}
	else {
		sz = -1;
	}

	CValue	t_value;
	CValue	t_key;
	int fromtype = value.GetType();

	// foreach 配列; _v または foreach 配列; _k, _v
	// _k にはハッシュならキー、配列/簡易配列なら0からのインデックスが入る
	CCell	*v_cell = &(st1.cell()[0]);
	CCell	*k_cell = NULL;
	if (st1.cell_size() == 3) {
		k_cell = &(st1.cell()[0]);
		v_cell = &(st1.cell()[2]);
	}

	CValueHash::const_iterator hash_iterator = (fromtype == F_TAG_HASH) ? value.hash().begin() : CValueHash::const_iterator();
	
	for(int foreachcount = 0; foreachcount < sz; ++foreachcount ) {
		// 代入する要素値を取得
		// （CValueの一時変数がこのフレームに確保されないよう、変換を伴う代入は補助関数で行う）
		if (isPseudoarray) {
			SetForeachElement(t_value, t_key, s_array[foreachcount], foreachcount);
		}
		else {// F_TAG_ARRAY
			if ( fromtype == F_TAG_ARRAY ) {
				t_value = value.array()[foreachcount];
				SetForeachKey(t_key, foreachcount);
			}
			else if ( fromtype == F_TAG_HASH ) {
				t_value = hash_iterator->second;
				t_key = hash_iterator->first;
				hash_iterator++;
			}
		}

		// 代入
		bool substerr = false;
		for(int n = 0; n < 2; ++n) {
			CCell *s_cell = n ? v_cell : k_cell;
			if ( s_cell == NULL ) {
				continue;
			}
			const CValue &subst_value = n ? t_value : t_key;
			int type = s_cell->value_GetType();
			if ( type == F_TAG_VARIABLE ) {
				pvm->variable().SetValue(s_cell->index, subst_value);
			}
			else if ( type == F_TAG_LOCALVARIABLE ) {
				lvar.SetValue(s_cell->name, subst_value);
			}
			else {
				substerr = true;
				break;
			}
		}
		if ( substerr ) {
			pvm->logger().Error(E_E, 28, dicfilename, st1.linecount);
			break;
		}

		ExecuteSubBlock(fr, line + 3, lvar, BRACE_LOOP, exitcode, output, UpperLvCandidatePool, inpool, pReturnExpr);

		if (exitcode == ST_BREAK) {
			exitcode = ST_NOP;
			break;
		}
		else if (exitcode == ST_RETURN)
			break;
		else if (exitcode == ST_RETURN_PARAM)
			break;
		else if (exitcode == ST_CONTINUE)
			exitcode = ST_NOP;
	}
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline(off)
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ApplyBinaryOp
 *  機能概要：  二項演算を行い、結果をo_cellに入れます
 *
 *  GetFormulaAnswerから分けてある理由は、そこで値の一時変数が演算の数だけ確保されて
 *  フレームが肥大化し、再帰のたびに積まれるスタックを圧迫するためです
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::ApplyBinaryOp(CExecFrame &fr, CCell &o_cell, int op, const CValue &lv, const CValue &rv, const CStatement &st)
{
	switch(op) {
	case F_TAG_PLUS:
		fr.at(st, o_cell).ansv() = lv + rv;
		break;
	case F_TAG_MINUS:
		WarnHashCalc(lv, rv, L"-", st);
		fr.at(st, o_cell).ansv() = lv - rv;
		break;
	case F_TAG_MUL:
		WarnHashCalc(lv, rv, L"*", st);
		fr.at(st, o_cell).ansv() = lv * rv;
		break;
	case F_TAG_DIV:
		WarnHashCalc(lv, rv, L"/", st);
		fr.at(st, o_cell).ansv() = lv / rv;
		break;
	case F_TAG_SURP:
		WarnHashCalc(lv, rv, L"%", st);
		fr.at(st, o_cell).ansv() = lv % rv;
		break;
	case F_TAG_IFEQUAL:
		fr.at(st, o_cell).ansv() = lv == rv;
		break;
	case F_TAG_IFDIFFER:
		fr.at(st, o_cell).ansv() = lv != rv;
		break;
	case F_TAG_IFGTEQUAL:
		fr.at(st, o_cell).ansv() = lv >= rv;
		break;
	case F_TAG_IFLTEQUAL:
		fr.at(st, o_cell).ansv() = lv <= rv;
		break;
	case F_TAG_IFGT:
		fr.at(st, o_cell).ansv() = lv > rv;
		break;
	case F_TAG_IFLT:
		fr.at(st, o_cell).ansv() = lv < rv;
		break;
	case F_TAG_IFIN:
		fr.at(st, o_cell).ansv().SetType(F_TAG_INT);
		fr.at(st, o_cell).ansv().i_value = _in_(lv, rv);
		break;
	case F_TAG_IFNOTIN:
		fr.at(st, o_cell).ansv().SetType(F_TAG_INT);
		fr.at(st, o_cell).ansv().i_value = not_in_(lv, rv);
		break;
	default:
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::SetIntAnswer
 *  機能概要：  項の演算結果に整数を入れます（論理演算用。GetFormulaAnswerのフレームを広げないため分けてある）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::SetIntAnswer(CExecFrame &fr, const CStatement &st, CCell &o_cell, int value)
{
	fr.at(st, o_cell).ansv() = CValue(value);
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline()
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetFormulaAnswer
 *  機能概要：  数式を演算して結果を返します
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetFormulaAnswer(CExecFrame &fr, CLocalVariable &lvar, CStatement &st)
{
	size_t	o_index = 0;
	CValue	lv_hold;	// 右辺で変数が消されても左辺が残るように、必要なときだけ左辺を複製しておく

	if ( st.serial_size() ) { //高速化用
		for (std::vector<CSerial>::iterator it = st.serial().begin(); it != st.serial().end(); it++) {
			o_index = it->tindex;
			CCell	&o_cell = st.cell()[o_index];
			if (o_cell.value_GetType() >= F_TAG_ORIGIN_VALUE) {
				fr.at(st, o_cell).ansv() = GetValueRefForCalc(fr, o_cell, st, lvar);
				break;
			}

			CCell	*s_cell = &(st.cell()[it->index[0]]);
			CCell	*d_cell = NULL;
			if ( it->index.size() >= 2 ) {
				d_cell = &(st.cell()[it->index[1]]);
			}

			switch(o_cell.value_GetType()) {
			case F_TAG_COMMA:
				{
					std_shared_ptr<CValue> tmp_ansv = fr.at(st, o_cell).ansv_shared_create();
					if (Comma(fr, *tmp_ansv.get(), it->index, st, lvar)) {
						pvm->logger().Error(E_E, 33, L",", dicfilename, st.linecount);
					}
					fr.at(st, o_cell).ansv_shared() = tmp_ansv;
				}
				break;
			case F_TAG_EQUAL:
			case F_TAG_EQUAL_D:
			case F_TAG_PLUSEQUAL:
			case F_TAG_PLUSEQUAL_D:
			case F_TAG_MINUSEQUAL:
			case F_TAG_MINUSEQUAL_D:
			case F_TAG_MULEQUAL:
			case F_TAG_MULEQUAL_D:
			case F_TAG_DIVEQUAL:
			case F_TAG_DIVEQUAL_D:
			case F_TAG_SURPEQUAL:
			case F_TAG_SURPEQUAL_D:
			case F_TAG_COMMAEQUAL:
				{
					// 代入文の最後の代入は結果を使わないので、値を複製しない
					bool need_answer = (st.type != ST_FORMULA_SUBST) || (it + 1 != st.serial().end());
					std_shared_ptr<CValue> tmp_ansv = fr.at(st, o_cell).ansv_shared_create();
					if (Subst(fr, o_cell.value_GetType(), *tmp_ansv.get(), it->index, st, lvar, need_answer)) {
						pvm->logger().Error(E_E, 33, L"=", dicfilename, st.linecount);
					}
					fr.at(st, o_cell).ansv_shared() = tmp_ansv;
				}
				break;
			case F_TAG_PLUS:
			case F_TAG_MINUS:
			case F_TAG_MUL:
			case F_TAG_DIV:
			case F_TAG_SURP:
			case F_TAG_IFEQUAL:
			case F_TAG_IFDIFFER:
			case F_TAG_IFGTEQUAL:
			case F_TAG_IFLTEQUAL:
			case F_TAG_IFGT:
			case F_TAG_IFLT:
			case F_TAG_IFIN:
			case F_TAG_IFNOTIN:
				{
					// 演算順序を必ず守るため、一旦左辺の結果を取ってから右辺と演算する
					// これを怠ると右辺から先に計算する
					// 演算そのものはApplyBinaryOpで行う（演算ごとに値の一時変数がこのフレームに
					// 確保されて、再帰のたびに積まれるフレームが大きくなるのを避けるため）

					const CValue& lv = GetLeftValueRef(fr, *s_cell, d_cell, lv_hold, st, lvar);
					const CValue& rv = GetValueRefForCalc(fr, *d_cell, st, lvar);
					ApplyBinaryOp(fr, o_cell, o_cell.value_GetType(), lv, rv, st);
					break;
				}
			case F_TAG_OR:
				{
					const CValue& lv = GetValueRefForCalc(fr, *s_cell, st, lvar);
					const bool r = lv.GetTruth() || GetValueRefForCalc(fr, *d_cell, st, lvar).GetTruth();
					SetIntAnswer(fr, st, o_cell, r ? 1 : 0);
					break;
				}
			case F_TAG_AND:
				{
					const CValue& lv = GetValueRefForCalc(fr, *s_cell, st, lvar);
					const bool r = lv.GetTruth() && GetValueRefForCalc(fr, *d_cell, st, lvar).GetTruth();
					SetIntAnswer(fr, st, o_cell, r ? 1 : 0);
					break;
				}
			case F_TAG_FUNCPARAM:
				{
					std_shared_ptr<CValue> tmp_ansv = fr.at(st, o_cell).ansv_shared_create();
					if (ExecFunctionWithArgs(fr, *tmp_ansv.get(), it->index, st, lvar)) {
						pvm->logger().Error(E_E, 33, st.cell()[it->index[0]].name, dicfilename, st.linecount);
					}
					fr.at(st, o_cell).ansv_shared() = tmp_ansv;
				}
				break;
			case F_TAG_SYSFUNCPARAM:
				if (ExecSystemFunctionWithArgs(fr, o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, CSystemFunction::GetNameFromIndex(st.cell()[it->index[0]].index), dicfilename, st.linecount);
				break;
			case F_TAG_ARRAYORDER:
				if (Array(fr, o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, L",", dicfilename, st.linecount);
				break;
			case F_TAG_FEEDBACK:
				if (Feedback(fr, o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, L"&", dicfilename, st.linecount);
				break;
			case F_TAG_EXC:
				fr.at(st, o_cell).ansv().SetType(F_TAG_INT);
				fr.at(st, o_cell).ansv().i_value = !GetValueRefForCalc(fr, *d_cell, st, lvar).GetTruth();
				break;
			default:
				pvm->logger().Error(E_E, 34, dicfilename, st.linecount);
				return emptyvalue;
			};
		}
	}

	return fr.at(st, st.cell()[o_index]).ansv();
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline(off)
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecSystemFunctionNoArg
 *  機能概要：  引数なしのシステム関数を実行し、結果を項のansvに入れます
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::ExecSystemFunctionNoArg(CExecFrame &fr, CCell &cell, CStatement &st, CLocalVariable &lvar)
{
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	std::vector<CCell *> pcellarg; //dummy
	std::vector<CValue> pvaluearg; //dummy
	fr.at(st, cell).ansv() =  pvm->sysfunction().Execute(cell.index, arg, pcellarg, pvaluearg, lvar, st.linecount, this);
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline()
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetValueRefForCalc
 *  機能概要：  与えられた項に対応する値へのポインタを取得します
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetValueRefForCalc(CExecFrame &fr, CCell &cell, CStatement &st, CLocalVariable &lvar)
{
	// 即値はv、関数/変数/演算子項ならansvから取得　関数/変数の場合その値や実行結果が取得される

	// %[n]処理
	if (cell.value_GetType() == F_TAG_SYSFUNCPARAM) {
		if ( cell.index == CSystemFunction::HistoryIndex() ) {
			ExecHistoryP2(fr, cell, st);
		}
	}

	// 演算が完了している（はずの）項ならそれを返す
	if (cell.value_GetType() < F_TAG_ORIGIN_VALUE)
		return fr.at(st, cell).ansv();

	// 即値ならそれをそのまま返す
	if (cell.value_GetType() <= F_TAG_STRING)
		return cell.value();

	// 関数なら実行して結果を、変数ならその内容を返す
	switch(cell.value_GetType()) {
	case F_TAG_STRING_EMBED:
		SolveEmbedCell(fr, cell, st, lvar);
		return fr.at(st, cell).ansv();
	case F_TAG_SYSFUNC:
		// 引数なしのシステム関数（フレームを小さく保つため別の関数に出している）
		ExecSystemFunctionNoArg(fr, cell, st, lvar);
		return fr.at(st, cell).ansv();
	case F_TAG_USERFUNC: {
		CFunction	*pfunc = GetUserFunction(cell);
		if (pfunc == NULL) {
			pvm->logger().Error(E_E, 71, cell.name, dicfilename, st.linecount);
			return emptyvalue;
		}
		CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
		CLocalVariable	t_lvar(*pvm);
		pfunc->ExecuteToValue(fr.at(st, cell).ansv(), arg, t_lvar);
		return fr.at(st, cell).ansv();
	}
	case F_TAG_VARIABLE:
		if (pvm->variable().GetPtr(cell.index) != NULL) {
			return pvm->variable().GetPtr(cell.index)->call_watcher(*pvm,fr.at(st, cell).ansv());
		}
		else {
			return emptyvalue;
		}
	case F_TAG_LOCALVARIABLE:
		if (lvar.GetPtr(cell.name) != NULL) {
			return lvar.GetPtr(cell.name)->call_watcher(*pvm,fr.at(st, cell).ansv());
		}
		else {
			return emptyvalue;
		}
	default:
		pvm->logger().Error(E_E, 16, dicfilename, st.linecount);
		return emptyvalue;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetLeftValueRef
 *  機能概要：  二項演算の左辺の値を取得します
 *
 *  左辺が変数で、右辺がこれから実行される関数の項（引数なしの関数）のとき、
 *  右辺の関数がERASEVARで左辺の変数を消すと、参照が解放済みの値を指してしまいます。
 *  その場合だけ左辺の値をholdに複製して返します。
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetLeftValueRef(CExecFrame &fr, CCell &s_cell, const CCell *d_cell, CValue &hold, CStatement &st, CLocalVariable &lvar)
{
	const CValue &lv = GetValueRefForCalc(fr, s_cell, st, lvar);

	if (d_cell) {
		int	stype = s_cell.value_GetType();
		int	dtype = d_cell->value_GetType();
		if ((stype == F_TAG_VARIABLE || stype == F_TAG_LOCALVARIABLE) && (dtype == F_TAG_USERFUNC || dtype == F_TAG_SYSFUNC)) {
			hold = lv;
			return hold;
		}
	}

	return lv;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ReindexUserFunctions
 *  機能概要：  F_TAG_USERFUNCのindexを再設定します
 * -----------------------------------------------------------------------
 */
int CFunction::ReindexUserFunctions(void)
{
	int error = 0;

	for ( size_t i = 0 ; i < statement.size() ; ++i ) {
		CStatement &st = statement[i];

		for ( size_t j = 0 ; j < st.cell_size() ; ++j ) {
			CCell &cl = st.cell()[j];

			if ( cl.value_GetType() == F_TAG_USERFUNC ) {
				ptrdiff_t index = pvm->function_parse().GetFunctionIndexFromName(cl.name);
				if ( index < 0 ) {
					pvm->logger().Error(E_E, 71, dicfilename, st.linecount);
					error += 1;
				}
				else {
					cl.index = index;
				}
			}
		}
	}

	return error;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::SolveEmbedCell
 *  機能概要：  %埋め込み付文字列項の値を求めます
 * -----------------------------------------------------------------------
 */
void	CFunction::SolveEmbedCell(CExecFrame &fr, CCell &cell, CStatement &st, CLocalVariable &lvar)
{
	// 分割する位置を求める
	int	solve_src;	// 種別 0/1/2/3=ローカル変数/変数/関数/システム関数
	size_t	max_len = 0;	// 最長一致検索用

	if (cell.index >= 0) {
		// embed.lazy,offで読み込み時に確定済み（CParser0::FixEmbedName）
		solve_src = (int)cell.depth;
		max_len   = (size_t)cell.index;
	}
	else if (cell.value_const().s_value[0] == L'_') {
		// ローカル変数
		solve_src = 0;
		max_len   = lvar.GetMacthedLongestNameLength(cell.value_const().s_value);
	}
	else {
		// 変数
		solve_src = 1;
		max_len   = pvm->variable().GetMacthedLongestNameLength(cell.value_const().s_value);
		// 関数
		size_t	t_len = 0;
		for(std::vector<CFunction>::iterator it = pvm->function_exec().func.begin(); it != pvm->function_exec().func.end(); it++)
			if (!it->name.compare(0,it->namelen,cell.value_const().s_value,0,it->namelen))
				if (t_len < it->namelen)
					t_len = it->namelen;
		if (t_len > max_len) {
			solve_src = 2;
			max_len   = t_len;
		}
		// システム関数
		if ( max_len < (size_t)CSystemFunction::GetMaxNameLength() ) {
			t_len = CSystemFunction::FindIndexLongestMatch(cell.value_const().s_value,max_len);
			if (t_len > max_len) {
				solve_src = 3;
				max_len   = t_len;
			}
		}
	}
	// 存在しなければ全体が文字列ということになる
	if (!max_len) {
		fr.at(st, cell).ansv()     = L"%" + cell.value_const().s_value;
		fr.at(st, cell).emb_ansv() = L"%" + cell.value_const().s_value;
		return;
	}

	// 関数/システム関数の場合は引数部分を探す
	size_t	len = cell.value_const().s_value.size();
	if (solve_src >= 2) {
		size_t	depth = 1;
		size_t i = 0;
		for(i = max_len + 1; i < len; i++) {
			depth += ((cell.value_const().s_value[i] == L'(') - (cell.value_const().s_value[i] == L')'));
			if (!depth)
				break;
		}
		if (!depth)
			max_len = i + 1;
	}

	// 配列部分を探す
	size_t	depth = 1;
	size_t i = 0;
	for(i = max_len + 1; i < len; i++) {
		if (!depth && cell.value_const().s_value[i] != L'[')
			break;
		depth += ((cell.value_const().s_value[i] == L'[') - (cell.value_const().s_value[i] == L']'));
	}
	if (!depth)
		max_len = i;

	// 埋め込まれた要素とそれ以降の文字列に分割する
	//yaya::string_t	s_value(cell.value_const().s_value.substr(0, max_len));
	//yaya::string_t	d_value(cell.value_const().s_value.substr(max_len, len - max_len));
	yaya::string_t::const_iterator it_split = cell.value_const().s_value.begin() + max_len;
	yaya::string_t s_value(cell.value_const().s_value.begin(),it_split);
	yaya::string_t d_value(it_split,cell.value_const().s_value.end());

	// 埋め込まれた要素を数式に変換する　失敗なら全体が文字列
	CStatement	t_state(ST_FORMULA, st.linecount);
	if (pvm->parser0().ParseEmbedString(s_value, t_state, dicfilename, st.linecount)) {
		fr.at(st, cell).ansv()       = L"%" + cell.value_const().s_value;
		fr.at(st, cell).emb_ansv() = L"%" + cell.value_const().s_value;
		return;
	}

	// 埋め込み要素の値を取得して応答文字列を作成
	// （t_stateはこの関数の文ではないので、文の項の数だけの専用のフレームで評価する）
	CExecFrame	t_frame(t_state.cell_size());
	yaya::string_t	result = GetFormulaAnswer(t_frame, lvar, t_state).GetValueString();
	fr.at(st, cell).emb_ansv()  = result;
	fr.at(st, cell).ansv()      = result + d_value;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Comma
 *  機能概要：  ,演算子を処理します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::Comma(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	// 結合して配列値を作成
	CValueArray	t_array;

	for(std::vector<size_t>::iterator it = sid.begin(); it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(fr, st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			t_array.insert(t_array.end(), addv.array().begin(), addv.array().end());
		}
		else {
			t_array.emplace_back(CValue(addv));
		}
	}

	answer.SubstToArray(t_array);
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::CommaAdd
 *  機能概要：  ,=演算子を処理します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::CommaAdd(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	if ( answer.GetType() != F_TAG_ARRAY ) {
		CValue st(answer);
		answer.SetType(F_TAG_ARRAY);
		answer.array().emplace_back(st);
	}

	std::vector<size_t>::iterator it = sid.begin();
	it++; //最初＝左辺は代入先なので飛ばす

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(fr, st.cell()[*it], st, lvar);

		// 右辺の関数が左辺の変数を代入し直すことがあるので、右辺を評価したあとに配列を取り直す
		if ( answer.GetType() != F_TAG_ARRAY ) {
			CValue t_first(answer);
			answer.SetType(F_TAG_ARRAY);
			answer.array().emplace_back(t_first);
		}
		CValueArray &t_array = answer.array();

		if (&addv == &answer) {
			// _a ,= _a　自分自身を追加する場合は、先に複製しておく
			CValueArray t_self(t_array);
			t_array.insert(t_array.end(), t_self.begin(), t_self.end());
		}
		else if (addv.GetType() == F_TAG_ARRAY) {
			t_array.insert(t_array.end(), addv.array().begin(), addv.array().end());
		}
		else {
			t_array.emplace_back(CValue(addv));
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetSubstVariable
 *  機能概要：  代入先の変数（グローバル変数/ローカル変数の項）を取得します
 *
 *  変数はvectorに入っているので、関数を実行して変数が増えると得られたポインタは無効になります。
 *  関数を実行したあとは引き直してください。
 * -----------------------------------------------------------------------
 */
CVariable*	CFunction::GetSubstVariable(const CCell &vcell, CLocalVariable &lvar)
{
	if ( vcell.value_GetType() == F_TAG_VARIABLE ) {
		return pvm->variable().GetPtr(vcell.index);
	}
	else {
		return lvar.GetPtr(vcell.name);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Subst
 *  機能概要：  代入演算子を処理します
 *
 *  need_answerがfalseなら、変数への代入ではanswerに結果を入れません
 *  （入れると文字列や配列を変数と共有し、次の+=などで毎回全体が複製されて2乗の時間がかかる）
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::Subst(CExecFrame &fr, int type, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar, bool need_answer)
{
	CCell	*sid_0_cell = &(st.cell()[sid[0]]);
	CCell	*sid_1_cell = &(st.cell()[sid[1]]);

	int sid_0_cell_type = sid_0_cell->value_GetType();

	//既存変数への代入の場合だけは特殊扱いする
	if ( sid_0_cell_type == F_TAG_VARIABLE || sid_0_cell_type == F_TAG_LOCALVARIABLE ) {
		CVariable* pSubstTo = GetSubstVariable(*sid_0_cell, lvar);

		CValue varback;

		if ( pSubstTo ) {
			// 変更前の値はsetterにしか使わない
			// 常に取っておくと配列を共有するため、書き換えのたびに配列が丸ごと複製される
			if ( pSubstTo->setter.size() ) {
				varback = pSubstTo->value();
			}

			// 右辺の関数やEVALの中で変数が増えるとpSubstToは無効になるので、値だけを持っておき、
			// 代入が終わったら変数を引き直す
			pSubstTo->value();
			std_shared_ptr<CValue> substToHolder = pSubstTo->value_shared();
			pSubstTo = NULL;
			CValue &substTo = *substToHolder;

			answer.array_clear();
			answer.hash_clear();

			switch(type) {
			case F_TAG_EQUAL:
			case F_TAG_EQUAL_D:
				substTo = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
				break;
			case F_TAG_PLUSEQUAL:
			case F_TAG_PLUSEQUAL_D:
				substTo += GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
				break;
			case F_TAG_MINUSEQUAL:
			case F_TAG_MINUSEQUAL_D:
				{
					const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
					WarnHashCalc(substTo, rv, L"-=", st);
					substTo -= rv;
				}
				break;
			case F_TAG_MULEQUAL:
			case F_TAG_MULEQUAL_D:
				{
					const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
					WarnHashCalc(substTo, rv, L"*=", st);
					substTo *= rv;
				}
				break;
			case F_TAG_DIVEQUAL:
			case F_TAG_DIVEQUAL_D:
				{
					const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
					WarnHashCalc(substTo, rv, L"/=", st);
					substTo /= rv;
				}
				break;
			case F_TAG_SURPEQUAL:
			case F_TAG_SURPEQUAL_D:
				{
					const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
					WarnHashCalc(substTo, rv, L"%=", st);
					substTo %= rv;
				}
				break;

				//カンマ特殊処理
			case F_TAG_COMMAEQUAL:
				if (CommaAdd(fr, substTo, sid, st, lvar)) {
					return 1;
				}
				break;
			default:
				return 1;
			};

			// **HACK** constにしてarrayの場合answerと強制共有
			// 後で代入演算がもしあった時に配列の時の代入コストを省略できる
			if ( need_answer ) {
				answer = const_cast<const CValue&>(substTo);
			}
			else {
				answer = CValue();
			}

			// グローバル変数の場合、削除済みの場合があるのでここで再Enable
			if ( sid_0_cell_type == F_TAG_VARIABLE ) {
				pvm->variable().EnableValue(sid_0_cell->index);
			}

			pSubstTo = GetSubstVariable(*sid_0_cell, lvar);
			if ( ! pSubstTo ) {
				return 0;
			}
			// 右辺の中でERASEVARされていても、代入した値を変数に戻す
			if ( pSubstTo->value_shared() != substToHolder ) {
				pSubstTo->value_shared() = substToHolder;
			}

			CValue setter_result;
			if ( pSubstTo->call_setter(*pvm, varback, setter_result) ) {
				// setterの中で変数が増えることがあるので、もう一度引き直す
				pSubstTo = GetSubstVariable(*sid_0_cell, lvar);
				if ( pSubstTo ) {
					pSubstTo->value() = setter_result;
				}
			}

			return 0;
		}
	}

	// 代入元の値を取得　演算子つきなら演算も行う
	switch(type) {
	case F_TAG_EQUAL:
	case F_TAG_EQUAL_D:
		answer = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
		break;
	case F_TAG_PLUSEQUAL:
	case F_TAG_PLUSEQUAL_D:
		{
			// 演算順序を必ず守るため、一旦左辺の結果を取ってから右辺と演算する
			// これを怠ると右辺から先に計算する
			// 以降同じ
			const CValue &lv = GetCompoundSubstLeftValue(fr, sid, st, lvar);
			answer = lv + GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_MINUSEQUAL:
	case F_TAG_MINUSEQUAL_D:
		{
			const CValue &lv = GetCompoundSubstLeftValue(fr, sid, st, lvar);
			const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
			WarnHashCalc(lv, rv, L"-=", st);
			answer = lv - rv;
			break;
		}
	case F_TAG_MULEQUAL:
	case F_TAG_MULEQUAL_D:
		{
			const CValue &lv = GetCompoundSubstLeftValue(fr, sid, st, lvar);
			const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
			WarnHashCalc(lv, rv, L"*=", st);
			answer = lv * rv;
			break;
		}
	case F_TAG_DIVEQUAL:
	case F_TAG_DIVEQUAL_D:
		{
			const CValue &lv = GetCompoundSubstLeftValue(fr, sid, st, lvar);
			const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
			WarnHashCalc(lv, rv, L"/=", st);
			answer = lv / rv;
			break;
		}
	case F_TAG_SURPEQUAL:
	case F_TAG_SURPEQUAL_D:
		{
			const CValue &lv = GetCompoundSubstLeftValue(fr, sid, st, lvar);
			const CValue &rv = GetValueRefForCalc(fr, *sid_1_cell, st, lvar);
			WarnHashCalc(lv, rv, L"%=", st);
			answer = lv % rv;
			break;
		}
	case F_TAG_COMMAEQUAL:
		if (Comma(fr, answer, sid, st, lvar))
			return 1;
		break;
	default:
		return 1;
	};

	// 代入を実行
 	// 配列要素への代入は操作が複雑なので、さらに他の関数へ処理を渡す
	switch(sid_0_cell->value_GetType()) {
	case F_TAG_VARIABLE:
		pvm->variable().SetValue(sid_0_cell->index, answer);
		return 0;
	case F_TAG_LOCALVARIABLE:
		lvar.SetValue(sid_0_cell->name, answer);
		return 0;
	case F_TAG_ARRAYORDER: {
			if (sid[0] > 0)
				return SubstToArray(fr, st.cell()[sid[0] - 1], *sid_0_cell, answer, st, lvar, sid[0]);
			else
				return 1;
		}
	default:
 		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetCompoundSubstLeftValue
 *  機能概要：  複合代入（+= -= *= /= %=）の左辺の値を取得します
 *
 *  左辺が汎用配列の範囲外の要素やハッシュの無いキーの場合は、未定義の変数と同じくVOIDを返します。
 *  要素があるかどうかは、SubstToArrayが書き込む先と同じ値で調べます（watcherは呼びません）。
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetCompoundSubstLeftValue(CExecFrame &fr, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	&ocell = st.cell()[sid[0]];

	if (ocell.value_GetType() == F_TAG_ARRAYORDER && sid[0] > 0) {
		CCell	&vcell = st.cell()[sid[0] - 1];

		// 変数はwatcherを呼ばずに値を直接見る
		const CValue	*pcontainer = NULL;
		if (vcell.value_GetType() == F_TAG_HOOKBRACKETOUT) {
			ptrdiff_t upper = FindUpperArrayOrder(st, sid[0]);
			if (upper >= 1)
				pcontainer = &(fr.at(st, st.cell()[upper]).ansv());
		}
		else if (vcell.value_GetType() == F_TAG_VARIABLE) {
			CVariable *pvar = pvm->variable().GetPtr(vcell.index);
			pcontainer = pvar ? &(pvar->value_const()) : &emptyvalue;
		}
		else if (vcell.value_GetType() == F_TAG_LOCALVARIABLE) {
			CVariable *pvar = lvar.GetPtr(vcell.name);
			pcontainer = pvar ? &(pvar->value_const()) : &emptyvalue;
		}

		if (pcontainer) {
			CValue	t_order;
			EncodeArrayOrder(vcell, fr.at(st, ocell).order(), lvar, t_order);
			if (pcontainer->IsMissingElement(t_order))
				return emptyvalue;
		}
	}

	return GetValueRefForCalc(fr, ocell, st, lvar);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::SubstToArray
 *  機能概要：  配列要素への代入を処理します
 *
 *  oindexはocell（配列序数演算子）のセル位置です。
 *  多次元配列（a[x][y] = ...）の場合、vcellは手前の次元の"]"になるので、
 *  手前の次元の配列序数演算子が持っている値を更新し、さらに手前の次元へ代入し直します。
 *  spreadは代入する値が配列のときに展開して差し込むかどうかです（CValue::SetArrayValue）。
 *  手前の次元へ代入し直すときは、展開すると入れ子が崩れるのでfalseにします。
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::SubstToArray(CExecFrame &fr, CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar, size_t oindex, bool spread)
{
	// 序数を取得
	CValue	t_order;
	EncodeArrayOrder(vcell, fr.at(st, ocell).order(), lvar, t_order);

	if (t_order.GetType() == F_TAG_UNKNOWN)
		return 1;

	// 多次元配列なら手前の次元の配列序数演算子を探す
	ptrdiff_t upper = -1;
	if (vcell.value_GetType() == F_TAG_HOOKBRACKETOUT) {
		upper = FindUpperArrayOrder(st, oindex);
		if (upper < 1)
			return 1;
	}
	else {
		// 1次元で変数が存在しwatcherも無いなら、変数の値を直接書き換える
		// 値をコピーしてから書き換えると配列を共有するため、そのたびに配列が丸ごと複製される
		CVariable *pvar = NULL;
		if (vcell.value_GetType() == F_TAG_VARIABLE) {
			pvar = pvm->variable().GetPtr(vcell.index);
		}
		else if (vcell.value_GetType() == F_TAG_LOCALVARIABLE) {
			pvar = lvar.GetPtr(vcell.name);
		}
		if (pvar != NULL && pvar->watcher.empty()) {
			pvar->value().SetArrayValue(t_order, answer, spread);
			if (vcell.value_GetType() == F_TAG_VARIABLE) {
				pvm->variable().EnableValue(vcell.index);
			}
			return 0;
		}
	}

	// 値を取得
	CValue	value = (upper >= 0) ? fr.at(st, st.cell()[upper]).ansv() : GetValueRefForCalc(fr, vcell, st, lvar);

	// 更新
	value.SetArrayValue(t_order, answer, spread);

	// 代入
	switch(vcell.value_GetType()) {
	case F_TAG_VARIABLE:
		pvm->variable().SetValue(vcell.index, value);
		return 0;
	case F_TAG_LOCALVARIABLE:
		lvar.SetValue(vcell.name, value);
		return 0;
	case F_TAG_HOOKBRACKETOUT:
		return SubstToArray(fr, st.cell()[upper - 1], st.cell()[upper], value, st, lvar, upper, false);
	default:
		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::WarnHashCalc
 *  機能概要：  ハッシュを - * / % の演算に使った場合に警告を出します
 * -----------------------------------------------------------------------
 */
void	CFunction::WarnHashCalc(const CValue &lv, const CValue &rv, const yaya::char_t *op, const CStatement &st)
{
	if (lv.IsHash() || rv.IsHash()) {
		pvm->logger().Error(E_W, 24, op, dicfilename, st.linecount);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::FindUpperArrayOrder
 *  機能概要：  多次元配列（a[x][y]）で、oindexの配列序数演算子の手前の次元の
 *  　　　　　  配列序数演算子のセル位置を返します
 *
 *  返値　　：  セル位置/-1=見つからない
 * -----------------------------------------------------------------------
 */
ptrdiff_t	CFunction::FindUpperArrayOrder(CStatement &st, size_t oindex)
{
	if (oindex < 1 || st.cell()[oindex - 1].value_GetType() != F_TAG_HOOKBRACKETOUT)
		return -1;

	// 手前の"]"に対応する"["を探す
	ptrdiff_t depth = 0;
	ptrdiff_t i = static_cast<ptrdiff_t>(oindex) - 1;
	for( ; i >= 0; i--) {
		int type = st.cell()[i].value_GetType();
		if (F_TAG_ISOUT(type))
			depth++;
		else if (F_TAG_ISIN(type))
			depth--;
		if (!depth)
			break;
	}

	// "["の直前が配列序数演算子であるはず
	if (i < 1 || st.cell()[i].value_GetType() != F_TAG_HOOKBRACKETIN ||
		st.cell()[i - 1].value_GetType() != F_TAG_ARRAYORDER)
		return -1;

	return i - 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::FindFeedbackArrayOrder
 *  機能概要：  &演算子（findexのセル位置）の対象である配列序数演算子のセル位置を返します
 *  　　　　　  多次元配列（&a[x][y]）の場合は最後の次元の配列序数演算子になります
 *
 *  返値　　：  セル位置/-1=見つからない
 * -----------------------------------------------------------------------
 */
ptrdiff_t	CFunction::FindFeedbackArrayOrder(CStatement &st, size_t findex)
{
	for(std::vector<CSerial>::const_iterator it = st.serial().begin(); it != st.serial().end(); ++it) {
		if (it->tindex == findex) {
			if (it->index.size() < 2)
				return -1;
			size_t oindex = it->index[1];
			if (oindex < 1 || st.cell()[oindex].value_GetType() != F_TAG_ARRAYORDER)
				return -1;
			return static_cast<ptrdiff_t>(oindex);
		}
	}
	return -1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::RefreshUpperArrayOrder
 *  機能概要：  多次元配列（a[x][y]）で、oindexの配列序数演算子より手前の次元の
 *  　　　　　  配列序数演算子を根元から順に評価し直し、値を最新にします
 *
 *  &演算子の書き戻しは関数の実行後に行うため、実行前に取得した手前の次元の値を
 *  そのまま使うと、関数内で変更された内容が失われます
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::RefreshUpperArrayOrder(CExecFrame &fr, CStatement &st, size_t oindex, CLocalVariable &lvar)
{
	std::vector<size_t> uppers;
	for(ptrdiff_t upper = FindUpperArrayOrder(st, oindex); upper >= 1; upper = FindUpperArrayOrder(st, upper)) {
		uppers.push_back(static_cast<size_t>(upper));
	}

	for(std::vector<size_t>::reverse_iterator itu = uppers.rbegin(); itu != uppers.rend(); ++itu) {
		std::vector<CSerial>::iterator its = st.serial().begin();
		for( ; its != st.serial().end(); ++its) {
			if (its->tindex == *itu)
				break;
		}
		if (its == st.serial().end())
			return 1;
		if (Array(fr, st.cell()[*itu], its->index, st, lvar))
			return 1;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Array
 *  機能概要：  配列[]演算子を処理します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::Array(CExecFrame &fr, CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	*v_cell = &(st.cell()[sid[0]]);
	CCell	*n_cell = &(st.cell()[sid[1]]);

	// 序数を取得
	fr.at(st, anscell).order() = GetValueRefForCalc(fr, *n_cell, st, lvar);

	CValue	t_order;
	EncodeArrayOrder(*v_cell, fr.at(st, anscell).order(), lvar, t_order);

	if (t_order.GetType() == F_TAG_UNKNOWN) {
		fr.at(st, anscell).ansv().SetType(F_TAG_VOID);
		return 1;
	}

	// 値を取得
	fr.at(st, anscell).ansv() = GetValueRefForCalc(fr, *v_cell, st, lvar)[t_order];

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::_in_
 *  機能概要：  _in_演算子を処理します
 * -----------------------------------------------------------------------
 */
bool CFunction::_in_(const CValue &src, const CValue &dst)
{
	if (src.IsString() && dst.IsString())
		return (dst.s_value.find(src.s_value) != yaya::string_t::npos) ? 1 : 0;

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::not_in_
 *  機能概要：  !_in_演算子を処理します
 * -----------------------------------------------------------------------
 */
bool CFunction::not_in_(const CValue &src, const CValue &dst)
{
	return ! _in_(src,dst);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::GetUserFunction
 *  機能概要：  F_TAG_USERFUNCの項が指す関数を、現在の関数表から取得します
 *
 *  項のindexは解析したときの関数表での位置です。実行中にDICUNLOAD/UNDEFFUNCなどで
 *  関数表が差し替えられると位置がずれるため、名前が一致しなければ名前で引き直します。
 *  位置がずれるのは差し替えたrequestの中（古い関数表を取っておいている間）だけです。
 *
 *  返値　　：  関数　見つからなければNULL
 * -----------------------------------------------------------------------
 */
CFunction*	CFunction::GetUserFunction(const CCell &cell)
{
	CFunctionDef	&def = pvm->function_exec();

	if (!pvm->func_swapped()) {
		return &def.func[size_t(cell.index)];
	}

	if (cell.index >= 0 && size_t(cell.index) < def.func.size() && def.func[size_t(cell.index)].name == cell.name) {
		return &def.func[size_t(cell.index)];
	}

	ptrdiff_t	index = def.GetFunctionIndexFromName(cell.name);
	if (index < 0) {
		return NULL;
	}
	return &def.func[size_t(index)];
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecFunctionWithArgs
 *  機能概要：  引数付きの関数を実行します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::ExecFunctionWithArgs(CExecFrame &fr, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	&fcell = st.cell()[sid[0]];

	// 引数作成
	// 引数の組み立てと&引数の書き戻しは、実行中（再帰のたびに積まれる）のフレームを小さく
	// 保つため別の関数に分けてある
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);	
	std::vector<size_t>	feedback_info;	// &引数があるときだけ使う（BuildFunctionArgsを参照）
	BuildFunctionArgs(fr, arg, feedback_info, sid, st, lvar);

	// 関数を取得　引数の中で関数表が差し替えられることがあるので、引数を作ってから引く
	CFunction	*pfunc = GetUserFunction(fcell);
	if (pfunc == NULL) {
		pvm->logger().Error(E_E, 71, fcell.name, dicfilename, st.linecount);
		answer = CValue();
		return 0;
	}

	// 実行
	CLocalVariable	t_lvar(*pvm);
	pfunc->ExecuteToValue(answer, arg, t_lvar);

	// フィードバック
	if (feedback_info.empty())
		return 0;
	return WriteBackFeedback(fr, t_lvar, feedback_info, sid, st, lvar);
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline(off)
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::BuildFunctionArgs
 *  機能概要：  ユーザー関数に渡す引数（_argvになる配列）を作ります
 *
 *  feedbackには、&引数があるときだけ、各引数について3つずつ値を入れます。
 *  　_argv のどこから(offset)・何個を(width)占めるか（配列引数は展開されるので
 *  　引数の位置とは一致しない）と、配列を渡したか(isarray 1/0)です。
 *  &引数が無いときは空のままです（呼び出しのたびにvectorを確保しないため）
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE void	CFunction::BuildFunctionArgs(CExecFrame &fr, CValue &arg, std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	std::vector<size_t>::iterator it = sid.begin() + 1;
	std::vector<size_t>::size_type sidsize = sid.size();

	bool has_feedback = false;
	for(std::vector<size_t>::iterator fit = it; fit != sid.end(); fit++) {
		if (st.cell()[*fit].value_GetType() == F_TAG_FEEDBACK) {
			has_feedback = true;
			break;
		}
	}
	if (has_feedback) {
		feedback.reserve((sidsize - 1) * 3);
	}
	if (sidsize > 2) {
		arg.array().reserve(sidsize - 1);
	}

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(fr, st.cell()[*it], st, lvar);
		
		const size_t offset = arg.array_size();
		bool isarray = false;
		if (addv.GetType() == F_TAG_ARRAY) {
			if ( sidsize <= 2 ) { //配列1つのみが与えられている->最適化のためスマートポインタ代入のみで済ませる
				arg.array_shared() = addv.array_shared();
			}
			else {
				arg.array().insert(arg.array().end(), addv.array().begin(), addv.array().end());
			}
			isarray = true;
		}
		else {
			arg.array().emplace_back(addv);
		}
		if (has_feedback) {
			feedback.push_back(offset);
			feedback.push_back(arg.array_size() - offset);
			feedback.push_back(isarray ? 1 : 0);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::WriteBackFeedback
 *  機能概要：  &引数の値を、呼ばれた関数が書き換えた_argvから呼び出し元の変数へ書き戻します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
YAYA_NOINLINE char	CFunction::WriteBackFeedback(CExecFrame &fr, CLocalVariable &t_lvar, const std::vector<size_t> &feedback, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	std::vector<size_t>::iterator it;
	const CValue *v_argv = &(t_lvar.GetArgvPtr()->value_const());
	size_t	i = 0;
	int	errcount = 0;
	const size_t	argv_size = v_argv->array_size();

	for(it = sid.begin() + 1; it != sid.end(); it++, i++) {
		if (st.cell()[*it].value_GetType() == F_TAG_FEEDBACK) {
			const size_t	argoffset  = feedback[i * 3];
			const size_t	argwidth   = feedback[i * 3 + 1];
			const bool		argisarray = (feedback[i * 3 + 2] != 0);

			CValue	v_value;
			if (argisarray) {
				// 配列を渡していたときは、展開された範囲を配列に戻して書き戻す
				// 呼ばれた関数が _argv を縮めていたら、残っている分だけを戻す
				size_t	wbeg = std::min(argoffset, argv_size);
				size_t	wend = std::min(argoffset + argwidth, argv_size);
				v_value = CValue(F_TAG_ARRAY, 0/*dmy*/);
				v_value.array().assign(v_argv->array().begin() + wbeg, v_argv->array().begin() + wend);
			}
			else if (argoffset < argv_size) {
				v_value = v_argv->array()[argoffset];
			}
			else {
				// 呼ばれた関数が _argv を縮めていたら、その引数は書き戻さない
				continue;
			}

			if (fr.at(st, st.cell()[*it]).order_const().GetType() != F_TAG_NOP) {
				// 配列要素へは通常の代入と同じく配列序数演算子の位置から書き戻す
				// （多次元配列 &a[x][y] でも手前の次元をたどれるように）
				ptrdiff_t oindex = FindFeedbackArrayOrder(st, *it);
				if (oindex < 1 || RefreshUpperArrayOrder(fr, st, static_cast<size_t>(oindex), lvar))
					errcount++;
				else
					errcount += SubstToArray(fr, st.cell()[oindex - 1], st.cell()[oindex], v_value, st, lvar, static_cast<size_t>(oindex));
			}
			else {
				switch(st.cell()[(*it) + 1].value_GetType()) {
				case F_TAG_VARIABLE:
					pvm->variable().SetValue(st.cell()[(*it) + 1].index, v_value);
					break;
				case F_TAG_LOCALVARIABLE:
					lvar.SetValue(st.cell()[(*it) + 1].name, v_value);
					break;
				default:
					break;
				};
			}
		}
	}

	return errcount ? 1 : 0;
}

#if defined(_MSC_VER) && _MSC_VER < 1300
# pragma auto_inline()
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecSystemFunctionWithArgs
 *  機能概要：  引数付きのシステム関数を実行します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::ExecSystemFunctionWithArgs(CExecFrame &fr, CCell& cell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	// 関数の格納位置を取得
	std::vector<size_t>::iterator it = sid.begin();
	size_t	func_index = *it;
	size_t	index = st.cell()[func_index].index;
	it++;

	// 引数作成
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	std::vector<CCell *> pcellarg;
	std::vector<CValue> valuearg;
	std::vector<size_t>::size_type sidsize = sid.size();

	// 引数が2つ以上のとき、3本のvectorが1個ずつ伸びて何度も確保し直されるのを避ける
	if (sidsize > 2) {
		arg.array().reserve(sidsize - 1);
		pcellarg.reserve(sidsize - 1);
		valuearg.reserve(sidsize - 1);
	}

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(fr, st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			if ( sidsize <= 2 ) { //配列1つのみが与えられている->最適化のためスマートポインタ代入のみで済ませる
				arg.array_shared() = addv.array_shared();
			}
			else {
				arg.array().insert(arg.array().end(), addv.array().begin(), addv.array().end());
			}
		}
		else {
			arg.array().emplace_back(CValue(addv));
		}

		valuearg.emplace_back(addv);
		pcellarg.emplace_back(&(st.cell()[*it]));
	}

	// 実行　%[n]処理関数のみ特例扱い
	if (index == CSystemFunction::HistoryIndex())
		ExecHistoryP1(fr, func_index - 2, cell, arg, st);
	else
		fr.at(st, cell).ansv() = pvm->sysfunction().Execute(index, arg, pcellarg, valuearg, lvar, st.linecount, this);

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecHistoryP1
 *  機能概要：  %[n]（置換済の値の再利用）を処理します（前処理）
 *
 *  処理は二段階で行われます。前処理では本処理のための値を演算子の項へセットします。
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecHistoryP1(CExecFrame &fr, size_t start_index, CCell& cell, const CValue &arg, CStatement &st)
{
	if (arg.array_size()) {
		fr.at(st, cell).ansv()    = CValue((yaya::int_t)start_index);
		fr.at(st, cell).order()   = arg.array()[0];
	}
	else {
		pvm->logger().Error(E_E, 90, dicfilename, st.linecount);
		fr.at(st, cell).ansv().SetType(F_TAG_VOID);
		fr.at(st, cell).order().SetType(F_TAG_VOID);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::ExecHistoryP2
 *  機能概要：  %[n]（置換済の値の再利用）を処理します（本処理）
 *
 *  処理は二段階で行われます。本処理では前処理で埋め込んだ値を参照して値を取得します。
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecHistoryP2(CExecFrame &fr, CCell& cell, CStatement &st)
{
	if (!fr.at(st, cell).order_const().IsNum())
		return;

	yaya::int_t	index = fr.at(st, cell).order_const().GetValueInt();
	if (index < 0)
		return;

	ptrdiff_t start = (ptrdiff_t)std::min(yaya::int_t(st.cell().size())-1,fr.at(st, cell).ansv_const().GetValueInt());

	for(ptrdiff_t i = start ; i >= 0; i--) {
		if (st.cell()[i].value_GetType() == F_TAG_STRING_EMBED) {
			if (!index) {
				fr.at(st, cell).ansv_shared() = std_make_shared<CValue>(fr.at(st, st.cell()[i]).emb_ansv_const());
				return;
			}
			index--;
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::Feedback
 *  機能概要：  &演算子を処理します
 *
 *  返値　　：  0/1=成功/エラー
 * -----------------------------------------------------------------------
 */
char	CFunction::Feedback(CExecFrame &fr, CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	*v_cell = &(st.cell()[sid[1]]);

	// 値は右辺をそのままコピー
	fr.at(st, anscell).ansv() = GetValueRefForCalc(fr, *v_cell, st, lvar);

	// 右辺が配列序数を指定する演算子だった場合はそこから序数をコピー
	// 配列でなかった場合は序数を格納する変数の型をNOPにしてフラグとする
	if (v_cell->value_GetType() == F_TAG_ARRAYORDER)
		fr.at(st, anscell).order_shared() = fr.at(st, *v_cell).order_shared();
	else
		fr.at(st, anscell).order().SetType(F_TAG_NOP);

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::EncodeArrayOrder
 *  機能概要：  配列の序数を作成して返します
 *
 *  CValue operator [] は引数として要素数2以上の配列型のCValueを要求します。
 *  （第一引数が序数、第二引数がデリミタ）
 *  この関数はそれを作成します。
 *
 *  エラーが発生した場合は型のない（F_TAG_UNKNOWN）値を返します。（呼び出し側はこれを見てエラー処理します）
 * -----------------------------------------------------------------------
 */
void CFunction::EncodeArrayOrder(CCell &vcell, const CValue &order, CLocalVariable &lvar, CValue &result)
{
	result.SetType(F_TAG_ARRAY);

	// 序数
	switch(order.GetType()) {
	case F_TAG_ARRAY:
		result = order;
		break;
	default:
		result.array().emplace_back(CValue(order));
		break;
	};

	// デリミタ
	if (result.array_size() < 2) {
		CValue	adddlm(VAR_DELIMITER);
		if (vcell.value_GetType() == F_TAG_VARIABLE)
			adddlm = pvm->variable().GetDelimiter(vcell.index);
		else if (vcell.value_GetType() == F_TAG_LOCALVARIABLE)
			adddlm = lvar.GetDelimiter(vcell.name);
		result.array().emplace_back(adddlm);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunction::FeedLineToTail
 *  機能概要：  現在の{}の終端"}"まで実行行カウンタを送ります
 *
 *  実際に送るのは"}"の1つ手前の行の位置です
 * -----------------------------------------------------------------------
 */
void	CFunction::FeedLineToTail(size_t&line)
{
	size_t	t_statelenm1 = statelenm1;

	size_t	depth = 1;
	line++;
	for( ; line < t_statelenm1; line++) {
		depth += ((statement[line].type == ST_OPEN) - (statement[line].type == ST_CLOSE));
		if (!depth)
			break;
	}

	line--;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunctionDef::GetFunctionIndexFromName
 *  機能概要：  関数名に対応する配列の序数を取得します
 * -----------------------------------------------------------------------
 */
ptrdiff_t CFunctionDef::GetFunctionIndexFromName(const yaya::string_t& name)
{
	if ( map.empty() ) {
		RebuildFunctionMap();
	}

	yaya::indexmap::const_iterator it = map.find(name);
	if ( it != map.end() ) {
		return it->second;
	}
	return -1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunctionDef::AddFunctionIndex
 *  機能概要：  関数名に対応する配列の序数を追加します
 * -----------------------------------------------------------------------
 */
void CFunctionDef::AddFunctionIndex(const yaya::string_t& name,size_t index)
{
	map.insert(yaya::indexmap::value_type(name, index));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunctionDef::ClearFunctionIndex
 *  機能概要：  配列の序数キャッシュ用ハッシュを削除します
 * -----------------------------------------------------------------------
 */
void CFunctionDef::ClearFunctionIndex(void)
{
	map.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFunctionDef::RebuildFunctionMap
 *  機能概要：  配列の序数キャッシュ用ハッシュを構築します
 * -----------------------------------------------------------------------
 */
void CFunctionDef::RebuildFunctionMap(void)
{
	ClearFunctionIndex();
	for (size_t fcnt = 0; fcnt < func.size(); ++fcnt) {
		map.insert(yaya::indexmap::value_type(func[fcnt].name,fcnt));
	}
}

void CFunctionDef::deep_copy_func(CFunctionDef &from)
{
	for (size_t fcnt = 0; fcnt < from.func.size(); ++fcnt) {
		func[fcnt].deep_copy_statement(from.func[fcnt]);
	}
}
