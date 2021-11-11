// 
// AYA version 5
//
// 中間コードをログに出力するクラス　CLogExCode
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif


#include <vector>

#include "logexcode.h"
#include "ayavm.h"

#include "function.h"
#include "log.h"
#include "globaldef.h"
#include "sysfunc.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CLogExCode::OutExecutionCodeForCheck
 *  機能概要：  構文解析結果（中間コード）をログに記録します
 * -----------------------------------------------------------------------
 */
void	CLogExCode::OutExecutionCodeForCheck(void)
{
	vm.logger().Message(4);

    yaya::string_t tmpstr;
	int	i = 0;
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++, i++) {
		// 関数の定義番号
		tmpstr = L"[" + yaya::ws_itoa(i) + L"] ";
		vm.logger().Write(tmpstr);
		vm.logger().Write(L"------------------------------------------------------------------------\n");
		// 関数の名前
		vm.logger().Write(L"       ");
		vm.logger().Write(it->name);
		vm.logger().Write(L"\n");
		vm.logger().Write(L"       ------------------------------------------------------------------------\n");
		// 関数の重複回避モード
		vm.logger().Write(L"Duplication evasion mode : ");
		vm.logger().Write(CSelecter::ChoiceTypeToString(it->dupl_func.GetType()));
		vm.logger().Write(L"\n");
		// 関数が記述されている辞書ファイル名
		vm.logger().Write(L"dic filename : ");
		vm.logger().Write(it->dicfilename);
		vm.logger().Write(L"\n");
		// 関数の動作記述
		yaya::string_t indent = L" ";
		int	j = 0;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++, j++) {
			// 行番号（[関数内の行番号/辞書ファイル中の行番号]）
			tmpstr = L"[" + yaya::ws_itoa(j) + L" / " + yaya::ws_itoa(it2->linecount) + L"] ";
			vm.logger().Write(tmpstr);
			// ステートメントの種類別にログに記録
			yaya::string_t	formula;
			switch(it2->type) {
			case ST_UNKNOWN:
				vm.logger().Write(indent);
				vm.logger().Write(L"?unknown statement(unknown)\n");
				break;
			case ST_NOP:
				vm.logger().Write(indent);
				vm.logger().Write(L"(no operation)\n");
				break;
			case ST_OPEN:
				vm.logger().Write(indent);
				vm.logger().Write(L"{\n");
				indent += OUTDIC_INDENT;
				break;
			case ST_CLOSE:
				if (indent.size() >= OUTDIC_INDENT_SIZE)
					indent.erase(indent.size() - OUTDIC_INDENT_SIZE, OUTDIC_INDENT_SIZE);
				vm.logger().Write(indent);
				vm.logger().Write(L"}\n");
				break;
			case ST_COMBINE:
				vm.logger().Write(indent);
				vm.logger().Write(L"--\n");
				break;
			case ST_BREAK:
				vm.logger().Write(indent);
				vm.logger().Write(L"break (jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L")\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_CONTINUE:
				vm.logger().Write(indent);
				vm.logger().Write(L"continue (jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L")\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_RETURN:
				vm.logger().Write(indent);
				vm.logger().Write(L"return (jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L")\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_ELSE:
				vm.logger().Write(indent);
				vm.logger().Write(L"else (jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L")\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_FORMULA:
				vm.logger().Write(indent);
				vm.logger().Write(L"?unknown statement(formula)\n");
				break;
			case ST_FORMULA_OUT_FORMULA:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L" (formula output)\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n");
				break;
			case ST_FORMULA_SUBST:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L" (substitution)\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n");
				break;
			case ST_IF:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"if conditions : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_ELSEIF:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"elseif conditions : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_WHILE:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"while conditions : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_SWITCH:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"switch conditions : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_FOR:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"for initialization : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_FOREACH:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				formula.insert(0, L"foreach array : ");
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"jump to : ");
				tmpstr = yaya::ws_itoa(it2->jumpto) + L"\n";
				vm.logger().Write(tmpstr);
				break;
			case ST_PARALLEL:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L" (parallel formula output)\n             ");
				vm.logger().Write(indent);
				vm.logger().Write(L"operation order : ");
				StructSerialString(&(*it2), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L"\n");
				break;
			case ST_VOID:
				vm.logger().Write(indent);
				StructCellString(&(it2->cell()), formula);
				vm.logger().Write(formula);
				vm.logger().Write(L" (void formula - no output)\n");
				break;
			default:
				vm.logger().Write(indent);
				vm.logger().Write(L"?unknown statement(?)\n");
				break;
			};
		}
		vm.logger().Write(L"\n");
	}

	vm.logger().Message(8);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLogExCode::StructCellString
 *  機能概要：  OutDicInfoForCheckから呼ばれます。数式を文字列化します
 * -----------------------------------------------------------------------
 */
