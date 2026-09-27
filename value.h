// 
// AYA version 5
//
// 値を扱うクラス　CValue
// written by umeici. 2004
// 
// CValueは型フラグと型別の情報を持っています。その中にvector<CValue>という可変長配列と
// map<CValue,CValue>というハッシュがあり、要素にも配列やハッシュを持たせることができます。
// （かつては要素をCValueSubという配列を持たないクラスで保持しており、汎用配列を多次元化出来ない
// というAYA5の制限はこの構造に由来していました）
// 

#ifndef	VALUEH
#define	VALUEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <memory>
//modified by Neo 2015.oct
#include <climits>
//modified by Neo 2015.oct

#include "manifest.h"
#include "globaldef.h"

class CValue;

// ハッシュのキーや集合の要素の順序（定義はvalue.cpp）
struct CValueLess
{
	bool operator()(const CValue &lhs, const CValue &rhs) const;
};

typedef std::vector<CValue> CValueArray;
typedef std::map<CValue, CValue, CValueLess> CValueHash;

class	CValue
{
protected:
	int	type;						// 型
public:
	yaya::string_t	s_value;				// 文字列値
	double			d_value;				// 実数値
	yaya::int_t		i_value;				// 整数値

private:
	mutable std_shared_ptr<CValueArray> m_array;		// 汎用配列
    mutable std_shared_ptr<CValueHash> m_hash;  // ハッシュ

private:
	int CalcEscalationTypeNum(const int rhs) const;
	int CalcEscalationTypeStr(const int rhs) const;

public:
	int Compare(const CValue &value) const;
	int Great(const CValue &value) const;
	int Less(const CValue &value) const;

	CValue(const CValue &rhs) :
		//i,dはサイズが小さいのでコピーしたほうが手っ取り早い
		type(rhs.type), d_value(rhs.d_value), i_value(rhs.i_value)
	{
		if ( type == F_TAG_ARRAY ) {
			m_array = rhs.m_array;
		}
        else if (type == F_TAG_HASH) {
            m_hash = rhs.m_hash;
        }
		else if ( type == F_TAG_STRING ) {
			s_value = rhs.s_value;
		}
	}
	CValue& operator =(const CValue &rhs)
	{
		type = rhs.type;
		//i,dはサイズが小さいのでコピーしたほうが手っ取り早い
		i_value = rhs.i_value;
		d_value = rhs.d_value;

		if ( type == F_TAG_ARRAY ) {
			m_array = rhs.m_array;
		}
        else if (type == F_TAG_HASH) {
            m_hash = rhs.m_hash;
        }
		else {
			m_array.reset();
            m_hash.reset(); // reset((CValueHash*)NULL)だとNULLでも参照カウンタが確保される
			if ( type == F_TAG_STRING ) {
				s_value = rhs.s_value;
			}
		}
		return *this;
	}

	CValue(int value) :
		type(F_TAG_INT) , d_value(0.0) , i_value(value) { }

	CValue(yaya::int_t value) :
		type(F_TAG_INT) , d_value(0.0) , i_value(value) { }

	CValue(double value) :
		type(F_TAG_DOUBLE) , d_value(value), i_value(0) { }

	CValue(const yaya::string_t &value) :
		type(F_TAG_STRING) , s_value(value), d_value(0.0), i_value(0) { }

	CValue(const yaya::char_t *value) :
		type(F_TAG_STRING) , s_value(value), d_value(0.0), i_value(0) { }
	
	CValue(int tp, int) : type(tp), d_value(0.0), i_value(0) { }	// 型指定して初期化
	CValue(void) : type(F_TAG_VOID), d_value(0.0), i_value(0) { }
	~CValue(void) {}

	inline void		SetType(int tp) { type = tp; }
	inline int		GetType(void) const { return type; }

	inline bool		IsVoid(void) const { return type == F_TAG_VOID; }
	inline bool		IsString(void) const { return type == F_TAG_STRING || type == F_TAG_VOID; }
	inline bool		IsStringReal(void) const { return type == F_TAG_STRING; }
	inline bool		IsInt(void) const { return type == F_TAG_INT || type == F_TAG_VOID; }
	inline bool		IsIntReal(void) const { return type == F_TAG_INT; }
	inline bool		IsDouble(void) const { return type == F_TAG_DOUBLE || type == F_TAG_VOID; }
	inline bool		IsDoubleReal(void) const { return type == F_TAG_DOUBLE; }
	inline bool		IsArray(void) const { return type == F_TAG_ARRAY; }
    inline bool     IsHash(void) const { return type == F_TAG_HASH; }

	inline bool		IsNum(void) const { return type == F_TAG_INT || type == F_TAG_DOUBLE || type == F_TAG_VOID; }

