#ifndef YAYA_STRTYPE_H__
#define YAYA_STRTYPE_H__

#include <cwchar>
#include <string>
#include <map>

#include <memory>

#include "fix_old_compiler.h"

namespace yaya {
	typedef wchar_t char_t;
	typedef std::basic_string<char_t> string_t;
	typedef std::int64_t int_t;

	typedef std_shared_ptr<string_t> share_string_t;
	typedef std_shared_ptr<const string_t> const_share_string_t;

	typedef std::map<yaya::string_t,int> indexmap;

	template<class T> void shared_ptr_deep_copy(const std_shared_ptr<T> &from,std_shared_ptr<T> &to) {
		if ( from.get() ) {
			to.reset(new T(*from));
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

#endif // #ifndef YAYA_STRTYPE_H__
