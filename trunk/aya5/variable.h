// 
// AYA version 5
//
// �ϐ��������N���X�@CVariable/CLVSubStack/CLocalVariable/CGlobalVariable
// written by umeici. 2004
// 
// CVariable�N���X�͒l�̕ێ������s�Ȃ��܂���B
//
// CLVSubStack/CLocalVariable�̓��[�J���ϐ����Ǘ����܂��B
// �X�^�b�N�Ƃ͖��΂���Ŏ��ۂ̍\���͉ϒ��z��ł��i�����_���A�N�Z�X�����������̂Łj
//
// CLocalVariable�̃C���X�^���X�͊֐����s�J�n���ɍ쐬����A�֐�����Ԃ�ۂɔj������܂��B
// �\���Ƃ��Ă� vector<vector<CVariable>> �ŁA{}����q�̐[�����Ƀ��[�J���ϐ��̔z���
// �����Ă��邱�ƂɂȂ�܂��B{}����q�ɓ���ۂɔz�񂪒ǉ�����A�o��ۂɔj������܂��B
//
// CGlobalVariable�̓O���[�o���ϐ����Ǘ����܂��B
//

#ifndef	VARIABLEH
#define	VARIABLEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

#include "cell.h"
#include "globaldef.h"
#include "value.h"

class CAyaVM;

class	CVariable
{
public:
	yaya::string_t	name;					// ���O
	yaya::string_t	delimiter;				// �f���~�^


protected:
	char	erased;					// �������ꂽ���Ƃ������t���O�i�O���[�o���ϐ��Ŏg�p�j
									// 0/1=�L��/�������ꂽ
	mutable CValueSmartPtr m_value;				// �l

public:
	CVariable(const yaya::string_t &n)
	{
		name       = n;
		delimiter  = VAR_DELIMITER;

		erased     = 0;
	}

	CVariable(yaya::char_t *n)
	{
		name       = n;
		delimiter  = VAR_DELIMITER;

		erased     = 0;
	}

	CVariable(void)
	{
		name       = L"";
		delimiter  = VAR_DELIMITER;

		erased     = 0;
	}

	~CVariable(void) {}

	void	Enable(void) { erased = 0; }
	void	Erase(void) {
		erased = 1;
		m_value.reset();
	}
	char	IsErased(void) { return erased; }

	//////////////////////////////////////
	CValueSmartPtr &value_shared(void) const {
		return m_value;
	}
	const CValue &value_const(void) const {
		if ( ! m_value.get() ) {
			return emptyvalue;
		}
		return *m_value;
	}
	inline const CValue &value(void) const {
		return value_const();
	}
	CValue &value(void) {
		if ( ! m_value.get() ) {
			m_value.reset(new CValue);
		}
		else if ( m_value.use_count() >= 2 ) {
			CValue *pV = m_value.get();
			m_value.reset(new CValue(*pV));
		}
		return *m_value;
	}
};

//----

class	CLVSubStack
{
public:
	std::vector<CVariable> substack;
};

//----

class	CLocalVariable
{
protected:
	std::vector<CLVSubStack> stack;
	int	depth;

public:
	CLocalVariable(void);
	~CLocalVariable(void);

	CVariable	*GetArgvPtr(void);

	void	AddDepth(void);
	void	DelDepth(void);

	int		GetDepth(void) { return depth; }

	int		GetNumber(int depth);
	CVariable	*GetPtr(size_t depth,size_t index);

public:
	void	GetIndex(const yaya::char_t *name, int &id, int &dp);
	void	GetIndex(const yaya::string_t &name, int &id, int &dp);

	void	Make(const yaya::char_t *name);
	void	Make(const yaya::string_t &name);
	void	Make(const yaya::char_t *name, const CValue &value);
	void	Make(const yaya::string_t &name, const CValue &value);
	void	Make(const yaya::string_t &name, const CValueSub &value);
	void	Make(const yaya::char_t *name, const yaya::string_t &delimiter);
	void	Make(const yaya::string_t &name, const yaya::string_t &delimiter);

	const CValue& GetValue(const yaya::char_t *name);
	const CValue& GetValue(const yaya::string_t &name);

	CValueSmartPtr GetValuePtr(const yaya::char_t *name);
	CValueSmartPtr GetValuePtr(const yaya::string_t &name);
 
	yaya::string_t	GetDelimiter(const yaya::char_t *name);
	yaya::string_t	GetDelimiter(const yaya::string_t &name);

	void	SetDelimiter(const yaya::char_t *name, const yaya::string_t &value);
	void	SetDelimiter(const yaya::string_t &name, const yaya::string_t &value);

	void	SetValue(const yaya::char_t *name, const CValue &value);
	void	SetValue(const yaya::string_t &name, const CValue &value);

	size_t	GetMacthedLongestNameLength(const yaya::string_t &name);

	void	Erase(const yaya::string_t &name);
};

//----

class	CGlobalVariable
{
protected:
	std::vector<CVariable> var;
	yaya::indexmap varmap;

private:
	CAyaVM &vm;

	CGlobalVariable(void);

public:
	CGlobalVariable(CAyaVM &vmr) : vm(vmr) {
	}
	void	CompleteSetting(void)
	{
		std::vector<CVariable>(var).swap(var);
	}

	int		Make(const yaya::string_t &name, char erased);

	size_t	GetMacthedLongestNameLength(const yaya::string_t &name);

	int		GetIndex(const yaya::string_t &name);

	yaya::string_t	GetName(int index) { return var[index].name; }
	size_t		GetNumber(void) { return var.size(); }
	CVariable	*GetPtr(size_t index) { return &(var[index]); }

	CValue			&GetValueRef(int index) { return var[index].value(); }
	const CValue	&GetValueRef(int index) const { return var[index].value_const(); }

	CValueSmartPtr  GetValuePtr(int index) const { return var[index].value_shared(); }

	void	SetType(int index, int type) { var[index].value().SetType(type); }

	const CValue&	GetValue(int index) const { return var[index].value_const(); }

	yaya::string_t&	GetDelimiter(int index) { return var[index].delimiter; }

	void	SetValue(int index, const CValue &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, int value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, double value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const yaya::string_t &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const yaya::char_t *value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const CValueArray &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const CValueSub &value) { var[index].Enable(); var[index].value() = value; }
	void	SetDelimiter(int index, const yaya::string_t value) { var[index].Enable(); var[index].delimiter = value; }

	void	Erase(const yaya::string_t &name)
	{
		int	index = GetIndex(name);
		if (index >= 0) {
			var[index].Erase();
		}
	}

};

//----

#endif
