// 
// AYA version 5
//
// JSON/XMLの解析と出力　（FREADJSON/FREADXML/PARSEJSON/PARSEXML/FWRITEJSON/FWRITEXML/DUMPJSON/DUMPXML）
// JSONの解析にはparson、XMLの解析と出力にはtinyxml2を使用しています。
// JSONの出力はYAYAの64bit整数を誤差なく書くため自前で行っています。
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

// tinyxml2はplacement newを使うので、下のDEBUG用のnewマクロより前に読み込む
#include "tinyxml2/tinyxml2.h"
#include "parson/parson.h"

#include <string>
#include <math.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
# include <float.h>
#else
# include <cmath>
#endif

#include "jsonxml.h"
#include "ccct.h"
#include "globaldef.h"
#include "manifest.h"
#include "value.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  クラス名：  CNumericLocaleGuard
 *  機能概要：  生存中だけLC_NUMERICを"C"にします
 *
 *  YAYAは起動時にOSのロケールを設定するため、小数点が","のロケールでは
 *  parson(strtod)が"1.5"を読めず、sprintfは"1,5"を書いてしまう
 * -----------------------------------------------------------------------
 */
class CNumericLocaleGuard
{
private:
	std::string old_locale;

public:
	CNumericLocaleGuard(void)
	{
		const char *p = setlocale(LC_NUMERIC, NULL);
		if ( p ) {
			old_locale = p;
		}
		setlocale(LC_NUMERIC, "C");
	}
	~CNumericLocaleGuard(void)
	{
		if ( ! old_locale.empty() ) {
			setlocale(LC_NUMERIC, old_locale.c_str());
		}
	}
};

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
	CNumericLocaleGuard locale_guard;

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

/* -----------------------------------------------------------------------
 *  関数名  ：  WideToUtf8
 *  機能概要：  内部文字列をUTF-8文字列に変換します
 * -----------------------------------------------------------------------
 */
