#include <string>

#include <cstdlib>
#include <cstring>

#include "ccct.h"
#include "manifest.h"
#include "strconv.h"

static char* malloc_str(const std::string& str) {
	char* s = static_cast<char*>(malloc(str.length() + 1));
	memcpy(s, str.c_str(), str.length() + 1);
	return s;
}

static wchar_t* malloc_str(const std::wstring& str) {
	size_t len = (str.length() + 1) * sizeof(wchar_t);
	wchar_t* s = static_cast<wchar_t*>(malloc(len));
	memcpy(s, str.data(), len);
	return s;
}

/*
   template <class charT, class traits = std::char_traits<charT>,
   class Allocator = std::allocator<charT> >
   static charT * malloc_str(const std::basic_string<charT, traits, Allocator>& str) {
   typedef std::basic_string<charT, traits, Allocator> str_t;

   str_t::size_type len = (str.length() + 1) * sizeof(str_t::value_type);
   str_t::pointer_type s = static_cast<str_t::pointer_type>(std::malloc(len));
   std::memcpy(s, str.c_str(), len);
   return s;
   }
   */

char* Ccct::Ucs2ToMbcs(const wchar_t* wstr, int charset) {
	return Ucs2ToMbcs(std::wstring(wstr), charset);
}
char* Ccct::Ucs2ToMbcs(const std::wstring& wstr, int charset) {
	std::string utf8 = aya::strconv::convert_ucs_to_utf8<wchar_t>(wstr);
	std::string mbcs;
	if (charset == CHARSET_UTF8) {
		// 変換不要
		mbcs = utf8;
	}
	else if (charset == CHARSET_SJIS || charset == CHARSET_DEFAULT) {
		// 「OSデフォルト」は常にSJIS。
		mbcs = aya::strconv::convert(utf8, aya::strconv::UTF8, aya::strconv::CP932);
	}
	return malloc_str(mbcs);
}

wchar_t* Ccct::MbcsToUcs2(const char* mstr, int charset) {
	return MbcsToUcs2(std::string(mstr), charset);
}
wchar_t* Ccct::MbcsToUcs2(const std::string &mstr, int charset) {
	std::string utf8;
	if (charset == CHARSET_UTF8) {
		// 変換不要
		utf8 = mstr;
	}
	else if (charset == CHARSET_SJIS || charset == CHARSET_DEFAULT) {
		// 「OSデフォルト」は常にSJIS。
		utf8 = aya::strconv::convert(mstr, aya::strconv::CP932, aya::strconv::UTF8);
	}
	return malloc_str(aya::strconv::convert_utf8_to_ucs<wchar_t>(utf8));
}
