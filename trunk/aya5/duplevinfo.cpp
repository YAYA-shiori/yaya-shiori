// 
// AYA version 5
//
// �d����𐧌���s�Ȃ��N���X�@CDuplEvInfo
// - �又����
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <functional>

#include "selecter.h"

#include "log.h"
#include "zsfmt.h"
#include "globaldef.h"
#include "sysfunc.h"
#include "ayavm.h"

/* -----------------------------------------------------------------------
 *  �֐���  �F  CDuplEvInfo::Choice
 *  �@�\�T�v�F  ��₩��I�����ďo�͂��܂�
 * -----------------------------------------------------------------------
 */
CValue	CDuplEvInfo::Choice(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values, int mode)
{
	// �̈斈�̌�␔�Ƒ������X�V�@�ω����������ꍇ�͏��񏇏�������������
	if (UpdateNums(areanum, values))
		InitRoundOrder(vm,mode);

	// �l�̎擾�Ə��񐧌�
	CValue	result = GetValue(vm, areanum, values);

	lastroundorder = roundorder[index];

	// ����ʒu��i�߂�@���񂪊��������珄�񏇏�������������
	index++;
	if (index >= static_cast<int>(roundorder.size()) )
		InitRoundOrder(vm,mode);

	return result;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CDuplEvInfo::InitRoundOrder
 *  �@�\�T�v�F  ���񏇏������������܂�
 * -----------------------------------------------------------------------
 */
void	CDuplEvInfo::InitRoundOrder(CAyaVM &vm,int mode)
{
	// ������
	index = 0;
	roundorder.clear();
	roundorder.reserve(total);

	for(int i = 0; i < total; ++i) {
		if ( i != lastroundorder ) {
			roundorder.push_back(i);
		}
	}

	//�ً}���G���[���p
	if ( ! roundorder.size() ) {
		roundorder.push_back(0);
	}

    if (mode == CHOICETYPE_NONOVERLAP) {
		int n = roundorder.size();
		for ( int i = 0 ; i < n ; ++i ) {
			int s = vm.genrand_int(n);
			if ( i != s ) {
				int tmp = roundorder[i];
				roundorder[i] = roundorder[s];
				roundorder[s] = tmp;
			}
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CDuplEvInfo::UpdateNums
 *  �@�\�T�v�F  �̈斈�̌�␔�Ƒ������X�V���܂�
 *  �Ԓl�@�@�@  0/1=�ω��Ȃ�/����
 * -----------------------------------------------------------------------
 */
char	CDuplEvInfo::UpdateNums(int areanum, const std::vector<CVecValue> &values)
{
	// ���̌�␔��ۑ����Ă���
	std::vector<int>	bef_num(num.begin(), num.end());
	int	bef_numlenm1 = bef_num.size() - 1;

	// �̈斈�̌�␔�Ƒg�ݍ��킹�������X�V
	// ��␔�ɕω����������ꍇ�̓t���O�ɋL�^����
	num.clear();
	total = 1;
	char	changed = (areanum != bef_numlenm1) ? 1 : 0;
	for(int i = 0; i <= areanum; i++) {
		int	t_num = values[i].array.size();
		num.push_back(t_num);
		total *= t_num;
		if (i <= bef_numlenm1)
			if (bef_num[i] != t_num)
				changed = 1;
	}

	return changed;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CDuplEvInfo::GetValue
 *  �@�\�T�v�F  ���݂̏���ʒu����l���擾���܂�
 *
 *  �i�[�̈悪������Ȃ��ꍇ�͂�������̂܂܏o���̂Œl�̌^���ی삳��܂��B
 *  �̈悪��������ꍇ�͂����͕�����Ƃ��Č�������܂��̂ŁA������^�ł̏o�͂ƂȂ�܂��B
 * -----------------------------------------------------------------------
 */
CValue	CDuplEvInfo::GetValue(CAyaVM &vm,int areanum, const std::vector<CVecValue> &values)
{
	int	t_index = roundorder[index];
	
	vm.sysfunction().SetLso(t_index);

	if (areanum) {
		yaya::string_t	result;
		for ( int i = 0; i <= areanum; i++ ) {
			if ( num[i] ) {
				int	next = t_index/num[i];
				result += values[i].array[t_index - next*(num[i])].GetValueString();
				t_index = next;
			}
		}
		return CValue(result);
	}
	else
		return values[0].array[t_index];
}
