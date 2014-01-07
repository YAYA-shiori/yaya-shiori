/******************************************************************************
	バベル -babel-
		バベルモジュールソースファイル
											Coded by Wraith	in July 14, 2002.
******************************************************************************/

//　Tab幅を４文字に設定して表示させてください。

///////////////////////////////////////////////////////////////////////////////
//
//	■ babel.cpp
//		http://tricklib.com/cxx/ex/babel/babel.cpp
//
//	□ 関連ファイル
//		本モジュールのヘッダファイル
//		http://tricklib.com/cxx/ex/babel/bbl.h
//		本モジュールで使用するスコアマッピングCSV
//		http://tricklib.com/cxx/ex/babel/scoremap.csv
//		本モジュールで使用するCP932マッピングCSV
//		http://tricklib.com/cxx/ex/babel/cp932.csv
//		本モジュールで使用するUTCマッピングCSV
//		http://tricklib.com/cxx/ex/babel/utc.csv
//		本モジュールで使用するアップルマッピングCSV
//		http://tricklib.com/cxx/ex/babel/apple.csv
//		本モジュールで使用するSJIS-EUCマッピングCSV
//		http://tricklib.com/cxx/ex/babel/sjis-euc.csv
//		本モジュールで使用するEUC-UNICODEマッピングCSV
//		http://tricklib.com/cxx/ex/babel/euc-uni.csv
//		本モジュールで使用するCP932マッピングデータファイル
//		http://tricklib.com/cxx/ex/babel/c932-uni.dat
//		http://tricklib.com/cxx/ex/babel/uni-c932.dat
//		本モジュールで使用するUTCマッピングデータファイル
//		http://tricklib.com/cxx/ex/babel/utc-uni.dat
//		http://tricklib.com/cxx/ex/babel/uni-utc.dat
//		本モジュールで使用するSJIS-EUCマッピングデータファイル
//		http://tricklib.com/cxx/ex/babel/sjis-euc.dat
//		http://tricklib.com/cxx/ex/babel/euc-sjis.dat
//		本モジュールで使用するEUC-UNICODEマッピングデータファイル
//		http://tricklib.com/cxx/ex/babel/uni-euc.dat
//		http://tricklib.com/cxx/ex/babel/euc2-uni.dat
//		http://tricklib.com/cxx/ex/babel/euc3-uni.dat
//		本モジュールで使用するスコアマッピングデータファイル
//		http://tricklib.com/cxx/ex/babel/sjis-scr.dat
//		http://tricklib.com/cxx/ex/babel/euc-scr.dat
//		http://tricklib.com/cxx/ex/babel/uni-scr.dat
//		全ファイルパック
//		http://tricklib.com/cxx/ex/babel/babel.lzh
//		http://tricklib.com/cxx/ex/babel/babel.zip
//
//	□ リファレンス・サポートページ
//		http://tricklib.com/cxx/ex/babel/
//
//	□ ライセンス情報
//		http://tricklib.com/license.htm
//

#include <map>

#include "babel.h"

/******************************************************************************
	□■□■                      TrickPalace                       □■□■
	■□■□              http://www.trickpalace.net/               ■□■□
******************************************************************************/

#define ARRAY_SIZE(X)  		(sizeof(X)/sizeof(X[0]))
#define ARRAY_END(X)   		(X +ARRAY_SIZE(X))
#define AD_LIBTIUM(XXX)		if (!(XXX)) ; else (*(XXX))
#define SET_FORWARD(XXX)	XXX = XXX

//inline unsigned char & unsignize(char & X) {
//	return (unsigned char &)X;
//}
#if defined(__GNUC__)
inline const unsigned char unsignize(const char X) {
	return (const unsigned char)X;
}
#else
inline const unsigned char & unsignize(const char & X) {
	return (const unsigned char &)X;
}
#endif
//inline unsigned int & unsignize(int & X) {
//	return (unsigned int &)X;
//}
#if defined(__GNUC__)
inline const unsigned int unsignize(const int X) {
	return (const unsigned int)X;
}
#else
inline const unsigned int & unsignize(const int & X) {
	return (const unsigned int &)X;
}
#endif
//inline wchar_t & unsignize(wchar_t & X) {
//	return (wchar_t &)X;
//}
inline const wchar_t & unsignize(const wchar_t & X) {
	return (const wchar_t &)X;
}

/******************************************************************************
	□■□■                       cppll ML                         □■□■
	■□■□           http://www.trickpalace.net/cppll/            ■□■□
******************************************************************************/

//
//  ●babel
//
namespace babel {

//
//  ▼各種基本データクラス [ 〆 ]
//

//	variation one for performance
//typedef unsigned char bbl_char;
typedef unsigned int bbl_char;
typedef unsigned int bbl_code;
typedef std::map<bbl_code, bbl_code> bbl_map;
typedef std::pair<bbl_code, bbl_code> bbl_pair;
#if	defined(__BBL_USE_UNICODE__)
typedef std::map<bbl_code, bbl_wstring> bbl_map_1_n;
typedef std::pair<bbl_code, bbl_wstring> bbl_pair_1_n;
typedef std::map<bbl_wstring, bbl_code> bbl_map_n_1;
typedef std::pair<bbl_wstring, bbl_code> bbl_pair_n_1;
#endif	//	defined(__BBL_USE_UNICODE__)

#if defined(__BBL_USING_STDMAP_TABLE__)
typedef std::map<bbl_code, int> bbl_score_map;
typedef std::pair<bbl_code, int> bbl_score_pair;
#endif	//	defined(__BBL_USING_STDMAP_TABLE__)

#if defined(__BBL_USING_STATIC_TABLE__)
class bbl_table {
	const bbl_code ***value;
	bbl_table();
  public:
	bbl_table(const bbl_code ***X_value) :value(X_value) {}
//	static bbl_table get_instance(const bbl_code **X) {
//		return *((bbl_table*)(X));
//	}
	const bbl_code operator[](int index) const {
		assert(0 <= index);
		if (0xFFFF < index) {
			return 0;
		}
		bbl_code result = 0;
		const int high = index >> 8;
		const bbl_code * const * const X_wire = value[high];
		if (X_wire) {
			const int mid = (index & 0xFF) >>5;
			const bbl_code * const X_line = X_wire[mid];
			if (X_line) {
				const int low = index & 0x1F;
				result = X_line[low];
			}
		}
		return result;
	}
};
template<
	int e00, int e01, int e02, int e03, int e04, int e05, int e06, int e07,
	int e08, int e09, int e0A, int e0B, int e0C, int e0D, int e0E, int e0F,
	int e10, int e11, int e12, int e13, int e14, int e15, int e16, int e17,
	int e18, int e19, int e1A, int e1B, int e1C, int e1D, int e1E, int e1F>
class bbl_line {
  public:
	static const bbl_code value[];
};
template<
	int e00, int e01, int e02, int e03, int e04, int e05, int e06, int e07,
	int e08, int e09, int e0A, int e0B, int e0C, int e0D, int e0E, int e0F,
	int e10, int e11, int e12, int e13, int e14, int e15, int e16, int e17,
	int e18, int e19, int e1A, int e1B, int e1C, int e1D, int e1E, int e1F>
const bbl_code bbl_line<
	e00, e01, e02, e03, e04, e05, e06, e07, e08, e09, e0A, e0B, e0C, e0D, e0E, e0F,
	e10, e11, e12, e13, e14, e15, e16, e17, e18, e19, e1A, e1B, e1C, e1D, e1E, e1F>::value[] = {
	e00, e01, e02, e03, e04, e05, e06, e07, e08, e09, e0A, e0B, e0C, e0D, e0E, e0F,
	e10, e11, e12, e13, e14, e15, e16, e17, e18, e19, e1A, e1B, e1C, e1D, e1E, e1F,
};
//typedef bbl_line<	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
//					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0> bbl_null_line;
class bbl_null_line {
  public:
	static const bbl_code *value;
};
const bbl_code *bbl_null_line::value = NULL;

template<	class line0, class line1, class line2, class line3,
			class line4, class line5, class line6, class line7>
class bbl_wire {
  public:
	static const bbl_code *value[];
};
template<	class line0, class line1, class line2, class line3,
			class line4, class line5, class line6, class line7>
const bbl_code *bbl_wire<	line0, line1, line2, line3,
							line4, line5, line6, line7>::value[] = {
	line0::value,	line1::value,	line2::value,	line3::value,
	line4::value,	line5::value,	line6::value,	line7::value,
};

typedef const bbl_table bbl_score_map;
#endif	//	defined(__BBL_USING_STATIC_TABLE__)


//
//  ●変換テーブル
//
namespace bbl_transmap {

#if defined(__BBL_USING_STDMAP_TABLE__)
	const bbl_code sjis_euc_map[] = {
#		include	"sjis-euc.csv"
	};
#endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_code **sjis_euc_table[] = {
#		include "sjis-euc.dat"
	};
	const bbl_code **euc_sjis_table[] = {
#		include "euc-sjis.dat"
	};
#endif	//	defined(__BBL_USING_STATIC_TABLE__)

#if defined(__BBL_LOAD_UNICODE_MAP_CP932__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	const bbl_code cp932_map[] = {
#		include	"cp932.csv"
	//	■source file
	//	http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT
	//
	//	■perl script
	//	open(TXT, "CP932.TXT");
	//	open(CSV, "> cp932.csv");
	//	while($line = <TXT>) {
	//		if ($line =~ /^(0x.*)\t(0x.*)\t#[\t\s]*(.*)/) {
	//			print CSV "$1, $2,\t\x2F\x2F\t$3\n";
	//		}
	//	}
	//	close(TXT);
	//	close(CSV);
	//
	//	■information
	//	上記スクリプトで生成したファイルのままでも構わないのですが、
	//	プログラム上実際には 0x00 ～ 0x7F のマッピングに関しては使用
	//	していないので若干の効率化の為に添付の cp932.csv はその先頭
	//	の128行を削除しています。また、異なるマッピングでUNICODEに
	//	変換されたテキストをより綺麗な形でSJISに戻せるようにデータを
	//	追加しています。

	//	これは MS-Winddows で行われる変換に使用されるマッピングですが、
	//	UTCが公表してるマッピングやアップルのマッピングを使用する場合
	//	はそれぞれ __BBL_UNICODE_MAP_UTC__ もしくは __BBL_UNICODE_MAP_APPLE__
	//	 マクロを指定してください。
	};
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)

#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_code **cp932_unicode_table[] = {
#		include "c932-uni.dat"
	};
	const bbl_code **unicode_cp932_table[] = {
#		include "uni-c932.dat"
	};
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_CP932__)

#if	defined(__BBL_USE_UNICODE__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	const bbl_code euc_map[] = {
#		include "euc-uni.csv"
	};
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_code **euc2_unicode_table[] = {
#		include "euc2-uni.dat"
	};
	const bbl_code **euc3_unicode_table[] = {
#		include "euc3-uni.dat"
	};
	const bbl_code **unicode_euc_table[] = {
#		include "uni-euc.dat"
	};
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_USE_UNICODE__)

#if defined(__BBL_LOAD_UNICODE_MAP_UTC__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	const bbl_code utc_map[] = {
#		include	"utc.csv"
	//	■source file
	//	http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
	//
	//	■perl script
	//	open(TXT, "SHIFTJIS.TXT");
	//	open(CSV, "> utc.csv");
	//	while($line = <TXT>) {
	//		if ($line =~ /^(0x.*)\t(0x.*)\t#[\t\s]*(.*)/) {
	//			print CSV "$1, $2,\t\x2F\x2F\t$3\n";
	//		}
	//	}
	//	close(TXT);
	//	close(CSV);
	//
	//	■information
	//	cp932.csv と違いこのマッピングでは表示が優先され 0x5C が 0x00A5 に割り
	//	当てられていますので注意してください。また、このマッピングには半角カナ
	//	の部分のマッピングが存在しないので添付の utc.csv には便宜上 cp932.csv
	//	より該当部分をコピペしています。
	};
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)

#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_code **utc_unicode_table[] = {
#		include "utc-uni.dat"
	};
	const bbl_code **unicode_utc_table[] = {
#		include "uni-utc.dat"
	};
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_UTC__)

#if defined(__BBL_LOAD_UNICODE_MAP_APPLE__)
	const bbl_code appll_map[] = {
#		include	"apple.csv"
	//	■source file
	//	http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/JAPANESE.TXT
	//
	//	■perl script
	//	open(TXT, "JAPANESE.TXT");
	//	open(CSV, "> apple.csv");
	//	while($line = <TXT>) {
	//		if ($line =~ /^(0x.*)\t(0x.*)\t*#(.*)/) {
	//			$sjis = $1;
	//			$unicode_values = $2;
	//			$comment = $3;
	//			$unicode_values ~= s/\x2B/, /g;
	//			print CSV "$sjis, $unicode_values, 0x0000,\t\x2F\x2F\t$comment\n";
	//		}
	//	}
	//	close(TXT);
	//	close(CSV);
	//
	//	■information
	//	cp932.csv と違いこのマッピングでは表示が優先され 0x5C が 0x00A5 に割り
	//	当てられていますので注意してください。また、このマッピングは他の二つの
	//	マッピングとは異なり Shift_JIS 上での１文字が複数ワードに割り当てられて
	//	いる文字も存在します。また、この複数ワードに割り当てられている文字は
	//	プライベート領域のコードが使用されていたり、Windows などでは対応されて
	//	いないコードが使用されている為に、Apple 社のマシン以外でそれらの文字が
	//	正しく表示されることは期待できません。
	};
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_APPLE__)

	const bbl_code half_to_full_jis_map1[] = {	//	濁点
		0x33, 0x2574,
		0x36, 0x252C, 0x37, 0x252E, 0x38, 0x2530, 0x39, 0x2532, 0x3A, 0x2534,
		0x3B, 0x2536, 0x3C, 0x2538, 0x3D, 0x253A, 0x3E, 0x253C, 0x3F, 0x253E,
		0x40, 0x2540, 0x41, 0x2542, 0x42, 0x2545, 0x43, 0x2547, 0x44, 0x2549,
		0x4A, 0x2550, 0x4B, 0x2553, 0x4C, 0x2556, 0x4D, 0x2559, 0x4E, 0x255C,
	};
	const bbl_code half_to_full_jis_map2[] = {	//	半濁点
		0x4A, 0x2551, 0x4B, 0x2554, 0x4C, 0x2557, 0x4D, 0x255A, 0x4E, 0x255D,
	};
	const bbl_code half_to_full_jis_map3[] = {	//	単文字
				0x2123, 0x2156, 0x2157, 0x2122, 0x2126, 0x2572, 0x2521,
		0x2523, 0x2525, 0x2527, 0x2529, 0x2563, 0x2565, 0x2567, 0x2543,
		0x213C, 0x2522, 0x2524, 0x2526, 0x2528, 0x252A, 0x252B, 0x252D,
		0x252F, 0x2531, 0x2533, 0x2535, 0x2537, 0x2539, 0x253B, 0x253D,
		0x253F, 0x2541, 0x2544, 0x2546, 0x2548, 0x254A, 0x254B, 0x254C,
		0x254D, 0x254E, 0x254F, 0x2552, 0x2555, 0x2558, 0x255B, 0x255E,
		0x255F, 0x2560, 0x2561, 0x2562, 0x2564, 0x2566, 0x2568, 0x2569,
		0x256A, 0x256B, 0x256C, 0x256D, 0x256F, 0x2573, 0x212B, 0x212C,
	};

