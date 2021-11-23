#pragma once

#ifdef FIX_OLD_COMPILER


#ifndef _MSVC_LANG
//C++11 or older

#include <boost/shared_ptr.hpp>

#define std_shared_ptr   boost::shared_ptr

#define nullptr 0

#else

#define std_shared_ptr   std::shared_ptr

#endif //_MSVC_LANG


#define AYX_WIN64_AWARE


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
#define AYX_WIN64_AWARE __w64
#endif


#if _MSC_VER <= 1200

#define for if(0);else for

namespace std {
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
};

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

#else

#include <cstdint>

#endif //1200


#ifndef _MSVC_LANG
//C++11 or older
#define constexpr const
#define emplace_back(p) push_back(p)
#endif //_MSVC_LANG


#endif //MSC_VER



#endif //FIX_OLD_COMPILER

