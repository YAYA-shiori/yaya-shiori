// 
// AYA version 5
//
// �\�����/���ԃR�[�h�̐������s���N���X�@CParser1
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
 *  �֐���  �F  CParser1::CheckExecutionCode
 *  �@�\�T�v�F  �\����́i�ƒ��ԃR�[�h�����j�̍ŏI�����ƁA�������̌������s���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckExecutionCode(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++)
		      errcount += CheckExecutionCode1(*it2, it->dicfilename);

	errcount += SetBreakJumpNo();
	errcount += CheckCaseSyntax();
	errcount += CheckIfSyntax();
	errcount += CheckElseSyntax();
	errcount += CheckForSyntax();
	errcount += CheckForeachSyntax();
	errcount += SetIfJumpNo();

	CompleteSetting();

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CParser1::CheckExecutionCode1
 *  �@�\�T�v�F  �\����́i�ƒ��ԃR�[�h�����j�̍ŏI�����ƁA�������̌������s���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
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
 *  �֐���  �F  CParser1::CheckNomialCount
 *  �@�\�T�v�F  ���Z���̍��������킩�m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckNomialCount(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CSerial>::iterator it = st.serial().begin(); it != st.serial().end(); it++) {
		int t_type = st.cell()[it->tindex].value_GetType();

		//���Z�Ɋ֌W���Ă鍀�̐����m�F �񍀉��Z�q�Ȃ̂ɒP���Ŏg���Ă�Ƃ�
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
 *  �֐���  �F  CParser1::CheckSubstSyntax
 *  �@�\�T�v�F  ������Z�q�̍��ӂ��ϐ��ł��邩���������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckSubstSyntax(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(size_t i = 0; i < st.cell_size(); ++i) {
		int t_type = st.cell()[i].value_GetType();

		if (t_type >= F_TAG_EQUAL && t_type <= F_TAG_COMMAEQUAL) {
			// ������Z�q�����̐擪�A�������͍Ō���ɂ���ꍇ�̓G���[
			if (!i || i == static_cast<size_t>(st.cell_size()) - 1) {
				vm.logger().Error(E_E, 29, dicfilename, st.linecount);
				errcount++;
			}
			// �擪/�Ō���łȂ��ʒu�̏ꍇ�͂���Ɋm�F
			else {
				int	before = i - 1;
				// ������Z�q�̎�O�͕ϐ��A���[�J���ϐ��A���X�N�E�F�A�u���P�b�g("]")��
				// �����ꂩ�ł���͂��Ȃ̂ŁA������m�F
				if (st.cell()[before].value_GetType() != F_TAG_VARIABLE &&
					st.cell()[before].value_GetType() != F_TAG_LOCALVARIABLE &&
					st.cell()[before].value_GetType() != F_TAG_HOOKBRACKETOUT) {
					vm.logger().Error(E_E, 29, dicfilename, st.linecount);
					errcount++;
				}
				// ��O�����X�N�E�F�A�u���P�b�g("]")�������ꍇ�̓u���P�b�g��O�̕ϐ����m�F
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
 *  �֐���  �F  CParser1::CheckFeedbackOperatorPos
 *  �@�\�T�v�F  �t�B�[�h�o�b�N���Z�q�̈ʒu�̐��������������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
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
 *  �֐���  �F  CParser1::SetFormulaType
 *  �@�\�T�v�F  �����̎�ނ𔻒肵�܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::SetFormulaType(CStatement& st, const yaya::string_t& dicfilename)
{
	if (st.type != ST_FORMULA)
		return 0;

	int	sz = st.cell_size();
	// ������������ꍇ�̓G���[
	if (!sz) {
		vm.logger().Error(E_E, 26, dicfilename, st.linecount);
		return 1;
	}

	// ������̏ꍇ�͒P���ȏo��
	if (sz == 1) {
		st.type = ST_FORMULA_OUT_FORMULA;
		return 0;
	}

	// �Ō�Ɍv�Z���鉉�Z�q������n���ۂ��Ŏ�ނ𔻒�
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
 *  �֐���  �F  CParser1::SetBreakJumpNo
 *  �@�\�T�v�F  break/continue/return�������݂̏����P�ʂ��甲����ۂɃW�����v����
 *  �@�@�@�@�@  ��ѐ�̍s�ԍ����擾���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::SetBreakJumpNo(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckCaseSyntax
 *  �@�\�T�v�F  case�̒����"{"�������Ă��邩���m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckCaseSyntax(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckIfSyntax
 *  �@�\�T�v�F  if/elseif/else/switch/while�̒����"{"�������Ă��邩���m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckIfSyntax(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckElseSyntax
 *  �@�\�T�v�F  elseif/else�̒��O��"}"�����݂��邩���m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckElseSyntax(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckForSyntax
 *  �@�\�T�v�F  for���� for formulaA; formulaB; formulaC; { �Ƃ����`���ƂȂ��Ă��邩��
 *  �@�@�@�@�@  �m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckForSyntax(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckForeachSyntax
 *  �@�\�T�v�F  for���� foreach formula; vm.variable(); { �Ƃ����`���ƂȂ��Ă��邩���m�F���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckForeachSyntax(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
		int	beftype[2]  = { ST_UNKNOWN, ST_UNKNOWN };
		int	befcelltype = F_TAG_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (beftype[1] == ST_FOREACH) {
				if (beftype[0] == ST_FORMULA_OUT_FORMULA &&
					(befcelltype == F_TAG_VARIABLE || befcelltype == F_TAG_LOCALVARIABLE)) {
					// ����Ő�����
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
 *  �֐���  �F  CParser1::SetIfJumpNo
 *  �@�\�T�v�F  if/elseif/else/switch/for/foreach/while�\������E�o����ۂ�
 *  �@�@�@�@�@  ��ѐ�̍s�ԍ����擾���܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::SetIfJumpNo(void)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
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
 *  �֐���  �F  CParser1::CheckFunctionArgument
 *  �@�\�T�v�F  �֐��ֈ�����n�����Z�q�̈ʒu���K�������m�F���܂�
 *  �@�@�@�@�@  �܂��A�Y�֐����V�X�e���֐��ł������ꍇ�͉��Z�q�����������܂�
 *
 *  �Ԓl�@�@�F  1/0=�G���[/����
 * -----------------------------------------------------------------------
 */
char	CParser1::CheckFunctionArgument(CStatement& st, const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	if (st.type >= ST_FORMULA_OUT_FORMULA && st.type <= ST_VOID) {
		int	beftype = F_TAG_UNKNOWN;
	
		if ( st.cell_size() ) { //�������p
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
 *  �֐���  �F  CParser1::CompleteSetting
 *  �@�\�T�v�F  ���ԃR�[�h�����̏I������
 * -----------------------------------------------------------------------
 */
void	CParser1::CompleteSetting(void)
{
	for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++)
		it->CompleteSetting();

	std::vector<CFunction>(vm.function()).swap(vm.function());

	vm.variable().CompleteSetting();
}

