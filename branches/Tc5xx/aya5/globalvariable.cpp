// 
// AYA version 5
//
// �O���[�o���ϐ��������N���X�@CGlobalVariable
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "variable.h"

#include "ayavm.h"
#include "basis.h"
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
 *  �֐���  �F  CGlobalVariable::Make
 *  �@�\�T�v�F  �ϐ����쐬���܂�
 *  �����@�@�F  erased 0/1=�L�����/�������
 *
 *  �Ԓl�@�@�F  �ǉ������ʒu
 *
 *  �w�肳�ꂽ���O�̕ϐ������ɑ��݂��Ă����ꍇ�͒ǉ��͍s�킸�A�����̈ʒu��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
int	CGlobalVariable::Make(const yaya::string_t &name, char erased)
{
	// ���ɑ��݂��邩���ׁA���݂��Ă����炻���Ԃ�
	yaya::indexmap::const_iterator it = varmap.find(name);
	if ( it != varmap.end() ) {
		int idx = it->second;
		if (!vm.basis().IsRun()) {
			if (erased)
				var[idx].Erase();
			else
				var[idx].Enable();
		}
		return idx;
	}

	// �쐬
	CVariable	addvariable(name);
	if (erased)
		addvariable.Erase();
	else
		addvariable.Enable();
	var.push_back(addvariable);

	int idx = var.size() - 1;
	varmap.insert(yaya::indexmap::value_type(name,idx));
	return idx;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CGlobalVariable::GetMacthedLongestNameLength
 *  �@�\�T�v�F  �w�肳�ꂽ������Ƀ}�b�`���閼�O�����ϐ���T���A�}�b�`����������Ԃ��܂�
 *
 *  �������������ꍇ�͍Œ��̂��̂�Ԃ��܂��B������Ȃ������ꍇ��0��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
size_t	CGlobalVariable::GetMacthedLongestNameLength(const yaya::string_t &name)
{
	size_t	max_len = 0;

	for(std::vector<CVariable>::iterator it = var.begin(); it != var.end(); it++) {
		size_t	len = it->name.size();
		if (!it->IsErased() && max_len < len && !name.compare(0,len,it->name,0,len))
			max_len = len;
	}

	return max_len;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CGlobalVariable::GetIndex
 *  �@�\�T�v�F  �w�肳�ꂽ���O�̕ϐ��̈ʒu��Ԃ��܂�
 *
 *  ������Ȃ������ꍇ��-1��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
int		CGlobalVariable::GetIndex(const yaya::string_t &name)
{
	yaya::indexmap::const_iterator it = varmap.find(name);
	if ( it != varmap.end() ) {
		int idx = it->second;
		return var[idx].IsErased() ? -1 : idx;
	}
	return -1;
}
