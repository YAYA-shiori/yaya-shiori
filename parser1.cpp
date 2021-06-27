// 
// AYA version 5
//
// 構文解析/中間コードの生成を行うクラス　CParser1
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "parser1.h"

#include "ayavm.h"
#include "function.h"
#include "log.h"
#include "messages.h"
#include "globaldef.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckExecutionCode
 *  機能概要：  構文解析（と中間コード生成）の最終処理と、正当性の検査を行います
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckExecutionCode(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
		      errcount += CheckExecutionCode1(*it2, it->dicfilename);
		}
	}

	errcount += SetBreakJumpNo(dicfilename);
	errcount += CheckCaseSyntax(dicfilename);
	errcount += CheckIfSyntax(dicfilename);
	errcount += CheckElseSyntax(dicfilename);
	errcount += CheckForSyntax(dicfilename);
	errcount += CheckForeachSyntax(dicfilename);
	errcount += SetIfJumpNo(dicfilename);

	CompleteSetting();

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckExecutionCode1
 *  機能概要：  構文解析（と中間コード生成）の最終処理と、正当性の検査を行います
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckExecutionCode1(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	errcount += CheckNomialCount(st, dicfilename);
	errcount += CheckSubstSyntax(st, dicfilename);
	errcount += CheckFeedbackOperatorPos(st, dicfilename);
	errcount += SetFormulaType(st, dicfilename);
	errcount += CheckFunctionArgument(st, dicfilename);

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckNomialCount
 *  機能概要：  演算式の項数が正常か確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckNomialCount(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CSerial>::iterator it = st.serial().begin(); it != st.serial().end(); it++) {
		int t_type = st.cell()[it->tindex].value_GetType();

		//演算に関係してる項の数を確認 二項演算子なのに単項で使われてるとか
		if ( t_type >= F_TAG_ORIGIN && t_type < F_TAG_ORIGIN_VALUE ) {
			if ( static_cast<int>(it->index.size()) < formulatag_params[t_type-F_TAG_ORIGIN] ) {
				vm.logger().Error(E_E, 22, dicfilename, st.linecount);
				errcount++;
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckSubstSyntax
 *  機能概要：  代入演算子の左辺が変数であるかを検査します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckSubstSyntax(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(size_t i = 0; i < st.cell_size(); ++i) {
		int t_type = st.cell()[i].value_GetType();

		if (t_type >= F_TAG_EQUAL && t_type <= F_TAG_COMMAEQUAL) {
			// 代入演算子が式の先頭、もしくは最後尾にある場合はエラー
			if (!i || i == static_cast<size_t>(st.cell_size()) - 1) {
				vm.logger().Error(E_E, 29, dicfilename, st.linecount);
				errcount++;
			}
			// 先頭/最後尾でない位置の場合はさらに確認
			else {
				int	before = i - 1;
				// 代入演算子の手前は変数、ローカル変数、閉じスクウェアブラケット("]")の
				// いずれかであるはずなので、これを確認
				if (st.cell()[before].value_GetType() != F_TAG_VARIABLE &&
					st.cell()[before].value_GetType() != F_TAG_LOCALVARIABLE &&
					st.cell()[before].value_GetType() != F_TAG_HOOKBRACKETOUT) {
					vm.logger().Error(E_E, 29, dicfilename, st.linecount);
					errcount++;
				}
				// 手前が閉じスクウェアブラケット("]")だった場合はブラケット手前の変数を確認
				if (st.cell()[before].value_GetType() == F_TAG_HOOKBRACKETOUT) {
					int	depth = 1;
					int     j = 0;
					for(j = before - 1; j >= 0; j--) {
						if (st.cell()[j].value_GetType() == F_TAG_HOOKBRACKETOUT)
							depth++;
						else if (st.cell()[j].value_GetType() == F_TAG_HOOKBRACKETIN)
							depth--;
						else {
							if (!depth)
								break;
						}
					}
					if (j < 1) {
						vm.logger().Error(E_E, 29, dicfilename, st.linecount);
						errcount++;
					}
					else if (st.cell()[j].value_GetType() != F_TAG_ARRAYORDER ||
						(st.cell()[j - 1].value_GetType() != F_TAG_VARIABLE &&
						st.cell()[j - 1].value_GetType() != F_TAG_LOCALVARIABLE)) {
						vm.logger().Error(E_E, 29, dicfilename, st.linecount);
						errcount++;
					}
				}
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckFeedbackOperatorPos
 *  機能概要：  フィードバック演算子の位置の正当性を検査します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckFeedbackOperatorPos(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	int	sz = st.cell_size();
	for(int i = 0; i < sz; i++)
		if (st.cell()[i].value_GetType() == F_TAG_FEEDBACK) {
			if (i < 2 || i == sz - 1) {
				vm.logger().Error(E_E, 87, dicfilename, st.linecount);
				errcount++;
			}
			else if ((st.cell()[i - 2].value_GetType() != F_TAG_BRACKETIN &&
				st.cell()[i - 2].value_GetType() != F_TAG_COMMA)
				||
				(st.cell()[i - 1].value_GetType() != F_TAG_INT ||
				st.cell()[i - 1].value_const().i_value != 0)
				||
				(st.cell()[i + 1].value_GetType() != F_TAG_VARIABLE &&
				st.cell()[i + 1].value_GetType() != F_TAG_LOCALVARIABLE)) {
				vm.logger().Error(E_E, 87, dicfilename, st.linecount);
				errcount++;
			}
		}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::SetFormulaType
 *  機能概要：  数式の種類を判定します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::SetFormulaType(CStatement& st, const yaya::string_t& dicfilename)
{
	if (st.type != ST_FORMULA)
		return 0;

	int	sz = st.cell_size();
	// 項が一つも無い場合はエラー
	if (!sz) {
		vm.logger().Error(E_E, 26, dicfilename, st.linecount);
		return 1;
	}

	// 項が一つの場合は単純な出力
	if (sz == 1) {
		st.type = ST_FORMULA_OUT_FORMULA;
		return 0;
	}

	// 最後に計算する演算子が代入系か否かで種類を判定
	int	lastsr = st.serial_size();
	if (!lastsr) {
		vm.logger().Error(E_E, 83, dicfilename, st.linecount);
		return 1;
	}

	int	lasttag = st.cell()[st.serial()[lastsr - 1].tindex].value_GetType();
	if (lasttag >= F_TAG_EQUAL &&
		lasttag <= F_TAG_COMMAEQUAL)
		st.type = ST_FORMULA_SUBST;
	else
		st.type = ST_FORMULA_OUT_FORMULA;

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::SetBreakJumpNo
 *  機能概要：  break/continue/return文が現在の処理単位から抜ける際にジャンプする
 *  　　　　　  飛び先の行番号を取得します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::SetBreakJumpNo(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		std::vector<CVecint>	dline;
		CVecint	addvecint;
		dline.push_back(addvecint);
		int	depth = 0;
		for(size_t i = 0; i < it->statement.size(); ++i) {
			// {
			if (it->statement[i].type == ST_OPEN) {
				CVecint	addvecint2;
				dline.push_back(addvecint2);
				depth++;
			}
			// }
			else if (it->statement[i].type == ST_CLOSE) {
				if (depth >= 0) {
					int	len = dline[depth].i_array.size();
					for(int j = 0; j < len; j++)
						it->statement[dline[depth].i_array[j]].jumpto = i;
					dline.erase(dline.end() - 1);
					depth--;
				}
			}
			// break/continue/return
			else if (it->statement[i].type == ST_BREAK ||
				it->statement[i].type == ST_CONTINUE ||
				it->statement[i].type == ST_RETURN) {
				if (depth < 0) {
					vm.logger().Error(E_E, 31, it->dicfilename, it->statement[i].linecount);
					errcount++;
					break;
				}
				else
					dline[depth].i_array.push_back(i);
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckCaseSyntax
 *  機能概要：  caseの直後に"{"が続いているかを確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckCaseSyntax(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	casev = 0;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type == ST_FORMULA_SUBST) {
				if (!wcsncmp(PREFIX_CASE_VAR, it2->cell()[0].name.c_str(),PREFIX_CASE_VAR_SIZE)) {
					casev = 1;
					continue;
				}
			}

			if (casev && it2->type != ST_OPEN) {
				vm.logger().Error(E_E, 51, it->dicfilename, it2->linecount);
				errcount++;
			}
			casev = 0;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckIfSyntax
 *  機能概要：  if/elseif/else/switch/whileの直後に"{"が続いているかを確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckIfSyntax(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	beftype = ST_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type != ST_OPEN) {
				switch(beftype) {
				case ST_IF:
					vm.logger().Error(E_E, 35, it->dicfilename, it2->linecount);
					errcount++;
					break;
				case ST_ELSEIF:
					vm.logger().Error(E_E, 36, it->dicfilename, it2->linecount);
					errcount++;
					break;
				case ST_ELSE:
					vm.logger().Error(E_E, 37, it->dicfilename, it2->linecount);
					errcount++;
					break;
				case ST_SWITCH:
					vm.logger().Error(E_E, 38, it->dicfilename, it2->linecount);
					errcount++;
					break;
				case ST_WHILE:
					vm.logger().Error(E_E, 39, it->dicfilename, it2->linecount);
					errcount++;
					break;
				default:
					break;
				};
			}
			beftype = it2->type;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckElseSyntax
 *  機能概要：  elseif/elseの直前に"}"が存在するかを確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckElseSyntax(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	beftype = ST_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type == ST_ELSEIF ||
				it2->type == ST_ELSE) {
				if (beftype != ST_CLOSE) {
					vm.logger().Error(E_E, 47, it->dicfilename, it2->linecount);
					errcount++;
				}
			}
			beftype = it2->type;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckForSyntax
 *  機能概要：  for文が for formulaA; formulaB; formulaC; { という形式となっているかを
 *  　　　　　  確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckForSyntax(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	beftype[3] = { ST_UNKNOWN, ST_UNKNOWN, ST_UNKNOWN };
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (beftype[2] == ST_FOR) {
				if (beftype[1] != ST_FORMULA_OUT_FORMULA &&
					beftype[1] != ST_FORMULA_SUBST) {
					vm.logger().Error(E_E, 40, it->dicfilename, it2->linecount);
					errcount++;
				}
				if (beftype[0] != ST_FORMULA_OUT_FORMULA &&
					beftype[0] != ST_FORMULA_SUBST) {
					vm.logger().Error(E_E, 41, it->dicfilename, it2->linecount);
					errcount++;
				}
				if (it2->type != ST_OPEN) {
					vm.logger().Error(E_E, 42, it->dicfilename, it2->linecount);
					errcount++;
				}
			}
			beftype[2] = beftype[1];
			beftype[1] = beftype[0];
			beftype[0] = it2->type;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckForeachSyntax
 *  機能概要：  for文が foreach formula; vm.variable(); { という形式となっているかを確認します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckForeachSyntax(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	beftype[2]  = { ST_UNKNOWN, ST_UNKNOWN };
		int	befcelltype = F_TAG_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (beftype[1] == ST_FOREACH) {
				if (beftype[0] == ST_FORMULA_OUT_FORMULA &&
					(befcelltype == F_TAG_VARIABLE || befcelltype == F_TAG_LOCALVARIABLE)) {
					// これで正しい
				}
				else {
					vm.logger().Error(E_E, 43, it->dicfilename, it2->linecount);
					errcount++;
				}
				if (it2->type != ST_OPEN) {
					vm.logger().Error(E_E, 44, it->dicfilename, it2->linecount);
					errcount++;
				}
			}
			beftype[1]  = beftype[0];
			beftype[0]  = it2->type;
			befcelltype = (it2->cell_size() == 1) ? it2->cell()[0].value_GetType() : F_TAG_UNKNOWN;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::SetIfJumpNo
 *  機能概要：  if/elseif/else/switch/for/foreach/while構文から脱出する際の
 *  　　　　　  飛び先の行番号を取得します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::SetIfJumpNo(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		std::vector<int>	dline;
		dline.push_back(-1);
		std::vector<int>	ifchain;
		ifchain.push_back(0);
		int	depth = 0;
		for(size_t i = 0; i < it->statement.size(); i++) {
			// {
			if (it->statement[i].type == ST_OPEN) {
				depth++;
				dline.push_back(-1);
				ifchain.push_back(0);
			}
			// }
			else if (it->statement[i].type == ST_CLOSE) {
				if (depth >= 0) {
					dline.erase(dline.end() - 1);
					ifchain.erase(ifchain.end() - 1);
					depth--;
					if (depth >= 0) {
						if (dline[depth] != -1)
							it->statement[dline[depth]].jumpto = i;
						dline[depth] = -1;
						if (ifchain[depth] == 1)
							ifchain[depth] = 2;
					}
				}
			}
			// if
			else if (it->statement[i].type == ST_IF) {
				if (depth < 0) {
					vm.logger().Error(E_E, 58, it->dicfilename, it->statement[i].linecount);
					errcount++;
					break;
				}
				else {
					dline[depth]   = i;
					ifchain[depth] = 1;
				}
			}
			// switch/for/foreach/while
			else if (it->statement[i].type == ST_SWITCH ||
				it->statement[i].type == ST_FOR ||
				it->statement[i].type == ST_FOREACH ||
				it->statement[i].type == ST_WHILE) {
				if (depth < 0) {
					vm.logger().Error(E_E, 58, it->dicfilename, it->statement[i].linecount);
					errcount++;
					break;
				}
				else {
					dline[depth]   = i;
					ifchain[depth] = 0;
				}
			}
			// elseif
			else if (it->statement[i].type == ST_ELSEIF) {
				if (depth < 0) {
					vm.logger().Error(E_E, 67, it->dicfilename, it->statement[i].linecount);
					errcount++;
					break;
				}
				else {
					if (ifchain[depth] == 2) {
						dline[depth]   = i;
						ifchain[depth] = 1;
					}
					else {
						vm.logger().Error(E_E, 68, it->dicfilename, it->statement[i].linecount);
						errcount++;
						break;
					}
				}
			}
			// else
			else if (it->statement[i].type == ST_ELSE) {
				if (depth < 0) {
					vm.logger().Error(E_E, 67, it->dicfilename, it->statement[i].linecount);
					errcount++;
					break;
				}
				else {
					if (ifchain[depth] == 2) {
						dline[depth]   = i;
						ifchain[depth] = 0;
					}
					else {
						vm.logger().Error(E_E, 69, it->dicfilename, it->statement[i].linecount);
						errcount++;
						break;
					}
				}
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CheckFunctionArgument
 *  機能概要：  関数へ引数を渡す演算子の位置が適正かを確認します
 *  　　　　　  また、該関数がシステム関数であった場合は演算子を書き換えます
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckFunctionArgument(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	if (st.type >= ST_FORMULA_OUT_FORMULA && st.type <= ST_VOID) {
		int	beftype = F_TAG_UNKNOWN;
	
		if ( st.cell_size() ) { //高速化用
			for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++) {
				if (it->value_GetType() == F_TAG_FUNCPARAM) {
					if (it == st.cell().begin()) {
						vm.logger().Error(E_E, 70, dicfilename, st.linecount);
						errcount++;
					}
					if (beftype == F_TAG_SYSFUNC)
						it->value_SetType(F_TAG_SYSFUNCPARAM);
					else if (beftype != F_TAG_USERFUNC) {
						vm.logger().Error(E_E, 71, dicfilename, st.linecount);
						errcount++;
					}
				}
				beftype = it->value_GetType();
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser1::CompleteSetting
 *  機能概要：  中間コード生成の終了処理
 * -----------------------------------------------------------------------
 */
void	CParser1::CompleteSetting(void)
{
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		it->CompleteSetting();

	vm.variable().CompleteSetting();
}

