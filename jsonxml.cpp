// 
// AYA version 5
//
// JSON/XMLの解析　（FREADJSON/FREADXML/PARSEJSON/PARSEXML）
// JSONの解析にはparson、XMLの解析にはtinyxml2を使用しています。
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

// tinyxml2はplacement newを使うので、下のDEBUG用のnewマクロより前に読み込む
#include "tinyxml2/tinyxml2.h"
#include "parson/parson.h"

#include <string>
#include <math.h>

#include "jsonxml.h"
#include "ccct.h"
#include "globaldef.h"
#include "manifest.h"
#include "value.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  Utf8ToWide
 *  機能概要：  UTF-8文字列を内部文字列に変換します
 * -----------------------------------------------------------------------
 */
static yaya::string_t Utf8ToWide(const char *str)
{
	yaya::string_t result;
	if ( str ) {
		Ccct::MbcsToUcs2Buf(result, str, CHARSET_UTF8);
	}
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CutUtf8Bom
 *  機能概要：  先頭のUTF-8 BOMを除去します
 * -----------------------------------------------------------------------
 */
void CutUtf8Bom(std::string &str)
{
	if ( str.size() >= 3 &&
		static_cast<unsigned char>(str[0]) == 0xEF &&
		static_cast<unsigned char>(str[1]) == 0xBB &&
		static_cast<unsigned char>(str[2]) == 0xBF ) {
		str.erase(0, 3);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonNumberToValue
 *  機能概要：  JSONの数値をCValueにします
 *
 *  parsonは数値を実数で保持するため、整数値でint_tに収まるものは整数、それ以外は実数にします
 * -----------------------------------------------------------------------
 */
static CValue JsonNumberToValue(double d)
{
	// 2^63 = 9223372036854775808
	if ( d >= -9223372036854775808.0 && d < 9223372036854775808.0 && floor(d) == d ) {
		return CValue(static_cast<yaya::int_t>(d));
	}
	return CValue(d);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonValueToValue
 *  機能概要：  parsonのJSON_ValueをCValueに変換します
 *
 *  オブジェクト→ハッシュ、配列→配列、文字列→文字列、数値→整数/実数
 *  true/false→1/0、null→空値
 * -----------------------------------------------------------------------
 */
static void JsonValueToValue(const JSON_Value *jv, CValue &out)
{
	switch ( json_value_get_type(jv) ) {
	case JSONString:
		out = Utf8ToWide(json_value_get_string(jv));
		break;
	case JSONNumber:
		out = JsonNumberToValue(json_value_get_number(jv));
		break;
	case JSONBoolean:
		out = CValue(json_value_get_boolean(jv) ? 1 : 0);
		break;
	case JSONArray:
		{
			const JSON_Array *ja = json_value_get_array(jv);
			size_t n = json_array_get_count(ja);

			out = CValue(F_TAG_ARRAY, 0/*dmy*/);
			CValueArray &arr = out.array();
			arr.resize(n);
			for ( size_t i = 0; i < n; ++i ) {
				JsonValueToValue(json_array_get_value(ja, i), arr[i]);
			}
		}
		break;
	case JSONObject:
		{
			const JSON_Object *jo = json_value_get_object(jv);
			size_t n = json_object_get_count(jo);

			out = CValue(F_TAG_HASH, 0/*dmy*/);
			CValueHash &hash = out.hash();
			for ( size_t i = 0; i < n; ++i ) {
				CValue key(Utf8ToWide(json_object_get_name(jo, i)));
				JsonValueToValue(json_object_get_value_at(jo, i), hash[key]);
			}
		}
		break;
	default: // JSONNull
		out = CValue();
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonToValue
 *  機能概要：  UTF-8のJSONを解析してCValueにします
 *
 *  返値　　：　成功時true
 * -----------------------------------------------------------------------
 */
bool JsonToValue(const std::string &utf8, CValue &out)
{
	JSON_Value *root = json_parse_string_with_comments(utf8.c_str());
	if ( ! root ) {
		return false;
	}

	JsonValueToValue(root, out);
	json_value_free(root);
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  XmlElementToValue
 *  機能概要：  XMLの要素をCValueに変換します
 *
 *  要素は以下のキーを持つハッシュになります
 *    name     : タグ名
 *    attr     : 属性名→属性値のハッシュ
 *    children : 子要素の配列（出現順）
 *    text     : 直下のテキスト（CDATAを含む）を連結したもの
 * -----------------------------------------------------------------------
 */
static void XmlElementToValue(const tinyxml2::XMLElement *elem, CValue &out)
{
	CValue attr(F_TAG_HASH, 0/*dmy*/);
	for ( const tinyxml2::XMLAttribute *a = elem->FirstAttribute(); a; a = a->Next() ) {
		attr.hash()[CValue(Utf8ToWide(a->Name()))] = CValue(Utf8ToWide(a->Value()));
	}

	CValue children(F_TAG_ARRAY, 0/*dmy*/);
	std::string text;
	for ( const tinyxml2::XMLNode *node = elem->FirstChild(); node; node = node->NextSibling() ) {
		const tinyxml2::XMLElement *child = node->ToElement();
		if ( child ) {
			children.array().emplace_back(CValue());
			XmlElementToValue(child, children.array().back());
			continue;
		}
		const tinyxml2::XMLText *t = node->ToText();
		if ( t && t->Value() ) {
			text += t->Value();
		}
	}

	out = CValue(F_TAG_HASH, 0/*dmy*/);
	CValueHash &hash = out.hash();
	hash[CValue(L"name")] = CValue(Utf8ToWide(elem->Name()));
	hash[CValue(L"attr")] = attr;
	hash[CValue(L"children")] = children;
	hash[CValue(L"text")] = CValue(Utf8ToWide(text.c_str()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  XmlToValue
 *  機能概要：  UTF-8のXMLを解析してルート要素をCValueにします
 *
 *  返値　　：　成功時true　失敗時はerrstrにtinyxml2のエラー内容を入れます
 * -----------------------------------------------------------------------
 */
bool XmlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr)
{
	tinyxml2::XMLDocument doc;

	if ( doc.Parse(utf8.c_str(), utf8.size()) != tinyxml2::XML_SUCCESS ) {
		errstr = Utf8ToWide(doc.ErrorStr());
		return false;
	}

	const tinyxml2::XMLElement *root = doc.RootElement();
	if ( ! root ) {
		errstr = L"no root element";
		return false;
	}

	XmlElementToValue(root, out);
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  XmlDetectCharset
 *  機能概要：  XML宣言のencodingから文字コードを判定します
 *
 *  返値　　：　CHARSET_*　BOM付き・宣言なし・不明な文字コードの場合はCHARSET_UTF8
 * -----------------------------------------------------------------------
 */
int XmlDetectCharset(const std::string &bytes)
{
	std::string::size_type pos = 0;

	if ( bytes.size() >= 3 &&
		static_cast<unsigned char>(bytes[0]) == 0xEF &&
		static_cast<unsigned char>(bytes[1]) == 0xBB &&
		static_cast<unsigned char>(bytes[2]) == 0xBF ) {
		return CHARSET_UTF8;
	}

	while ( pos < bytes.size() && (bytes[pos] == ' ' || bytes[pos] == '\t' || bytes[pos] == '\r' || bytes[pos] == '\n') ) {
		++pos;
	}
	if ( bytes.compare(pos, 5, "<?xml") != 0 ) {
		return CHARSET_UTF8;
	}

	std::string::size_type end = bytes.find("?>", pos);
	if ( end == std::string::npos ) {
		return CHARSET_UTF8;
	}

	std::string decl = bytes.substr(pos, end - pos);
	std::string::size_type enc = decl.find("encoding");
	if ( enc == std::string::npos ) {
		return CHARSET_UTF8;
	}

	std::string::size_type q = decl.find_first_of("\"'", enc);
	if ( q == std::string::npos ) {
		return CHARSET_UTF8;
	}
	std::string::size_type qe = decl.find(decl[q], q + 1);
	if ( qe == std::string::npos ) {
		return CHARSET_UTF8;
	}

	std::string name = decl.substr(q + 1, qe - q - 1);
	for ( std::string::size_type i = 0; i < name.size(); ++i ) {
		if ( name[i] >= 'a' && name[i] <= 'z' ) {
			name[i] = static_cast<char>(name[i] - 'a' + 'A');
		}
	}

	if ( name == "WINDOWS-31J" || name == "CP932" || name == "MS932" || name == "SHIFT-JIS" || name == "X-SJIS" ) {
		return CHARSET_SJIS;
	}

	int charset = Ccct::CharsetTextToID(name.c_str());
	if ( charset == CHARSET_DEFAULT || charset == CHARSET_BINARY ) {
		return CHARSET_UTF8;
	}
	return charset;
}
