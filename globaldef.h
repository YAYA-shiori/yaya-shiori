#ifndef YAYA_GLOBALDEF_H_
#define YAYA_GLOBALDEF_H_

#ifdef __cplusplus

#include <cwchar>
#include <string>
#include <map>

#include <memory>

#endif

//=============================================================================================================
// compiler compat section
//=============================================================================================================

#if defined(_MSVC_LANG) || __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1600)

#include <memory>

#define std_shared_ptr  std::shared_ptr
#define std_make_shared std::make_shared

// freebsdでdefine nullptr 0を行うと
// aya5.cpp:L507のloghandler_list.emplace_back(nullptr)で
// 型不一致のエラーが出るのでdefineしない。

#else
//C++11 or older (VC6など) : std::shared_ptrが無いので自前のものを使う

#include "sharedptr.h"

#define std_shared_ptr   yaya::shared_ptr
#define std_make_shared  yaya::make_shared

#define nullptr 0

#endif // C++11


#define AYX_WIN64_AWARE




#ifdef _MSVC_LANG
# define CPP_STD_VER (_MSVC_LANG/100)
#else
# ifdef _MSC_VER
#  if _MSC_VER > 1600
#   define CPP_STD_VER 2011
#  else
#   define CPP_STD_VER 1998
#  endif
# else
#  define CPP_STD_VER 2020
# endif
#endif



#ifdef _MSC_VER


#pragma warning (disable: 4786)
#pragma warning (disable: 4819)

#pragma warning (3: 4032 4057 4092)
#pragma warning (3: 4115 4121 4125 4130 4132 4134 4152 4189)
#pragma warning (3: 4207 4208 4209 4211 4212 4220 4223 4234 4238)
#pragma warning (3: 4355 4504 4505 4515)
//#pragma warning (3: 4663 4665 4670 4671 4672 4673 4674)
#pragma warning (3: 4665 4670 4671 4672 4673 4674)
//#pragma warning (3: 4701 4702 4705 4706 4709 4727)
#pragma warning (3: 4701 4705 4706 4709 4727)


#if _MSC_VER >= 1300
#undef AYX_WIN64_AWARE
#define AYX_WIN64_AWARE __w64
#endif


#if _MSC_VER <= 1200

#define for if(0);else for


#ifdef __cplusplus
namespace std {
#endif
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
#ifdef __cplusplus
};
#endif

#define	INT64_IS_NOT_STD

#ifndef LLONG_MIN
#define LLONG_MIN _I64_MIN
#endif
#ifndef LLONG_MAX
#define LLONG_MAX _I64_MAX
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX _UI64_MAX
#endif

#define ULL_DEF(p) p ## Ui64
#define LL_DEF(p) p ## i64

// VC6の標準ライブラリには std::min / std::max が無い
// (min)のように括弧で囲むのは、windows.hのmin/maxマクロに展開させないため
#ifdef __cplusplus
#include <algorithm>

namespace std {
	template<class T> inline const T& (min)(const T &a,const T &b) {
		return (b < a) ? b : a;
	}
	template<class T> inline const T& (max)(const T &a,const T &b) {
		return (a < b) ? b : a;
	}
};
#endif

#else

#include <cstdint>

#ifndef __cplusplus
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif

#define ULL_DEF(p) p ## ULL
#define LL_DEF(p) p ## LL

#endif //1200

#else

#include <cstdint>

#endif //MSC_VER

#ifndef LL_DEF

#define ULL_DEF(p) p ## ULL
#define LL_DEF(p) p ## LL

#endif // LL_DEF

#if CPP_STD_VER < 2011

//C++11 or older
#define constexpr const
#define emplace_back(p) push_back(p)

#define _Pre_notnull_
#define _Always_(p)
#define _Printf_format_string_

#define LVALUE_MODIFIER 

#define std_move(d) (d)

#else

#define std_move(d) std::move(d)

#define LVALUE_MODIFIER &

#endif //CPP_STD_VER



//=============================================================================================================
// yaya type section
//=============================================================================================================

#ifdef __cplusplus

namespace yaya {
#if CPP_STD_VER >= 2011
	struct memory_error:std::exception{
		virtual const char*what()const noexcept{
			return "memory error";
		}
	};
#else
	struct memory_error:std::exception{
		virtual const char*what()const{
			return "memory error";
		}
	};
#endif
	typedef wchar_t char_t;
	typedef std::basic_string<char_t> string_t;
	typedef std::int64_t int_t;
	typedef std::int64_t time_t;

	typedef std_shared_ptr<string_t> share_string_t;
	typedef std_shared_ptr<const string_t> const_share_string_t;

	typedef std::map<yaya::string_t,size_t> indexmap;

	template<class T> void shared_ptr_deep_copy(const std_shared_ptr<T> &from,std_shared_ptr<T> &to) {
		if( from.get() ) {
			to = std_make_shared<T>(*from);
		}
		else {
			to.reset();
		}
	}

#ifdef _WIN64
	typedef std::int64_t native_signed;
#else
	typedef int AYX_WIN64_AWARE native_signed;
#endif

}; // namespace yaya {

#endif //__cplusplus

#endif // YAYA_GLOBALDEF_H_