#	if defined(__BBL_USING_STDMAP_TABLE__)
	bbl_map sjis_euc;
	bbl_map euc_sjis;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_table sjis_euc(sjis_euc_table);
	const bbl_table euc_sjis(euc_sjis_table);
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#if defined(__BBL_LOAD_UNICODE_MAP_CP932__)
//	bbl_map sjis_unicode;
//	bbl_map unicode_sjis;
#	if defined(__BBL_USING_STDMAP_TABLE__)
	bbl_map cp932_unicode;
	bbl_map unicode_cp932;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_table cp932_unicode(cp932_unicode_table);
	const bbl_table unicode_cp932(unicode_cp932_table);
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_CP932__)

#if	defined(__BBL_USE_UNICODE__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	bbl_map euc_unicode;
	bbl_map &euc2_unicode = euc_unicode;
	bbl_map &euc3_unicode = euc_unicode;
	bbl_map unicode_euc;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_table euc2_unicode(euc2_unicode_table);
	const bbl_table euc3_unicode(euc3_unicode_table);
	const bbl_table unicode_euc(unicode_euc_table);
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_USE_UNICODE__)

#if defined(__BBL_LOAD_UNICODE_MAP_UTC__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	bbl_map utc_unicode;
	bbl_map unicode_utc;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_table utc_unicode(utc_unicode_table);
	const bbl_table unicode_utc(unicode_utc_table);
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_UTC__)
#if defined(__BBL_LOAD_UNICODE_MAP_APPLE__)
	bbl_map apple_unicode;
	bbl_map_1_n apple_unicode_n;
	bbl_map unicode_apple;
	bbl_map_n_1 unicode_n_apple;
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_APPLE__)
	bbl_map half_to_full_jis1;
	bbl_map half_to_full_jis2;

};	//	bbl_transmap

//
//  ●スコアテーブル
//
namespace bbl_scoremap {
#if defined(__BBL_USING_STDMAP_TABLE__)
	const int sampling_score_map[] = {
		//	score, sjis-code, euc-code, utf8-code,	//	sjis-char
#		include	"scoremap.csv"
		//
		//	babel 自身を利用し、ローカルマシンの Temporary Internet Files 中の
		//	全テキストからスコアを作成。出現数をスコアとし、スコアが 100 に満た
		//	ない文字や UNICODE 上で 0xFF 以下のコードとなる文字は除外。
		//	日本語以外(外国語)のテキストも含まれる為、若干のノイズも混じってい
		//	ると思われる。
	};

	bbl_score_map sjis_score;
	bbl_score_map euc_score;
	bbl_score_map utf8_score;

	const int measure_point = sampling_score_map[0];
#endif	//	defined(__BBL_USING_STDMAP_TABLE__)

#if defined(__BBL_USING_STATIC_TABLE__)
	const bbl_code **sjis_score_table[] = {
#		include "sjis-scr.dat"
	};
	const bbl_code **euc_score_table[] = {
#		include "euc-scr.dat"
	};
	const bbl_code **unicode_score_table[] = {
#		include "uni-scr.dat"
	};

	bbl_score_map sjis_score(sjis_score_table);
	bbl_score_map euc_score(euc_score_table);
	bbl_score_map unicode_score(unicode_score_table);

	const int measure_point = 395578;	//	sampling_score_map[0];
#endif	//	defined(__BBL_USING_STATIC_TABLE__)

};	//	bbl_scoremap


template<class T> inline void append_token(
		unsigned int *i, T *buffer, const std::basic_string<T> & token) {
	const int margin_size = translate_buffer_full_size -translate_buffer_size;
	unsigned int & j = *i;
	assert(token.length() <= margin_size);
	token.copy(buffer +j, margin_size);
	j += token.length();
}

//template<class T> inline void append_broken_char(
//		unsigned int *i, T *buffer, const std::basic_string<T> & broken_char = (const std::basic_string<T> &)bbl_term::get_broken_char()) {
//	const int margin_size = translate_buffer_full_size -translate_buffer_size;
//	unsigned int & j = *i;
//	assert(broken_char.length() <= margin_size);
//	broken_char.copy(buffer +j, margin_size);
//	j += broken_char.length();
//}

template<class T> inline void append_broken_char(
		unsigned int *i, T *buffer, const std::basic_string<T> & broken_char = (const std::basic_string<T> &)bbl_term::get_broken_char()) {
	append_token(i, buffer, broken_char);
}

#if	defined(__BBL_USE_UNICODE__)

#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
inline bool is_private_use_area(wchar_t unicode_char) {
	return 0xE000 <= unicode_char && unicode_char <= 0xF8FF;
}
#endif //	!defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)

//::::::
void ansi_to_unicode_engine::translate() {
//	void ansi_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			bbl_char current_char = unsignize(untranslated_buffer.at(i));
			if (current_char <= 0x7F) {
				unicode[j++] = current_char;
			} else {
				append_broken_char(&j, unicode);
			}
			++i;
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void unicode_to_ansi_engine::translate() {
//	void unicode_to_ansi_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type ansi[translate_buffer_full_size];

#ifdef	__BBL_SMALL_BOM_LOGIC__
	from_string_type::size_type p_bom = 0;
	while(from_string_type::npos != (p_bom = untranslated_buffer.find(bbl_term::unicode_bom))) {
		untranslated_buffer.erase(p_bom, 1);
	}
#endif	//	__BBL_SMALL_BOM_LOGIC__

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			wchar_t current_wchar = unsignize(untranslated_buffer.at(i));
			if (current_wchar <= 0x7F) {
				ansi[j++] = (unsigned char)current_wchar;
			} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
				if (!is_private_use_area(current_wchar)) {
					append_broken_char(&j, ansi);
				}
#else
				append_broken_char(&j, ansi);
#endif
			}
			++i;
		}
		ansi[j] = '\0';
		translated_buffer += ansi;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

//:::::::
#endif	//	defined(__BBL_USE_UNICODE__)


const bool sjis_lead_byte[] = { // 0x81～0x9F || 0xE0～0xFC
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
};
inline bool is_sjis_lead_byte(bbl_char lead_byte) {
	assert(lead_byte <= 0xFF);
	return sjis_lead_byte[lead_byte];
//	variation one for performance
//	return	0x81 <= lead_byte && lead_byte <= 0x9F ||
//			0xE0 <= lead_byte && lead_byte <= 0xFC;
}

const bool sjis_trail_byte[] = { // 0x40～0x7E || 0x80～0xFC
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
};
inline bool is_sjis_trail_byte(bbl_char trail_byte) {
	assert(trail_byte <= 0xFF);
	return sjis_trail_byte[trail_byte];
//	variation one for performance
//	return	0x40 <= trail_byte && trail_byte <= 0x7E ||
//			0x80 <= trail_byte && trail_byte <= 0xFC;
//	variation one for performance
//	return	0x40 <= trail_byte && 0x7F != trail_byte && trail_byte <= 0xFC;
}


void sjis_to_euc_engine::translate() {
//	void sjis_to_euc_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type euc[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length - i;

		if (rest_length <= 0) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_sjis_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_sjis_lead_byte(lead_char)) {
			//	漢字(全角文字)・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x40 <= trail_char && trail_char <= 0xFC && trail_char != 0x7F) {
					if (lead_char < 0xFA) {
						lead_char <<= 1;
						lead_char &= 0xFF;
						if (trail_char <= 0x9E) {
							if (lead_char <= 0x3E) {
								lead_char -= 0x61;
							} else {
								lead_char += 0x1F;
							}
							if (0x80 <= trail_char) {
								trail_char += 0x60;
							} else {
								trail_char += 0x61;
							}
						} else {
							if (lead_char <= 0x3E) {
								lead_char -= 0x60;
							} else {
								lead_char += 0x20;
							}
							trail_char += 0x02;
						}
						euc[j++] = lead_char;
						euc[j++] = trail_char;
						i+=2;
					} else {
						bbl_code sjis_code = lead_char *0x100 +trail_char;
						bbl_code euc_code = bbl_transmap::sjis_euc[sjis_code];
						if (0 != euc_code) {
							euc[j++] = euc_code /0x100;
							euc[j++] = euc_code & 0xFF;
							i+=2;
						} else {
							append_broken_char(&j, euc);
							++i;
						}
					}
				} else {
					append_broken_char(&j, euc);
					++i;
				}
			} else if (0xA1 <= lead_char && lead_char <= 0xDF) {
			//	半角カナ・・・
				euc[j++] = 0x8E;
				euc[j++] = lead_char;
				++i;
			} else {
			//	その他の半角・・・
				euc[j++] = lead_char;
				++i;
			}
		}
		euc[j] = '\0';
		translated_buffer += euc;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

inline bool is_euc_aux_lead_byte(bbl_char aux_lead_byte) {
	assert(aux_lead_byte <= 0xFF);
	return 0x8F == aux_lead_byte;
}

const bool euc_lead_byte[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
};
inline bool is_euc_lead_byte(bbl_char lead_byte) {

//	not include aux lead byte

	assert(lead_byte <= 0xFF);
	return euc_lead_byte[lead_byte];
//	variation one for performance
//	return 0x8E <= lead_byte && lead_byte <=0xFE &&
//			0x8F != lead_byte && 0xA0 != lead_byte;
}
inline bool is_euc_trail_byte(bbl_char trail_byte) {
	assert(trail_byte <= 0xFF);
	return 0x80 <= trail_byte;
}


#if	!defined(__BBL_STRICT_TRANSLATE__)
void euc_to_sjis_engine::translate() {
//	void euc_to_sjis_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type sjis[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		unsigned char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_euc_lead_byte(next_char)) {
			break;
		}
		if (rest_length < 3 && is_euc_aux_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_euc_lead_byte(lead_char)) {
			//	漢字(全角文字) or 半角カナ・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x8E == lead_char && 0xA1 <= trail_char && trail_char <= 0xDF) {
				//	半角カナ・・・
					sjis[j++] = trail_char;
					i+=2;
				} else if (0x80 <= trail_char) {
					bbl_code euc_code = lead_char *0x100 +trail_char;
					if (!(0xF9A1 <= euc_code && euc_code <= 0xFCFE)) {
						if (lead_char & 0x01) {
							lead_char >>= 1;
							if (lead_char < 0x6F) {
								lead_char += 0x31;
							} else {
								lead_char += 0x71;
							}
							if (trail_char > 0xDF) {
								trail_char -= 0x60;
							} else {
								trail_char -= 0x61;
							}
						} else {
							lead_char >>= 1;
							if (lead_char <= 0x6F) {
								lead_char += 0x30;
							} else {
								lead_char += 0x70;
							}
							trail_char -= 0x02;
						}
						sjis[j++] = lead_char;
						sjis[j++] = trail_char;
						i+=2;
					} else {
						bbl_code sjis_code = bbl_transmap::euc_sjis[euc_code];
						if (0 != sjis_code) {
							sjis[j++] = sjis_code /0x100;
							sjis[j++] = sjis_code & 0xFF;
							i+=2;
						} else {
							append_broken_char(&j, sjis);
							++i;
						}
					}
				} else {
					append_broken_char(&j, sjis);
					++i;
				}
			} else if (is_euc_aux_lead_byte(lead_char)) {
			//	補助漢字・・・
				unsigned int k = i +2;
				if (untranslated_length <= k) {
					break;
				}
				//	SJIS には補助漢字を格納できない。
				append_broken_char(&j, sjis);
				i+=3;
			} else {
			//	その他の半角・・・
				sjis[j++] = lead_char;
				++i;
			}
		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
#endif	//	!defined(__BBL_STRICT_TRANSLATE__)

#if defined(__BBL_LOAD_UNICODE_MAP_CP932__)
void sjis_cp932_to_unicode_engine::translate() {
//	void sjis_cp932_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		unsigned char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_sjis_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_sjis_lead_byte(lead_char)) {
			//	漢字(全角文字)・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x40 <= trail_char && trail_char <= 0xFC && trail_char != 0x7F) {
					bbl_code sjis_code = lead_char *0x100 +trail_char;
					bbl_code unicode_code = bbl_transmap::cp932_unicode[sjis_code];
					if (0 != unicode_code) {
						unicode[j++] = (wchar_t)unicode_code;
						i+=2;
					} else {
						append_broken_char(&j, unicode);
						i+=2;
					}
				} else {
					append_broken_char(&j, unicode);
					++i;
				}
			} else if (0xA1 <= lead_char && lead_char <= 0xDF) {
			//	半角カナ・・・
				bbl_code unicode_code = bbl_transmap::cp932_unicode[lead_char];
				unicode[j++] = (wchar_t)unicode_code;
				++i;
			} else {
			//	その他の半角・・・
				unicode[j++] = lead_char;
				++i;
			}
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void unicode_to_sjis_cp932_engine::translate() {
//	void unicode_to_sjis_cp932_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type sjis[translate_buffer_full_size];

#ifdef	__BBL_SMALL_BOM_LOGIC__
	from_string_type::size_type p_bom = 0;
	while(from_string_type::npos != (p_bom = untranslated_buffer.find(bbl_term::unicode_bom))) {
		untranslated_buffer.erase(p_bom, 1);
	}
#endif	//	__BBL_SMALL_BOM_LOGIC__

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			wchar_t current_wchar = unsignize(untranslated_buffer.at(i));
			bool is_broken_char = false;
			bbl_code sjis_code;
			if (current_wchar <= 0x7F) {
				sjis_code = current_wchar;
			} else {
				sjis_code = bbl_transmap::unicode_cp932[current_wchar];
				if (0 == sjis_code) {
					is_broken_char = true;
				}
			}

			if (!is_broken_char) {
				if (sjis_code <= 0xFF) {
					sjis[j++] = (unsigned char)sjis_code;
				} else {
					sjis[j++] = sjis_code >> 8;
					sjis[j++] = 0xFF & sjis_code;
				}
			} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
				if (!is_private_use_area(current_wchar)) {
					append_broken_char(&j, sjis);
				}
#else
				append_broken_char(&j, sjis);
#endif
			}
			++i;
		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_CP932__)

#if defined(__BBL_LOAD_UNICODE_MAP_UTC__)
void sjis_utc_to_unicode_engine::translate() {
//	void sjis_utc_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		unsigned char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_sjis_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_sjis_lead_byte(lead_char)) {
			//	漢字(全角文字)・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x40 <= trail_char && trail_char <= 0xFC && trail_char != 0x7F) {
					bbl_code sjis_code = lead_char *0x100 +trail_char;
					bbl_code unicode_code = bbl_transmap::utc_unicode[sjis_code];
					if (0 != unicode_code) {
						unicode[j++] = (wchar_t)unicode_code;
						i+=2;
					} else {
						append_broken_char(&j, unicode);
						i+=2;
					}
				} else {
					append_broken_char(&j, unicode);
					++i;
				}
			} else {
			//	半角カナ＆その他の半角・・・
				bbl_code unicode_code = bbl_transmap::utc_unicode[lead_char];
				if (0 != unicode_code) {
					unicode[j++] = (wchar_t)unicode_code;
				} else {
					unicode[j++] = lead_char;
				}
				++i;
			}
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void unicode_to_sjis_utc_engine::translate() {
//	void unicode_to_sjis_utc_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type sjis[translate_buffer_full_size];

#ifdef	__BBL_SMALL_BOM_LOGIC__
	from_string_type::size_type p_bom = 0;
	while(from_string_type::npos != (p_bom = untranslated_buffer.find(bbl_term::unicode_bom))) {
		untranslated_buffer.erase(p_bom, 1);
	}
#endif	//	__BBL_SMALL_BOM_LOGIC__

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			wchar_t current_wchar = unsignize(untranslated_buffer.at(i));
			bool is_broken_char = false;
			bbl_code sjis_code = bbl_transmap::unicode_utc[current_wchar];
			if (0 == sjis_code) {
				if (current_wchar <= 0x7F) {
					sjis_code = current_wchar;
				} else {
					is_broken_char = true;
				}
			}

			if (!is_broken_char) {
				if (sjis_code <= 0xFF) {
					sjis[j++] = (unsigned char)sjis_code;
				} else {
					sjis[j++] = sjis_code >> 8;
					sjis[j++] = 0xFF & sjis_code;
				}
			} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
				if (!is_private_use_area(current_wchar)) {
					append_broken_char(&j, sjis);
				}
#else
				append_broken_char(&j, sjis);
#endif
			}
			++i;
		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_UTC__)

