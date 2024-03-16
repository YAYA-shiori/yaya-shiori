// 
// AYA version 5
//
// �֐��������N���X�@CFunction
// - �又����
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

CFunction::CFunction(CAyaVM& vmr, const yaya::string_t& n, const yaya::string_t& df, int lc) : pvm(&vmr), name(n), dicfilename(df), linecount(lc), dicfilename_fullpath(vmr.basis().ToFullPath(df))
{
	statelenm1 = 0;
	namelen = name.size();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::deep_copy_statement
 *  �@�\�T�v�F  CStatement�z����f�B�[�v�R�s�[���܂�
 * -----------------------------------------------------------------------
 */
void    CFunction::deep_copy_statement(CFunction &from)
{
	for ( size_t i = 0 ; i < from.statement.size() ; ++i ) {
		statement[i].deep_copy(from.statement[i]);
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::CompleteSetting
 *  �@�\�T�v�F  �֐��̍\�z�����������i�������̓ǂݍ��݂����������j�ۂɌĂ΂�܂�
 *  �@�@�@�@�@  ���s�̍ۂɕK�v�ȍŌ�̏������������s�Ȃ��܂�
 * -----------------------------------------------------------------------
 */
void	CFunction::CompleteSetting(void)
{
	statelenm1 = statement.size() - 1;
}

CFunction::ExecutionResult	CFunction::Execute() {
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	return Execute(arg);
}
CFunction::ExecutionResult	CFunction::Execute(const CValue& arg) {
	CLocalVariable	lvar(*pvm);
	return Execute(arg, lvar);
}
/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Execute
 *  �@�\�T�v�F  �֐������s���܂�
 *
 *  ����CValue arg�͕K���z��^�ł��Barray����ł���Έ����̖����R�[���ƂȂ�܂�
 * -----------------------------------------------------------------------
 */
CFunction::ExecutionResult	CFunction::Execute(const CValue &arg, CLocalVariable &lvar)
{
	int exitcode = ST_NOP;

	// _argv���쐬
	lvar.SetValue(L"_argv", arg);
	// _argc���쐬
	CValue	t_argc((yaya::int_t)arg.array_size());
	lvar.SetValue(L"_argc", t_argc);
	//_FUNC_NAME_���쐬
	lvar.SetValue(L"_FUNC_NAME_", this->name);

	// ���s
	if (!pvm->call_limit().AddCall(name)) {
		CBasisFuncPos shiori_OnCallLimit;
		ptrdiff_t funcpos = shiori_OnCallLimit.Find(*pvm, L"shiori.OnCallLimit");
		size_t lock = pvm->call_limit().temp_unlock();

		if(funcpos >= 0) {
			//_argv[0] = dicname _argv[1] = linenum
			CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
			arg.array().emplace_back(CValueSub(dicfilename));
			arg.array().emplace_back(CValueSub((yaya::int_t)linecount));

			pvm->function_exec().func[funcpos].Execute(arg);
		}
		else {
			pvm->logger().Error(E_E, 97, dicfilename, linecount);
		}

		pvm->call_limit().reset_lock(lock);
		pvm->call_limit().DeleteCall();
		return ExecutionResult(pvm);
	}
	ExecutionResult result(NULL);
	Execute_SEHbody(result,lvar, exitcode);
	
	pvm->call_limit().DeleteCall();

	for ( size_t i = 0 ; i < statement.size() ; ++i ) {
		statement[i].cell_cleanup();
	}

	return result;
}

void CFunction::Execute_SEHhelper(CFunction::ExecutionResult& aret, CLocalVariable& lvar, int& exitcode)
{
	aret = ExecuteInBrace(1, lvar, BRACE_DEFAULT, exitcode, NULL, 0);
}

void CFunction::Execute_SEHbody(ExecutionResult& retas, CLocalVariable& lvar, int& exitcode)
{
#ifdef _WINDOWS
	__try
#else
	try
#endif
	{
		Execute_SEHhelper(retas, lvar, exitcode);
	}
#ifdef _WINDOWS
	__except (EXCEPTION_EXECUTE_HANDLER)
#else
	catch(...)
#endif
	{
		throw yaya::memory_error();
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ExecuteInBrace
 *  �@�\�T�v�F  {}�����s���A���ʂ��ЂƂԂ��܂�
 *  �����@�@�@  type     ����{}�̎�ʁB������0�`�̏ꍇ��switch�\���̍ۂ̌�⒊�o�ʒu
 *  �@�@�@�@�@  exitcode �I���R�[�h�BST_NOP/ST_BREAK/ST_RETURN/ST_CONTINUE=�ʏ�/break/return/continue
 *
 *  "{}"���̊e�X�e�[�g�����g�����s���܂��B����line�Ŏw�肳���ʒu������s���J�n���A"}"�ɓ˂�������܂�
 *  �������s���Ă����܂��B
 *  �Ԓl�͎��s���I������"}"�̈ʒu�ł��B
 * -----------------------------------------------------------------------
 */
CFunction::ExecutionInBraceResult	CFunction::ExecuteInBrace(size_t line, CLocalVariable &lvar, yaya::int_t type, int &exitcode, std::vector<CVecValue>* UpperLvCandidatePool,bool inpool)
{
	// �J�n���̏���
	lvar.AddDepth();
	CDuplEvInfo* pdupl = statement[line-1].dupl_block.get();
	const bool	 inmutiarea = statement[line-1].ismutiarea;		// pool�p

	// ���s
	CSelecter	output(pvm, pdupl, (ptrdiff_t)type);
	bool		exec_end	 = 0;		// ����{}�̎��s���I�����邽�߂̃t���O 1�ŏI��
	bool		ifflg		 = 0;		// if-elseif-else����p�B1�ł��̃u���b�N�������������Ƃ�����
	bool		notpoolblock = 0;		// pool�p
	const bool	ispoolbegin	 = !inpool;	// pool�p
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
		meltblock = 0;
	}

	const bool inpool_to_next = (!inmutiarea ? !notpoolblock : false);

	size_t t_statelenm1 = statelenm1;

	size_t i = 0;
	for(i = line; i < t_statelenm1; i++) {
		CStatement& st = statement[i];

		switch(st.type) {
		case ST_OPEN: {					// "{"
			ExecutionInBraceResult info = ExecuteInBrace(i + 1, lvar, BRACE_DEFAULT, exitcode, UpperLvCandidatePool, inpool_to_next);
			i = info.linenum;
			output.Append(info.Output());
			break;
		}
		case ST_CLOSE:					// "}"�@���@�֐��I�[��"}"�͂�����ʂ�Ȃ�
			exec_end = 1;
			break;
		case ST_COMBINE:				// "--"
			output.AddArea();
			break;
		case ST_FORMULA_OUT_FORMULA:	// �o�́i�����B�z��A�������֐����܂܂��j
			output.Append(GetFormulaAnswer(lvar, st));
			break;
		case ST_FORMULA_SUBST:			// ���
			GetFormulaAnswer(lvar, st);
			break;
		case ST_IF:						// if
			ifflg = 0;
			if (GetFormulaAnswer(lvar, st).GetTruth()) {
				ExecutionInBraceResult info = ExecuteInBrace(i + 2, lvar, BRACE_DEFAULT, exitcode, UpperLvCandidatePool, inpool_to_next);
				i = info.linenum;
				output.Append(info.Output());
				ifflg = 1;
			}
			else
				i = st.jumpto;
			break;
		case ST_ELSEIF:					// elseif
			if (ifflg)
				i = st.jumpto;
			else if (GetFormulaAnswer(lvar, st).GetTruth()) {
				ExecutionInBraceResult info = ExecuteInBrace(i + 2, lvar, BRACE_DEFAULT, exitcode, UpperLvCandidatePool, inpool_to_next);
				i = info.linenum;
				output.Append(info.Output());
				ifflg = 1;
			}
			else
				i = st.jumpto;
			break;
		case ST_ELSE:					// else
			if (ifflg)
				i = st.jumpto;
			else {
				ExecutionInBraceResult info = ExecuteInBrace(i + 2, lvar, BRACE_DEFAULT, exitcode, UpperLvCandidatePool, inpool_to_next);
				i = info.linenum;
				output.Append(info.Output());
			}
			break;
		case ST_PARALLEL:				// parallel
			{
		        const CValue& val = GetFormulaAnswer(lvar, st);
				if (val.GetType() == F_TAG_ARRAY) {
					for(size_t j = 0; j < val.array_size(); ++j)
						output.Append(CValue(val.array()[j]));
				}
				else
					output.Append(val);
			}
			break;
		case ST_VOID:				// void
			{
				//���s�������Ď̂Ă�
		        GetFormulaAnswer(lvar, st);
			}
			break;
		case ST_WHILE:					// while
			{
				size_t loop_max = pvm->call_limit().GetMaxLoop();
				size_t loop_cur = 0;

				while ( (loop_max == 0) || (loop_max > loop_cur++) ) {
					if (!GetFormulaAnswer(lvar, st).GetTruth())
						break;
					CValue t_value = ExecuteInBrace(i + 2, lvar, BRACE_LOOP, exitcode, UpperLvCandidatePool, inpool_to_next);
					output.Append(t_value);

					if (exitcode == ST_BREAK) {
						exitcode = ST_NOP;
						break;
					}
					else if (exitcode == ST_RETURN)
						break;
					else if (exitcode == ST_CONTINUE)
						exitcode = ST_NOP;
				}

				if (loop_max && loop_max <= loop_cur ) {
					CBasisFuncPos shiori_OnLoopLimit;
					ptrdiff_t funcpos = shiori_OnLoopLimit.Find(*pvm, L"shiori.OnLoopLimit");

					if (funcpos >= 0) {
						//_argv[0] = dicname _argv[1] = linenum
						CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
						arg.array().emplace_back(CValueSub(dicfilename));
						arg.array().emplace_back(CValueSub(st.linecount));

						pvm->function_exec().func[funcpos].Execute(arg);
					}
					else {
						pvm->logger().Error(E_E, 98, dicfilename, st.linecount);
					}
				}

				i = st.jumpto;
				break;
			}
		case ST_FOR:					// for
			{
				GetFormulaAnswer(lvar, st); //for���p�����[�^

				size_t loop_max = pvm->call_limit().GetMaxLoop();
				size_t loop_cur = 0;

				while ( (loop_max == 0) || (loop_max > loop_cur++) ) {
					if (!GetFormulaAnswer(lvar, statement[i + 1]).GetTruth()) //for���p�����[�^
						break;
					CValue t_value = ExecuteInBrace(i + 4, lvar, BRACE_LOOP, exitcode, UpperLvCandidatePool, inpool_to_next);
					output.Append(t_value);

					if (exitcode == ST_BREAK) {
						exitcode = ST_NOP;
						break;
					}
					else if (exitcode == ST_RETURN)
						break;
					else if (exitcode == ST_CONTINUE)
						exitcode = ST_NOP;

					GetFormulaAnswer(lvar, statement[i + 2]); //for��O�p�����[�^
				}

				if (loop_max && loop_max <= loop_cur ) {
					CBasisFuncPos shiori_OnLoopLimit;
					ptrdiff_t funcpos = shiori_OnLoopLimit.Find(*pvm, L"shiori.OnLoopLimit");

					if (funcpos >= 0) {
						//_argv[0] = dicname _argv[1] = linenum
						CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
						arg.array().emplace_back(CValueSub(dicfilename));
						arg.array().emplace_back(CValueSub(st.linecount));

						pvm->function_exec().func[funcpos].Execute(arg);
					}
					else {
						pvm->logger().Error(E_E, 98, dicfilename, st.linecount);
					}
				}

				i = st.jumpto;
				break;
			}
		case ST_SWITCH: {				// switch
				yaya::int_t sw_index = GetFormulaAnswer(lvar, st).GetValueInt();
				if (sw_index < 0)
					sw_index = BRACE_SWITCH_OUT_OF_RANGE;
				ExecutionInBraceResult info = ExecuteInBrace(i + 2, lvar, sw_index, exitcode, NULL, 0);
				i = info.linenum;
				output.Append(info.Output());
			}
			break;
		case ST_FOREACH:				// foreach
			Foreach(lvar, output, i, exitcode, UpperLvCandidatePool, inpool_to_next);
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
		default:						// ���m�̃X�e�[�g�����g
			pvm->logger().Error(E_E, 82, dicfilename, st.linecount);
			break;
		};
		if (exec_end)
			break;

		if (exitcode != ST_NOP)
			FeedLineToTail(i);
	}
	#undef POOL_TO_NEXT

	// ��₩��o�͂�I�яo���@����q�̐[����0�Ȃ�d�����������
	if (inpool&&!ispoolbegin) {
		std::vector<CValue>& thepool = UpperLvCandidatePool->rbegin()->array;
		std::vector<CValue>& thispool = output.values[0].array;

		if(notpoolblock || inmutiarea)
			thepool.insert(thepool.end(), output.Output());
		else
			thepool.insert(thepool.end(), thispool.begin(), thispool.end());
		output.clear();
	}

	if(meltblock){
		std::vector<CValue>& pool_of_uplv = UpperLvCandidatePool->rbegin()->array;
		CValue result = output.Output();

		if (result.GetType() == F_TAG_ARRAY) {
			for(size_t j = 0; j < result.array_size(); ++j) {
				pool_of_uplv.emplace_back(CValue(result.array()[j]));
			}
		}
		else
			pool_of_uplv.emplace_back(result);
		output.clear();
	}
	// �I�����̏���
	lvar.DelDepth();

	return CFunction::ExecutionInBraceResult( output,i );
}
/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Foreach
 *  �@�\�T�v�F  foreach�������s���܂�
 *  �Ԓl�@�@�F  0/1=���[�v�E�o/���[�v���s
 *
 *  ���ۂɑ���̂�"}"��1��O�̍s�̈ʒu�ł�
 * -----------------------------------------------------------------------
 */
void	CFunction::Foreach(CLocalVariable &lvar, CSelecter &output,size_t line,int &exitcode, std::vector<CVecValue>* UpperLvCandidatePool, bool inpool)
{
	CStatement &st0 = statement[line];
	CStatement &st1 = statement[line + 1];

	// ����l�����߂�
	// ���ӁFforeach���̕���p��������邽�ߕK���Q�Ƃɂ͂��Ȃ�����
	const CValue value = GetFormulaAnswer(lvar, st0);

	// ����l�̗v�f�������߂�
	// �ȈՔz�񂩂ϐ�����̎擾�̏ꍇ�A���̕ϐ��ɐݒ肳��Ă���f���~�^���擾����
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
	int type;
	int fromtype = value.GetType();
	CValueHash::const_iterator hash_iterator = value.hash().begin();
	
	for(int foreachcount = 0; foreachcount < sz; ++foreachcount ) {
		// �������v�f�l���擾
		if (isPseudoarray) {
			t_value = s_array[foreachcount];
		}
		else {// F_TAG_ARRAY
			if ( fromtype == F_TAG_ARRAY ) {
				t_value = value.array()[foreachcount];
			}
			else if ( fromtype == F_TAG_HASH ) {
				t_value.SetType(F_TAG_ARRAY);
				t_value.array().push_back(hash_iterator->first);
				t_value.array().push_back(hash_iterator->second);
				hash_iterator++;
			}
		}

		// ���
		type = st1.cell()[0].value_GetType();
		if ( type == F_TAG_VARIABLE ) {
			pvm->variable().SetValue(st1.cell()[0].index, t_value);
		}
		else if ( type == F_TAG_LOCALVARIABLE ) {
			lvar.SetValue(st1.cell()[0].name, t_value);
		}
		else {
			pvm->logger().Error(E_E, 28, dicfilename, st1.linecount);
			break;
		}

		t_value = ExecuteInBrace(line + 3, lvar, BRACE_LOOP, exitcode, UpperLvCandidatePool, inpool);
		output.Append(t_value);

		if (exitcode == ST_BREAK) {
			exitcode = ST_NOP;
			break;
		}
		else if (exitcode == ST_RETURN)
			break;
		else if (exitcode == ST_CONTINUE)
			exitcode = ST_NOP;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::GetFormulaAnswer
 *  �@�\�T�v�F  ���������Z���Č��ʂ�Ԃ��܂�
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetFormulaAnswer(CLocalVariable &lvar, CStatement &st)
{
	size_t	o_index = 0;

	if ( st.serial_size() ) { //�������p
		for (std::vector<CSerial>::iterator it = st.serial().begin(); it != st.serial().end(); it++) {
			o_index = it->tindex;
			CCell	&o_cell = st.cell()[o_index];
			if (o_cell.value_GetType() >= F_TAG_ORIGIN_VALUE) {
				o_cell.ansv() = GetValueRefForCalc(o_cell, st, lvar);
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
					std_shared_ptr<CValue> tmp_ansv = o_cell.ansv_shared_create();
					if (Comma(*tmp_ansv.get(), it->index, st, lvar)) {
						pvm->logger().Error(E_E, 33, L",", dicfilename, st.linecount);
					}
					o_cell.ansv_shared() = tmp_ansv;
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
					std_shared_ptr<CValue> tmp_ansv = o_cell.ansv_shared_create();
					if (Subst(o_cell.value_GetType(), *tmp_ansv.get(), it->index, st, lvar)) {
						pvm->logger().Error(E_E, 33, L"=", dicfilename, st.linecount);
					}
					o_cell.ansv_shared() = tmp_ansv;
				}
				break;
			case F_TAG_PLUS:
				{
					// ���Z������K����邽�߁A��U���ӂ̌��ʂ�����Ă���E�ӂƉ��Z����
					// �����ӂ�ƉE�ӂ����Ɍv�Z����
					// �ȍ~����

					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv + GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_MINUS:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv - GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_MUL:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv * GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_DIV:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv / GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_SURP:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv % GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFEQUAL:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv == GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFDIFFER:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv != GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFGTEQUAL:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv >= GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFLTEQUAL:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv <= GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFGT:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv > GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFLT:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = lv < GetValueRefForCalc(*d_cell, st, lvar);
					break;
				}
			case F_TAG_IFIN:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv().SetType(F_TAG_INT);
					o_cell.ansv().i_value = _in_(lv,GetValueRefForCalc(*d_cell, st, lvar));
					break;
				}
			case F_TAG_IFNOTIN:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv().SetType(F_TAG_INT);
					o_cell.ansv().i_value = not_in_(lv,GetValueRefForCalc(*d_cell, st, lvar));
					break;
				}
			case F_TAG_OR:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = CValue(lv.GetTruth() || GetValueRefForCalc(*d_cell, st, lvar).GetTruth());
					break;
				}
			case F_TAG_AND:
				{
					const CValue& lv = GetValueRefForCalc(*s_cell, st, lvar);
					o_cell.ansv() = CValue(lv.GetTruth() && GetValueRefForCalc(*d_cell, st, lvar).GetTruth());
					break;
				}
			case F_TAG_FUNCPARAM:
				{
					std_shared_ptr<CValue> tmp_ansv = o_cell.ansv_shared_create();
					if (ExecFunctionWithArgs(*tmp_ansv.get(), it->index, st, lvar)) {
						pvm->logger().Error(E_E, 33, pvm->function_exec().func[st.cell()[it->index[0]].index].name, dicfilename, st.linecount);
					}
					o_cell.ansv_shared() = tmp_ansv;
				}
				break;
			case F_TAG_SYSFUNCPARAM:
				if (ExecSystemFunctionWithArgs(o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, CSystemFunction::GetNameFromIndex(st.cell()[it->index[0]].index), dicfilename, st.linecount);
				break;
			case F_TAG_ARRAYORDER:
				if (Array(o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, L",", dicfilename, st.linecount);
				break;
			case F_TAG_FEEDBACK:
				if (Feedback(o_cell, it->index, st, lvar))
					pvm->logger().Error(E_E, 33, L"&", dicfilename, st.linecount);
				break;
			case F_TAG_EXC:
				o_cell.ansv().SetType(F_TAG_INT);
				o_cell.ansv().i_value = !GetValueRefForCalc(*d_cell, st, lvar).GetTruth();
				break;
			default:
				pvm->logger().Error(E_E, 34, dicfilename, st.linecount);
				return emptyvalue;
			};
		}
	}

	return st.cell()[o_index].ansv();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::GetValueRefForCalc
 *  �@�\�T�v�F  �^����ꂽ���ɑΉ�����l�ւ̃|�C���^���擾���܂�
 * -----------------------------------------------------------------------
 */
const CValue& CFunction::GetValueRefForCalc(CCell &cell, CStatement &st, CLocalVariable &lvar)
{
	// ���l��v�A�֐�/�ϐ�/���Z�q���Ȃ�ansv����擾�@�֐�/�ϐ��̏ꍇ���̒l����s���ʂ��擾�����

	// %[n]����
	if (cell.value_GetType() == F_TAG_SYSFUNCPARAM) {
		if ( cell.index == CSystemFunction::HistoryIndex() ) {
			ExecHistoryP2(cell, st);
		}
	}

	// ���Z���������Ă���i�͂��́j���Ȃ炻���Ԃ�
	if (cell.value_GetType() < F_TAG_ORIGIN_VALUE)
		return cell.ansv();

	// ���l�Ȃ炻������̂܂ܕԂ�
	if (cell.value_GetType() <= F_TAG_STRING)
		return cell.value();

	// �֐��Ȃ���s���Č��ʂ��A�ϐ��Ȃ炻�̓��e��Ԃ�
	switch(cell.value_GetType()) {
	case F_TAG_STRING_EMBED:
		SolveEmbedCell(cell, st, lvar);
		return cell.ansv();
	case F_TAG_SYSFUNC: {
			CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
			std::vector<CCell *> pcellarg; //dummy
			std::vector<CValue> pvaluearg; //dummy
			cell.ansv() =  pvm->sysfunction().Execute(cell.index, arg, pcellarg, pvaluearg, lvar, st.linecount, this);
			return cell.ansv();
		}
	case F_TAG_USERFUNC: {
		CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
		CLocalVariable	t_lvar(*pvm);
		cell.ansv() = pvm->function_exec().func[cell.index].Execute(arg, t_lvar);
		return cell.ansv();
	}
	case F_TAG_VARIABLE:
		return pvm->variable().GetPtr(cell.index)->call_watcher(*pvm,cell.ansv());
	case F_TAG_LOCALVARIABLE:
		return lvar.GetPtr(cell.name)->call_watcher(*pvm,cell.ansv());
	default:
		pvm->logger().Error(E_E, 16, dicfilename, st.linecount);
		return emptyvalue;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ReindexUserFunctions
 *  �@�\�T�v�F  F_TAG_USERFUNC��index���Đݒ肵�܂�
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
 *  �֐���  �F  CFunction::SolveEmbedCell
 *  �@�\�T�v�F  %���ߍ��ݕt�����񍀂̒l�����߂܂�
 * -----------------------------------------------------------------------
 */
void	CFunction::SolveEmbedCell(CCell &cell, CStatement &st, CLocalVariable &lvar)
{
	// ��������ʒu�����߂�
	int	solve_src;	// ��� 0/1/2/3=���[�J���ϐ�/�ϐ�/�֐�/�V�X�e���֐�
	size_t	max_len = 0;	// �Œ���v�����p

	if (cell.value_const().s_value[0] == L'_') {
		// ���[�J���ϐ�
		solve_src = 0;
		max_len   = lvar.GetMacthedLongestNameLength(cell.value_const().s_value);
	}
	else {
		// �ϐ�
		solve_src = 1;
		max_len   = pvm->variable().GetMacthedLongestNameLength(cell.value_const().s_value);
		// �֐�
		size_t	t_len = 0;
		for(std::vector<CFunction>::iterator it = pvm->function_exec().func.begin(); it != pvm->function_exec().func.end(); it++)
			if (!it->name.compare(0,it->namelen,cell.value_const().s_value,0,it->namelen))
				if (t_len < it->namelen)
					t_len = it->namelen;
		if (t_len > max_len) {
			solve_src = 2;
			max_len   = t_len;
		}
		// �V�X�e���֐�
		if ( max_len < (size_t)CSystemFunction::GetMaxNameLength() ) {
			t_len = CSystemFunction::FindIndexLongestMatch(cell.value_const().s_value,max_len);
			if (t_len > max_len) {
				solve_src = 3;
				max_len   = t_len;
			}
		}
	}
	// ���݂��Ȃ���ΑS�̂�������Ƃ������ƂɂȂ�
	if (!max_len) {
		cell.ansv()     = L"%" + cell.value_const().s_value;
		cell.emb_ansv() = L"%" + cell.value_const().s_value;
		return;
	}

	// �֐�/�V�X�e���֐��̏ꍇ�͈���������T��
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

	// �z�񕔕���T��
	size_t	depth = 1;
	size_t i = 0;
	for(i = max_len + 1; i < len; i++) {
		if (!depth && cell.value_const().s_value[i] != L'[')
			break;
		depth += ((cell.value_const().s_value[i] == L'[') - (cell.value_const().s_value[i] == L']'));
	}
	if (!depth)
		max_len = i;

	// ���ߍ��܂ꂽ�v�f�Ƃ���ȍ~�̕�����ɕ�������
	//yaya::string_t	s_value(cell.value_const().s_value.substr(0, max_len));
	//yaya::string_t	d_value(cell.value_const().s_value.substr(max_len, len - max_len));
	yaya::string_t::const_iterator it_split = cell.value_const().s_value.begin() + max_len;
	yaya::string_t s_value(cell.value_const().s_value.begin(),it_split);
	yaya::string_t d_value(it_split,cell.value_const().s_value.end());

	// ���ߍ��܂ꂽ�v�f�𐔎��ɕϊ�����@���s�Ȃ�S�̂�������
	CStatement	t_state(ST_FORMULA, st.linecount);
	if (pvm->parser0().ParseEmbedString(s_value, t_state, dicfilename, st.linecount)) {
		cell.ansv()       = L"%" + cell.value_const().s_value;
		cell.emb_ansv() = L"%" + cell.value_const().s_value;
		return;
	}

	// ���ߍ��ݗv�f�̒l���擾���ĉ�����������쐬
	yaya::string_t	result = GetFormulaAnswer(lvar, t_state).GetValueString();
	cell.emb_ansv()  = result;
	cell.ansv()      = result + d_value;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Comma
 *  �@�\�T�v�F  ,���Z�q���������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::Comma(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	// �������Ĕz��l���쐬
	CValueArray	t_array;

	for(std::vector<size_t>::iterator it = sid.begin(); it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			t_array.insert(t_array.end(), addv.array().begin(), addv.array().end());
		}
		else {
			t_array.emplace_back(CValueSub(addv));
		}
	}

	answer.SubstToArray(t_array);
	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::CommaAdd
 *  �@�\�T�v�F  ,=���Z�q���������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::CommaAdd(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	if ( answer.GetType() != F_TAG_ARRAY ) {
		CValueSub st(answer);
		answer.SetType(F_TAG_ARRAY);
		answer.array().emplace_back(st);
	}
	CValueArray &t_array = answer.array();

	std::vector<size_t>::iterator it = sid.begin();
	it++; //�ŏ������ӂ͑����Ȃ̂Ŕ�΂�

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			t_array.insert(t_array.end(), addv.array().begin(), addv.array().end());
		}
		else {
			t_array.emplace_back(CValueSub(addv));
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Subst
 *  �@�\�T�v�F  ������Z�q���������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::Subst(int type, CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	*sid_0_cell = &(st.cell()[sid[0]]);
	CCell	*sid_1_cell = &(st.cell()[sid[1]]);

	int sid_0_cell_type = sid_0_cell->value_GetType();

	//�����ϐ��ւ̑���̏ꍇ�����͓��ꈵ������
	if ( sid_0_cell_type == F_TAG_VARIABLE || sid_0_cell_type == F_TAG_LOCALVARIABLE ) {
		CVariable* pSubstTo;

		if ( sid_0_cell_type == F_TAG_VARIABLE ) {
			pSubstTo = pvm->variable().GetPtr(sid_0_cell->index);
		}
		else {
			pSubstTo = lvar.GetPtr(sid_0_cell->name);
		}

		CValue varback;

		if ( pSubstTo ) {
			varback = pSubstTo->value();
			CValue &substTo = pSubstTo->value();

			answer.array_clear();

			switch(type) {
			case F_TAG_EQUAL:
			case F_TAG_EQUAL_D:
				substTo = GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;
			case F_TAG_PLUSEQUAL:
			case F_TAG_PLUSEQUAL_D:
				substTo += GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;
			case F_TAG_MINUSEQUAL:
			case F_TAG_MINUSEQUAL_D:
				substTo -= GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;
			case F_TAG_MULEQUAL:
			case F_TAG_MULEQUAL_D:
				substTo *= GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;
			case F_TAG_DIVEQUAL:
			case F_TAG_DIVEQUAL_D:
				substTo /= GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;
			case F_TAG_SURPEQUAL:
			case F_TAG_SURPEQUAL_D:
				substTo %= GetValueRefForCalc(*sid_1_cell, st, lvar);
				break;

				//�J���}���ꏈ��
			case F_TAG_COMMAEQUAL:
				if (CommaAdd(substTo, sid, st, lvar)) {
					return 1;
				}
				break;
			default:
				return 1;
			};

			// **HACK** const�ɂ���array�̏ꍇanswer�Ƌ������L
			// ��ő�����Z���������������ɔz��̎��̑���R�X�g���ȗ��ł���
			answer = const_cast<const CValue&>(substTo);

			// �O���[�o���ϐ��̏ꍇ�A�폜�ς݂̏ꍇ������̂ł����ō�Enable
			if ( sid_0_cell_type == F_TAG_VARIABLE ) {
				pvm->variable().EnableValue(sid_0_cell->index);
			}

			pSubstTo->call_setter(*pvm,varback);

			return 0;
		}
	}

	// ������̒l���擾�@���Z�q���Ȃ牉�Z���s��
	switch(type) {
	case F_TAG_EQUAL:
	case F_TAG_EQUAL_D:
		answer = GetValueRefForCalc(*sid_1_cell, st, lvar);
		break;
	case F_TAG_PLUSEQUAL:
	case F_TAG_PLUSEQUAL_D:
		{
			// ���Z������K����邽�߁A��U���ӂ̌��ʂ�����Ă���E�ӂƉ��Z����
			// �����ӂ�ƉE�ӂ����Ɍv�Z����
			// �ȍ~����
			const CValue &lv = GetValueRefForCalc(*sid_0_cell, st, lvar);
			answer = lv + GetValueRefForCalc(*sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_MINUSEQUAL:
	case F_TAG_MINUSEQUAL_D:
		{
			const CValue &lv = GetValueRefForCalc(*sid_0_cell, st, lvar);
			answer = lv - GetValueRefForCalc(*sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_MULEQUAL:
	case F_TAG_MULEQUAL_D:
		{
			const CValue &lv = GetValueRefForCalc(*sid_0_cell, st, lvar);
			answer = lv * GetValueRefForCalc(*sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_DIVEQUAL:
	case F_TAG_DIVEQUAL_D:
		{
			const CValue &lv = GetValueRefForCalc(*sid_0_cell, st, lvar);
			answer = lv / GetValueRefForCalc(*sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_SURPEQUAL:
	case F_TAG_SURPEQUAL_D:
		{
			const CValue &lv = GetValueRefForCalc(*sid_0_cell, st, lvar);
			answer = lv % GetValueRefForCalc(*sid_1_cell, st, lvar);
			break;
		}
	case F_TAG_COMMAEQUAL:
		if (Comma(answer, sid, st, lvar))
			return 1;
		break;
	default:
		return 1;
	};

	// ��������s
 	// �z��v�f�ւ̑���͑��삪���G�Ȃ̂ŁA����ɑ��̊֐��֏�����n��
	switch(sid_0_cell->value_GetType()) {
	case F_TAG_VARIABLE:
		pvm->variable().SetValue(sid_0_cell->index, answer);
		return 0;
	case F_TAG_LOCALVARIABLE:
		lvar.SetValue(sid_0_cell->name, answer);
		return 0;
	case F_TAG_ARRAYORDER: {
			if (sid[0] > 0)
				return SubstToArray(st.cell()[sid[0] - 1], *sid_0_cell, answer, st, lvar);
			else
				return 1;
		}
	default:
 		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::SubstToArray
 *  �@�\�T�v�F  �z��v�f�ւ̑�����������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::SubstToArray(CCell &vcell, CCell &ocell, CValue &answer, CStatement &st, CLocalVariable &lvar)
{
	// �������擾
	CValue	t_order;
	EncodeArrayOrder(vcell, ocell.order(), lvar, t_order);

	if (t_order.GetType() == F_TAG_UNKNOWN)
		return 1;

	// �l���擾
	CValue	value = GetValueRefForCalc(vcell, st, lvar);

	// �X�V
    if (value.GetType() == F_TAG_HASH) {
        if (answer.GetType() == F_TAG_HASH) {
            if (answer.hash().empty()) {
                value.hash().erase(t_order.array()[0]);
            }
        }
        else {
            value.hash()[CValueSub(t_order.array()[0])] = CValueSub(answer);
        }
    }
    else {
	    value.SetArrayValue(t_order, answer);
    }

	// ���
	switch(vcell.value_GetType()) {
	case F_TAG_VARIABLE:
		pvm->variable().SetValue(vcell.index, value);
		return 0;
	case F_TAG_LOCALVARIABLE:
		lvar.SetValue(vcell.name, value);
		return 0;
	default:
		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Array
 *  �@�\�T�v�F  �z��[]���Z�q���������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::Array(CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	*v_cell = &(st.cell()[sid[0]]);
	CCell	*n_cell = &(st.cell()[sid[1]]);

	// �������擾
	anscell.order() = GetValueRefForCalc(*n_cell, st, lvar);

	CValue	t_order;
	EncodeArrayOrder(*v_cell, anscell.order(), lvar, t_order);

	if (t_order.GetType() == F_TAG_UNKNOWN) {
		anscell.ansv().SetType(F_TAG_VOID);
		return 1;
	}

	// �l���擾
	anscell.ansv() = GetValueRefForCalc(*v_cell, st, lvar)[t_order];

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::_in_
 *  �@�\�T�v�F  _in_���Z�q���������܂�
 * -----------------------------------------------------------------------
 */
bool CFunction::_in_(const CValue &src, const CValue &dst)
{
	if (src.IsString() && dst.IsString())
		return (dst.s_value.find(src.s_value) != yaya::string_t::npos) ? 1 : 0;

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::not_in_
 *  �@�\�T�v�F  !_in_���Z�q���������܂�
 * -----------------------------------------------------------------------
 */
bool CFunction::not_in_(const CValue &src, const CValue &dst)
{
	return ! _in_(src,dst);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ExecFunctionWithArgs
 *  �@�\�T�v�F  �����t���̊֐������s���܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::ExecFunctionWithArgs(CValue &answer, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	// �֐��̊i�[�ʒu���擾
	std::vector<size_t>::iterator it = sid.begin();
	size_t index = st.cell()[*it].index;
	it++;

	// �����쐬
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);	
	std::vector<size_t>::size_type sidsize = sid.size();

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			if ( sidsize <= 2 ) { //�z��1�݂̂��^�����Ă���->�œK���̂��߃X�}�[�g�|�C���^����݂̂ōς܂���
				arg.array_shared() = addv.array_shared();
			}
			else {
				arg.array().insert(arg.array().end(), addv.array().begin(), addv.array().end());
			}
		}
		else {
			arg.array().emplace_back(CValueSub(addv));
		}
	}

	// ���s
	CLocalVariable	t_lvar(*pvm);
	answer = pvm->function_exec().func[index].Execute(arg, t_lvar);

	// �t�B�[�h�o�b�N
	const CValue *v_argv = &(t_lvar.GetArgvPtr()->value_const());
	int	i = 0;
	int	errcount = 0;

	for(it = sid.begin() + 1; it != sid.end(); it++, i++) {
		if (st.cell()[*it].value_GetType() == F_TAG_FEEDBACK) {
			CValue	v_value;
			v_value = v_argv->array()[i];

			if (st.cell()[*it].order_const().GetType() != F_TAG_NOP)
				errcount += SubstToArray(st.cell()[(*it) + 1], st.cell()[*it], v_value, st, lvar);
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

	assert((errcount == 0)||(errcount == 1));
	return errcount ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ExecSystemFunctionWithArgs
 *  �@�\�T�v�F  �����t���̃V�X�e���֐������s���܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::ExecSystemFunctionWithArgs(CCell& cell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	// �֐��̊i�[�ʒu���擾
	std::vector<size_t>::iterator it = sid.begin();
	size_t	func_index = *it;
	size_t	index = st.cell()[func_index].index;
	it++;

	// �����쐬
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	std::vector<CCell *> pcellarg;
	std::vector<CValue> valuearg;
	std::vector<size_t>::size_type sidsize = sid.size();

	for( ; it != sid.end(); it++) {
		const CValue &addv = GetValueRefForCalc(st.cell()[*it], st, lvar);
		
		if (addv.GetType() == F_TAG_ARRAY) {
			if ( sidsize <= 2 ) { //�z��1�݂̂��^�����Ă���->�œK���̂��߃X�}�[�g�|�C���^����݂̂ōς܂���
				arg.array_shared() = addv.array_shared();
			}
			else {
				arg.array().insert(arg.array().end(), addv.array().begin(), addv.array().end());
			}
		}
		else {
			arg.array().emplace_back(CValueSub(addv));
		}

		valuearg.emplace_back(addv);
		pcellarg.emplace_back(&(st.cell()[*it]));
	}

	// ���s�@%[n]�����֐��̂ݓ��ስ��
	if (index == CSystemFunction::HistoryIndex())
		ExecHistoryP1(func_index - 2, cell, arg, st);
	else
		cell.ansv() = pvm->sysfunction().Execute(index, arg, pcellarg, valuearg, lvar, st.linecount, this);

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ExecHistoryP1
 *  �@�\�T�v�F  %[n]�i�u���ς̒l�̍ė��p�j���������܂��i�O�����j
 *
 *  �����͓�i�K�ōs���܂��B�O�����ł͖{�����̂��߂̒l�����Z�q�̍��փZ�b�g���܂��B
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecHistoryP1(size_t start_index, CCell& cell, const CValue &arg, CStatement &st)
{
	if (arg.array_size()) {
		cell.ansv()    = CValue((yaya::int_t)start_index);
		cell.order()   = arg.array()[0];
	}
	else {
		pvm->logger().Error(E_E, 90, dicfilename, st.linecount);
		cell.ansv().SetType(F_TAG_VOID);
		cell.order().SetType(F_TAG_VOID);
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::ExecHistoryP2
 *  �@�\�T�v�F  %[n]�i�u���ς̒l�̍ė��p�j���������܂��i�{�����j
 *
 *  �����͓�i�K�ōs���܂��B�{�����ł͑O�����Ŗ��ߍ��񂾒l���Q�Ƃ��Ēl���擾���܂��B
 * -----------------------------------------------------------------------
 */
void	CFunction::ExecHistoryP2(CCell& cell, CStatement &st)
{
	if (!cell.order_const().IsNum())
		return;

	yaya::int_t	index = cell.order_const().GetValueInt();
	if (index < 0)
		return;

	ptrdiff_t start = (ptrdiff_t)std::min(yaya::int_t(st.cell().size())-1,cell.ansv_const().GetValueInt());

	for(ptrdiff_t i = start ; i >= 0; i--) {
		if (st.cell()[i].value_GetType() == F_TAG_STRING_EMBED) {
			if (!index) {
				cell.ansv_shared() = st.cell()[i].emb_shared_get();
				return;
			}
			index--;
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::Feedback
 *  �@�\�T�v�F  &���Z�q���������܂�
 *
 *  �Ԓl�@�@�F  0/1=����/�G���[
 * -----------------------------------------------------------------------
 */
char	CFunction::Feedback(CCell &anscell, std::vector<size_t> &sid, CStatement &st, CLocalVariable &lvar)
{
	CCell	*v_cell = &(st.cell()[sid[1]]);

	// �l�͉E�ӂ����̂܂܃R�s�[
	anscell.ansv() = GetValueRefForCalc(*v_cell, st, lvar);

	// �E�ӂ��z�񏘐����w�肷�鉉�Z�q�������ꍇ�͂������珘�����R�s�[
	// �z��łȂ������ꍇ�͏������i�[����ϐ��̌^��NOP�ɂ��ăt���O�Ƃ���
	if (v_cell->value_GetType() == F_TAG_ARRAYORDER)
		anscell.order_shared() = v_cell->order_shared();
	else
		anscell.order().SetType(F_TAG_NOP);

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::EncodeArrayOrder
 *  �@�\�T�v�F  �z��̏������쐬���ĕԂ��܂�
 *
 *  CValue operator [] �͈����Ƃ��ėv�f��2�ȏ�̔z��^��CValue��v�����܂��B
 *  �i�������������A���������f���~�^�j
 *  ���̊֐��͂�����쐬���܂��B
 *
 *  �G���[�����������ꍇ�͌^�̂Ȃ��iF_TAG_UNKNOWN�j�l��Ԃ��܂��B�i�Ăяo�����͂�������ăG���[�������܂��j
 * -----------------------------------------------------------------------
 */
void CFunction::EncodeArrayOrder(CCell &vcell, const CValue &order, CLocalVariable &lvar, CValue &result)
{
	result.SetType(F_TAG_ARRAY);

	// ����
	switch(order.GetType()) {
	case F_TAG_ARRAY:
		result = order;
		break;
	default:
		result.array().emplace_back(CValueSub(order));
		break;
	};

	// �f���~�^
	if (result.array_size() < 2) {
		CValueSub	adddlm(VAR_DELIMITER);
		if (vcell.value_GetType() == F_TAG_VARIABLE)
			adddlm = pvm->variable().GetDelimiter(vcell.index);
		else if (vcell.value_GetType() == F_TAG_LOCALVARIABLE)
			adddlm = lvar.GetDelimiter(vcell.name);
		result.array().emplace_back(adddlm);
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunction::FeedLineToTail
 *  �@�\�T�v�F  ���݂�{}�̏I�["}"�܂Ŏ��s�s�J�E���^�𑗂�܂�
 *
 *  ���ۂɑ���̂�"}"��1��O�̍s�̈ʒu�ł�
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
 *  �֐���  �F  CFunctionDef::GetFunctionIndexFromName
 *  �@�\�T�v�F  �֐����ɑΉ�����z��̏������擾���܂�
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
 *  �֐���  �F  CFunctionDef::AddFunctionIndex
 *  �@�\�T�v�F  �֐����ɑΉ�����z��̏�����ǉ����܂�
 * -----------------------------------------------------------------------
 */
void CFunctionDef::AddFunctionIndex(const yaya::string_t& name,size_t index)
{
	map.insert(yaya::indexmap::value_type(name, index));
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunctionDef::ClearFunctionIndex
 *  �@�\�T�v�F  �z��̏����L���b�V���p�n�b�V�����폜���܂�
 * -----------------------------------------------------------------------
 */
void CFunctionDef::ClearFunctionIndex(void)
{
	map.clear();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFunctionDef::RebuildFunctionMap
 *  �@�\�T�v�F  �z��̏����L���b�V���p�n�b�V�����\�z���܂�
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