static std::string WideToUtf8(const yaya::string_t &str)
{
	std::string result;
	char *p = Ccct::Ucs2ToMbcs(str, CHARSET_UTF8);
	if ( p ) {
		result = p;
		free(p);
	}
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsFiniteDouble
 *  機能概要：  NaNでも無限大でもなければtrue
 * -----------------------------------------------------------------------
 */
static bool IsFiniteDouble(double d)
{
#if defined(_MSC_VER)
	return _finite(d) != 0;
#else
	return std::isfinite(d);
#endif
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonAppendIndent
 *  機能概要：  改行とdepth段のインデントを追加します
 * -----------------------------------------------------------------------
 */
static void JsonAppendIndent(yaya::string_t &out, int depth)
{
	out += L'\n';
	for ( int i = 0; i < depth; ++i ) {
		out += L"    ";
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonAppendString
 *  機能概要：  JSONの文字列リテラルを追加します
 *
 *  " \ と制御文字だけをエスケープし、それ以外（非ASCIIを含む）はそのまま書きます
 * -----------------------------------------------------------------------
 */
static void JsonAppendString(yaya::string_t &out, const yaya::string_t &str)
{
	static const yaya::char_t hex[] = L"0123456789abcdef";

	out += L'"';
	for ( yaya::string_t::size_type i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		switch ( c ) {
		case L'"':  out += L"\\\""; break;
		case L'\\': out += L"\\\\"; break;
		case L'\b': out += L"\\b"; break;
		case L'\f': out += L"\\f"; break;
		case L'\n': out += L"\\n"; break;
		case L'\r': out += L"\\r"; break;
		case L'\t': out += L"\\t"; break;
		default:
			if ( c >= 0 && c < 0x20 ) {
				out += L"\\u00";
				out += hex[(c >> 4) & 0xF];
				out += hex[c & 0xF];
			}
			else {
				out += c;
			}
			break;
		}
	}
	out += L'"';
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonAppendDouble
 *  機能概要：  JSONの数値（実数）を追加します
 *
 *  読み戻して同じ値になる最短に近い表記にし、実数だとわかるよう小数点か指数を必ず付けます
 *  NaNと無限大はJSONで表せないのでnullにします
 * -----------------------------------------------------------------------
 */
static void JsonAppendDouble(yaya::string_t &out, double d)
{
	if ( ! IsFiniteDouble(d) ) {
		out += L"null";
		return;
	}

	char buf[64];
	sprintf(buf, "%.15g", d);
	if ( strtod(buf, NULL) != d ) {
		sprintf(buf, "%.17g", d);
	}

	// 指数の桁数はCRTによって違う（VC6は"e-008"、gccは"e-08"）ので先頭の0を除いて揃える
	char *e = strpbrk(buf, "eE");
	if ( e ) {
		char *digits = e + 1;
		if ( *digits == '+' || *digits == '-' ) {
			++digits;
		}
		char *nz = digits;
		while ( *nz == '0' && *(nz + 1) ) {
			++nz;
		}
		if ( nz != digits ) {
			memmove(digits, nz, strlen(nz) + 1);
		}
	}

	for ( const char *p = buf; *p; ++p ) {
		out += static_cast<yaya::char_t>(*p);
	}
	if ( ! strpbrk(buf, ".eE") ) {
		out += L".0";
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  JsonAppendValue
 *  機能概要：  CValueをJSONにして追加します
 *
 *  ハッシュ→オブジェクト（キーは文字列にする）、配列→配列、整数・実数→数値
 *  文字列→文字列、空（VOID）→null
 * -----------------------------------------------------------------------
 */
static void JsonAppendValue(yaya::string_t &out, const CValue &value, bool pretty, int depth)
{
	switch ( value.GetType() ) {
	case F_TAG_INT:
		out += yaya::ws_lltoa(value.i_value);
		break;
	case F_TAG_DOUBLE:
		JsonAppendDouble(out, value.d_value);
		break;
	case F_TAG_STRING:
		JsonAppendString(out, value.s_value);
		break;
	case F_TAG_ARRAY:
		{
			const CValueArray &arr = value.array();
			if ( arr.empty() ) {
				out += L"[]";
				break;
			}
			out += L'[';
			for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
				if ( it != arr.begin() ) {
					out += L',';
				}
				if ( pretty ) {
					JsonAppendIndent(out, depth + 1);
				}
				JsonAppendValue(out, *it, pretty, depth + 1);
			}
			if ( pretty ) {
				JsonAppendIndent(out, depth);
			}
			out += L']';
		}
		break;
	case F_TAG_HASH:
		{
			const CValueHash &hash = value.hash();
			if ( hash.empty() ) {
				out += L"{}";
				break;
			}
			out += L'{';
			for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
				if ( it != hash.begin() ) {
					out += L',';
				}
				if ( pretty ) {
					JsonAppendIndent(out, depth + 1);
				}
				JsonAppendString(out, it->first.GetValueString());
				out += pretty ? L": " : L":";
				JsonAppendValue(out, it->second, pretty, depth + 1);
			}
			if ( pretty ) {
				JsonAppendIndent(out, depth);
			}
			out += L'}';
		}
		break;
	default: // F_TAG_VOID
		out += L"null";
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ValueToJson
 *  機能概要：  CValueをJSONの文字列にします
 * -----------------------------------------------------------------------
 */
void ValueToJson(const CValue &value, bool pretty, yaya::string_t &out)
{
	CNumericLocaleGuard locale_guard;

	out.erase();
	JsonAppendValue(out, value, pretty, 0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsXmlName
 *  機能概要：  XMLの要素名・属性名として使えればtrue
 *
 *  ASCIIの範囲だけ規則どおりに調べ、非ASCIIの文字はすべて許します
 * -----------------------------------------------------------------------
 */
static bool IsXmlName(const yaya::string_t &name)
{
	if ( name.empty() ) {
		return false;
	}
	for ( yaya::string_t::size_type i = 0; i < name.size(); ++i ) {
		yaya::char_t c = name[i];
		if ( c >= 0x80 ) {
			continue;
		}
		if ( (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || c == L'_' || c == L':' ) {
			continue;
		}
		if ( i > 0 && ((c >= L'0' && c <= L'9') || c == L'-' || c == L'.') ) {
			continue;
		}
		return false;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HashFind
 *  機能概要：  ハッシュからキーの値を探します　無ければNULL
 * -----------------------------------------------------------------------
 */
static const CValue *HashFind(const CValue &hash, const yaya::char_t *key)
{
	CValueHash::const_iterator it = hash.hash().find(CValue(key));
	if ( it == hash.hash().end() ) {
		return NULL;
	}
	return &(it->second);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ValueToXmlElement
 *  機能概要：  要素のハッシュ（name/attr/children/text）からXMLの要素を作ります
 *
 *  nameだけ必須で、ほかのキーは無くてもよい。textは子要素より前に置きます
 *  返値　　：　作った要素　失敗時はNULLでerrstrに詳細（作りかけの要素はdocが解放する）
 * -----------------------------------------------------------------------
 */
static tinyxml2::XMLElement *ValueToXmlElement(const CValue &value, tinyxml2::XMLDocument &doc, yaya::string_t &errstr)
{
	if ( ! value.IsHash() ) {
		errstr = L"element is not a hash";
		return NULL;
	}

	const CValue *name = HashFind(value, L"name");
	if ( ! name || ! name->IsStringReal() || ! IsXmlName(name->s_value) ) {
		errstr = L"invalid element name";
		if ( name && name->IsStringReal() ) {
			errstr += L" : " + name->s_value;
		}
		return NULL;
	}

	tinyxml2::XMLElement *elem = doc.NewElement(WideToUtf8(name->s_value).c_str());

	const CValue *attr = HashFind(value, L"attr");
	if ( attr && ! attr->IsVoid() ) {
		if ( ! attr->IsHash() ) {
			errstr = L"attr is not a hash : " + name->s_value;
			return NULL;
		}
		const CValueHash &attrs = attr->hash();
		for ( CValueHash::const_iterator it = attrs.begin(); it != attrs.end(); ++it ) {
			yaya::string_t attr_name = it->first.GetValueString();
			if ( ! IsXmlName(attr_name) ) {
				errstr = L"invalid attribute name : " + name->s_value + L" " + attr_name;
				return NULL;
			}
			if ( it->second.IsArray() || it->second.IsHash() ) {
				errstr = L"attribute value is not a scalar : " + name->s_value + L" " + attr_name;
				return NULL;
			}
			elem->SetAttribute(WideToUtf8(attr_name).c_str(), WideToUtf8(it->second.GetValueString()).c_str());
		}
	}

	const CValue *text = HashFind(value, L"text");
	if ( text && ! text->IsVoid() ) {
		if ( text->IsArray() || text->IsHash() ) {
			errstr = L"text is not a scalar : " + name->s_value;
			return NULL;
		}
		yaya::string_t str = text->GetValueString();
		if ( ! str.empty() ) {
			elem->InsertEndChild(doc.NewText(WideToUtf8(str).c_str()));
		}
	}

	const CValue *children = HashFind(value, L"children");
	if ( children && ! children->IsVoid() ) {
		if ( ! children->IsArray() ) {
			errstr = L"children is not an array : " + name->s_value;
			return NULL;
		}
		const CValueArray &arr = children->array();
		for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
			tinyxml2::XMLElement *child = ValueToXmlElement(*it, doc, errstr);
			if ( ! child ) {
				return NULL;
			}
			elem->InsertEndChild(child);
		}
	}

	return elem;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ValueToXml
 *  機能概要：  要素のハッシュをXML宣言付きのXMLの文字列にします
 *
 *  返値　　：　成功時true　失敗時はerrstrに詳細
 * -----------------------------------------------------------------------
 */
bool ValueToXml(const CValue &value, const char *encoding, bool pretty, yaya::string_t &out, yaya::string_t &errstr)
{
	tinyxml2::XMLDocument doc;

	tinyxml2::XMLElement *root = ValueToXmlElement(value, doc, errstr);
	if ( ! root ) {
		return false;
	}

	std::string decl = std::string("xml version=\"1.0\" encoding=\"") + encoding + "\"";
	doc.InsertEndChild(doc.NewDeclaration(decl.c_str()));
	doc.InsertEndChild(root);

	tinyxml2::XMLPrinter printer(0, ! pretty);
	doc.Print(&printer);

	out = Utf8ToWide(printer.CStr());
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  XmlCharsetName
 *  機能概要：  文字コードからXML宣言のencodingに書く名前を返します
 *
 *  OSデフォルトの場合はコードページから決めます
 * -----------------------------------------------------------------------
 */
const char *XmlCharsetName(int charset)
{
	switch ( charset ) {
	case CHARSET_SJIS:   return "Shift_JIS";
	case CHARSET_EUCJP:  return "EUC-JP";
	case CHARSET_JIS:    return "ISO-2022-JP";
	case CHARSET_BIG5:   return "Big5";
	case CHARSET_GB2312: return "GB2312";
	case CHARSET_EUCKR:  return "EUC-KR";
	case CHARSET_DEFAULT:
		{
			unsigned int cp = Ccct::ccct_getcodepage(CHARSET_DEFAULT);
			switch ( cp ) {
			case 0:     return "UTF-8"; // POSIX
			case 65001: return "UTF-8";
			case 932:   return "Shift_JIS";
			case 936:   return "GB2312";
			case 949:   return "EUC-KR";
			case 950:   return "Big5";
			default:
				{
					static char name[32];
					sprintf(name, "windows-%u", cp);
					return name;
				}
			}
		}
	default:
		return "UTF-8";
	}
}
