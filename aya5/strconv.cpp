#include <map>

#include "strconv.h"

const int aya::strconv::UTF8 = 0;
const int aya::strconv::CP932 = 1;
std::map<std::string, int> *aya::strconv::encoding_name_to_id = NULL;

std::string aya::strconv::convert(
    const std::string& orig, int from, int to) throw (aya::bad_encoding_exception) {

    if (from == UTF8) {
	if (to == CP932) {
	    return conv_utf8_to_cp932(orig);
	}
	else {
	    throw aya::bad_encoding_exception(to);
	}
    }
    else if (from == CP932) {
	if (to == UTF8) {
	    return conv_cp932_to_utf8(orig);
	}
	else {
	    throw aya::bad_encoding_exception(to);
	}
    }
    else {
	throw aya::bad_encoding_exception(from);
    }
}

std::string aya::strconv::convert(
    const std::string& orig,
    const std::string& from,
    const std::string& to) throw (aya::bad_encoding_exception) {
    
    init_encoding_name_table();

	std::map<std::string, int>::iterator ite = encoding_name_to_id->find(from);
    if (ite == encoding_name_to_id->end()) {
	// not found
	throw aya::bad_encoding_exception(from);
    }
    int from_id = ite->second;

    ite = encoding_name_to_id->find(to);
    if (ite == encoding_name_to_id->end()) {
	// not found
	throw aya::bad_encoding_exception(to);
    }
    int to_id = ite->second;

    if (from_id != to_id) {
	return convert(orig, from_id, to_id);
    }
    else {
	return orig;
    }
}

std::string aya::strconv::conv_cp932_to_utf8(const std::string& src) throw () {
    // 出力の長さは入力の1.6倍になると仮定する。
	std::string out;
    out.reserve(static_cast<std::string::size_type>(src.length() * 1.6));

    for (std::string::const_iterator ite = src.begin(); ite != src.end(); ite++) {
	unsigned char c = *ite;
	// 0x81～0x9F, 0xE0～0xFCは2バイト文字の1バイト目である。
	if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) {
	    ite++;
	    if (ite == src.end()) {
		// 2バイト目が無い。エラーだが例外を投げても仕方無いのでここで変換終了。
		break;
	    }
	    unsigned int word = c << 8 | (*ite & 0xff);
	    append_utf8(out, table_cp932_to_unicode[word]);
	}
	else {
	    unsigned int word = c;
	    append_utf8(out, table_cp932_to_unicode[word]);
	}
    }

    return out;
}

std::string aya::strconv::conv_utf8_to_cp932(const std::string& src) throw () {
    // 出力の長さは入力の0.8倍になると仮定する。
	std::string out;
    out.reserve(static_cast<std::string::size_type>(src.length() * 0.8));

	std::string::const_iterator end = src.end();
    for (std::string::const_iterator ite = src.begin(); ite != end; ) {
	unsigned int unicode = fetch_utf8(ite, end);
	if (unicode > 0xffff) {
	    continue; // 3バイト以上あるので、素通しするとseg fault。
	}
	unsigned int sjis = table_unicode_to_cp932[unicode];
	if (sjis > 0xff) {
	    // 2バイトある。
	    out += static_cast<char>(sjis >> 8);
	    out += static_cast<char>(sjis);
	}
	else {
	    // 1バイト
	    out += static_cast<char>(sjis);
	}
    }

    return out;
}
