// 
// AYA version 5
//
// 変数を扱うクラス　CVariable/CLVSubStack/CLocalVariable/CGlobalVariable
// written by umeici. 2004
// 
// CVariableクラスは値の保持しか行ないません。
//
// CLVSubStack/CLocalVariableはローカル変数を管理します。
// スタックとは名ばかりで実際の構造は可変長配列です（ランダムアクセスしたかったので）
//
// CLocalVariableのインスタンスは関数実行開始時に作成され、関数から返る際に破棄されます。
// 構造としては vector<vector<CVariable>> で、{}入れ子の深さ毎にローカル変数の配列を
// 持っていることになります。{}入れ子に入る際に配列が追加され、出る際に破棄されます。
//
// CGlobalVariableはグローバル変数を管理します。
//

#ifndef	VARIABLEH
#define	VARIABLEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <map>
#include <memory>

#include "cell.h"
#include "globaldef.h"
#include "value.h"
#include "fix_old_compiler.h"

class CAyaVM;

class	CVariable
{
public:
	aya::string_t	name;					// 名前
	aya::string_t	delimiter;				// デリミタ


protected:
	char	erased;					// 消去されたことを示すフラグ（グローバル変数で使用）
									// 0/1=有効/消去された
	mutable std_shared_ptr<CValue> m_value;				// 値

public:
	CVariable(const aya::string_t &n)
	{
		name       = n;
		delimiter  = VAR_DELIMITER;

		erased     = 0;
	}

	CVariable(aya::char_t *n)
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
	std_shared_ptr<CValue> &value_shared(void) const {
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
	void	GetIndex(const aya::char_t *name, int &id, int &dp);
	void	GetIndex(const aya::string_t &name, int &id, int &dp);

	void	Make(const aya::char_t *name);
	void	Make(const aya::string_t &name);
	void	Make(const aya::char_t *name, const CValue &value);
	void	Make(const aya::string_t &name, const CValue &value);
	void	Make(const aya::string_t &name, const CValueSub &value);
	void	Make(const aya::char_t *name, const aya::string_t &delimiter);
	void	Make(const aya::string_t &name, const aya::string_t &delimiter);

	const CValue& GetValue(const aya::char_t *name);
	const CValue& GetValue(const aya::string_t &name);

	CValue*	GetValuePtr(const aya::char_t *name);
	CValue*	GetValuePtr(const aya::string_t &name);

	aya::string_t	GetDelimiter(const aya::char_t *name);
	aya::string_t	GetDelimiter(const aya::string_t &name);

	void	SetDelimiter(const aya::char_t *name, const aya::string_t &value);
	void	SetDelimiter(const aya::string_t &name, const aya::string_t &value);

	void	SetValue(const aya::char_t *name, const CValue &value);
	void	SetValue(const aya::string_t &name, const CValue &value);

	size_t	GetMacthedLongestNameLength(const aya::string_t &name);

	void	Erase(const aya::string_t &name);
};

//----

class	CGlobalVariable
{
protected:
	std::vector<CVariable> var;
	aya::indexmap varmap;

private:
	CAyaVM &vm;

	CGlobalVariable(void);

public:
	CGlobalVariable(CAyaVM &vmr) : vm(vmr) {
	}
	void	CompleteSetting(void)
	{
	}

	int		Make(const aya::string_t &name, char erased);

	size_t	GetMacthedLongestNameLength(const aya::string_t &name);

	int		GetIndex(const aya::string_t &name);

	aya::string_t	GetName(int index) { return var[index].name; }
	size_t		GetNumber(void) { return var.size(); }
	CVariable	*GetPtr(size_t index) { return &(var[index]); }

	CValue			*GetValuePtr(int index) { return &(var[index].value()); }
	const CValue	*GetValuePtr(int index) const { return &(var[index].value_const()); }

	void	SetType(int index, int type) { var[index].value().SetType(type); }

	const CValue&	GetValue(int index) const { return var[index].value_const(); }

	aya::string_t&	GetDelimiter(int index) { return var[index].delimiter; }

	void	SetValue(int index, const CValue &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, int value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, double value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const aya::string_t &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const aya::char_t *value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const CValueArray &value) { var[index].Enable(); var[index].value() = value; }
	void	SetValue(int index, const CValueSub &value) { var[index].Enable(); var[index].value() = value; }
	void	SetDelimiter(int index, const aya::string_t value) { var[index].Enable(); var[index].delimiter = value; }

	void	EnableValue(int index) { var[index].Enable(); }

	void	Erase(const aya::string_t &name)
	{
		int	index = GetIndex(name);
		if (index >= 0) {
			var[index].Erase();
		}
	}

};

//----

#endif
