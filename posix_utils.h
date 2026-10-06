/* -*- c++ -*-
   POSIX環境下で使うユーティリティ類。
*/
#ifndef _POSIX_UTILS_H_INCLUDED_
#define _POSIX_UTILS_H_INCLUDED_
#include <string>

std::string lc(const std::string& fname); // lower case
std::string get_fname(const std::string& fpath);
std::string get_extension(const std::string& fname);
std::string drop_extension(const std::string& fname);
std::string change_extension(const std::string& fname, const std::string& ext);
std::string::size_type file_content_search(const std::string& file, const std::string& str);
std::string::size_type bm_search(const std::string& world, const std::string& data);
std::string posix_path(const std::wstring& path); // UTF-8 にして fix_filepath を通す
void fix_filepath(std::string& str);
void fix_filepath(std::wstring& str);

#endif