#if defined(__BBL_LOAD_UNICODE_MAP_APPLE__)
void sjis_apple_to_unicode_engine::translate() {
//	void sjis_apple_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length;

		if (rest_length <= 0) {
			break;
		}
		unsigned char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_sjis_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_sjis_lead_byte(lead_char)) {
			//	漢字(全角文字)・・・
				const unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x40 <= trail_char && trail_char <= 0xFC && trail_char != 0x7F) {
					bbl_code sjis_code = lead_char *0x100 +trail_char;
					bbl_code unicode_code = bbl_transmap::apple_unicode[sjis_code];
					if (0 != unicode_code) {
						unicode[j++] = (wchar_t)unicode_code;
						i+=2;
					} else {
						bbl_wstring unicode_string = bbl_transmap::apple_unicode_n[sjis_code];
						if (0 < unicode_string.length()) {
							append_token(&j, unicode, unicode_string);
							i+=2;
						} else {
							append_broken_char(&j, unicode);
							i+=2;
						}
					}
				} else {
					append_broken_char(&j, unicode);
					++i;
				}
			} else {
			//	半角カナ＆その他の半角・・・
				bbl_code unicode_code = bbl_transmap::apple_unicode[lead_char];
				if (0 != unicode_code) {
					unicode[j++] = (wchar_t)unicode_code;
				} else {
					bbl_wstring unicode_string = bbl_transmap::apple_unicode_n[lead_char];
					if (0 < unicode_string.length()) {
						append_token(&j, unicode, unicode_string);
					} else {
						unicode[j++] = lead_char;
					}
				}
				++i;
			}
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void unicode_to_sjis_apple_engine::translate() {
//	void unicode_to_sjis_apple_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type sjis[translate_buffer_full_size];

#ifdef	__BBL_SMALL_BOM_LOGIC__
	from_string_type::size_type p_bom = 0;
	while(from_string_type::npos != (p_bom = untranslated_buffer.find(bbl_term::unicode_bom))) {
		untranslated_buffer.erase(p_bom, 1);
	}
#endif	//	__BBL_SMALL_BOM_LOGIC__

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;
		if (rest_length <= 0) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			const wchar_t current_wchar = unsignize(untranslated_buffer.at(i));

			//	0x????	+0x20DD
			//	0x????	+0xF87A
			//	0x????	+0xF87E
			//	0x????	+0xF87F

			//	0xF860	+２文字
			//	0xF861	+３文字
			//	0xF862	+４文字

			unsigned int code_length;
			switch(current_wchar) {
				case 0xF860:
					code_length = 3;
					break;
				case 0xF861:
					code_length = 4;
					break;
				case 0xF862:
					code_length = 5;
					break;
				default:
					code_length = 2;
					break;
			}

			const unsigned int k = i +code_length -1;
			if (untranslated_length <= k) {
				break;
			}

			bool is_broken_char = false;
			bbl_code sjis_code = bbl_transmap::unicode_n_apple[untranslated_buffer.substr(i, code_length)];
			if (0 == sjis_code) {
				code_length = 1;
				if (2 == code_length) {
					sjis_code = bbl_transmap::unicode_apple[current_wchar];
					if (0 == sjis_code) {
						if (current_wchar <= 0x7F) {
							sjis_code = current_wchar;
						} else {
							is_broken_char = true;
						}
					}
				} else {
					is_broken_char = true;
				}
			}

			if (!is_broken_char) {
				if (sjis_code <= 0xFF) {
					sjis[j++] = (unsigned char)sjis_code;
				} else {
					sjis[j++] = sjis_code >> 8;
					sjis[j++] = 0xFF & sjis_code;
				}
			} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
				if (!is_private_use_area(current_wchar)) {
					append_broken_char(&j, sjis);
				}
#else
				append_broken_char(&j, sjis);
#endif
			}
			i += code_length;

		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void unicode_to_sjis_apple_engine::flush() {
//	void unicode_to_sjis_apple_translate_flush(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	const unsigned int rest_length = untranslated_buffer.length();
	if (1 == rest_length) {
		to_string_type::value_type sjis[8];
		unsigned int i = 0, j = 0;

		const wchar_t current_wchar = unsignize(untranslated_buffer.at(i));
		bool is_broken_char = false;

		bbl_code sjis_code = bbl_transmap::unicode_apple[current_wchar];
		if (0 == sjis_code) {
			if (current_wchar <= 0x7F) {
				sjis_code = current_wchar;
				sjis[j++] = (unsigned char)sjis_code;
			} else {
				is_broken_char = true;
			}
		}
		if (!is_broken_char) {
			if (sjis_code <= 0xFF) {
				sjis[j++] = (unsigned char)sjis_code;
			} else {
				sjis[j++] = sjis_code >> 8;
				sjis[j++] = 0xFF & sjis_code;
			}
			untranslated_buffer = bbl_term::get_empty();
		} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
			if (!is_private_use_area(current_wchar)) {
				append_broken_char(&j, sjis);
			}
#else
			append_broken_char(&j, sjis);
#endif
		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	} else {
		if (1 < rest_length) {
			to_string_type::value_type sjis[8];
			unsigned int j = 0;
			append_broken_char(&j, sjis);
			sjis[j] = '\0';
			translated_buffer += sjis;
		}
	}

	base_type::flush();
}
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_APPLE__)

const unsigned int utf8_char_length[] = { // 0xA1～0xFE
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0,
};
const int utf8_lead_mask[] = {
	0x3F, 0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
};
inline unsigned int get_utf8_char_length(bbl_char lead_byte) {
	assert(lead_byte <= 0xFF);
	return utf8_char_length[lead_byte];
//	variation one for performance
//	return 0x8E <= lead_byte && lead_byte <=0xFE &&
//			0x8F != lead_byte && 0xA0 != lead_byte;
}

#if	defined(__BBL_USE_UNICODE__)
void unicode_to_utf8_engine::translate() {
//	void unicode_to_utf8_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type utf8[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;
		if (rest_length <= 0) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_code current_wchar = unsignize(untranslated_buffer.at(i));
#if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			if (2 == get_base_wstring_size()) {
#endif	//  defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
#if	!defined(__UNICODE_CHAR_SIZE_4__)
				if (0xD800 <= current_wchar && current_wchar <= 0xDBFF) {
					unsigned int k = i +1;
					if (untranslated_length <= k) {
						break;
					}
					bbl_code trail_wchar = unsignize(untranslated_buffer.at(k));
					if (0xDC00 <= trail_wchar && trail_wchar <= 0xDFFF) {
						current_wchar = ((current_wchar -0xD800) +0x0040) *0x400 +(trail_wchar -0xDC00);
						++i;
					}
				}
#endif	//	!defined(__UNICODE_CHAR_SIZE_4__)
#if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			}
#endif	//  defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			if (current_wchar < 0x80) {
				utf8[j++] = (unsigned char)current_wchar;
				++i;
			} else if (current_wchar < 0x800) {
				utf8[j++] = 0xC0 | (current_wchar >> 6);
				utf8[j++] = 0x80 | 0x3F & current_wchar;
				++i;
			} else if (current_wchar < 0x10000) {
				utf8[j++] = 0xE0 | (current_wchar >> 12);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 6);
				utf8[j++] = 0x80 | 0x3F & current_wchar;
				++i;
			} else if (current_wchar < 0x200000) {
				utf8[j++] = 0xF0 | (current_wchar >> 18);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 12);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 6);
				utf8[j++] = 0x80 | 0x3F & current_wchar;
				++i;
			} else if (current_wchar < 0x400000) {
				assert(false);	//	多分どっかがバグってるか、文字が壊れている。
				utf8[j++] = 0xF8 | (current_wchar >> 24);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 18);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 12);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 6);
				utf8[j++] = 0x80 | 0x3F & current_wchar;
				++i;
			} else {
				assert(false);	//	多分どっかがバグってるか、文字が壊れている。
				utf8[j++] = 0xFC | (current_wchar >> 30);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 24);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 18);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 12);
				utf8[j++] = 0x80 | 0x3F & (current_wchar >> 6);
				utf8[j++] = 0x80 | 0x3F & current_wchar;
				++i;
			}
		}
		utf8[j] = '\0';
		translated_buffer += utf8;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void utf8_to_unicode_engine::translate() {
//	void utf8_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (rest_length < get_utf8_char_length(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			int char_length = get_utf8_char_length(lead_char);
			if (1 <= char_length) {
				if (untranslated_length < i +char_length) {
					break;
				}
				bbl_code current_code = utf8_lead_mask[char_length] & lead_char;
				for(int k = 1; k < char_length; ++k) {
					current_code <<= 6;
					current_code |= 0x3F & unsignize(untranslated_buffer.at(i +k));
				}
				if (current_code <= 0xFFFF) {
					unicode[j++] = (wchar_t)current_code;
				} else {
					if (current_code <= 0x10FFFF) {
						const bbl_code lead_char = (current_code /0x400) |0xD800;
						const bbl_code trail_char = (current_code &0x3FF) |0xDC00;
						unicode[j++] = (wchar_t)lead_char;
						unicode[j++] = (wchar_t)trail_char;
					} else {
						append_broken_char(&j, unicode);
					}
				}
			} else {
				char_length = 1;
				append_broken_char(&j, unicode);
			}
			i += char_length;
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
#endif	//	defined(__BBL_USE_UNICODE__)

const bbl_string jis_KO_ascii = "\x1B(B";
const bbl_string jis_KO_roma = "\x1B(J";
const bbl_string jis_KI_2byte = "\x1B$B";
const bbl_string jis_KI_1byte = "\x1B(I";
const bbl_string jis_KI_3byte_aux = "\x1B$(D";
//const bbl_string jis_KI_3byte_aux = "\x1B$D"; ←これは単なる間違い？
const bbl_string jis_SO = "\x0E";
const bbl_string jis_SI = "\x0F";

enum {
	jis_ascii = 0,
	jis_K2,
	jis_K1,
	jis_K3_aux,
	jis_S
};

void sjis_to_jis_engine::translate() {
//	void sjis_to_jis_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type jis[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	int next_status = status;
	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_sjis_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_sjis_lead_byte(lead_char)) {
			//	漢字(全角文字)・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				bool is_broken_char = false;
				if (0x40 <= trail_char && trail_char <= 0xFC && trail_char != 0x7F) {
					if (lead_char < 0xFA) {
						if (jis_K2 != status) {
							next_status = jis_K2;
							break;
						}
						if (lead_char < 0xA0) {
							lead_char -= 0x70;
						} else {
							lead_char -= 0xB0;
						}
						if (0x80 <= trail_char) {
							trail_char--;
						}
						lead_char <<= 1;
						if (0x9E <= trail_char) {
							trail_char -= 0x5E;
						} else {
							--lead_char;
						}
						trail_char -= 0x1F;
						jis[j++] = lead_char;
						jis[j++] = trail_char;
						i+=2;
					} else {
						bbl_code sjis_code = lead_char *0x100 +trail_char;
						bbl_code euc_code = bbl_transmap::sjis_euc[sjis_code];
						if (0 != euc_code) {
							if (jis_K2 != status) {
								next_status = jis_K2;
								break;
							}
							bbl_code jis_code = euc_code & 0x7F7F;
							jis[j++] = jis_code /0x100;
							jis[j++] = jis_code & 0xFF;
							i+=2;
						} else {
							is_broken_char = true;
						}
					}
				} else {
					is_broken_char = true;
				}
				if (is_broken_char) {
					if (jis_ascii != status) {
						next_status = jis_ascii;
						break;
					}
					append_broken_char(&j, jis);
					++i;
				}
			} else if (0xA1 <= lead_char && lead_char <= 0xDF) {
			//	半角カナ・・・
				if (jis_K1 != status) {
					next_status = jis_K1;
					break;
				}
				jis[j++] = lead_char -0x80;
				++i;
			} else {
			//	その他の半角・・・
				if (jis_ascii != status) {
					next_status = jis_ascii;
					break;
				}
				jis[j++] = lead_char;
				++i;
			}
		}
		jis[j] = '\0';
		translated_buffer += jis;
		if (next_status != status) {
			switch(next_status) {
				case jis_ascii:
					translated_buffer += (jis_S != status) ? jis_KO_ascii: jis_SO;
					break;
				case jis_K2:
					translated_buffer += jis_KI_2byte;
					break;
				case jis_K1:
					translated_buffer += jis_KI_1byte;
					break;
				case jis_S:
					translated_buffer += jis_SI;
					break;
			}
			status = next_status;
		}
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void sjis_to_jis_engine::flush() {
//	void sjis_to_jis_translate_flush(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	switch(status) {
		case jis_K2:
			translated_buffer += jis_KO_ascii;
			status = jis_ascii;
			break;
		case jis_K1:
			translated_buffer += jis_KO_ascii;
			status = jis_ascii;
			break;
		case jis_S:
			translated_buffer += jis_SO;
			status = jis_ascii;
			break;
	}
	base_type::flush();
}

void euc_to_jis_engine::translate() {
//	void euc_to_jis_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type jis[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	int next_status = status;
	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_euc_lead_byte(next_char)) {
			break;
		}
		if (rest_length < 3 && is_euc_aux_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_euc_lead_byte(lead_char)) {
			//	漢字(全角文字) or 半角カナ・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				bool is_broken_char = false;
				if (0x8E == lead_char && 0xA1 <= trail_char && trail_char <= 0xDF) {
				//	半角カナ・・・
					if (jis_K1 != status) {
						next_status = jis_K1;
						break;
					}
					jis[j++] = trail_char -0x80;
					i+=2;
				} else if (0x80 <= trail_char) {
					if (jis_K2 != status) {
						next_status = jis_K2;
						break;
					}
					jis[j++] = lead_char & 0x7F;
					jis[j++] = trail_char & 0x7F;
					i+=2;
				} else {
					is_broken_char = true;
				}
				if (is_broken_char) {
					if (jis_ascii != status) {
						next_status = jis_ascii;
						break;
					}
					append_broken_char(&j, jis);
					++i;
				}
			} else if (is_euc_aux_lead_byte(lead_char)) {
			//	補助漢字・・・
				unsigned int k = i +2;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char second_char = unsignize(untranslated_buffer.at(i +1));
				bbl_char third_char = unsignize(untranslated_buffer.at(i +2));

				if (0x80 <= second_char && 0x80 <= third_char) {
					if (jis_K3_aux != status) {
						next_status = jis_K3_aux;
						break;
					}
					jis[j++] = second_char & 0x7F;
					jis[j++] = third_char & 0x7F;
					i+=3;
				} else {
					if (jis_ascii != status) {
						next_status = jis_ascii;
						break;
					}
					append_broken_char(&j, jis);
					i+=3;
				}
			} else {
			//	その他の半角・・・
				if (jis_ascii != status) {
					next_status = jis_ascii;
					break;
				}
				jis[j++] = lead_char;
				++i;
			}
		}
		jis[j] = '\0';
		translated_buffer += jis;
		if (next_status != status) {
			switch(next_status) {
				case jis_ascii:
					translated_buffer += (jis_S != status) ? jis_KO_ascii: jis_SO;
					break;
				case jis_K2:
					translated_buffer += jis_KI_2byte;
					break;
				case jis_K1:
					translated_buffer += jis_KI_1byte;
					break;
				case jis_K3_aux:
					translated_buffer += jis_KI_3byte_aux;
					break;
				case jis_S:
					translated_buffer += jis_SI;
					break;
			}
			status = next_status;
		}
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void euc_to_jis_engine::flush() {
//	void euc_to_jis_translate_flush(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	switch(status) {
		case jis_K2:
			translated_buffer += jis_KO_ascii;
			status = jis_ascii;
			break;
		case jis_K1:
			translated_buffer += jis_KO_ascii;
			status = jis_ascii;
			break;
		case jis_K3_aux:
			translated_buffer += jis_KO_ascii;
			status = jis_ascii;
			break;
		case jis_S:
			translated_buffer += jis_SO;
			status = jis_ascii;
			break;
	}
	base_type::flush();
}

