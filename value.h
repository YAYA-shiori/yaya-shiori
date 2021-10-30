// 
// AYA version 5
//
// 値を扱うクラス　CValue/CValueSub
// written by umeici. 2004
// 
// CValueは型フラグと型別の情報を持っています。その中にvector<CValueSub>という可変長配列があり、
// これが汎用配列となります。CValueSubクラスは配列を持たないほかはCValueと類似しています（汎用
// 配列を多次元化出来ない、というAYA5の制限はこの構造に由来しています）。
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
class CValueSub;

typedef std::vector<CValueSub> CValueArray;
typedef std::map<CValueSub, CValueSub> CValueHash;

class	CValueSub
{
	friend class CValue;

protected:
	int	type;						// 型
public:
	yaya::string_t	s_value;				// 文字列値
	double	d_value;				// 実数値
	yaya::int_t		i_value;				// 整数値

private:
	int CalcEscalationTypeNum(const int rhs) const;
	int CalcEscalationTypeStr(const int rhs) const;

public:
	CValueSub(void) :
		type(F_TAG_VOID) , s_value(), d_value(0.0), i_value(0) { }

	CValueSub(int value) :
		type(F_TAG_INT) , d_value(0.0) , i_value(value) { }

	CValueSub(yaya::int_t value) :
		type(F_TAG_INT) , d_value(0.0) , i_value(value) { }

	CValueSub(double value) :
		type(F_TAG_DOUBLE) , d_value(value), i_value(0) { }

	CValueSub(const yaya::string_t &value) :
		type(F_TAG_STRING) , s_value(value), d_value(0.0), i_value(0) { }

	CValueSub(const yaya::char_t *value) :
		type(F_TAG_STRING) , s_value(value), d_value(0.0), i_value(0) { }

	CValueSub(const CValue &v);

	~CValueSub(void) {}

	inline int		GetType(void) const { return type; }
	inline void		SetType(int tp) { type = tp; }

	inline bool		IsVoid(void) const { return type == F_TAG_VOID; }
	inline bool		IsString(void) const { return type == F_TAG_STRING || type == F_TAG_VOID; }
	inline bool		IsStringReal(void) const { return type == F_TAG_STRING; }
	inline bool		IsInt(void) const { return type == F_TAG_INT || type == F_TAG_VOID; }
	inline bool		IsIntReal(void) const { return type == F_TAG_INT; }
	inline bool		IsDouble(void) const { return type == F_TAG_DOUBLE || type == F_TAG_VOID; }
	inline bool		IsDoubleReal(void) const { return type == F_TAG_DOUBLE; }

	inline bool		IsNum(void) const { return type == F_TAG_INT || type == F_TAG_DOUBLE || type == F_TAG_VOID; }

	yaya::int_t		GetValueInt(void) const;
	double			GetValueDouble(void) const;
	yaya::string_t	GetValueString(void) const;

	CValueSub	&operator =(yaya::int_t value) LVALUE_MODIFIER;
	CValueSub	&operator =(double value) LVALUE_MODIFIER;
	CValueSub	&operator =(const yaya::string_t &value) LVALUE_MODIFIER;
	#if CPP_STD_VER > 2011
	CValueSub	&operator =(yaya::string_t&& value) LVALUE_MODIFIER;
	#endif
	CValueSub	&operator =(const yaya::char_t *value) LVALUE_MODIFIER;
	CValueSub	&operator =(const CValue &v) LVALUE_MODIFIER;

	CValueSub	operator +(const CValueSub &value) const;
	CValueSub	operator -(const CValueSub &value) const;
	CValueSub	operator *(const CValueSub &value) const;
	CValueSub	operator /(const CValueSub &value) const;
	CValueSub	operator %(const CValueSub &value) const;

	void	operator +=(const CValueSub &value) LVALUE_MODIFIER;
	void	operator -=(const CValueSub &value) LVALUE_MODIFIER;
	void	operator *=(const CValueSub &value) LVALUE_MODIFIER;
	void	operator /=(const CValueSub &value) LVALUE_MODIFIER;
	void	operator %=(const CValueSub &value) LVALUE_MODIFIER;

	inline bool operator ==(const CValueSub &value) const {
		return Compare(value);
	}
	inline bool operator !=(const CValueSub &value) const {
		return !operator==(value);
	}
	inline bool operator <(const CValueSub &value) const {
		return Less(value);
	}

	bool Less(const CValueSub &value) const;
	bool Compare(const CValueSub &value) const;
};


class	CValue
{
	friend class CValueSub;

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

	int Compare(const CValue &value) const;
	int Great(const CValue &value) const;
	int Less(const CValue &value) const;

public:
	CValue(const CValue &rhs)
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
            m_hash.reset((CValueHash*)NULL);
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
	
	CValue(const CValueSub &value) : d_value(0.0), i_value(0)
	{
		switch(value.GetType()) {
		case F_TAG_INT:
			type      = F_TAG_INT;
			i_value   = value.i_value;
			break;
		case F_TAG_DOUBLE:
			type      = F_TAG_DOUBLE;
			d_value   = value.d_value;
			break;
		case F_TAG_STRING:
			type     = F_TAG_STRING;
			s_value  = value.s_value;
			break;
		default:
			type     = F_TAG_VOID;
			break;
		};
	}

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
            if ( m_hash.get() ) {
                return m_hash->size() != 0;
            }
            else {
                return 0;
            }
		default:
			break;
		};
		return false;
	}

	yaya::int_t		GetValueInt(void) const;
	double			GetValueDouble(void) const;
	yaya::string_t	GetValueString(void) const;
	yaya::string_t	GetValueStringForLogging(void) const;

	void	SetArrayValue(const CValue &oval, const CValue &value);

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
	CValue	&operator =(const CValueSub &value) LVALUE_MODIFIER;

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
	CValueHash::size_type hash_size(void) const {
		if ( ! m_hash.get() ) {
			return 0;
		}
		else {
			return m_hash->size();
		}
	}
	std_shared_ptr<CValueHash> &hash_shared(void) const {
		return m_hash;
	}
	const CValueHash& hash(void) const {
		if ( ! m_hash.get() ) {
			m_hash.reset(new CValueHash);
		}
		return *m_hash;
	}
	CValueHash& hash(void) {
		if ( ! m_hash.get() ) {
			m_hash.reset(new CValueHash);
		}
		else if ( m_hash.use_count() >= 2 ) {
			CValueHash *pV = m_hash.get();
			m_hash.reset(new CValueHash(*pV));
		}
		return *m_hash;
	}
	inline void array_clear(void) {
		m_array.reset();
	}
};

//----

//からっぽ変数（ダミー用）
extern const CValue emptyvalue;

#endif
