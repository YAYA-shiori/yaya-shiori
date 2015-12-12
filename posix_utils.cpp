/* -*- c++ -*-
   POSIX環境下で使うユーティリティ類。
   */
#include <fstream>
#include <string>

#include <memory>

#include "posix_utils.h"

std::string lc(const std::string& fname) {
	std::string result;
	for (std::string::const_iterator ite = fname.begin(); ite != fname.end(); ite++) {
		if (*ite >= 'A' && *ite <= 'Z') {
			result += (*ite + 0x20);
		}
		else {
			result += *ite;
		}
	}
	return result;
}

std::string get_fname(const std::string& fpath) {
	std::string::size_type slash_pos = fpath.rfind('/');
	if (slash_pos == std::string::npos) {
		return fpath;
	}
	else {
		return fpath.substr(slash_pos + 1);
	}
}

std::string get_extension(const std::string& fname) {
	std::string::size_type period_pos = fname.rfind('.');
	if (period_pos == std::string::npos) {
		return std::string();
	}
	else {
		return fname.substr(period_pos + 1);
	}
}

std::string drop_extension(const std::string& fname) {
	std::string::size_type period_pos = fname.rfind('.');
	if (period_pos == std::string::npos) {
		return fname;
	}
	else {
		return fname.substr(0, period_pos);
	}
}

std::string change_extension(const std::string& fname, const std::string& extension) {
	return drop_extension(fname) + '.' + extension;
}

std::string::size_type file_content_search(const std::string& file, const std::string& str) {
	std::string content;

	std::ifstream is(file.c_str());
	char buf[1024];
	int len;

	while (is.good()) {
		len = is.read(buf, sizeof buf).gcount();
		if (len == 0) {
			break;
		}
		content.append(buf, len);
	}
	is.close();

	return bm_search(content, str);
}

std::string::size_type bm_search(const std::string& world, const std::string& data) {
	std::string::size_type data_len = data.length();
	std::unique_ptr<std::string::size_type[]> skip(new std::string::size_type[256]);
	for (std::string::size_type i = 0; i < 256; i++) {
		skip[i] = data_len;
	}
	for (std::string::size_type i = 0; i < data_len-1; i++) {
		skip[static_cast<unsigned char>(data[i])] =
			data_len - i - 1;
	}
	std::string::size_type limit = world.length() - data.length();
	for (std::string::size_type i = 0;
			i <= limit;
			i += skip[static_cast<unsigned char>(world[i+data_len-1])]) {
		if (world[i+data_len-1] != data[data_len-1]) {
			continue;
		}

		bool matched = true;
		for (std::string::size_type j = 0; j < data_len; j++) {
			if (world[i+j] != data[j]) {
				matched = false;
				break;
			}
		}
		if (matched) {
			return i;
		}
	}
	return std::string::npos;
}

std::wstring widen(const std::string& str) {
	std::wstring ws;
	ws.reserve(str.size());
	for (std::string::const_iterator ite = str.begin(); ite != str.end(); ite++) {
		ws += static_cast<wchar_t>(*ite);
	}
	return ws;
}

std::string narrow(const std::wstring& str) {
	std::string s;
	s.reserve(str.size());
	for (std::wstring::const_iterator ite = str.begin(); ite != str.end(); ite++) {
		s += static_cast<char>(*ite);
	}
	return s;
}

void fix_filepath(std::string& str) {
	// \は/にし、重複した/を消して一つにする。
	for (std::string::iterator ite = str.begin(); ite != str.end(); ite++) {
		if (*ite == '\\') {
			*ite = '/';
		}
	}
	while (true) {
		std::string::size_type pos = str.find("//");
		if (pos == std::string::npos) {
			break;
		}
		str.erase(pos, 1);
	}
}

void fix_filepath(std::wstring& str) {
	// \は/にし、重複した/を消して一つにする。
	for (std::wstring::iterator ite = str.begin(); ite != str.end(); ite++) {
		if (*ite == L'\\') {
			*ite = L'/';
		}
	}
	while (true) {
		std::wstring::size_type pos = str.find(L"//");
		if (pos == std::wstring::npos) {
			break;
		}
		str.erase(pos, 1);
	}
}