#if	defined(__BBL_USE_UNICODE__)
void euc_to_unicode_engine::translate() {
//	void euc_to_unicode_translate(bbl_string &untranslated_buffer, bbl_wstring &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_wstring	to_string_type;

	to_string_type::value_type unicode[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (1 == rest_length && is_euc_lead_byte(next_char)) {
			break;
		}
		if (rest_length < 3 && is_euc_aux_lead_byte(next_char)) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}
			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if (is_euc_lead_byte(lead_char)) {
			//	漢字(全角文字) or 半角カナ・・・
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				if (0x40 <= trail_char && trail_char <= 0xFE && trail_char != 0x7F) {
					bbl_code euc_code = lead_char *0x100 +trail_char;
					bbl_code unicode_code = bbl_transmap::euc2_unicode[euc_code];
					if (0 != unicode_code) {
						unicode[j++] = (wchar_t)unicode_code;
						i+=2;
					} else {
						append_broken_char(&j, unicode);
						i+=2;
					}
				} else {
					append_broken_char(&j, unicode);
					++i;
				}
			} else if (is_euc_aux_lead_byte(lead_char)) {
			//	補助漢字・・・
				unsigned int k = i +2;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char second_char = unsignize(untranslated_buffer.at(i +1));
				bbl_char third_char = unsignize(untranslated_buffer.at(i +2));

#	if defined(__BBL_USING_STDMAP_TABLE__)
				bbl_code euc_code = lead_char *0x10000 +second_char *0x100 +third_char;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
				bbl_code euc_code = second_char *0x100 +third_char;
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)

				bbl_code unicode_code = bbl_transmap::euc3_unicode[euc_code];

				if (0 != unicode_code) {
					unicode[j++] = (wchar_t)unicode_code;
					i+=3;
				} else {
					append_broken_char(&j, unicode);
					i+=3;
				}
			} else {
			//	その他の半角・・・
				unicode[j++] = lead_char;
				++i;
			}
		}
		unicode[j] = L'\0';
		translated_buffer += unicode;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}

void unicode_to_euc_engine::translate() {
//	void unicode_to_euc_translate(bbl_wstring &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_wstring	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type euc[translate_buffer_full_size];

#ifdef	__BBL_SMALL_BOM_LOGIC__
	from_string_type::size_type p_bom = 0;
	while(from_string_type::npos != (p_bom = untranslated_buffer.find(bbl_term::unicode_bom))) {
		untranslated_buffer.erase(p_bom, 1);
	}
#endif	//	__BBL_SMALL_BOM_LOGIC__

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}

		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			wchar_t current_wchar = unsignize(untranslated_buffer.at(i));
			bool is_broken_char = false;
			bbl_code euc_code;
			if (current_wchar <= 0x7F) {
				euc_code = current_wchar;
			} else {
				euc_code = bbl_transmap::unicode_euc[current_wchar];
				if (0 == euc_code) {
					is_broken_char = true;
				}
			}

			if (!is_broken_char) {
				if (euc_code <= 0xFF) {
					euc[j++] = (unsigned char)euc_code;
				} else if (euc_code <= 0xFFFF) {
					euc[j++] = euc_code >> 8;
					euc[j++] = 0xFF & euc_code;
				} else {
					assert(euc_code <= 0xFFFFFF);
					euc[j++] = euc_code >> 16;
					euc[j++] = 0xFF & (euc_code >> 8);
					euc[j++] = 0xFF & euc_code;
				}
			} else {
#if !defined(__BBL_DISABLE_DISPEL_PRIVATE_USE_AREA__)
				if (!is_private_use_area(current_wchar)) {
					append_broken_char(&j, euc);
				}
#else
				append_broken_char(&j, euc);
#endif
			}
			++i;
		}
		euc[j] = '\0';
		translated_buffer += euc;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
#endif	//	defined(__BBL_USE_UNICODE__)

#if	!defined(__BBL_STRICT_TRANSLATE__)
void jis_to_sjis_engine::translate() {
//	void jis_to_sjis_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type sjis[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		if (1 == rest_length && (jis_K2 == status || jis_S == status)) {
			break;
		}
		if (rest_length < 3 && jis_K3_aux == status) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (rest_length < 3 && '\x1B' == next_char) {
			break;
		}
		if (rest_length < 4 && '\x1B' == next_char && '$' == next_char && '(' == next_char) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if ('\x1B' == lead_char) {
				if (untranslated_length <= i +2) {
					break;
				}
				const bbl_string status_mark = untranslated_buffer.substr(i, 3);
				if (jis_KO_ascii == status_mark) {
					status = jis_ascii;
					i += 3;
				} else if (jis_KO_roma == status_mark) {
					status = jis_ascii;
					i += 3;
				} else if (jis_KI_2byte == status_mark) {
					status = jis_K2;
					i += 3;
				} else if (jis_KI_1byte == status_mark) {
					status = jis_K1;
					i += 3;
				} else {
					if (untranslated_length <= i +3 && '\x1B' == next_char && '$' == next_char && '(' == next_char) {
						break;
					}
					const bbl_string status_mark = untranslated_buffer.substr(i, 4);
					if (jis_KI_3byte_aux == status_mark) {
						status = jis_K3_aux;
						i += 4;
					} else {
						i += 3;
					}
				}
				break;
			}
			if ('\x0E' == lead_char) {
				status = jis_ascii;
				++i;
				break;
			}
			if ('\x0F' == lead_char) {
				status = jis_S;
				++i;
				break;
			}

			if (jis_K2 == status) {
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				bbl_code euc_code = (lead_char *0x100 +trail_char) | 0x8080;
				if (!(0xF9A1 <= euc_code && euc_code <= 0xFCFE)) {
					if (lead_char & 0x01) {
						trail_char += 0x1F;
						if(0x7F <= trail_char) {
							++trail_char;
						}
					} else {
						trail_char += 0x7E;
					}
					lead_char = (lead_char -0x21) /2 +0x81;
					if (0xA0 <= lead_char) {
						lead_char += 0x40;
					}
					sjis[j++] = lead_char;
					sjis[j++] = trail_char;
					i += 2;
				} else {
					bbl_code sjis_code = bbl_transmap::euc_sjis[euc_code];
					if (0 != sjis_code) {
						sjis[j++] = sjis_code /0x100;
						sjis[j++] = sjis_code & 0xFF;
						i+=2;
					} else {
						append_broken_char(&j, sjis);
						++i;
					}
				}
			} else if (jis_K3_aux == status) {
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				//	SJIS には補助漢字を格納できない。
				append_broken_char(&j, sjis);
				i+=2;
			} else if (jis_ascii == status) {
				sjis[j++] = lead_char;
				++i;
			} else {
				assert(jis_K1 == status || jis_S == status);
				sjis[j++] = lead_char +0x80;
				++i;
			}
		}
		sjis[j] = '\0';
		translated_buffer += sjis;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void jis_to_sjis_engine::flush() {
//	void jis_to_sjis_translate_flush(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	status = jis_ascii;
	base_type::flush();
}
#endif	//	!defined(__BBL_STRICT_TRANSLATE__)

void jis_to_euc_engine::translate() {
//	void jis_to_euc_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type euc[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	while(true) {
		unsigned int j, rest_length;
		j = 0;
		rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		if (1 == rest_length && (jis_K2 == status || jis_S == status)) {
			break;
		}
		if (rest_length < 3 && jis_K3_aux == status) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (rest_length < 3 && '\x1B' == next_char) {
			break;
		}
		if (rest_length < 4 && '\x1B' == next_char && '$' == next_char && '(' == next_char) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if ('\x1B' == lead_char) {
				if (untranslated_length <= i +2) {
					break;
				}
				const bbl_string status_mark = untranslated_buffer.substr(i, 3);
				if (jis_KO_ascii == status_mark) {
					status = jis_ascii;
					i += 3;
				} else if (jis_KI_2byte == status_mark) {
					status = jis_K2;
					i += 3;
				} else if (jis_KI_1byte == status_mark) {
					status = jis_K1;
					i += 3;
				} else {
					if (untranslated_length <= i +3 && '\x1B' == next_char && '$' == next_char && '(' == next_char) {
						break;
					}
					const bbl_string status_mark = untranslated_buffer.substr(i, 4);
					if (jis_KI_3byte_aux == status_mark) {
						status = jis_K3_aux;
						i += 4;
					} else {
						i += 3;
					}
				}
				break;
			}
			if ('\x0E' == lead_char) {
				status = jis_ascii;
				++i;
				break;
			}
			if ('\x0F' == lead_char) {
				status = jis_S;
				++i;
				break;
			}

			if (jis_K2 == status) {
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				euc[j++] = lead_char |0x80;
				euc[j++] = trail_char |0x80;
				i += 2;
			} else if (jis_K3_aux == status) {
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				euc[j++] = 0x8F;
				euc[j++] = lead_char |0x80;
				euc[j++] = trail_char |0x80;
				i += 2;
			} else if (jis_ascii == status) {
				euc[j++] = lead_char;
				++i;
			} else {
				assert(jis_K1 == status || jis_S == status);
				euc[j++] = 0x8E;
				euc[j++] = lead_char +0x80;
				++i;
			}
		}
		euc[j] = '\0';
		translated_buffer += euc;
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void jis_to_euc_engine::flush() {
//	void jis_to_euc_translate_flush(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	status = jis_ascii;
	base_type::flush();
}

void jis_to_iso2022jp_engine::translate() {
//	void jis_to_iso2022jp_translate(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	to_string_type::value_type iso2022jp[translate_buffer_full_size];

	unsigned int i = 0;
	const unsigned untranslated_length = untranslated_buffer.length();

	int next_status = to_status;
	while(true) {
		unsigned int j, rest_length;
		j = 0; rest_length = untranslated_length -i;

		if (rest_length <= 0) {
			break;
		}
		if (1 == rest_length && (jis_K2 == from_status || jis_S == from_status)) {
			break;
		}
		bbl_char next_char = unsignize(untranslated_buffer.at(i));
		if (rest_length < 3 && '\x1B' == next_char) {
			break;
		}
		while(true) {
			if (untranslated_length <= i || translate_buffer_size <= j) {
				break;
			}

			bbl_char lead_char = unsignize(untranslated_buffer.at(i));
			if ('\x1B' == lead_char) {
				if (untranslated_length <= i +2) {
					break;
				}
				bbl_string status_mark = untranslated_buffer.substr(i, 3);
				if (jis_KO_ascii == status_mark) {
					from_status = jis_ascii;
				} else if (jis_KI_2byte == status_mark) {
					from_status = jis_K2;
				} else if (jis_KI_1byte == status_mark) {
					from_status = jis_K1;
				}
				i += 3;
				break;
			}
			if ('\x0E' == lead_char) {
				from_status = jis_ascii;
				++i;
				break;
			}
			if ('\x0F' == lead_char) {
				from_status = jis_S;
				++i;
				break;
			}

			if (jis_K2 == from_status) {
				unsigned int k = i +1;
				if (untranslated_length <= k) {
					break;
				}
				if (jis_K2 != to_status) {
					next_status = jis_K2;
					break;
				}
				bbl_char trail_char = unsignize(untranslated_buffer.at(k));
				iso2022jp[j++] = lead_char;
				iso2022jp[j++] = trail_char;
				i += 2;
			} else if (jis_ascii == from_status) {
				if (jis_ascii != to_status) {
					next_status = jis_ascii;
					break;
				}
				iso2022jp[j++] = lead_char;
				++i;
			} else {
				assert(jis_K1 == from_status || jis_S == from_status);
				if (0x21 <= lead_char && lead_char <= 0x5F) {
					bbl_code iso2022jp_code = bbl_transmap::half_to_full_jis1[lead_char];
					if (0 == iso2022jp_code) {
						iso2022jp_code = bbl_transmap::half_to_full_jis_map3[lead_char -0x21];
					} else {
						unsigned int k = i +1;
						if (untranslated_length <= k) {
							break;
						}
						bbl_char trail_char = unsignize(untranslated_buffer.at(k));
						if (0x5E != trail_char) {
							if (0x5F == trail_char) {
								iso2022jp_code = bbl_transmap::half_to_full_jis2[lead_char];
							} else {
								iso2022jp_code = 0;
							}
							if (0 == iso2022jp_code) {
								iso2022jp_code = bbl_transmap::half_to_full_jis_map3[lead_char -0x21];
							} else {
								++i;
							}
						} else {
							++i;
						}
					}
					if (jis_K2 != to_status) {
						next_status = jis_K2;
						break;
					}
					iso2022jp[j++] = iso2022jp_code /0x100;
					iso2022jp[j++] = iso2022jp_code & 0xFF;
					++i;
				} else {
					if (jis_ascii != to_status) {
						next_status = jis_ascii;
						break;
					}
					append_broken_char(&j, iso2022jp);
					++i;
				}
			}
		}
		iso2022jp[j] = '\0';
		translated_buffer += iso2022jp;
		if (next_status != to_status) {
			switch(next_status) {
				case jis_ascii:
					translated_buffer += (jis_S != to_status) ? jis_KO_ascii: jis_SO;
					break;
				case jis_K2:
					translated_buffer += jis_KI_2byte;
					break;
				case jis_K1:
					assert(false);
					translated_buffer += jis_KI_1byte;
					break;
				case jis_S:
					assert(false);
					translated_buffer += jis_SI;
					break;
			}
			to_status = next_status;
		}
	}
	SET_FORWARD(untranslated_buffer).substr(i);
}
void jis_to_iso2022jp_engine::flush() {
//	void jis_to_iso2022jp_translate_flush(bbl_string &untranslated_buffer, bbl_string &translated_buffer) {
//		typedef	bbl_string	from_string_type;
//		typedef	bbl_string	to_string_type;

	if (jis_K1 == from_status || jis_S == from_status) {
		unsigned int rest_length = untranslated_buffer.length();
		if (1 <= rest_length) {
			assert(1 == rest_length);
			bbl_char lead_char = unsignize(untranslated_buffer.at(0));
			if (0x21 <= lead_char && lead_char <= 0x5F) {
				if (jis_K2 != to_status) {
					to_status = jis_K2;
					translated_buffer += jis_KI_2byte;
				}
				to_string_type::value_type iso2022jp[10];
				bbl_code iso2022jp_code = bbl_transmap::half_to_full_jis1[lead_char -0x21];
				iso2022jp[0] = iso2022jp_code /0x100;
				iso2022jp[1] = iso2022jp_code & 0xFF;
				iso2022jp[2] = 0x00;
				SET_FORWARD(untranslated_buffer).substr(1);
				translated_buffer += iso2022jp;
			}
		}
	}
	from_status = jis_ascii;
	switch(to_status) {
		case jis_K2:
			translated_buffer += jis_KO_ascii;
			to_status = jis_ascii;
			break;
		case jis_K1:
			assert(false);
			translated_buffer += jis_KO_ascii;
			to_status = jis_ascii;
			break;
		case jis_S:
			assert(false);
			translated_buffer += jis_SO;
			to_status = jis_ascii;
			break;
	}
	base_type::flush();
}

