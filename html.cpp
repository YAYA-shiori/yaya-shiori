//
// AYA version 5
//
// HTMLの解析　（FREADHTML/PARSEHTML）
// 解析にはGumbo（HTML5準拠のパーサ）を使用しています。
//
// 結果はFREADXML/PARSEXMLと同じ形（name/attr/children/textのハッシュ）の<html>要素になります。
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

// Gumbo本体はCで書かれているので、下のDEBUG用のnewマクロより前に読み込む
#include "gumbo/src/gumbo.h"

#include <string>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
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

// 入れ子の深さの上限（<html>を1段目として数える。再帰でスタックを使い切らないため。
// DebugビルドのEXE（スタック1MB）で350段ほどが限界）
#define HTML_MAX_DEPTH	128

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlUtf8ToWide
 *  機能概要：  UTF-8文字列を内部文字列に変換します
 * -----------------------------------------------------------------------
 */
static yaya::string_t HtmlUtf8ToWide(const std::string &str)
{
	yaya::string_t result;
	if ( ! str.empty() ) {
		Ccct::MbcsToUcs2Buf(result, str.c_str(), CHARSET_UTF8);
	}
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlAsciiLower
 *  機能概要：  ASCIIの大文字を小文字にします（HTMLのタグ名は大文字小文字を区別しない）
 * -----------------------------------------------------------------------
 */
static std::string HtmlAsciiLower(const std::string &str)
{
	std::string result(str);
	for ( std::string::size_type i = 0 ; i < result.size() ; ++i ) {
		if ( result[i] >= 'A' && result[i] <= 'Z' ) {
			result[i] = static_cast<char>(result[i] - 'A' + 'a');
		}
	}
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlElementName
 *  機能概要：  要素のタグ名を返します
 *
 *  Gumboが知っているタグは正規化された（小文字の）名前、知らないタグは書かれていた名前です。
 *  HTML名前空間のタグ名は小文字にします
 * -----------------------------------------------------------------------
 */
static std::string HtmlElementName(const GumboElement &elem)
{
	std::string name;

	if ( elem.tag == GUMBO_TAG_UNKNOWN ) {
		GumboStringPiece piece = elem.original_tag;
		gumbo_tag_from_original_text(&piece);
		if ( piece.data && piece.length ) {
			name.assign(piece.data, piece.length);
		}
	}
	else {
		const char *normalized = gumbo_normalized_tagname(elem.tag);
		if ( normalized ) {
			name = normalized;
		}
	}

	if ( elem.tag_namespace == GUMBO_NAMESPACE_HTML ) {
		name = HtmlAsciiLower(name);
	}
	return name;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlAttributeName
 *  機能概要：  属性の名前を返します　XLink/XML/XMLNSの名前空間の属性は接頭辞（xlink:など）を付けます
 * -----------------------------------------------------------------------
 */
static std::string HtmlAttributeName(const GumboAttribute &attr)
{
	std::string name(attr.name ? attr.name : "");

	switch ( attr.attr_namespace ) {
	case GUMBO_ATTR_NAMESPACE_XLINK:
		return "xlink:" + name;
	case GUMBO_ATTR_NAMESPACE_XML:
		return "xml:" + name;
	case GUMBO_ATTR_NAMESPACE_XMLNS:
		// 属性xmlns自体はxmlnsのまま、xmlns:xlinkなどは接頭辞を付ける
		return (name == "xmlns") ? name : "xmlns:" + name;
	default:
		return name;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlElementToValue
 *  機能概要：  HTMLの要素をCValueに変換します
 *
 *  要素は以下のキーを持つハッシュになります（XMLと同じ）
 *    name     : タグ名（小文字）
 *    attr     : 属性名→属性値のハッシュ
 *    children : 子要素の配列（出現順）
 *    text     : 直下のテキスト（CDATAを含む）を連結したもの
 *
 *  コメントは含めません。子要素がある要素では、空白だけのテキストも含めません
 *
 *  返値　　：　深さの上限を超えたらfalse
 * -----------------------------------------------------------------------
 */
static bool HtmlElementToValue(const GumboElement &elem, CValue &out, int depth)
{
	if ( depth >= HTML_MAX_DEPTH ) {
		return false;
	}

	CValue attr(F_TAG_HASH, 0/*dmy*/);
	for ( unsigned int i = 0 ; i < elem.attributes.length ; ++i ) {
		const GumboAttribute *a = static_cast<const GumboAttribute *>(elem.attributes.data[i]);
		attr.hash()[CValue(HtmlUtf8ToWide(HtmlAttributeName(*a)))] = CValue(HtmlUtf8ToWide(a->value ? a->value : ""));
	}

	// 空白だけのテキストは子要素があるときだけ捨てるので、先に子要素の有無を調べる
	bool has_child_element = false;
	for ( unsigned int i = 0 ; i < elem.children.length ; ++i ) {
		const GumboNode *node = static_cast<const GumboNode *>(elem.children.data[i]);
		if ( node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_TEMPLATE ) {
			has_child_element = true;
			break;
		}
	}

	CValue children(F_TAG_ARRAY, 0/*dmy*/);
	std::string text;
	for ( unsigned int i = 0 ; i < elem.children.length ; ++i ) {
		const GumboNode *node = static_cast<const GumboNode *>(elem.children.data[i]);
		switch ( node->type ) {
		case GUMBO_NODE_ELEMENT:
		case GUMBO_NODE_TEMPLATE:
			children.array().emplace_back(CValue());
			if ( ! HtmlElementToValue(node->v.element, children.array().back(), depth + 1) ) {
				return false;
			}
			break;
		case GUMBO_NODE_TEXT:
		case GUMBO_NODE_CDATA:
			if ( node->v.text.text ) {
				text += node->v.text.text;
			}
			break;
		case GUMBO_NODE_WHITESPACE:
			if ( ! has_child_element && node->v.text.text ) {
				text += node->v.text.text;
			}
			break;
		default:
			// コメント・処理命令は無視する
			break;
		}
	}

	out = CValue(F_TAG_HASH, 0/*dmy*/);
	CValueHash &hash = out.hash();
	hash[CValue(L"name")] = CValue(HtmlUtf8ToWide(HtmlElementName(elem)));
	hash[CValue(L"attr")] = attr;
	hash[CValue(L"children")] = children;
	hash[CValue(L"text")] = CValue(HtmlUtf8ToWide(text));
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlToValue
 *  機能概要：  UTF-8のHTMLを解析して<html>要素をCValueにします
 *
 *  返値　　：　成功時true　失敗時はerrstrに内容を入れます
 * -----------------------------------------------------------------------
 */
bool HtmlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr)
{
	GumboOptions options = kGumboDefaultOptions;
	options.max_errors = 0; // 解析エラーの一覧は使わない

	GumboOutput *output = gumbo_parse_with_options(&options, utf8.c_str(), utf8.size());
	if ( ! output ) {
		errstr = L"failed to parse HTML";
		return false;
	}

	bool ok = false;
	if ( output->root && (output->root->type == GUMBO_NODE_ELEMENT || output->root->type == GUMBO_NODE_TEMPLATE) ) {
		ok = HtmlElementToValue(output->root->v.element, out, 0);
		if ( ! ok ) {
			errstr = L"element nesting is too deep";
		}
	}
	else {
		errstr = L"no root element";
	}

	gumbo_destroy_output(&options, output);
	return ok;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlFindCharsetInMeta
 *  機能概要：  <meta ...>の中身から charset=名前 を探します
 *
 *  <meta charset="...">と<meta http-equiv="Content-Type" content="text/html; charset=...">の
 *  どちらも「charset」「空白」「=」「空白」「引用符」「名前」の並びなので、まとめて探せます
 * -----------------------------------------------------------------------
 */
static bool HtmlFindCharsetInMeta(const std::string &tag, std::string &name)
{
	std::string::size_type pos = 0;
	while ( (pos = tag.find("charset", pos)) != std::string::npos ) {
		std::string::size_type p = pos + 7;
		pos = p;
		while ( p < tag.size() && isspace(static_cast<unsigned char>(tag[p])) ) {
			++p;
		}
		if ( p >= tag.size() || tag[p] != '=' ) {
			continue;
		}
		++p;
		while ( p < tag.size() && isspace(static_cast<unsigned char>(tag[p])) ) {
			++p;
		}
		if ( p < tag.size() && (tag[p] == '"' || tag[p] == '\'') ) {
			++p;
		}
		std::string::size_type e = p;
		while ( e < tag.size() && ! isspace(static_cast<unsigned char>(tag[e])) &&
			tag[e] != '"' && tag[e] != '\'' && tag[e] != ';' && tag[e] != '/' && tag[e] != '>' ) {
			++e;
		}
		if ( e > p ) {
			name = tag.substr(p, e - p);
			return true;
		}
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HtmlDetectCharset
 *  機能概要：  <meta>のcharsetから文字コードを判定します
 *
 *  返値　　：　CHARSET_*　BOM付き・宣言なし・不明な文字コードの場合はCHARSET_UTF8
 *
 *  HTML標準と同じく先頭1024バイトだけを調べます。コメントの中は読みません
 * -----------------------------------------------------------------------
 */
int HtmlDetectCharset(const std::string &bytes)
{
	if ( bytes.size() >= 3 &&
		static_cast<unsigned char>(bytes[0]) == 0xEF &&
		static_cast<unsigned char>(bytes[1]) == 0xBB &&
		static_cast<unsigned char>(bytes[2]) == 0xBF ) {
		return CHARSET_UTF8;
	}

	std::string head = HtmlAsciiLower(bytes.substr(0, 1024));
	std::string::size_type pos = 0;

	while ( (pos = head.find('<', pos)) != std::string::npos ) {
		if ( head.compare(pos, 4, "<!--") == 0 ) {
			std::string::size_type end = head.find("-->", pos + 4);
			if ( end == std::string::npos ) {
				break;
			}
			pos = end + 3;
			continue;
		}

		// "<meta" の直後が空白・/・>のときだけメタタグとして扱う（<metadata>などを除く）
		if ( head.compare(pos, 5, "<meta") == 0 && pos + 5 < head.size() &&
			(isspace(static_cast<unsigned char>(head[pos + 5])) || head[pos + 5] == '/' || head[pos + 5] == '>') ) {
			std::string::size_type end = head.find('>', pos);
			std::string tag = head.substr(pos + 5, (end == std::string::npos) ? std::string::npos : end - pos - 5);
			std::string name;
			if ( HtmlFindCharsetInMeta(tag, name) ) {
				// 名前の表記ゆれ（大文字小文字、Windows-31J/CP932などの別名）はCharsetTextToIDが吸収する
				int charset = Ccct::CharsetTextToID(name.c_str());
				if ( charset == CHARSET_DEFAULT || charset == CHARSET_BINARY ) {
					return CHARSET_UTF8;
				}
				return charset;
			}
			if ( end == std::string::npos ) {
				break;
			}
			pos = end;
			continue;
		}

		++pos;
	}

	return CHARSET_UTF8;
}
