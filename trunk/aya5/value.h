// 
// AYA version 5
//
// �l�������N���X�@CValue/CValueSub
// written by umeici. 2004
// 
// CValue�͌^�t���O�ƌ^�ʂ̏��������Ă��܂��B���̒���vector<CValueSub>�Ƃ����ϒ��z�񂪂���A
// ���ꂪ�ėp�z��ƂȂ�܂��BCValueSub�N���X�͔z��������Ȃ��ق���CValue�Ɨގ����Ă��܂��i�ėp
// �z��𑽎������o���Ȃ��A�Ƃ���AYA5�̐����͂��̍\���ɗR�����Ă��܂��j�B
// 

#ifndef	VALUEH
#define	VALUEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <boost/shared_ptr.hpp>

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
	int	type;						// �^
public:
	yaya::string_t	s_value;				// ������l
	double	d_value;				// �����l
	int		i_value;				// �����l

private:
	int CalcEscalationTypeNum(const int rhs) const;
	int CalcEscalationTypeStr(const int rhs) const;

public:
	CValueSub(void) :
		type(F_TAG_VOID) , s_value(L""), d_value(0.0), i_value(0) { }

	CValueSub(int value) :
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

	int		GetValueInt(void) const;
	double	GetValueDouble(void) const;
	yaya::string_t	GetValueString(void) const;

	CValueSub	&operator =(int value);
	CValueSub	&operator =(double value);
	CValueSub	&operator =(const yaya::string_t &value);
	CValueSub	&operator =(const yaya::char_t *value);
	CValueSub	&operator =(const CValue &v);

	CValueSub	operator +(const CValueSub &value) const;
	CValueSub	operator -(const CValueSub &value) const;
	CValueSub	operator *(const CValueSub &value) const;
	CValueSub	operator /(const CValueSub &value) const;
	CValueSub	operator %(const CValueSub &value) const;

	inline CValueSub operator ==(const CValueSub &value) const {
		return CValueSub(Compare(value));
	}
	inline CValueSub operator !=(const CValueSub &value) const {
		return CValueSub(1 - Compare(value));
	}
	inline bool operator <(const CValueSub &value) const {
		return Less(value);
	}

	bool Less(const CValueSub &value) const;
	int Compare(const CValueSub &value) const;
};

//----

class	CValue
{
	friend class CValueSub;

protected:
	int	type;						// �^
public:
	yaya::string_t	s_value;				// ������l
	double	d_value;				// �����l
	int		i_value;				// �����l

private:
	mutable boost::shared_ptr<CValueArray> m_array;		// �ėp�z��
    mutable boost::shared_ptr<CValueHash> m_hash;  // �n�b�V��

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
		//i,d�̓T�C�Y���������̂ŃR�s�[�����ق��������葁��
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
		//i,d�̓T�C�Y���������̂ŃR�s�[�����ق��������葁��
		i_value = rhs.i_value;
		d_value = rhs.d_value;

		if ( type == F_TAG_ARRAY ) {
			m_array = rhs.m_array;
		}
        else if (type == F_TAG_HASH) {
            m_hash = rhs.m_hash;
        }
		else {
			m_array.reset((CValueArray*)NULL);
            m_hash.reset((CValueHash*)NULL);
			if ( type == F_TAG_STRING ) {
				s_value = rhs.s_value;
			}
		}
		return *this;
	}

	CValue(int value) :
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

	CValue(int tp, int) : type(tp), d_value(0.0), i_value(0) { }	// �^�w�肵�ď�����
	CValue(void) : type(F_TAG_VOID), d_value(0.0), i_value(0) { }
	~CValue(void) {}

	inline void	SetType(int tp) { type = tp; }
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

	char	GetTruth(void) const
	{
		switch(type) {
		case F_TAG_VOID:   return 0;
		case F_TAG_INT:	   return (i_value)        ? 1 : 0;
		case F_TAG_DOUBLE: return (d_value == 0.0) ? 0 : 1;
		case F_TAG_STRING: return (s_value == L"") ? 0 : 1;
		case F_TAG_ARRAY:
			if ( m_array.get() ) {
				return m_array->size() != 0;
			}
			else {
				return 0;
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
		return 0;
	}

	int		GetValueInt(void) const;
	double	GetValueDouble(void) const;
	yaya::string_t	GetValueString(void) const;
	yaya::string_t	GetValueStringForLogging(void) const;

	void	SetArrayValue(const CValue &oval, const CValue &value);

	int		DecodeArrayOrder(int &order, int &order1, yaya::string_t &delimiter) const;

	CValue	&operator =(int value);
	CValue	&operator =(double value);
	CValue	&operator =(const yaya::string_t &value);
	CValue	&operator =(const yaya::char_t *value);
	CValue	&operator =(const CValueArray &value);
    CValue  &operator =(const CValueHash &value);
	CValue	&operator =(const CValueSub &value);

	CValue	operator +(const CValue &value) const;
	CValue	operator -(const CValue &value) const;
	CValue	operator *(const CValue &value) const;
	CValue	operator /(const CValue &value) const;
	CValue	operator %(const CValue &value) const;

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
		return CValue((GetTruth() || value.GetTruth()) ? 1 : 0);
	}
	inline CValue operator && (const CValue &value) const {
		return CValue((GetTruth() && value.GetTruth()) ? 1 : 0);
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
	boost::shared_ptr<CValueArray> &array_shared(void) const {
		return m_array;
	}
	const CValueArray& array(void) const {
		if ( ! m_array.get() ) {
			m_array.reset(new CValueArray);
		}
		return *m_array;
	}
	CValueArray& array(void) {
		if ( ! m_array.get() ) {
			m_array.reset(new CValueArray);
		}
		else if ( m_array.use_count() >= 2 ) {
			CValueArray *pV = m_array.get();
			m_array.reset(new CValueArray(*pV));
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
	boost::shared_ptr<CValueHash> &hash_shared(void) const {
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
};

//----

//������ەϐ��i�_�~�[�p�j
extern const CValue emptyvalue;

#endif