/******************************************************************************
	□■□■                          cuppa                         □■□■
	■□■□                 http://www.unittest.org/               ■□■□
******************************************************************************/

#if defined(__BBL_USE_BINARY__)
const bbl_wstring WORD_to_unicode(const bbl_binary &X) {
	typedef unsigned short BASE_ALIGN;
	assert(2 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length /sizeof(BASE_ALIGN);

#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		bbl_wstring result((const bbl_wstring::value_type *)X.data(), result_length);

		if (result_length *sizeof(BASE_ALIGN) < length) {
			result += bbl_term::get_broken_char();
		}
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
		const BASE_ALIGN *source = (const BASE_ALIGN *)X.data();
		bbl_wstring result(result_length, 0);
		for(unsigned int i = 0; i < result_length; ++i) {
			result[i] = (bbl_wstring::value_type)*source++;
		}

		if (result_length *sizeof(BASE_ALIGN) < length) {
			result += bbl_term::get_broken_char();
		}
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
}

const bbl_binary unicode_to_WORD(const bbl_wstring &X) {
	typedef unsigned short BASE_ALIGN;
	assert(2 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length *sizeof(BASE_ALIGN);
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		return bbl_binary((const bbl_binary::value_type *)X.data(), result_length);
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
		assert(4 == get_base_wstring_size());
		const bbl_binary::value_type *source = (const bbl_binary::value_type *)X.data();
		bbl_binary result(result_length, 0);
		unsigned int i = 0;
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		if (base_endian::little_endian == get_base_endian()) {
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__LITTLE_ENDIAN_COMPUTER__)
			while(i < result_length) {
				result[i++] = (bbl_binary::value_type)*source++;
				result[i++] = (bbl_binary::value_type)*source++;
				assert(0 == source[0]);
				assert(0 == source[1]);
				source += 2;
			}
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		} else {
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__BIG_ENDIAN_COMPUTER__)
			assert(base_endian::big_endian == get_base_endian());
			while(i < result_length) {
				assert(0 == source[0]);
				assert(0 == source[1]);
				source += 2;
				result[i++] = (bbl_binary::value_type)*source++;
				result[i++] = (bbl_binary::value_type)*source++;
			}
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		}
#endif
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
}

const bbl_wstring cross_WORD_to_unicode(const bbl_binary &X) {
	typedef unsigned short BASE_ALIGN;
	assert(2 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const BASE_ALIGN *source = (const BASE_ALIGN *)X.data();
	const unsigned int result_length = length /sizeof(BASE_ALIGN);
	bbl_wstring result(result_length, 0);
	for(unsigned int i = 0; i < result_length; ++i) {
		result[i] = (bbl_wstring::value_type)WORD_cross_endian(*source++);
	}
	if (result_length *sizeof(BASE_ALIGN) < length) {
		result += bbl_term::get_broken_char();
	}
	return result;
}

const bbl_binary unicode_to_cross_WORD(const bbl_wstring &X) {
	typedef unsigned short BASE_ALIGN;
	assert(2 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length *sizeof(BASE_ALIGN);
	const bbl_binary::value_type *source = (const bbl_binary::value_type *)X.data();
	bbl_binary result(result_length, 0);
	unsigned int i = 0;
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		while(i < result_length) {
			result[i +1] = (bbl_binary::value_type)*source++;
			result[i +0] = (bbl_binary::value_type)*source++;
			i += 2;
		}
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)

		assert(4 == get_base_wstring_size());
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		if (base_endian::little_endian == get_base_endian()) {
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__LITTLE_ENDIAN_COMPUTER__)
			while(i < result_length) {
				assert(0 == source[0]);
				assert(0 == source[1]);
				source += 2;
				result[i +1] = (bbl_binary::value_type)*source++;
				result[i +0] = (bbl_binary::value_type)*source++;
				i += 2;
			}
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		} else {
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__BIG_ENDIAN_COMPUTER__)
			assert(base_endian::big_endian == get_base_endian());
			while(i < result_length) {
				result[i +1] = (bbl_binary::value_type)*source++;
				result[i +0] = (bbl_binary::value_type)*source++;
				i += 2;
				assert(0 == source[0]);
				assert(0 == source[1]);
				source += 2;
			}
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		}
#	endif

#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
	return result;
}

#if	defined(__BBL_USE_UTF32__)
const bbl_wstring DWORD_to_unicode(const bbl_binary &X) {
	typedef unsigned long BASE_ALIGN;
	assert(4 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length /sizeof(BASE_ALIGN);
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
		bbl_wstring result((const bbl_wstring::value_type *)X.data(), result_length);

		if (result_length *sizeof(BASE_ALIGN) < length) {
			result += bbl_term::get_broken_char();
		}
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		const BASE_ALIGN *source = (const BASE_ALIGN *)X.data();
		bbl_wstring result(result_length, 0);
		for(unsigned int i = 0; i < result_length; ++i) {
			result[i] = (bbl_wstring::value_type)*source++;
		}

		if (result_length *sizeof(BASE_ALIGN) < length) {
			result += bbl_term::get_broken_char();
		}
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
}

const bbl_binary unicode_to_DWORD(const bbl_wstring &X) {
	typedef unsigned long BASE_ALIGN;
	assert(4 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length *sizeof(BASE_ALIGN);
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
		return bbl_binary((const bbl_binary::value_type *)X.data(), result_length);
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		assert(2 == get_base_wstring_size());
		const bbl_binary::value_type *source = (const bbl_binary::value_type *)X.data();
		bbl_binary result(result_length, 0);
		unsigned int i = 0;
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		if (base_endian::little_endian == get_base_endian()) {
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__LITTLE_ENDIAN_COMPUTER__)
			while(i < result_length) {
				result[i++] = (bbl_binary::value_type)*source++;
				result[i++] = (bbl_binary::value_type)*source++;
				result[i++] = 0;
				result[i++] = 0;
			}
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		} else {
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__BIG_ENDIAN_COMPUTER__)
			assert(base_endian::big_endian == get_base_endian());
			while(i < result_length) {
				result[i++] = 0;
				result[i++] = 0;
				result[i++] = (bbl_binary::value_type)*source++;
				result[i++] = (bbl_binary::value_type)*source++;
			}
#endif
#if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		}
#endif
		return result;
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
}

const bbl_wstring cross_DWORD_to_unicode(const bbl_binary &X) {
	typedef unsigned long BASE_ALIGN;
	assert(4 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const BASE_ALIGN *source = (const BASE_ALIGN *)X.data();
	const unsigned int result_length = length /sizeof(BASE_ALIGN);
	bbl_wstring result(result_length, 0);
	for(unsigned int i = 0; i < result_length; ++i) {
		result[i] = (bbl_wstring::value_type)DWORD_cross_endian(*source++);
	}
	if (result_length *sizeof(BASE_ALIGN) < length) {
		result += bbl_term::get_broken_char();
	}
	return result;
}
const bbl_binary unicode_to_cross_DWORD(const bbl_wstring &X) {
	typedef unsigned long BASE_ALIGN;
	assert(4 == sizeof(BASE_ALIGN));
	const unsigned int length = X.length();
	const unsigned int result_length = length *sizeof(BASE_ALIGN);
	const bbl_binary::value_type *source = (const bbl_binary::value_type *)X.data();
	bbl_binary result(result_length, 0);
	unsigned int i = 0;
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	if (sizeof(BASE_ALIGN) == get_base_wstring_size()) {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
		while(i < result_length) {
			result[i +3] = (bbl_binary::value_type)*source++;
			result[i +2] = (bbl_binary::value_type)*source++;
			result[i +1] = (bbl_binary::value_type)*source++;
			result[i +0] = (bbl_binary::value_type)*source++;
			i += 4;
		}
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	} else {
#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
		assert(2 == get_base_wstring_size());
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		if (base_endian::little_endian == get_base_endian()) {
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__LITTLE_ENDIAN_COMPUTER__)
			while(i < result_length) {
				i += 2;
				//result[i++] = 0;
				//result[i++] = 0;
				result[i +1] = (bbl_binary::value_type)*source++;
				result[i +0] = (bbl_binary::value_type)*source++;
				i += 2;
			}
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		} else {
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__) || defined(__BIG_ENDIAN_COMPUTER__)
			assert(base_endian::big_endian == get_base_endian());
			while(i < result_length) {
				result[i +1] = (bbl_binary::value_type)*source++;
				result[i +0] = (bbl_binary::value_type)*source++;
				i += 4;
				//i += 2;
				//result[i++] = 0;
				//result[i++] = 0;
			}
#	endif
#	if		defined(__UNKNOWN_ENDIAN_COMPUTER__)
		}
#	endif

#endif
#if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
	}
#endif
	return result;
}
#endif	//	defined(__BBL_USE_UTF32__)

template<class from_string_type, class to_string_type>
inline void binary_engine_translate(unsigned int mask, from_string_type &untranslated_buffer, to_string_type &translated_buffer, const to_string_type (core_translater)(const from_string_type&)) {
	unsigned int rest_length = untranslated_buffer.length();
	unsigned int pack_length = rest_length & ~mask;
	translated_buffer += core_translater(untranslated_buffer.substr(0, pack_length));
	SET_FORWARD(untranslated_buffer).substr(pack_length);
}
template<class from_string_type, class to_string_type>
inline void binary_engine_flush(unsigned int mask, from_string_type &untranslated_buffer, to_string_type &translated_buffer, const to_string_type (core_translater)(const from_string_type&)) {
	unsigned int rest_length = untranslated_buffer.length();
	unsigned int pack_length = rest_length & ~mask;
	translated_buffer += core_translater(untranslated_buffer.substr(0, pack_length));
	untranslated_buffer.erase();
	if (rest_length != pack_length) {
		translated_buffer += bbl_term::get_broken_char();
	}
}

void WORD_to_unicode_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, WORD_to_unicode);
}
void WORD_to_unicode_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, WORD_to_unicode);
}

void unicode_to_WORD_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, unicode_to_WORD);
}
void unicode_to_WORD_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, unicode_to_WORD);
}

void cross_WORD_to_unicode_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, cross_WORD_to_unicode);
}
void cross_WORD_to_unicode_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, cross_WORD_to_unicode);
}

void unicode_to_cross_WORD_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, unicode_to_cross_WORD);
}
void unicode_to_cross_WORD_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(1, untranslated_buffer, translated_buffer, unicode_to_cross_WORD);
}

#if	defined(__BBL_USE_UTF32__)
void DWORD_to_unicode_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, DWORD_to_unicode);
}
void DWORD_to_unicode_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, DWORD_to_unicode);
}

void unicode_to_DWORD_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, unicode_to_DWORD);
}
void unicode_to_DWORD_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, unicode_to_DWORD);
}

void cross_DWORD_to_unicode_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, cross_DWORD_to_unicode);
}
void cross_DWORD_to_unicode_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, cross_DWORD_to_unicode);
}

void unicode_to_cross_DWORD_engine::translate() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_translate<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, unicode_to_cross_DWORD);
}
void unicode_to_cross_DWORD_engine::flush() {
	//	VC のヽ(｀Д´)ノアホォ！
	binary_engine_flush<from_string_type, to_string_type>(3, untranslated_buffer, translated_buffer, unicode_to_cross_DWORD);
}
#endif	//	defined(__BBL_USE_UTF32__)
#endif	//	defined(__BBL_USE_BINARY__)


/******************************************************************************
	□■□■                          cuppa                         □■□■
	■□■□                 http://www.unittest.org/               ■□■□
******************************************************************************/

#if defined(__BBL_USE_SELECTORS__)

#define MAKE_ORDER_CODE(from, to)		((from) *0x100 +(to))

#if	defined(__BBL_USE_UNICODE__)
template<class engine, class binary_engine = engine>
class engine_type_object {
  public:
	typedef engine											target_engine;
	typedef binary_engine									target_binary_engine;
	typedef typename target_engine::bbl_translater_type		bbl_translater_type;
	typedef typename bbl_translater_type::from_string_type	from_string_type;
	typedef typename bbl_translater_type::to_string_type	to_string_type;

	static bbl_translater_type create() { return target_engine::create(); }
	static const to_string_type ignite(const from_string_type &X)
	{ return target_engine::ignite(X); }

	static bbl_translater_type binary_create() { return target_binary_engine::create(); }
	static const to_string_type binary_ignite(const from_string_type &X)
	{ return target_binary_engine::ignite(X); }
};

template<class engine>
class binary_engine_type_object {
  public:
	typedef engine	target_engine;
};

template<class T> inline T & get_null_object() { return *((T*)NULL); }

template<
	class first_engine,
	class second_engine,
	class second_binary_engine
> inline const engine_type_object<
	second_engine,
	twin_translate_engine<first_engine, second_binary_engine>
> & operator >> (
	const binary_engine_type_object<first_engine> &,
	const engine_type_object<second_engine, second_binary_engine> &) {

	return get_null_object<
		engine_type_object<
			second_engine,
			twin_translate_engine<first_engine, second_binary_engine>
		>
	>();
}

template<
	class first_engine,
	class first_binary_engine,
	class second_engine
> inline const engine_type_object<
	first_engine,
	twin_translate_engine<first_binary_engine, second_engine>
> & operator >> (
	const engine_type_object<first_engine, first_binary_engine> &,
	const binary_engine_type_object<second_engine> &) {

	return get_null_object<
		engine_type_object<
			first_engine,
			twin_translate_engine<first_binary_engine, second_engine>
		>
	>();
}

template<
	class first_engine,
	class first_binary_engine,
	class second_engine,
	class second_binary_engine
> inline const engine_type_object<
	twin_translate_engine<first_engine, second_engine>,
	twin_translate_engine<first_binary_engine, second_binary_engine>
> & operator >> (
	const engine_type_object<first_engine, first_binary_engine> &,
	const engine_type_object<second_engine, second_binary_engine> &) {

	return get_null_object<
		engine_type_object<
			twin_translate_engine<first_engine, second_engine>,
			twin_translate_engine<first_binary_engine, second_binary_engine>
		>
	>();
}
#endif	//	defined(__BBL_USE_UNICODE__)


#if	defined(__BBL_USE_UNICODE__) && defined(__BBL_USE_BINARY__) && defined(__UNKNOWN_ENDIAN_COMPUTER__)
int cross_base_encoding(int X_base_encoding) {
	using namespace base_encoding;
	switch(X_base_encoding) {
		case utf16:
			return utf16le;
		case utf16le:
			if (base_endian::little_endian == get_base_endian()) {
				return utf16le;
			} else {
				return utf16be;
			}
		case utf16be:
			if (base_endian::big_endian == get_base_endian()) {
				return utf16le;
			} else {
				return utf16be;
			}
#	if	defined(__BBL_USE_UTF32__)
		case utf32:
			return utf32le;
		case utf32le:
			if (base_endian::little_endian == get_base_endian()) {
				return utf32le;
			} else {
				return utf32be;
			}
		case utf32be:
			if (base_endian::big_endian == get_base_endian()) {
				return utf32le;
			} else {
				return utf32be;
			}
#	endif
		default:
			return X_base_encoding;
	}
}
#else
#define cross_base_encoding(X)		X
#endif