	bool	GetTruth(void) const
	{
		switch(type) {
		case F_TAG_VOID:   return false;
		case F_TAG_INT:	   return i_value != 0;
		case F_TAG_DOUBLE: return d_value != 0.0;
		case F_TAG_STRING: return s_value.size() != 0;
		case F_TAG_ARRAY:
			if( m_array.get() ) {
				return m_array->size() != 0;
			}
			else {
				return false;
			}
        case F_TAG_HASH:
            return hash_size() != 0;
		default:
			break;
		};
		return false;
	}

	yaya::int_t		GetValueInt(void) const;
	double			GetValueDouble(void) const;
	yaya::string_t	GetValueString(void) const;
	yaya::string_t	GetValueStringForLogging(void) const;

	void	SetArrayValue(const CValue &oval, const CValue &value, bool spread = true);

	bool	DecodeArrayOrder(size_t& order, size_t& order1, yaya::string_t& delimiter) const;

	CValue	&operator =(yaya::int_t value) LVALUE_MODIFIER;
	CValue	&operator =(double value) LVALUE_MODIFIER;
	CValue	&operator =(const yaya::string_t &value) LVALUE_MODIFIER;
	#if CPP_STD_VER > 2011
	CValue	&operator =(yaya::string_t&& value) LVALUE_MODIFIER;
	#endif
	CValue	&operator =(const yaya::char_t *value) LVALUE_MODIFIER;
	CValue	&operator =(const CValueArray &value) LVALUE_MODIFIER;
    CValue  &operator =(const CValueHash &value) LVALUE_MODIFIER;

	void SubstToArray(CValueArray &value) LVALUE_MODIFIER;

	CValue	operator +(const CValue &value) const;
	CValue	operator -(const CValue &value) const;
	CValue	operator *(const CValue &value) const;
	CValue	operator /(const CValue &value) const;
	CValue	operator %(const CValue &value) const;

	void	operator +=(const CValue &value) LVALUE_MODIFIER;
	void	operator -=(const CValue &value) LVALUE_MODIFIER;
	void	operator *=(const CValue &value) LVALUE_MODIFIER;
	void	operator /=(const CValue &value) LVALUE_MODIFIER;
	void	operator %=(const CValue &value) LVALUE_MODIFIER;

	CValue	operator [](const CValue &value) const;

	inline CValue operator ==(const CValue &value) const {
		return CValue(Compare(value));
	}
	inline CValue operator !=(const CValue &value) const {
		return CValue(1 - Compare(value));
	}
	inline CValue operator >(const CValue &value) const {
		return CValue(Great(value));
	}
	inline CValue operator <=(const CValue &value) const {
		return CValue(1 - Great(value));
	}
	inline CValue operator <(const CValue &value) const {
		return CValue(Less(value));
	}
	inline CValue operator >=(const CValue &value) const {
		return CValue(1 - Less(value));
	}

	inline CValue operator ||(const CValue &value) const {
		return CValue(GetTruth() || value.GetTruth());
	}
	inline CValue operator && (const CValue &value) const {
		return CValue(GetTruth() && value.GetTruth());
	}

	//////////////////////////////////////////////
	CValueArray::size_type array_size(void) const {
		if ( ! m_array.get() ) {
			return 0;
		}
		else {
			return m_array->size();
		}
	}
	std_shared_ptr<CValueArray> &array_shared(void) const {
		return m_array;
	}
	const CValueArray& array(void) const {
		if( ! m_array.get() ) {
			m_array=std_make_shared<CValueArray>();
		}
		return *m_array;
	}
	CValueArray& array(void) {
		if( ! m_array.get() ) {
			m_array=std_make_shared<CValueArray>();
		}
		else if ( m_array.use_count() >= 2 ) {
			CValueArray *pV = m_array.get();
			m_array=std_make_shared<CValueArray>(*pV);
		}
		return *m_array;
	}

	//////////////////////////////////////////////
	// ハッシュ操作（VC6ではCValueが不完全型の間にstd::mapを実体化できないため、定義はクラスの外）
	size_t hash_size(void) const;
	std_shared_ptr<CValueHash> &hash_shared(void) const {
		return m_hash;
	}
	const CValueHash& hash(void) const;
	CValueHash& hash(void);
	inline void array_clear(void) {
		m_array.reset();
	}
	inline void hash_clear(void) {
		m_hash.reset();
	}
};

//----

inline size_t CValue::hash_size(void) const
{
	if ( ! m_hash.get() ) {
		return 0;
	}
	else {
		return m_hash->size();
	}
}
inline const CValueHash& CValue::hash(void) const
{
	if ( ! m_hash.get() ) {
		m_hash.reset(new CValueHash);
	}
	return *m_hash;
}
inline CValueHash& CValue::hash(void)
{
	if ( ! m_hash.get() ) {
		m_hash.reset(new CValueHash);
	}
	else if ( m_hash.use_count() >= 2 ) {
		CValueHash *pV = m_hash.get();
		m_hash.reset(new CValueHash(*pV));
	}
	return *m_hash;
}

//からっぽ変数（ダミー用）
extern const CValue emptyvalue;

#endif
