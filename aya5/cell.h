// 
// AYA version 5
//
// �����̍��������N���X�@CCell/CSerial
// written by umeici. 2004
// 
// CCell�̓X�e�[�g�����g���̐����̍����ACSerial�͂��̉��Z������ێ����邾���̃N���X�ł��B
// ����͂����̃C���X�^���X�����N���X���s�Ȃ��܂��B
//

#ifndef	CELLH
#define	CELLH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>
#include <boost/shared_ptr.hpp>

#include "value.h"

class	CSerial
{
public:
	int	tindex;						// ���Z�q��CCell�ʒu�ԍ�
	std::vector<int> index;				// ���Z�Ώۂ�CCell�ʒu�ԍ��̃��X�g
									// ���Z�q��F_TAG_FUNCPARAM/F_TAG_SYSFUNCPARAM�̏ꍇ�́Aindex[0]���֐��������܂�
public:
	CSerial(int t) { tindex = t; }

	CSerial(void) {}
	~CSerial(void) {}
};

//----

class	CCell
{
public:
	yaya::string_t	name;			// ���̍���"���O"�ithis�����[�J���ϐ��̎��Ɏg�p���܂��j
	int		index;					// �ʒu�ԍ��ithis���ϐ�/���[�J���ϐ�/�֐��̍ۂɎg�p���܂��j
	int		depth;					// {}����q�̐[���ithis�����[�J���ϐ��̎��Ɏg�p���܂��j

private:
	int     m_type;									// m_value�Ƀ^�C�v�����i�[���Ȃ��ꍇ�Ɏg�p

	mutable boost::shared_ptr<CValue> m_value;		// �l�ithis�����e�����l�̍ۂɎg�p���܂��j
	mutable boost::shared_ptr<CValue> m_ansv;		// �l�ithis���ϐ�/���[�J���ϐ�/�֐��̍ۂɁA���̓��e/�Ԓl���i�[���܂��j
	mutable boost::shared_ptr<CValue> m_order;		// ���Z���Ɏg�p�i�z��̏������ꎞ�I�ɕێ����܂��j
	mutable boost::shared_ptr<CValue> m_emb_ansv;	// �l�i%[n]�ŎQ�Ƃ����l��ێ����܂��j

public:
	CCell(int t)
	{
		index = -1;
		depth = -1;
		m_type  = t;
	}

	CCell(void)
	{
		index = -1;
		depth = -1;
		m_type  = F_TAG_STRING; //�W����STRING
	}

	~CCell(void) {
	}

	//////////////////////////////////////
	void value_SetType(int type) {
		if ( ! m_value.get() ) {
			m_type = type;
		}
		else {
			m_value->SetType(type);
		}
	}
	int value_GetType(void) const {
		if ( ! m_value.get() ) {
			return m_type;
		}
		else {
			return m_value->GetType();
		}
	}
	//-------------------//
	boost::shared_ptr<CValue> &value_shared(void) const {
		return m_value;
	}
	const CValue &value_const(void) const {
		if ( ! m_value.get() ) {
			m_value.reset(new CValue(m_type,0));
		}
		return *m_value;
	}
	inline const CValue &value(void) const {
		return value_const();
	}
	CValue &value(void) {
		if ( ! m_value.get() ) {
			m_value.reset(new CValue(m_type,0));
		}
		else if ( m_value.use_count() >= 2 ) {
			CValue *pV = m_value.get();
			m_value.reset(new CValue(*pV));
		}
		return *m_value;
	}
	void value_Delete(void) {
		if ( m_value.get() ) {
			m_type = m_value->GetType();
			m_value.reset();
		}
	}
	//////////////////////////////////////
	boost::shared_ptr<CValue> &ansv_shared(void) const {
		return m_ansv;
	}
	const CValue &ansv_const(void) const {
		if ( ! m_ansv.get() ) {
			return emptyvalue;
		}
		return *m_ansv;
	}
	inline const CValue &ansv(void) const {
		return ansv_const();
	}
	CValue &ansv(void) {
		if ( ! m_ansv.get() ) {
			m_ansv.reset(new CValue());
		}
		else if ( m_ansv.use_count() >= 2 ) {
			CValue *pV = m_ansv.get();
			m_ansv.reset(new CValue(*pV));
		}
		return *m_ansv;
	}
	//////////////////////////////////////
	boost::shared_ptr<CValue> &order_shared(void) const {
		return m_order;
	}
	const CValue &order_const(void) const {
		if ( ! m_order.get() ) {
			return emptyvalue;
		}
		return *m_order;
	}
	inline const CValue &order(void) const {
		return order_const();
	}
	CValue &order(void) {
		if ( ! m_order.get() ) {
			m_order.reset(new CValue());
		}
		else if ( m_order.use_count() >= 2 ) {
			CValue *pV = m_order.get();
			m_order.reset(new CValue(*pV));
		}
		return *m_order;
	}
	//////////////////////////////////////
	boost::shared_ptr<CValue> &emb_ansv_shared(void) const {
		return m_emb_ansv;
	}
	const CValue &emb_ansv_const(void) const {
		if ( ! m_emb_ansv.get() ) {
			return emptyvalue;
		}
		return *m_emb_ansv;
	}
	inline const CValue &emb_ansv(void) const {
		return emb_ansv_const();
	}
	CValue &emb_ansv(void) {
		if ( ! m_emb_ansv.get() ) {
			m_emb_ansv.reset(new CValue());
		}
		else if ( m_emb_ansv.use_count() >= 2 ) {
			CValue *pV = m_emb_ansv.get();
			m_emb_ansv.reset(new CValue(*pV));
		}
		return *m_emb_ansv;
	}
	//////////////////////////////////////
	void tmpdata_cleanup(void) const {
		{
			const CValue &c = ansv_const();
			if ( (c.s_value.size() > 10000) || c.array_size() ) {
				m_ansv.reset((CValue*)NULL);
			}
		}
		{
			const CValue &c = emb_ansv_const();
			if ( (c.s_value.size() > 10000) || c.array_size() ) {
				m_emb_ansv.reset((CValue*)NULL);
			}
		}
	}
};

//----

#endif
