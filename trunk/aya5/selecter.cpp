// 
// AYA version 5
//
// �o�͂̑I�����s�Ȃ��N���X�@CSelecter
// - �又����
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "selecter.h"

#include "globaldef.h"
#include "sysfunc.h"
#include "ayavm.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 * CSelecter�R���X�g���N�^
 * -----------------------------------------------------------------------
 */
CSelecter::CSelecter(CAyaVM &vmr, CDuplEvInfo *dc, int aid) : vm(vmr), duplctl(dc), aindex(aid)
{
	areanum = 0;

	CVecValue	addvv;
	values.push_back(addvv);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::AddArea
 *  �@�\�T�v�F  �V�����o�͊i�[�p�̗̈��p�ӂ��܂�
 * -----------------------------------------------------------------------
 */
void	CSelecter::AddArea(void)
{
	// �ǉ��O�̗̈悪�󂾂����ꍇ�̓_�~�[�̋󕶎����ǉ�
	if (!values[areanum].array.size())
		Append(CValue());

	// �̈��ǉ�
	CVecValue	addvv;
	values.push_back(addvv);
	areanum++;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::Append
 *  �@�\�T�v�F  ���l��ǉ����܂�
 * -----------------------------------------------------------------------
 */
void	CSelecter::Append(const CValue &value)
{
	//�󕶎���͂���ς�ǉ����Ȃ��Ƃ܂���
	//if (value.GetType() == F_TAG_STRING && !value.s_value.size())
	//	return;
	if (value.GetType() == F_TAG_VOID)
		return;

	values[areanum].array.push_back(value);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::Output
 *  �@�\�T�v�F  �e�̈悩��l�𒊏o���ďo�͂��쐬���Ԃ��܂�
 *  �����@�@�F  duplctl �d��������ւ̃|�C���^
 *  �@�@�@�@�@          NULL�ɏꍇ�̓����_���I���œ��삵�܂�
 *
 *  �����_���I���͂��̃N���X�ŏ������܂��B
 *  nonoverlao/sequential�I����CDuplEvInfo�N���X�ɔC���܂��B
 * -----------------------------------------------------------------------
 */
#include <iostream>
CValue	CSelecter::Output(void)
{
	// switch�I��
	if (aindex >= BRACE_SWITCH_OUT_OF_RANGE) {
		vm.sysfunction().SetLso(aindex);
		return ChoiceByIndex();
	}

	// �̈悪1�����Ȃ��A����₪���݂��Ȃ��ꍇ�͏o�͂Ȃ�
	if (!areanum && !values[0].array.size()) {
		vm.sysfunction().SetLso(-1);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	// �Ō�̗̈悪�󂾂����ꍇ�̓_�~�[�̋󕶎����ǉ�
	if (!values[areanum].array.size())
		Append(CValue());

	// �����_���I��
	if (duplctl == NULL)
		return ChoiceRandom();

	// �d����𐧌�t���I��
	switch(duplctl->GetType()) {
	case CHOICETYPE_NONOVERLAP:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_NONOVERLAP);
	case CHOICETYPE_SEQUENTIAL:
		return duplctl->Choice(vm, areanum, values, CHOICETYPE_SEQUENTIAL);
	case CHOICETYPE_VOID:
		return CValue(F_TAG_NOP, 0/*dmy*/);
	case CHOICETYPE_ARRAY:
		return StructArray();
	case CHOICETYPE_RANDOM:
	default:
		return ChoiceRandom();
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::ChoiceRandom
 *  �@�\�T�v�F  �e�̈悩�烉���_���ɒl�𒊏o���ďo�͂��쐬���Ԃ��܂�
 *
 *  �i�[�̈悪������Ȃ��ꍇ�͂�������̂܂܏o���̂Œl�̌^���ی삳��܂��B
 *  �̈悪��������ꍇ�͂����͕�����Ƃ��Č�������܂��̂ŁA������^�ł̏o�͂ƂȂ�܂��B
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceRandom(void)
{
	if (areanum) {
		yaya::string_t	result;
		for(int i = 0; i <= areanum; i++)
			result += ChoiceRandom1(i).GetValueString();
		return CValue(result);
	}
	else
		return ChoiceRandom1(0);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::ChoiceRandom1
 *  �@�\�T�v�F  �w�肳�ꂽ�̈悩�烉���_���ɒl�𒊏o���܂�
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceRandom1(int index)
{
	if ( ! values[index].array.size() ) {
		return CValue();
	}

    int choice = vm.genrand_int(static_cast<int>(values[index].array.size()));

    vm.sysfunction().SetLso(choice);

    return values[index].array[choice];
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::ChoiceByIndex
 *  �@�\�T�v�F  �e�̈悩��w��ʒu�̒l�𒊏o���ďo�͂��쐬���Ԃ��܂�
 *
 *  �i�[�̈悪������Ȃ��ꍇ�͂�������̂܂܏o���̂Œl�̌^���ی삳��܂��B
 *  �̈悪��������ꍇ�͂����͕�����Ƃ��Č�������܂��̂ŁA������^�ł̏o�͂ƂȂ�܂��B
 *
 *  �w�����␔�����Ȃ��ꍇ�͋󕶎��񂪏o�͂���܂��B
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceByIndex()
{
	// �Ō�̗̈悪�󂾂����ꍇ�̓_�~�[�̋󕶎����ǉ�
	if (!values[areanum].array.size())
		Append(CValue());

	// �又��
	if (areanum) {
		yaya::string_t	result;
		for(int i = 0; i <= areanum; i++)
			result += ChoiceByIndex1(i).GetValueString();
		return CValue(result);
	}
	else
		return ChoiceByIndex1(0);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::ChoiceByIndex
 *  �@�\�T�v�F  �w�肳�ꂽ�̈悩��w��ʒu�̒l�𒊏o���܂�
 * -----------------------------------------------------------------------
 */
CValue	CSelecter::ChoiceByIndex1(int index)
{
	int	num = values[index].array.size();

	if (!num)
		return CValue();

	return (aindex >= 0 && aindex < num) ? values[index].array[aindex] : CValue();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::StructArray
 *  �@�\�T�v�F  �e�̈�̒l�����������ėp�z����쐬���Ԃ��܂�
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructArray()
{
	if (areanum) {
		CValue	result(F_TAG_ARRAY, 0/*dmy*/);
		for(int i = 0; i <= areanum; i++)
			result = result + StructArray1(i);
//			result = result + StructArray1(i).GetValueString();
		return result;
	}
	else
		return StructArray1(0);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSelecter::StructArray1
 *  �@�\�T�v�F  �w�肳�ꂽ�̈�̒l�����������ėp�z����쐬���܂�
 * -----------------------------------------------------------------------
 */
CValue CSelecter::StructArray1(int index)
{
	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

    for(size_t i = 0; i < values[index].array.size(); ++i) {
		const CValue &target = values[index].array[i];
		int	valtype = target.GetType();
		
		if (valtype == F_TAG_ARRAY) {
			result.array().insert(result.array().end(), target.array().begin(), target.array().end());
		}
		else {
			result.array().push_back(CValueSub(target));
		}
	}

    return result;
}