//
//	__UNKNOWN_ENDIAN_COMPUTER__ の場合、ここでは __LITTLE_ENDIAN_COMPUTER__
//	と同様にしておく。 cf. method_base::get_order_code()
//
#if	defined(__BBL_USE_UNICODE__)
#	if	defined(__BBL_USE_BINARY__)
#		if	defined(__LITTLE_ENDIAN_COMPUTER__) || defined(__UNKNOWN_ENDIAN_COMPUTER__)
typedef unicode_to_WORD_engine			unicode_to_utf16le_engine;
typedef unicode_to_cross_WORD_engine	unicode_to_utf16be_engine;
typedef WORD_to_unicode_engine			utf16le_to_unicode_engine;
typedef cross_WORD_to_unicode_engine	utf16be_to_unicode_engine;
#			if	defined(__BBL_USE_UTF32__)
typedef unicode_to_DWORD_engine			unicode_to_utf32le_engine;
typedef unicode_to_cross_DWORD_engine	unicode_to_utf32be_engine;
typedef DWORD_to_unicode_engine			utf32le_to_unicode_engine;
typedef cross_DWORD_to_unicode_engine	utf32be_to_unicode_engine;
#			endif
#		endif
#		if	defined(__BIG_ENDIAN_COMPUTER__)
typedef unicode_to_cross_WORD_engine	unicode_to_utf16le_engine;
typedef unicode_to_WORD_engine			unicode_to_utf16be_engine;
typedef cross_WORD_to_unicode_engine	utf16le_to_unicode_engine;
typedef WORD_to_unicode_engine			utf16be_to_unicode_engine;
#			if	defined(__BBL_USE_UTF32__)
typedef unicode_to_cross_DWORD_engine	unicode_to_utf32le_engine;
typedef unicode_to_DWORD_engine			unicode_to_utf32be_engine;
typedef cross_DWORD_to_unicode_engine	utf32le_to_unicode_engine;
typedef DWORD_to_unicode_engine			utf32be_to_unicode_engine;
#			endif
#		endif
#	endif
#endif


class method_base {
  public:
	int from_base_encoding;
	int to_base_encoding;
	method_base(int X_from_base_encoding, int X_to_base_encoding)
		:from_base_encoding(X_from_base_encoding), to_base_encoding(X_to_base_encoding) { }
	int get_order_code() {
		return MAKE_ORDER_CODE(cross_base_encoding(from_base_encoding), cross_base_encoding(to_base_encoding));
	}
};
template<class engine_type>
class create_engine :public method_base {
  public:
	typedef engine_type return_type;
	create_engine(int X_from_base_encoding, int X_to_base_encoding)
		:method_base(X_from_base_encoding, X_to_base_encoding) { }
};
template<class engine_type>
class create_binary_engine :public create_engine<engine_type> {
  public:
	typedef create_engine<engine_type> origin_type;
//	typedef origin_type::return_type return_type;
	create_binary_engine(int X_from_base_encoding, int X_to_base_encoding)
		:origin_type(X_from_base_encoding, X_to_base_encoding) { }
//	operator const origin_type & () const { return *this; }
};
template<class engine_type>
class ignite_translate :public method_base {
  public:
	typedef typename engine_type::from_string_type from_string_type;
	typedef typename engine_type::to_string_type to_string_type;
	typedef	to_string_type return_type;
	const from_string_type & target_string;
	ignite_translate(int X_from_base_encoding, int X_to_base_encoding, const from_string_type & X_target_string)
		:method_base(X_from_base_encoding, X_to_base_encoding), target_string(X_target_string) { }
};
template<class engine_type>
class ignite_binary_translate :public ignite_translate<engine_type> {
  public:
	typedef ignite_translate<engine_type> origin_type;
	typedef typename origin_type::from_string_type from_string_type;
	typedef typename origin_type::return_type return_type;
	ignite_binary_translate(int X_from_base_encoding, int X_to_base_encoding, const from_string_type & X_target_string)
		:origin_type(X_from_base_encoding, X_to_base_encoding, X_target_string) { }
//	operator const origin_type & () const { return *this; }
};

template<class engine> class method_selector {
  public:
	typedef typename engine::bbl_translater_type engine_type;
	typedef typename engine::to_string_type to_string_type;
	static engine_type call_method(const create_engine<engine_type> &) { return engine::create(); }
	static to_string_type call_method(const ignite_translate<engine_type> &X) { return engine::ignite(X.target_string); }
};
template<class engine> class method_selector_binarable {
  public:
	typedef typename engine::bbl_translater_type engine_type;
	typedef typename engine::to_string_type to_string_type;
	static engine_type call_method(const create_engine<engine_type> &) { return engine::create(); }
	static engine_type call_method(const create_binary_engine<engine_type> &) { return engine::binary_create(); }
	static to_string_type call_method(const ignite_translate<engine_type> &X) { return engine::ignite(X.target_string); }
	static to_string_type call_method(const ignite_binary_translate<engine_type> &X) { return engine::binary_ignite(X.target_string); }
};

template<class T> inline method_selector_binarable<T> & get_method_selector(const T &) {
	return get_null_object< method_selector_binarable<T> >();
}

template<class method> typename method::return_type call_method_ss(method X) {
	using namespace base_encoding;
	switch(X.get_order_code()) {

		case MAKE_ORDER_CODE(ansi, sjis):
		case MAKE_ORDER_CODE(ansi, jis):
		case MAKE_ORDER_CODE(ansi, euc):
			return method_selector< through_engine<bbl_string> >::call_method(X);
#if	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(ansi, utf8):
			return get_method_selector(
				engine_type_object<			through_engine<bbl_string>	>() >>
				binary_engine_type_object<	enbom_engine<bbl_string>	>()).call_method(X);
#	if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(ansi, utf16le):
			return get_method_selector(
				engine_type_object<			ansi_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(ansi, utf16be):
			return get_method_selector(
				engine_type_object<			ansi_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16be_engine	>()).call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(ansi, utf32le):
			return get_method_selector(
				engine_type_object<			ansi_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(ansi, utf32be):
			return get_method_selector(
				engine_type_object<			ansi_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32be_engine	>()).call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#	endif	//	defined(__BBL_USE_BINARY__)
#endif	//	defined(__BBL_USE_UNICODE__)

		case MAKE_ORDER_CODE(sjis, ansi):
#if	defined(__BBL_USE_UNICODE__)
			return method_selector< twin_translate_engine<sjis_to_unicode_engine, unicode_to_ansi_engine> >::call_method(X);
#else	//	defined(__BBL_USE_UNICODE__)
			return method_selector< through_engine<bbl_string> >::call_method(X);
#endif	//	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(sjis, jis):
			return method_selector<sjis_to_jis_engine>::call_method(X);
		case MAKE_ORDER_CODE(sjis, euc):
			return method_selector<sjis_to_euc_engine>::call_method(X);
		case MAKE_ORDER_CODE(sjis, iso2022jp):
			return method_selector< twin_translate_engine<sjis_to_jis_engine, jis_to_iso2022jp_engine> >::call_method(X);
#if	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(sjis, utf8):
			return get_method_selector(
				engine_type_object<			sjis_to_utf8_engine			>() >>
				binary_engine_type_object<	enbom_engine<bbl_string>	>()).call_method(X);
#	if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(sjis, utf16le):
			return get_method_selector(
				engine_type_object<			sjis_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(sjis, utf16be):
			return get_method_selector(
				engine_type_object<			sjis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16be_engine	>()).call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(sjis, utf32le):
			return get_method_selector(
				engine_type_object<			sjis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(sjis, utf32be):
			return get_method_selector(
				engine_type_object<			sjis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32be_engine	>()).call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#	endif	//	defined(__BBL_USE_BINARY__)
#endif	//	defined(__BBL_USE_UNICODE__)

		case MAKE_ORDER_CODE(jis, ansi):
#if	defined(__BBL_USE_UNICODE__)
			return method_selector< twin_translate_engine<jis_to_unicode_engine, unicode_to_ansi_engine> >::call_method(X);
#else	//	defined(__BBL_USE_UNICODE__)
			return method_selector< through_engine<bbl_string> >::call_method(X);
#endif	//	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(jis, sjis):
		case MAKE_ORDER_CODE(iso2022jp, sjis):
			return method_selector<jis_to_sjis_engine>::call_method(X);
		case MAKE_ORDER_CODE(jis, euc):
		case MAKE_ORDER_CODE(iso2022jp, euc):
			return method_selector<jis_to_euc_engine>::call_method(X);
		case MAKE_ORDER_CODE(jis, iso2022jp):
			return method_selector<jis_to_iso2022jp_engine>::call_method(X);
#if	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(jis, utf8):
		case MAKE_ORDER_CODE(iso2022jp, utf8):
			return get_method_selector(
				engine_type_object<			jis_to_utf8_engine			>() >>
				binary_engine_type_object<	enbom_engine<bbl_string>	>()).call_method(X);
#	if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(jis, utf16le):
		case MAKE_ORDER_CODE(iso2022jp, utf16le):
			return get_method_selector(
				engine_type_object<			jis_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(jis, utf16be):
		case MAKE_ORDER_CODE(iso2022jp, utf16be):
			return get_method_selector(
				engine_type_object<			jis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16be_engine	>()).call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(jis, utf32le):
		case MAKE_ORDER_CODE(iso2022jp, utf32le):
			return get_method_selector(
				engine_type_object<			jis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(jis, utf32be):
		case MAKE_ORDER_CODE(iso2022jp, utf32be):
			return get_method_selector(
				engine_type_object<			jis_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32be_engine	>()).call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#	endif	//	defined(__BBL_USE_BINARY__)
#endif	//	defined(__BBL_USE_UNICODE__)

		case MAKE_ORDER_CODE(euc, ansi):
#if	defined(__BBL_USE_UNICODE__)
			return method_selector< twin_translate_engine<euc_to_unicode_engine, unicode_to_ansi_engine> >::call_method(X);
#else	//	defined(__BBL_USE_UNICODE__)
			return method_selector< through_engine<bbl_string> >::call_method(X);
#endif	//	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(euc, sjis):
			return method_selector<euc_to_sjis_engine>::call_method(X);
		case MAKE_ORDER_CODE(euc, jis):
			return method_selector<euc_to_jis_engine>::call_method(X);
		case MAKE_ORDER_CODE(euc, iso2022jp):
			return method_selector< twin_translate_engine<euc_to_jis_engine, jis_to_iso2022jp_engine> >::call_method(X);
#if	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(euc, utf8):
			return get_method_selector(
				engine_type_object<			euc_to_utf8_engine			>() >>
				binary_engine_type_object<	enbom_engine<bbl_string>	>()).call_method(X);
#	if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(euc, utf16le):
			return get_method_selector(
				engine_type_object<			euc_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(euc, utf16be):
			return get_method_selector(
				engine_type_object<			euc_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf16be_engine	>()).call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(euc, utf32le):
			return get_method_selector(
				engine_type_object<			euc_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32le_engine	>()).call_method(X);
		case MAKE_ORDER_CODE(euc, utf32be):
			return get_method_selector(
				engine_type_object<			euc_to_unicode_engine 		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_utf32be_engine	>()).call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#	endif	//	defined(__BBL_USE_BINARY__)
#endif	//	defined(__BBL_USE_UNICODE__)

#if	defined(__BBL_USE_UNICODE__)
		case MAKE_ORDER_CODE(utf8, ansi):
			return method_selector< twin_translate_engine<utf8_to_unicode_engine, unicode_to_ansi_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf8, sjis):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_string>	>() >>
				engine_type_object<			utf8_to_sjis_engine			>()).call_method(X);
		case MAKE_ORDER_CODE(utf8, jis):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_string>	>() >>
				engine_type_object<			utf8_to_jis_engine			>()).call_method(X);
		case MAKE_ORDER_CODE(utf8, euc):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_string>	>() >>
				engine_type_object<			utf8_to_euc_engine			>()).call_method(X);
		case MAKE_ORDER_CODE(utf8, iso2022jp):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_string>	>() >>
				engine_type_object<			utf8_to_jis_engine			>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
#	if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(utf8, utf16le):
			return method_selector< twin_translate_engine<utf8_to_unicode_engine, unicode_to_utf16le_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf8, utf16be):
			return method_selector< twin_translate_engine<utf8_to_unicode_engine, unicode_to_utf16be_engine> >::call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(utf8, utf32le):
			return method_selector< twin_translate_engine<utf8_to_unicode_engine, unicode_to_utf32le_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf8, utf32be):
			return method_selector< twin_translate_engine<utf8_to_unicode_engine, unicode_to_utf32be_engine> >::call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)

		case MAKE_ORDER_CODE(utf16le, ansi):
			return get_method_selector(
				engine_type_object<			utf16le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_ansi_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16le, sjis):
			return get_method_selector(
				engine_type_object<			utf16le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_sjis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16le, jis):
			return get_method_selector(
				engine_type_object<			utf16le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16le, euc):
			return get_method_selector(
				engine_type_object<			utf16le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_euc_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16le, utf8):
			return method_selector< twin_translate_engine<utf16le_to_unicode_engine, unicode_to_utf8_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf16le, iso2022jp):
			return get_method_selector(
				engine_type_object<			utf16le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
//		case MAKE_ORDER_CODE(utf16le, utf16le):
//			return method_selector< through_engine<bbl_string> >::call_method(X);
		case MAKE_ORDER_CODE(utf16le, utf16be):
			return method_selector< twin_translate_engine<utf16le_to_unicode_engine, unicode_to_utf16be_engine> >::call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(utf16le, utf32le):
			return method_selector< twin_translate_engine<utf16le_to_unicode_engine, unicode_to_utf32le_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf16le, utf32be):
			return method_selector< twin_translate_engine<utf16le_to_unicode_engine, unicode_to_utf32be_engine> >::call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)

		case MAKE_ORDER_CODE(utf16be, ansi):
			return get_method_selector(
				engine_type_object<			utf16be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_ansi_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16be, sjis):
			return get_method_selector(
				engine_type_object<			utf16be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_sjis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16be, jis):
			return get_method_selector(
				engine_type_object<			utf16be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16be, euc):
			return get_method_selector(
				engine_type_object<			utf16be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_euc_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16be, utf8):
			return method_selector< twin_translate_engine<utf16be_to_unicode_engine, unicode_to_utf8_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf16be, iso2022jp):
			return get_method_selector(
				engine_type_object<			utf16be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf16be, utf16le):
			return method_selector< twin_translate_engine<utf16be_to_unicode_engine, unicode_to_utf16le_engine> >::call_method(X);
//		case MAKE_ORDER_CODE(utf16be, utf16be):
//			return method_selector< through_engine<bbl_string> >::call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(utf16be, utf32le):
			return method_selector< twin_translate_engine<utf16be_to_unicode_engine, unicode_to_utf32le_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf16be, utf32be):
			return method_selector< twin_translate_engine<utf16be_to_unicode_engine, unicode_to_utf32be_engine> >::call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)

#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(utf32le, ansi):
			return get_method_selector(
				engine_type_object<			utf32le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_ansi_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32le, sjis):
			return get_method_selector(
				engine_type_object<			utf32le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_sjis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32le, jis):
			return get_method_selector(
				engine_type_object<			utf32le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32le, euc):
			return get_method_selector(
				engine_type_object<			utf32le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_euc_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32le, utf8):
			return method_selector< twin_translate_engine<utf32le_to_unicode_engine, unicode_to_utf8_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf32le, iso2022jp):
			return get_method_selector(
				engine_type_object<			utf32le_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32le, utf16le):
			return method_selector< twin_translate_engine<utf32le_to_unicode_engine, unicode_to_WORD_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf32le, utf16be):
			return method_selector< twin_translate_engine<utf32le_to_unicode_engine, unicode_to_cross_WORD_engine> >::call_method(X);
