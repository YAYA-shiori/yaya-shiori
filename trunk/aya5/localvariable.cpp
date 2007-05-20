// 
// AYA version 5
//
// ���[�J���ϐ��������N���X�@CLocalVariable
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "globaldef.h"
#include "variable.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  CLocalVariable�R���X�g���N�^
 * -----------------------------------------------------------------------
 */
CLocalVariable::CLocalVariable(void)
{
	depth = -1;

	AddDepth();
}

/* -----------------------------------------------------------------------
 *  CLocalVariable�f�X�g���N�^
 * -----------------------------------------------------------------------
 */
CLocalVariable::~CLocalVariable(void)
{
	stack.clear();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetArgvAdr
 *  �@�\�T�v�F  _argv���i�[����Ă���ʒu�̃|�C���^��Ԃ��܂�
 *
 *  vector�͍Ċ��蓖�Ăɂ���ăA�h���X���ω�����̂ŁA�����œ�����l��
 *  �ꎞ�I�ɗ��p���邱�Ƃ����o���܂���B
 * -----------------------------------------------------------------------
 */
CVariable	*CLocalVariable::GetArgvPtr(void)
{
	return &(stack[0].substack[0]);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::AddDepth
 *  �@�\�T�v�F  ��K�w�[�����[�J���ϐ��X�^�b�N���쐬���܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::AddDepth(void)
{
	CLVSubStack	addsubstack;
	stack.push_back(addsubstack);
	depth++;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::DelDepth
 *  �@�\�T�v�F  �ł��[�����[�J���ϐ��X�^�b�N��j�����܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::DelDepth(void)
{
	if (stack.size()) {
		stack.erase(stack.end() - 1);
		depth--;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::Make
 *  �@�\�T�v�F  ���[�J���ϐ����쐬���܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::Make(const yaya::char_t *name)
{
	CVariable	addlv(name);
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name)
{
	CVariable	addlv(name);
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::char_t *name, const CValue &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const CValue &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const CValueSub &value)
{
	CVariable	addlv(name);
	addlv.value() = value;
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::char_t *name, const yaya::string_t &delimiter)
{
	CVariable	addlv(name);
	addlv.delimiter = delimiter;
	stack[depth].substack.push_back(addlv);
}

//----

void	CLocalVariable::Make(const yaya::string_t &name, const yaya::string_t &delimiter)
{
	CVariable	addlv(name);
	addlv.delimiter = delimiter;
	stack[depth].substack.push_back(addlv);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetIndex
 *  �@�\�T�v�F  �w�肳�ꂽ���O�̃��[�J���ϐ����X�^�b�N���猟�����܂�
 *
 *  ������Ȃ������ꍇ��-1��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::GetIndex(const yaya::char_t *name, int &id, int &dp)
{
	for(int i = stack.size() - 1; i >= 0; i--)
		for(int j = stack[i].substack.size() - 1; j >= 0; j--)
			if (!stack[i].substack[j].name.compare(name)) {
				if (stack[i].substack[j].IsErased()) {
					id = -1;
					dp = -1;
				}
				else {
					dp = i;
					id = j;
				}
				return;
			}

	id = -1;
	dp = -1;
}

//----

void	CLocalVariable::GetIndex(const yaya::string_t &name, int &id, int &dp)
{
	for(int i = stack.size() - 1; i >= 0; i--)
		for(int j = stack[i].substack.size() - 1; j >= 0; j--)
			if (!stack[i].substack[j].name.compare(name)) {
				if (stack[i].substack[j].IsErased()) {
					id = -1;
					dp = -1;
				}
				else {
					dp = i;
					id = j;
				}
				return;
			}

	id = -1;
	dp = -1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetValue
 *  �@�\�T�v�F  �w�肳�ꂽ���[�J���ϐ��̒l���擾���܂�
 *
 *  �w�肳�ꂽ�ϐ������݂��Ȃ������ꍇ�͋󕶎��񂪕Ԃ�܂��B
 * -----------------------------------------------------------------------
 */

CValue	CLocalVariable::GetValue(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].value();

	return CValue();
}

//----

CValue	CLocalVariable::GetValue(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].value();

	return CValue();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetValuePtr
 *  �@�\�T�v�F  �w�肳�ꂽ���[�J���ϐ��̃|�C���^���擾���܂�
 *
 *  �w�肳�ꂽ�ϐ������݂��Ȃ������ꍇ��NULL���Ԃ�܂��B
 * -----------------------------------------------------------------------
 */

CValue*	CLocalVariable::GetValuePtr(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return &(stack[dp].substack[id].value());

	return NULL;
}

//----

CValue*	CLocalVariable::GetValuePtr(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return &(stack[dp].substack[id].value());

	return NULL;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetNumber
 *  �@�\�T�v�F  �w�肳�ꂽ�X�^�b�N�̃��[�J���ϐ��̐����擾���܂�
 * -----------------------------------------------------------------------
 */
int		CLocalVariable::GetNumber(int depth)
{
	if (depth < 0 || depth >= (int)stack.size()) {
		return 0;
	}
	return stack[depth].substack.size();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetPtr
 *  �@�\�T�v�F  �w�肳�ꂽ�X�^�b�N�A�ʒu�̃��[�J���ϐ��ւ̃|�C���^���擾���܂�
 * -----------------------------------------------------------------------
 */
CVariable	*CLocalVariable::GetPtr(size_t depth,size_t index)
{
	if (depth >= stack.size()) {
		return NULL;
	}
	if (index >= stack[depth].substack.size()) {
		return NULL;
	}
	return &(stack[depth].substack[index]);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetDelimiter
 *  �@�\�T�v�F  �w�肳�ꂽ���[�J���ϐ��̃f���~�^���擾���܂�
 * -----------------------------------------------------------------------
 */
yaya::string_t	CLocalVariable::GetDelimiter(const yaya::char_t *name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].delimiter;

	return yaya::string_t(L"");
}

//----

yaya::string_t	CLocalVariable::GetDelimiter(const yaya::string_t &name)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0)
		return stack[dp].substack[id].delimiter;

	return yaya::string_t(L"");
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::SetDelimiter
 *  �@�\�T�v�F  �w�肳�ꂽ���[�J���ϐ��ɒl���i�[���܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::SetDelimiter(const yaya::char_t *name, const yaya::string_t &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].delimiter = value;
		return;
	}

	// �X�^�b�N���ɑ��݂��Ȃ���Εϐ���V���ɍ쐬���đ��
	Make(name, value);
}

//----

void	CLocalVariable::SetDelimiter(const yaya::string_t &name, const yaya::string_t &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].delimiter = value;
		return;
	}

	// �X�^�b�N���ɑ��݂��Ȃ���Εϐ���V���ɍ쐬���đ��
	Make(name, value);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::SetValue
 *  �@�\�T�v�F  �w�肳�ꂽ���[�J���ϐ��ɒl���i�[���܂�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::SetValue(const yaya::char_t *name, const CValue &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].value() = value;
		return;
	}

	// �X�^�b�N���ɑ��݂��Ȃ���Εϐ���V���ɍ쐬���đ��
	Make(name, value);
}

//----

void	CLocalVariable::SetValue(const yaya::string_t &name, const CValue &value)
{
	int	dp, id;
	GetIndex(name, id, dp);
	if (id >= 0) {
		stack[dp].substack[id].value() = value;
		return;
	}

	// �X�^�b�N���ɑ��݂��Ȃ���Εϐ���V���ɍ쐬���đ��
	Make(name, value);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::GetMacthedLongestNameLength
 *  �@�\�T�v�F  �w�肳�ꂽ������Ƀ}�b�`���閼�O�����ϐ���T���A�}�b�`����������Ԃ��܂�
 *
 *  �������������ꍇ�͍Œ��̂��̂�Ԃ��܂��B������Ȃ������ꍇ��0��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
size_t	CLocalVariable::GetMacthedLongestNameLength(const yaya::string_t &name)
{
	size_t	max_len = 0;

	for(yaya::native_signed i = stack.size() - 1; i >= 0; i--)
		for(yaya::native_signed j = stack[i].substack.size() - 1; j >= 0; j--) {
			size_t	len = stack[i].substack[j].name.size();
			if (!stack[i].substack[j].IsErased() &&
				max_len < len &&
				!name.compare(0,len,stack[i].substack[j].name,0,len))
				max_len = len;
		}

	return max_len;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLocalVariable::Erase
 *  �@�\�T�v�F  �w�肳�ꂽ�ϐ����������܂�
 *
 *  ���ۂɔz�񂩂�����킯�ł͂Ȃ��A�󕶎���������邾���ł�
 * -----------------------------------------------------------------------
 */
void	CLocalVariable::Erase(const yaya::string_t &name)
{
	int	id, dp;
	GetIndex(name, id, dp);
	if (id >= 0)
		SetValue(name, CValue());
}