void	CLogExCode::StructCellString(std::vector<CCell> *cellvector, yaya::string_t &formula)
{
	formula = L"";
    yaya::string_t	tmpstr;
	for(std::vector<CCell>::iterator it = cellvector->begin(); it != cellvector->end(); it++) {
		if (it->value_GetType() >= F_TAG_ORIGIN &&
			it->value_GetType() < F_TAG_FUNCPARAM) {
			formula += formulatag[it->value_GetType()];
			formula += L" ";
			continue;
		}
		switch(it->value_GetType()) {
		case F_TAG_NOP:
			formula += L"(NOP/VOID) ";
			break;
		case F_TAG_FUNCPARAM:
			formula += L"@ ";
			break;
		case F_TAG_SYSFUNCPARAM:
			formula += L"$ ";
			break;
		case F_TAG_ARRAYORDER:
			formula += L"# " ;
			break;
		case F_TAG_INT:
			tmpstr = L"(int)" + yaya::ws_itoa(it->value_const().i_value) + L" ";
			formula += tmpstr;
			break;
		case F_TAG_DOUBLE:
			tmpstr = L"(double)" + yaya::ws_ftoa(it->value_const().d_value) + L" ";
			formula += tmpstr;
			break;
		case F_TAG_STRING:
			formula += L"(string)";
			formula += it->value_const().s_value;
			formula += L" ";
			break;
		case F_TAG_STRING_EMBED:
			formula += L"(emb string)";
			formula += it->value_const().s_value;
			formula += L" ";
			break;
		case F_TAG_SYSFUNC:
			formula += L"(sysfunc)";
			formula += CSystemFunction::GetNameFromIndex(it->index);
			formula += L" ";
			break;
		case F_TAG_USERFUNC:
			formula += L"(func)";
			formula += vm.function_parse().func[it->index].name;
			formula += L" ";
			break;
		case F_TAG_VARIABLE:
			formula += L"(var)";
			formula += vm.variable().GetName(it->index);
			formula += L" ";
			break;
		case F_TAG_LOCALVARIABLE:
			formula += L"(localvar)";
			formula += it->name;
			formula += L" ";
			break;
		default:
			formula += L"(?UNKNOWN) ";
			break;
		};
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLogExCode::StructSerialString
 *  機能概要：  OutDicInfoForCheckから呼ばれます。数式の演算順序を文字列化します
 * -----------------------------------------------------------------------
 */
void	CLogExCode::StructSerialString(CStatement *st, yaya::string_t &formula)
{
	formula = L"";
    yaya::string_t	tmpstr;

	if ( st->serial_size() ) { //高速化用
		for(std::vector<CSerial>::iterator it = st->serial().begin(); it != st->serial().end(); it++) {
			if (it != st->serial().begin())
				formula += L" | ";

			int	type = st->cell()[it->tindex].value_const().GetType();

			if (type == F_TAG_FUNCPARAM)
				formula += L"@(";
			else if (type == F_TAG_SYSFUNCPARAM)
				formula += L"$(" ;
			else if (type == F_TAG_ARRAYORDER)
				formula += L"#(" ;
			else if (type >= F_TAG_ORIGIN && type < F_TAG_ORIGIN_VALUE) {
				formula += formulatag[type];
				formula += L"(";
			}
			else if (type >= F_TAG_ORIGIN_VALUE) {
				formula += L"direct" ;
				continue;
			}
			else {
				formula += L"unknown" ;
				continue;
			}

			for(std::vector<int>::iterator it2 = it->index.begin(); it2 != it->index.end(); it2++) {
				if (it2 != it->index.begin())
					formula += L",";
				tmpstr = yaya::ws_itoa(*it2);
				formula += tmpstr;
			}

			formula += L")";
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLogExCode::OutVariableInfoForCheck
 *  機能概要：  変数の定義状態をログに記録します
 * -----------------------------------------------------------------------
 */
void	CLogExCode::OutVariableInfoForCheck(void)
{
	vm.logger().Message(5);

    yaya::string_t	tmpstr;
	yaya::string_t	t_str;
	size_t	var_num = vm.variable().GetNumber();
	for(size_t	i = 0; i < var_num; i++) {
		CVariable	*var = vm.variable().GetPtr(i);
		// 変数の定義番号
		tmpstr = L"[" + yaya::ws_itoa(i) + L"] ";
		vm.logger().Write(tmpstr);
		// 変数の名前
		vm.logger().Write(var->name);
		vm.logger().Write(L" = ");
		// 変数の値
		switch(var->value().GetType()) {
		case F_TAG_INT:
			tmpstr = L"(int)" + yaya::ws_itoa(var->value_const().i_value) + L"\n";
			vm.logger().Write(tmpstr);
			break;
		case F_TAG_DOUBLE:
			tmpstr = L"(double)" + yaya::ws_ftoa(var->value_const().d_value) + L"\n";
			vm.logger().Write(tmpstr);
			break;
		case F_TAG_STRING:
			vm.logger().Write(L"(string)");
			vm.logger().Write(var->value_const().s_value);
			vm.logger().Write(L"\n");
			break;
		case F_TAG_ARRAY:
			vm.logger().Write(L"(array) : ");
			StructArrayString(var->value_const().array(), t_str);
			vm.logger().Write(t_str);
			vm.logger().Write(L"\n");
			break;
		case F_TAG_VOID:
			vm.logger().Write(L"(nop/void)\n");
			break;
		default:
			vm.logger().Write(L"(unknown type)\n");
			break;
		};
	}

	vm.logger().Message(8);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLogExCode::StructArrayString
 *  機能概要：  OutVariableInfoForCheckから呼ばれます。配列変数の内容を文字列化します
 * -----------------------------------------------------------------------
 */
void	CLogExCode::StructArrayString(const CValueArray &vs, yaya::string_t &enlist)
{
	enlist = L"";
    yaya::string_t	tmpstr;
	for(CValueArray::const_iterator it = vs.begin(); it != vs.end(); it++) {
		switch(it->GetType()) {
		case F_TAG_INT:
			tmpstr = L"(int)" + yaya::ws_itoa(it->i_value) + L" ";
			enlist += tmpstr;
			break;
		case F_TAG_DOUBLE:
			tmpstr = L"(double)" + yaya::ws_ftoa(it->d_value) + L" ";
			enlist += tmpstr;
			break;
		case F_TAG_STRING:
			enlist += L"(string)";
			enlist += it->s_value;
			enlist += L" ";
			break;
		case F_TAG_VOID:
			enlist += L"(nop/void) ";
			break;
		default:
			enlist += L"(?UNKNOWN) ";
			break;
		};
	}
}