//		case MAKE_ORDER_CODE(utf32le, utf32le):
//			return method_selector< through_engine<bbl_string> >::call_method(X);
		case MAKE_ORDER_CODE(utf32le, utf32be):
			return method_selector< twin_translate_engine<utf32le_to_unicode_engine, unicode_to_cross_DWORD_engine> >::call_method(X);

		case MAKE_ORDER_CODE(utf32be, ansi):
			return get_method_selector(
				engine_type_object<			utf32be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_ansi_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32be, sjis):
			return get_method_selector(
				engine_type_object<			utf32be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_sjis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32be, jis):
			return get_method_selector(
				engine_type_object<			utf32be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32be, euc):
			return get_method_selector(
				engine_type_object<			utf32be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_euc_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32be, utf8):
			return method_selector< twin_translate_engine<utf32be_to_unicode_engine, unicode_to_utf8_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf32be, iso2022jp):
			return get_method_selector(
				engine_type_object<			utf32be_to_unicode_engine	>() >>
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(utf32be, utf16le):
			return method_selector< twin_translate_engine<utf32be_to_unicode_engine, unicode_to_utf16le_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf32be, utf16be):
			return method_selector< twin_translate_engine<utf32be_to_unicode_engine, unicode_to_utf16be_engine> >::call_method(X);
		case MAKE_ORDER_CODE(utf32be, utf32le):
			return method_selector< twin_translate_engine<utf32be_to_unicode_engine, unicode_to_utf32le_engine> >::call_method(X);
//		case MAKE_ORDER_CODE(utf32be, utf32be):
//			return method_selector< through_engine<bbl_string> >::call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#	endif	//	defined(__BBL_USE_BINARY__)
#endif	//	defined(__BBL_USE_UNICODE__)

		default:
			return method_selector< through_engine<bbl_string> >::call_method(X);
	}
}

template<class method> typename method::return_type call_method_sw(method X) {
	using namespace base_encoding;
	switch(X.get_order_code()) {

		case MAKE_ORDER_CODE(ansi, unicode):
			return get_method_selector(
				engine_type_object<			ansi_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>()).call_method(X);
		case MAKE_ORDER_CODE(sjis, unicode):
			return get_method_selector(
				engine_type_object<			sjis_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>()).call_method(X);
		case MAKE_ORDER_CODE(jis, unicode):
		case MAKE_ORDER_CODE(iso2022jp, unicode):
			return get_method_selector(
				engine_type_object<			jis_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>()).call_method(X);
		case MAKE_ORDER_CODE(euc, unicode):
			return get_method_selector(
				engine_type_object<			euc_to_unicode_engine		>() >>
				binary_engine_type_object<	enbom_engine<bbl_wstring>	>()).call_method(X);
		case MAKE_ORDER_CODE(utf8, unicode):
			return method_selector<utf8_to_unicode_engine>::call_method(X);
#if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(utf16le, unicode):
			return method_selector<utf16le_to_unicode_engine>::call_method(X);
		case MAKE_ORDER_CODE(utf16be, unicode):
			return method_selector<utf16be_to_unicode_engine>::call_method(X);
#		if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(utf32le, unicode):
			return method_selector<utf32le_to_unicode_engine>::call_method(X);
		case MAKE_ORDER_CODE(utf32be, unicode):
			return method_selector<utf32be_to_unicode_engine>::call_method(X);
#		endif	//	defined(__BBL_USE_UTF32__)
#endif	//	defined(__BBL_USE_BINARY__)

		default:
#if	defined(__BBL_USE_BINARY__)
#	if	defined(__BBL_USE_UTF32__)
#		if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			if (2 == get_base_wstring_size()) {
#		endif
#		if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
				return method_selector<WORD_to_unicode_engine>::call_method(X);
#		endif
#		if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			} else {
#		endif
#		if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
				return method_selector<DWORD_to_unicode_engine>::call_method(X);
#		endif
#		if		defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			}
#		endif
#	else	//	defined(__BBL_USE_UTF32__)
			return method_selector<WORD_to_unicode_engine>::call_method(X);
#	endif	//	defined(__BBL_USE_UTF32__)
#else	//	defined(__BBL_USE_BINARY__)
			return method_selector< dull_engine<from_string_type, to_string_type> >::call_method(X);
#endif	//	defined(__BBL_USE_BINARY__)

	}
}

template<class method> typename method::return_type call_method_ws(method X) {
	using namespace base_encoding;
	switch(X.get_order_code()) {
		case MAKE_ORDER_CODE(unicode, ansi):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_ansi_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(unicode, sjis):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_sjis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(unicode, jis):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(unicode, euc):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_euc_engine		>()).call_method(X);
		case MAKE_ORDER_CODE(unicode, utf8):
			return method_selector<unicode_to_utf8_engine>::call_method(X);
		case MAKE_ORDER_CODE(unicode, iso2022jp):
			return get_method_selector(
				binary_engine_type_object<	debom_engine<bbl_wstring>	>() >>
				engine_type_object<			unicode_to_jis_engine		>() >>
				engine_type_object<			jis_to_iso2022jp_engine		>()).call_method(X);
#if	defined(__BBL_USE_BINARY__)
		case MAKE_ORDER_CODE(unicode, utf16le):
			return method_selector<unicode_to_utf16le_engine>::call_method(X);
		case MAKE_ORDER_CODE(unicode, utf16be):
			return method_selector<unicode_to_utf16be_engine>::call_method(X);
#	if	defined(__BBL_USE_UTF32__)
		case MAKE_ORDER_CODE(unicode, utf32le):
			return method_selector<unicode_to_utf32le_engine>::call_method(X);
		case MAKE_ORDER_CODE(unicode, utf32be):
			return method_selector<unicode_to_utf32be_engine>::call_method(X);
#	endif	//	defined(__BBL_USE_UTF32__)
#endif	//	defined(__BBL_USE_BINARY__)
		default:
#if	defined(__BBL_USE_BINARY__)
#	if	defined(__BBL_USE_UTF32__)
#		if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			if (2 == get_base_wstring_size()) {
#		endif
#		if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_2__)
				return method_selector<unicode_to_WORD_engine>::call_method(X);
#		endif
#		if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			} else {
#		endif
#		if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__) || defined(__UNICODE_CHAR_SIZE_4__)
				return method_selector<unicode_to_DWORD_engine>::call_method(X);
#		endif
#		if	defined(__UNICODE_CHAR_SIZE_UNKNOWN__)
			}
#		endif
#	else	//	defined(__BBL_USE_UTF32__)
			return method_selector<unicode_to_WORD_engine>::call_method(X);
#	endif	//	defined(__BBL_USE_UTF32__)
#else	//	defined(__BBL_USE_BINARY__)
			return method_selector< dull_engine<from_string_type, to_string_type> >::call_method(X);
#endif	//	defined(__BBL_USE_BINARY__)
	}
}

