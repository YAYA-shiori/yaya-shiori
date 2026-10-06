/* -*- c++ -*-
   POSIX環境下で使うユーティリティ類。
   */
#include <fstream>
#include <string>

#include <memory>

#if defined(POSIX)
# include <stdio.h>
# include <time.h>
# include <sys/resource.h>
# include <sys/time.h>
# include <unistd.h>
# if defined(__APPLE__)
#  include <sys/sysctl.h>
#  include <mach/mach.h>
# endif
#endif

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
	std::streamsize len;

	while (is.good()) {
		len = is.read(buf, sizeof buf).gcount();
		if (len == 0) {
			break;
		}
		content.append(buf,(size_t)len);
	}
	is.close();

	return bm_search(content, str);
}

std::string::size_type bm_search(const std::string& world, const std::string& data) {
	std::string::size_type data_len = data.length();
	if (data_len == 0 || world.length() < data_len) {
		return std::string::npos;
	}
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

size_t Ccct_ConvUnicodeToUTF8(std::string &buf,const wchar_t *pStrw);

std::string posix_path(const std::wstring& path) {
	// パスは UTF-8 として扱う（内部の文字列は UTF-16）
	std::string s;
	Ccct_ConvUnicodeToUTF8(s, path.c_str());
	fix_filepath(s);
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

#if defined(POSIX)
unsigned int posix_random_seed() {
	// 以前は gettimeofday の tv_usec（0～999999）だけだった
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned int seed = static_cast<unsigned int>(tv.tv_sec) * 1000003U ^ static_cast<unsigned int>(tv.tv_usec);

	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		seed ^= static_cast<unsigned int>(ts.tv_nsec) * 2654435761U;
		seed ^= static_cast<unsigned int>(ts.tv_sec) << 8;
	}
	seed ^= static_cast<unsigned int>(getpid()) << 16;
	return seed;
}

void posix_get_meminfo(posix_meminfo& mi) {
	mi.total_phys = 0;
	mi.avail_phys = 0;
	mi.total_virtual = 0;
	mi.avail_virtual = 0;

	unsigned long long page_size = 0;
	long ps = sysconf(_SC_PAGESIZE);
	if (ps > 0) {
		page_size = static_cast<unsigned long long>(ps);
	}
	unsigned long long used_virtual = 0;

#if defined(__linux__)
	// MemAvailable が無い古いカーネルでは MemFree + Buffers + Cached で代用する
	FILE* fp = fopen("/proc/meminfo", "r");
	if (fp != NULL) {
		char line[256];
		unsigned long long total = 0, avail = 0, mem_free = 0, buffers = 0, cached = 0;
		bool has_avail = false;
		while (fgets(line, sizeof(line), fp) != NULL) {
			unsigned long long v;
			if (sscanf(line, "MemTotal: %llu kB", &v) == 1) { total = v; }
			else if (sscanf(line, "MemAvailable: %llu kB", &v) == 1) { avail = v; has_avail = true; }
			else if (sscanf(line, "MemFree: %llu kB", &v) == 1) { mem_free = v; }
			else if (sscanf(line, "Buffers: %llu kB", &v) == 1) { buffers = v; }
			else if (sscanf(line, "Cached: %llu kB", &v) == 1) { cached = v; }
		}
		fclose(fp);
		mi.total_phys = total * 1024;
		mi.avail_phys = (has_avail ? avail : mem_free + buffers + cached) * 1024;
	}

	fp = fopen("/proc/self/statm", "r");
	if (fp != NULL) {
		unsigned long long pages;
		if (fscanf(fp, "%llu", &pages) == 1) {
			used_virtual = pages * page_size;
		}
		fclose(fp);
	}
#elif defined(__APPLE__)
	unsigned long long memsize = 0;
	size_t len = sizeof(memsize);
	if (sysctlbyname("hw.memsize", &memsize, &len, NULL, 0) == 0) {
		mi.total_phys = memsize;
	}

	// 空きと、捨てればすぐ使える inactive を足したものを「使えるメモリ」とする
	mach_port_t host = mach_host_self();
	vm_statistics64_data_t vs;
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
	if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vs), &count) == KERN_SUCCESS) {
		mi.avail_phys = (static_cast<unsigned long long>(vs.free_count) + vs.inactive_count) * page_size;
	}
	mach_port_deallocate(mach_task_self(), host);

	mach_task_basic_info_data_t ti;
	mach_msg_type_number_t ti_count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&ti), &ti_count) == KERN_SUCCESS) {
		used_virtual = ti.virtual_size;
	}
#else
	long pages = sysconf(_SC_PHYS_PAGES);
	if (pages > 0) {
		mi.total_phys = static_cast<unsigned long long>(pages) * page_size;
	}
# if defined(_SC_AVPHYS_PAGES)
	pages = sysconf(_SC_AVPHYS_PAGES);
	if (pages > 0) {
		mi.avail_phys = static_cast<unsigned long long>(pages) * page_size;
	}
# endif
#endif

	if (mi.avail_phys > mi.total_phys) {
		mi.avail_phys = mi.total_phys;
	}

	// 仮想メモリは、Windows と同じくプロセスが使えるアドレス空間の大きさと、その残り
	struct rlimit rl;
	if (getrlimit(RLIMIT_AS, &rl) == 0 && rl.rlim_cur != RLIM_INFINITY) {
		mi.total_virtual = static_cast<unsigned long long>(rl.rlim_cur);
	}
	else if (sizeof(void*) >= 8) {
		mi.total_virtual = 1ULL << 47;
	}
	else {
		mi.total_virtual = 3ULL << 30;
	}
	mi.avail_virtual = (mi.total_virtual > used_virtual) ? mi.total_virtual - used_virtual : 0;
}
#endif
