// 
// AYA version 5
//
// C++11より前のコンパイラ（VC6など）向けの shared_ptr / make_shared
// std::shared_ptr のうちYAYAが使う部分だけを実装したもの
//
// ・weak_ptr、カスタムデリータ、派生クラスからの変換は持たない
// ・参照カウントはスレッドセーフではない（YAYAは1スレッドで動くため）
// ・デリータは生成時に決まるので、破棄する場所でTが不完全型でもよい
//

#ifndef YAYA_SHAREDPTR_H_
#define YAYA_SHAREDPTR_H_

#include <cstddef>

namespace yaya {

namespace sp_detail {

	// 参照カウントの共通部分
	class sp_counted_base {
	private:
		long m_use_count;

		sp_counted_base(const sp_counted_base &);
		sp_counted_base &operator=(const sp_counted_base &);

	public:
		sp_counted_base(void) : m_use_count(1) { }
		virtual ~sp_counted_base(void) { }

		void add_ref(void) {
			++m_use_count;
		}
		void release(void) {
			if ( --m_use_count == 0 ) {
				delete this;
			}
		}
		long use_count(void) const {
			return m_use_count;
		}
	};

	// newで確保したポインタを持つ（shared_ptr(T*) / reset(T*)）
	template<class T> class sp_counted_ptr : public sp_counted_base {
	private:
		T *m_p;

	public:
		explicit sp_counted_ptr(T *p) : m_p(p) { }
		virtual ~sp_counted_ptr(void) {
			delete m_p;
		}
	};

	// 値を同じ領域に持つ（make_shared。確保が1回で済む）
	template<class T> class sp_counted_inplace : public sp_counted_base {
	public:
		T m_value;

		sp_counted_inplace(void) : m_value() { }
		template<class A1> explicit sp_counted_inplace(const A1 &a1) : m_value(a1) { }
		template<class A1,class A2> sp_counted_inplace(const A1 &a1,const A2 &a2) : m_value(a1,a2) { }
		template<class A1,class A2,class A3> sp_counted_inplace(const A1 &a1,const A2 &a2,const A3 &a3) : m_value(a1,a2,a3) { }
	};

} // namespace sp_detail

template<class T> class shared_ptr {
private:
	typedef shared_ptr<T> this_type;

	T *m_px;
	sp_detail::sp_counted_base *m_pn;

public:
	typedef T element_type;

	shared_ptr(void) : m_px(0), m_pn(0) { }

	explicit shared_ptr(T *p) : m_px(p), m_pn(0) {
		if ( p ) {
			try {
				m_pn = new sp_detail::sp_counted_ptr<T>(p);
			}
			catch(...) {
				delete p;
				throw;
			}
		}
	}

	// make_shared 用。pn の参照を1つ引き取る
	shared_ptr(T *p,sp_detail::sp_counted_base *pn) : m_px(p), m_pn(pn) { }

	shared_ptr(const shared_ptr &r) : m_px(r.m_px), m_pn(r.m_pn) {
		if ( m_pn ) {
			m_pn->add_ref();
		}
	}

	~shared_ptr(void) {
		if ( m_pn ) {
			m_pn->release();
		}
	}

	shared_ptr &operator=(const shared_ptr &r) {
		this_type(r).swap(*this);
		return *this;
	}

	void reset(void) {
		this_type().swap(*this);
	}
	void reset(T *p) {
		this_type(p).swap(*this);
	}

	void swap(shared_ptr &r) {
		T *tp = m_px;
		m_px = r.m_px;
		r.m_px = tp;

		sp_detail::sp_counted_base *tn = m_pn;
		m_pn = r.m_pn;
		r.m_pn = tn;
	}

	T *get(void) const {
		return m_px;
	}
	T &operator*(void) const {
		return *m_px;
	}
	T *operator->(void) const {
		return m_px;
	}

	long use_count(void) const {
		return m_pn ? m_pn->use_count() : 0;
	}
	bool unique(void) const {
		return use_count() == 1;
	}

	// if ( p ) / if ( !p ) 用
	typedef T *(this_type::*unspecified_bool_type)(void) const;
	operator unspecified_bool_type(void) const {
		return m_px ? &this_type::get : 0;
	}
	bool operator!(void) const {
		return m_px == 0;
	}
};

template<class T> inline bool operator==(const shared_ptr<T> &a,const shared_ptr<T> &b)
{
	return a.get() == b.get();
}
template<class T> inline bool operator!=(const shared_ptr<T> &a,const shared_ptr<T> &b)
{
	return a.get() != b.get();
}

template<class T> inline shared_ptr<T> make_shared(void)
{
	sp_detail::sp_counted_inplace<T> *pn = new sp_detail::sp_counted_inplace<T>();
	return shared_ptr<T>(&pn->m_value,pn);
}
template<class T,class A1> inline shared_ptr<T> make_shared(const A1 &a1)
{
	sp_detail::sp_counted_inplace<T> *pn = new sp_detail::sp_counted_inplace<T>(a1);
	return shared_ptr<T>(&pn->m_value,pn);
}
template<class T,class A1,class A2> inline shared_ptr<T> make_shared(const A1 &a1,const A2 &a2)
{
	sp_detail::sp_counted_inplace<T> *pn = new sp_detail::sp_counted_inplace<T>(a1,a2);
	return shared_ptr<T>(&pn->m_value,pn);
}
template<class T,class A1,class A2,class A3> inline shared_ptr<T> make_shared(const A1 &a1,const A2 &a2,const A3 &a3)
{
	sp_detail::sp_counted_inplace<T> *pn = new sp_detail::sp_counted_inplace<T>(a1,a2,a3);
	return shared_ptr<T>(&pn->m_value,pn);
}

} // namespace yaya

#endif // YAYA_SHAREDPTR_H_