template<>
manual_translate_engine<bbl_string, bbl_string>::bbl_translater_type
manual_translate_engine<bbl_string, bbl_string>::order(int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_ss(create_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}

template<>
const bbl_string manual_translate_engine<bbl_string, bbl_string>::ignite(const bbl_string &X, int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_ss(ignite_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}

#if	defined(__BBL_USE_UNICODE__)
template<>
manual_translate_engine<bbl_string, bbl_wstring>::bbl_translater_type
manual_translate_engine<bbl_string, bbl_wstring>::order(int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_sw(create_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}

template<>
const bbl_wstring manual_translate_engine<bbl_string, bbl_wstring>::ignite(const bbl_string &X, int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_sw(ignite_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}

template<>
manual_translate_engine<bbl_wstring, bbl_string>::bbl_translater_type
manual_translate_engine<bbl_wstring, bbl_string>::order(int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_ws(create_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}

template<>
const bbl_string manual_translate_engine<bbl_wstring, bbl_string>::ignite(const bbl_wstring &X, int X_from_base_encoding, int X_to_base_encoding) {

	return call_method_ws(ignite_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}

template<>
manual_translate_engine<bbl_wstring, bbl_wstring>::bbl_translater_type
manual_translate_engine<bbl_wstring, bbl_wstring>::order(int X_from_base_encoding, int X_to_base_encoding) {
	using namespace base_encoding;

	assert(unicode == X_from_base_encoding);
	assert(unicode == X_to_base_encoding);

	switch(MAKE_ORDER_CODE(X_from_base_encoding, X_to_base_encoding)) {
		default:
			return through_engine<bbl_wstring>::create();
	}
}

template<>
const bbl_wstring manual_translate_engine<bbl_wstring, bbl_wstring>::ignite(const bbl_wstring &X, int X_from_base_encoding, int X_to_base_encoding) {
	using namespace base_encoding;

	assert(unicode == X_from_base_encoding);
	assert(unicode == X_to_base_encoding);

	switch(MAKE_ORDER_CODE(X_from_base_encoding, X_to_base_encoding)) {
		default:
			return X;
	}
}
#endif	//	defined(__BBL_USE_UNICODE__)

#ifdef	__BBL_LARGE_BOM_LOGIC__
template<>
const translate_to_string_engine<bbl_string>::to_string_type translate_to_string_engine<bbl_string>::ignite(const from_string_type &X, int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_ss(ignite_binary_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}
template<>
translate_to_string_engine<bbl_string>::bbl_translater_type translate_to_string_engine<bbl_string>::order(int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_ss(create_binary_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}
template<>
const translate_to_string_engine<bbl_string>::to_string_type translate_to_binary_engine<bbl_string>::ignite(const from_string_type &X, int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_ss(ignite_binary_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}
template<>
translate_to_string_engine<bbl_string>::bbl_translater_type translate_to_binary_engine<bbl_string>::order(int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_ss(create_binary_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}
#if	defined(__BBL_USE_UNICODE__)
template<>
const translate_to_string_engine<bbl_wstring>::to_string_type translate_to_string_engine<bbl_wstring>::ignite(const from_string_type &X, int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_sw(ignite_binary_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}
template<>
translate_to_string_engine<bbl_wstring>::bbl_translater_type translate_to_string_engine<bbl_wstring>::order(int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_sw(create_binary_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}
template<>
const translate_to_string_engine<bbl_wstring>::to_string_type translate_to_binary_engine<bbl_wstring>::ignite(const from_string_type &X, int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_sw(ignite_binary_translate<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding, X));
}
template<>
translate_to_string_engine<bbl_wstring>::bbl_translater_type translate_to_binary_engine<bbl_wstring>::order(int X_from_base_encoding, int X_to_base_encoding) {
	return call_method_sw(create_binary_engine<bbl_translater_type>(X_from_base_encoding, X_to_base_encoding));
}
#endif	//	defined(__BBL_USE_UNICODE__)
#endif	//	__BBL_LARGE_BOM_LOGIC__

#endif	//	defined(__BBL_USE_SELECTORS__)

const bbl_binary utf32be_BOM("\x00\x00\xFE\xFF", 4);
const bbl_binary utf32le_BOM("\xFF\xFE\x00\x00", 4);
const bbl_binary utf8_FEFF = "\xEF\xBB\xBF";
const bbl_binary utf16be_BOM = "\xFE\xFF";
const bbl_binary utf16le_BOM = "\xFF\xFE";

const bbl_binary code_aspect("\x00\x0F\x1B"
	"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
	"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
	"\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
	"\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF"
	"\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF"
	"\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF"
	"\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
	"\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF", 0x83);

bbl_binary::size_type get_aspect_position(const bbl_binary &X) {
	bbl_binary::size_type pos = X.find_first_of(code_aspect);
	if (bbl_binary::npos == pos) {
		return pos;
	} else {
		return pos & ~0x3;
	}
}

analyze_result
analyze_base_encoding(const bbl_binary &org_X, const unsigned int max_scan_size) {

//	assert(max_scan_size <= 5000);

	const unsigned int core_scan_size = 0x100;
	assert(core_scan_size <= 5000);
	//	これ以上値を大きくするとスコアがオーバーフローする危険性がでてきます。
	//	オーバーフローを起こすことは分析の失敗を意味し、まとな分析結果を期待
	//	することはできません。

	const unsigned int length = org_X.length();
	const unsigned int scan_size = (length < max_scan_size) ? length: max_scan_size;

	if (0 == scan_size) {
		return analyze_result(base_encoding::unknown, base_encoding::binary);
	}

	bbl_binary head_X;
	const bbl_binary *pre_X;
	if (length < max_scan_size) {
		pre_X = &org_X;
	} else {
		head_X = org_X.substr(0, max_scan_size);
		pre_X = &head_X;
	}
	const bbl_binary &X = *pre_X;

	//	int babel::analyze_base_encoding::header_check(const bbl_binary &X)
	{
		//
		//	header check
		//

		//	const unsigned int length = X.length();
		const bbl_binary head_4bytes = X.substr(0, (length < 4) ? length: 4);

		//	4bytes marks
		if (0 == head_4bytes.find(utf32be_BOM)) {
			assert(0 != bbl_binary::npos);
			return base_encoding::utf32be;
		}
		if (0 == head_4bytes.find(utf32le_BOM)) {
			return base_encoding::utf32le;
		}

		//	3bytes mark
		if (0 == head_4bytes.find(utf8_FEFF)) {
			return base_encoding::utf8;
		}

		//	2bytes marks
		if (0 == head_4bytes.find(utf16be_BOM)) {
			return base_encoding::utf16be;
		}
		if (0 == head_4bytes.find(utf16le_BOM)) {
			return base_encoding::utf16le;
		}

		//	return base_encoding::unknown;
	}

	//	int babel::analyze_base_encoding::simple_scan(const bbl_binary &X)
	{
		//
		//	simple scan
		//

		const bbl_binary::size_type pNULNUL = X.find(bbl_binary("\x00\x00", 2));
		if (bbl_string::npos != pNULNUL) {
			switch(pNULNUL %4) {
				case 0:
					return analyze_result(base_encoding::utf32be, base_encoding::binary);
				case 2:
					return analyze_result(base_encoding::utf32le, base_encoding::binary);
			}
		}

		/*
		const bbl_binary::size_type pNUL = X.find("\x00");
		if (bbl_string::npos != pNUL) {
			if (0 == pNULNUL % 2) {
				return base_encoding::utf16be;
			} else {
				return base_encoding::utf16le;
			}
		}
		*/

		if (//bbl_string::npos != X.find(jis_KO_ascii) ||
			bbl_string::npos != X.find(jis_KI_2byte) ||
			bbl_string::npos != X.find(jis_KI_1byte) ||
			bbl_string::npos != X.find(jis_KI_3byte_aux)) {
			return base_encoding::jis;
		}

		//	return base_encoding::unknown;
	}

	//	int babel::analyze_base_encoding::scoring_scan(const bbl_binary &X)
	{
		//
		//	scoring scan

		class encode_scorer {
		  public:
			enum {
				neutral = 0,
				first_byte = 1,
				second_byte = 2,
			};
			int down_point;
			int up_point;
			int mode;
			int current_code;
			encode_scorer() :down_point(0), up_point(0), mode(neutral), current_code(0) {}
			void on_error() {
				mode = neutral;
				++down_point;
			}
			int get_point() {
				return up_point -(down_point *babel::bbl_scoremap::measure_point);
			}
		};
		class sjis_scorer :public encode_scorer {
		  public:
			void scan_char(const bbl_char &X) {
				babel::bbl_score_map &score_map = babel::bbl_scoremap::sjis_score;
				if (0x00 == X || 0xFD <= X) {
					on_error();
					return;
				}
				if (encode_scorer::neutral == mode) {
					current_code = X;
					if (babel::is_sjis_lead_byte(X)) {
						mode = encode_scorer::first_byte;
					} else {
						up_point += (int)score_map[current_code];
					}
				} else {
					assert(encode_scorer::first_byte == mode);
					if (babel::is_sjis_trail_byte(X)) {
						current_code *= 0x100;
						current_code += X;
						up_point += score_map[current_code];
						mode = encode_scorer::neutral;
					} else {
						on_error();
					}
				}
			}
		};
		class euc_scorer :public encode_scorer {
		  public:
			void scan_char(const bbl_char &X) {
				babel::bbl_score_map &score_map = babel::bbl_scoremap::euc_score;
				if (0x00 == X) {
					on_error();
					return;
				}
				if (encode_scorer::neutral == mode) {
					current_code = X;
					if (babel::is_euc_lead_byte(X)) {
					//	漢字(全角文字) or 半角カナの１バイト目・・・
						mode = encode_scorer::first_byte;
					} else if (babel::is_euc_aux_lead_byte(X)) {
					//	補助漢字・・・
						mode = -2;
					} else {
						//	SJIS 以外で１バイト文字が得点することはないのでコメントアウト
						//	up_point += score_map[current_code];
					}
				} else {
					if (encode_scorer::first_byte == mode) {
						if (babel::is_euc_trail_byte(X)) {
						//	漢字(全角文字) or 半角カナの２バイト目・・・
							current_code *= 0x100;
							current_code += X;
							up_point += score_map[current_code];
							mode = encode_scorer::neutral;
						} else {
							on_error();
						}
					} else if (mode < 0) {
					//	補助漢字・・・
						++mode;
						//	補助漢字が得点することはないので得点の処理は行わない。
					} else {
						on_error();
					}
				}
			}
		};
		class utf8_scorer :public encode_scorer {
//#	if defined(__BBL_USING_STATIC_TABLE__)
//			std::string current_char;
//#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
		  public:
			void scan_char(const bbl_char &X) {
#	if defined(__BBL_USING_STDMAP_TABLE__)
				babel::bbl_score_map &score_map = babel::bbl_scoremap::utf8_score;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
				babel::bbl_score_map &score_map = babel::bbl_scoremap::unicode_score;
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
				if (0x00 == X) {
					on_error();
					return;
				}
				if (encode_scorer::neutral == mode) {
					int char_length = babel::get_utf8_char_length(X);
					if (0 == char_length) {
						on_error();
					} else {
#	if defined(__BBL_USING_STDMAP_TABLE__)
						current_code = X;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
//						current_char = X;
						current_code = babel::utf8_lead_mask[char_length] & X;
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
						mode = 1 -char_length;
						if (encode_scorer::neutral == mode) {
							//	SJIS 以外で１バイト文字が得点することはないのでコメントアウト
							//	up_point += score_map[current_code];
						}
					}
				} else {
					assert(mode < 0);
					if (0x80 == (0xC0 & X)) {
#	if defined(__BBL_USING_STDMAP_TABLE__)
						current_code *= 0x100;
						current_code += X;
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#	if defined(__BBL_USING_STATIC_TABLE__)
						current_code <<= 6;
						current_code |= (0x3F & X);
#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
						++mode;
						if (encode_scorer::neutral == mode) {
//#	if defined(__BBL_USING_STDMAP_TABLE__)
							up_point += score_map[current_code];
//#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
//#	if defined(__BBL_USING_STATIC_TABLE__)
//							up_point += score_map[babel::utf8_to_unicode(current_char)[0]];
//#	endif	//	defined(__BBL_USING_STATIC_TABLE__)
						}
					} else {
						on_error();
					}
				}
			}
		};

		//	const unsigned int length = X.length();

		sjis_scorer sjis_monitor;
		euc_scorer euc_monitor;
		utf8_scorer utf8_monitor;

		bbl_char current_char;
		unsigned int over0x80_count = 0;
		unsigned int even_null_count = 0;
		unsigned int odd_null_count = 0;
		const bbl_binary::size_type start_pos = get_aspect_position(X);
		if (bbl_binary::npos == start_pos) {
			return base_encoding::ansi;
		}
//		const unsigned int scan_size = (length < core_scan_size) ? length: core_scan_size;
		const bbl_binary::size_type end_pos = (length < start_pos +core_scan_size) ? length: start_pos +core_scan_size;
		for(bbl_binary::size_type i = start_pos; i < end_pos; ++i) {
			current_char = unsignize(X.at(i));
			sjis_monitor.scan_char(current_char);
			euc_monitor.scan_char(current_char);
			utf8_monitor.scan_char(current_char);
			if (current_char & 0x80) {
				++over0x80_count;
			} else if (!current_char) {
				if (0 == i %2) {
					++even_null_count;
				} else {
					++odd_null_count;
				}
			}
		}

		class analyze_result_maker {
			typedef analyze_result_maker this_type;
			bool is_binary;
		  public:
			analyze_result_maker(bool X_is_binary = false) :is_binary(X_is_binary) { }
			this_type & set_is_binary(bool X_is_binary = true) {
				is_binary = X_is_binary;
				return *this;
			}
			babel::analyze_result operator () (const int hunch_result){
				using namespace babel;
				return analyze_result(hunch_result,
					(is_binary) ? babel::base_encoding::binary: hunch_result);
			}
		};
		analyze_result_maker result_maker;

		if (0 < even_null_count +odd_null_count) {
//			if (0 == even_null_count && 0 == odd_null_count) {
				result_maker.set_is_binary();
//			}
			if (even_null_count < odd_null_count) {
				return result_maker(base_encoding::utf16le);
			} else {
				return result_maker(base_encoding::utf16be);
			}
		}

		if (0 == over0x80_count) {
			if (//bbl_string::npos != X.find(jis_SO) ||
				bbl_string::npos != X.find(jis_SI)) {
				return base_encoding::jis;
			} else {
				return base_encoding::ansi;
			}
		}

//		//void DEBUG_trace_score(const sjis_scorer &sjis_monitor, const euc_scorer &euc_monitor, const utf8_scorer &utf8_monitor) {
//			std::cout << "sjis_monitor.down_point = " << sjis_monitor.down_point << std::endl;
//			std::cout << "euc_monitor.down_point = " << euc_monitor.down_point << std::endl;
//			std::cout << "utf8_monitor.down_point = " << utf8_monitor.down_point << std::endl;
//		//}

		const int sjis_point = sjis_monitor.get_point();
		const int euc_point = euc_monitor.get_point();
		const int utf8_point = utf8_monitor.get_point();

//		//void DEBUG_trace_score(int sjis_point, int euc_point, int utf8_point) {
//			std::cout << "sjis_point = " << sjis_point << std::endl;
//			std::cout << "euc_point = " << euc_point << std::endl;
//			std::cout << "utf8_point = " << utf8_point << std::endl;
//		//}

		if (sjis_point == euc_point && sjis_point == utf8_point) {

			//
			//	改行コードのチェック
			//
			if (bbl_binary::npos != X.find("\n")) {
				if (bbl_binary::npos != X.find("\r\n") || bbl_binary::npos != X.find("\n\r")) {
					//	windows/dos sjis
					return base_encoding::sjis;
				} else {
					//	unix euc
					return base_encoding::euc;
				}
			} else if (bbl_binary::npos != X.find("\r")) {
				//	apple sjis
				return base_encoding::sjis;
			}

			//
			//	エンコード指定のカンニング
			//
			const char **i;
			bbl_binary::size_type utf8_pos = X.find("UTF-8");
			bbl_binary::size_type sjis_pos = bbl_binary::npos;
			bbl_binary::size_type euc_pos = bbl_binary::npos;
			const char *sjis_names[] = {"Shift_JIS", "shift_jis", "sjis", "Shift-JIS", "shift-jis", "SJIS"};
			for(i = sjis_names; i < ARRAY_END(sjis_names); ++i) {
				bbl_binary::size_type pos = X.find(*i);
				if (bbl_binary::npos != pos) {
					if (bbl_binary::npos == sjis_pos || pos < sjis_pos) {
						sjis_pos = pos;
					}
				}
			}
			const char *euc_names[] = {"EUC", "euc"};
			for(i = euc_names; i < ARRAY_END(euc_names); ++i) {
				bbl_binary::size_type pos = X.find(*i);
				if (bbl_binary::npos != pos) {
					if (bbl_binary::npos == euc_pos || pos < euc_pos) {
						euc_pos = pos;
					}
				}
			}
			if (bbl_binary::npos != utf8_pos) {
				if ((bbl_binary::npos == sjis_pos || utf8_pos < sjis_pos) &&
					(bbl_binary::npos == euc_pos || utf8_pos < euc_pos)) {
					return base_encoding::utf8;
				}
			}
			if (bbl_binary::npos != sjis_pos) {
				if (bbl_binary::npos == euc_pos || sjis_pos < euc_pos) {
					return base_encoding::sjis;
				}
			}
			if (bbl_binary::npos != euc_pos) {
				return base_encoding::euc;
			}

			//	お手上げ
			return get_base_encoding();
		}
		if (euc_point < sjis_point) {
			if (utf8_point <= sjis_point) {
				if (1 <= sjis_monitor.down_point *100 /scan_size) {
					result_maker.set_is_binary();
				}
				return result_maker(base_encoding::sjis);
			} else {
				if (1 <= utf8_monitor.down_point *100 /scan_size) {
					result_maker.set_is_binary();
				}
				return result_maker(base_encoding::utf8);
			}
		} else {
			if (utf8_point <= euc_point) {
				if (1 <= euc_monitor.down_point *100 /scan_size) {
					result_maker.set_is_binary();
				}
				return result_maker(base_encoding::euc);
			} else {
				if (1 <= utf8_monitor.down_point *100 /scan_size) {
					result_maker.set_is_binary();
				}
				return result_maker(base_encoding::utf8);
			}
		}
	}

//	int result;
//	base_encoding::unknown == (result = header_check(X)) &&
//	base_encoding::unknown == (result = simple_scan(X)) &&
//	base_encoding::unknown == (result = scoring_scan(X));
//	return result;
}

void init_babel() {
static bool initialized = false;
	if (initialized) {
		return;
	} else {
		initialized = true;
	}

	const bbl_code *i;

	bbl_code sjis, euc, jis;
#if defined(__BBL_USING_STDMAP_TABLE__)
	i = bbl_transmap::sjis_euc_map;
	while(i < ARRAY_END(bbl_transmap::sjis_euc_map)) {
		sjis = *i++;
		euc = *i++;
		bbl_transmap::sjis_euc.insert(bbl_pair(sjis, euc));
		bbl_transmap::euc_sjis.insert(bbl_pair(euc, sjis));
	}
#endif	//	defined(__BBL_USING_STDMAP_TABLE__)

	bbl_code unicode;

#if defined(__BBL_LOAD_UNICODE_MAP_CP932__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	i = bbl_transmap::cp932_map;
	bbl_map::iterator cp932_unicode_end = bbl_transmap::cp932_unicode.end();
	bbl_map::iterator unicode_cp932_end = bbl_transmap::unicode_cp932.end();
	while(i < ARRAY_END(bbl_transmap::cp932_map)) {
		sjis = *i++;
		unicode = *i++;
		if (cp932_unicode_end == bbl_transmap::cp932_unicode.find(sjis)) {
			bbl_transmap::cp932_unicode.insert(bbl_pair(sjis, unicode));
		}
		if (unicode_cp932_end == bbl_transmap::unicode_cp932.find(unicode)) {
			bbl_transmap::unicode_cp932.insert(bbl_pair(unicode, sjis));
		}
	}
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_CP932__)

#if	defined(__BBL_USE_UNICODE__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	i = bbl_transmap::euc_map;
	bbl_map::iterator unicode_euc_end = bbl_transmap::unicode_euc.end();
	while(i < ARRAY_END(bbl_transmap::euc_map)) {
		euc = *i++;
		unicode = *i++;
		bbl_transmap::euc_unicode.insert(bbl_pair(euc, unicode));
		if (unicode_euc_end == bbl_transmap::unicode_euc.find(unicode)) {
			bbl_transmap::unicode_euc.insert(bbl_pair(unicode, euc));
		}
	}
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#endif	//	defined(__BBL_USE_UNICODE__)


#if defined(__BBL_LOAD_UNICODE_MAP_UTC__)
#	if defined(__BBL_USING_STDMAP_TABLE__)
	i = bbl_transmap::utc_map;
	bbl_map::iterator unicode_utc_end = bbl_transmap::unicode_utc.end();
	while(i < ARRAY_END(bbl_transmap::utc_map)) {
		sjis = *i++;
		unicode = *i++;
		bbl_transmap::utc_unicode.insert(bbl_pair(sjis, unicode));
		if (unicode_utc_end == bbl_transmap::unicode_utc.find(unicode)) {
			bbl_transmap::unicode_utc.insert(bbl_pair(unicode, sjis));
		}
	}
#	endif	//	defined(__BBL_USING_STDMAP_TABLE__)
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_UTC__)

#if defined(__BBL_LOAD_UNICODE_MAP_APPLE__)
	i = bbl_transmap::appll_map;
	bbl_map::iterator unicode_apple_end = bbl_transmap::unicode_apple.end();
	bbl_map_n_1::iterator unicode_n_apple_end = bbl_transmap::unicode_n_apple.end();
	bbl_wstring::value_type primary_unicode_n[6];
	while(i < ARRAY_END(bbl_transmap::appll_map)) {
		sjis = *i++;
		unicode = *i++;
		if (0x0000 == *i) {
			++i;

			bbl_transmap::apple_unicode.insert(bbl_pair(sjis, unicode));
			if (unicode_apple_end == bbl_transmap::unicode_apple.find(unicode)) {
				bbl_transmap::unicode_apple.insert(bbl_pair(unicode, sjis));
			}
		} else {
			primary_unicode_n[0] = unicode;
			int j = 1;
			do {
				assert(j < ARRAY_SIZE(primary_unicode_n));
				primary_unicode_n[j] = *i++;
			} while(0x0000 != primary_unicode_n[j++]);

			bbl_wstring unicode_n(primary_unicode_n);

			bbl_transmap::apple_unicode_n.insert(bbl_pair_1_n(sjis, unicode_n));
			if (unicode_n_apple_end == bbl_transmap::unicode_n_apple.find(unicode_n)) {
				bbl_transmap::unicode_n_apple.insert(bbl_pair_n_1(unicode_n, sjis));
			}
		}
	}
#endif	//	defined(__BBL_LOAD_UNICODE_MAP_APPLE__)

	i = bbl_transmap::half_to_full_jis_map1;
	bbl_code jis_half, jis_full;
	while(i < ARRAY_END(bbl_transmap::half_to_full_jis_map1)) {
		jis_half = *i++;
		jis_full = *i++;
		bbl_transmap::half_to_full_jis1.insert(bbl_pair(jis_half, jis_full));
	}
	i = bbl_transmap::half_to_full_jis_map2;
	while(i < ARRAY_END(bbl_transmap::half_to_full_jis_map2)) {
		jis_half = *i++;
		jis_full = *i++;
		bbl_transmap::half_to_full_jis2.insert(bbl_pair(jis_half, jis_full));
	}

#if defined(__BBL_USING_STDMAP_TABLE__)
	const int *ix = bbl_scoremap::sampling_score_map;
	int score;
	//bbl_code sjis, euc;
	bbl_code utf8;
	while(ix < ARRAY_END(bbl_scoremap::sampling_score_map)) {
		score = *ix++;
		sjis = *ix++;
		euc = *ix++;
		utf8 = *ix++;
		bbl_scoremap::sjis_score.insert(bbl_score_pair(sjis, score));
		bbl_scoremap::euc_score.insert(bbl_score_pair(euc, score));
		bbl_scoremap::utf8_score.insert(bbl_score_pair(utf8, score));
	}
#endif	//	defined(__BBL_USING_STDMAP_TABLE__)
}

}	//	namespace babel

/******************************************************************************
	□■□■                  Wraith the Trickster                  □■□■
	■□■□ ～I'll go with heaven's advantage and fool's wisdom.～ ■□■□
******************************************************************************/

