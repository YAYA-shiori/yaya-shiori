//
// AYA version 5
//
// YAML/TOMLの解析と出力　（FREADYAML/FREADTOML/PARSEYAML/PARSETOML/FWRITEYAML/FWRITETOML/DUMPYAML/DUMPTOML）
// どちらも自前で実装しています。
//
// YAMLは1つの文書だけからなる、設定ファイルなどでよく使われる範囲（サブセット）に対応します。
//   対応：ブロック/フロー形式のマッピングとシーケンス、クォート付き文字列、ブロックスカラー（| >）、
//         コメント、アンカーとエイリアス（& *）、マージキー（<<）、YAML 1.2 Core Schemaの型解決
//   非対応：複数の文書、複合キー（? ）、タグによる型の指定（!!strなど一部だけ扱う）
// TOMLはv1.0.0に対応します（インライン表の中の改行と末尾のカンマはv1.1と同じく許します）。
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <vector>
#include <map>
#include <set>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
# include <float.h>
#else
# include <cmath>
#endif

#include "yamltoml.h"
#include "jsonxml.h"
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

typedef yaya::string_t::size_type spos_t;

enum {
	YAML_MAX_DEPTH = 256,				// 入れ子の深さの上限
	YAML_MAX_ALIAS_NODES = 1000000,		// エイリアスで複製する要素数の合計の上限
	TOML_MAX_DEPTH = 256,				// 入れ子の深さの上限
	TOML_ARRAY_LINE_WIDTH = 80			// 整形時、これより長い配列は1要素1行にする
};

/* -----------------------------------------------------------------------
 *  関数名  ：  PrepareText
 *  機能概要：  UTF-8の入力を内部文字列にし、改行をLFに揃え、先頭のBOMを除きます
 * -----------------------------------------------------------------------
 */
static yaya::string_t PrepareText(const std::string &utf8)
{
	yaya::string_t src = Utf8ToWide(utf8.c_str());
	yaya::string_t dst;
	dst.reserve(src.size());

	spos_t i = 0;
	if ( ! src.empty() && src[0] == 0xFEFF ) {
		i = 1;
	}
	for ( ; i < src.size(); ++i ) {
		if ( src[i] == L'\r' ) {
			dst += L'\n';
			if ( i + 1 < src.size() && src[i + 1] == L'\n' ) {
				++i;
			}
		}
		else {
			dst += src[i];
		}
	}
	return dst;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsNanDouble / MakeInfinity / MakeNan
 *  機能概要：  NaNと無限大の判定と生成
 * -----------------------------------------------------------------------
 */
static bool IsNanDouble(double d)
{
#if defined(_MSC_VER)
	return _isnan(d) != 0;
#else
	return std::isnan(d);
#endif
}

static double MakeInfinity(bool negative)
{
	double inf = HUGE_VAL;
	return negative ? -inf : inf;
}

static double MakeNan(void)
{
	// inf - inf はVC6の最適化で0にされてしまうので、ビット列（quiet NaN）から作る
	union {
		std::uint64_t u;
		double d;
	} v;
	v.u = static_cast<std::uint64_t>(0x7FF80000) << 32;
	return v.d;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  HexDigitValue
 *  機能概要：  16進の1桁の値を返します　数字でなければ-1
 * -----------------------------------------------------------------------
 */
static int HexDigitValue(yaya::char_t c)
{
	if ( c >= L'0' && c <= L'9' ) {
		return c - L'0';
	}
	if ( c >= L'a' && c <= L'f' ) {
		return c - L'a' + 10;
	}
	if ( c >= L'A' && c <= L'F' ) {
		return c - L'A' + 10;
	}
	return -1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ReadHexDigits
 *  機能概要：  s[p]から16進digits桁を読み、pを進めます　桁が足りなければfalse
 * -----------------------------------------------------------------------
 */
static bool ReadHexDigits(const yaya::string_t &s, spos_t &p, int digits, unsigned long &cp)
{
	cp = 0;
	for ( int i = 0; i < digits; ++i ) {
		if ( p >= s.size() ) {
			return false;
		}
		int d = HexDigitValue(s[p]);
		if ( d < 0 ) {
			return false;
		}
		cp = cp * 16 + d;
		++p;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  AppendCodePoint
 *  機能概要：  Unicodeのコードポイントを追加します（wchar_tが16bitならサロゲートペアにする）
 *
 *  返値　　：　サロゲートの範囲や0x10FFFFを超える値ならfalse
 * -----------------------------------------------------------------------
 */
static bool AppendCodePoint(yaya::string_t &out, unsigned long cp)
{
	if ( cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ) {
		return false;
	}
	if ( sizeof(yaya::char_t) == 2 && cp >= 0x10000 ) {
		cp -= 0x10000;
		out += static_cast<yaya::char_t>(0xD800 + (cp >> 10));
		out += static_cast<yaya::char_t>(0xDC00 + (cp & 0x3FF));
	}
	else {
		out += static_cast<yaya::char_t>(cp);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  DigitsToInt
 *  機能概要：  数字の列（区切りの_は除いたもの）を整数にします
 *
 *  返値　　：　不正な桁があるか、整数の範囲を超えるならfalse
 * -----------------------------------------------------------------------
 */
static bool DigitsToInt(const yaya::string_t &digits, int base, bool negative, yaya::int_t &out)
{
	if ( digits.empty() ) {
		return false;
	}

	std::uint64_t limit = static_cast<std::uint64_t>(1) << 63;
	if ( ! negative ) {
		limit -= 1;
	}

	std::uint64_t v = 0;
	for ( spos_t i = 0; i < digits.size(); ++i ) {
		int d = HexDigitValue(digits[i]);
		if ( d < 0 || d >= base ) {
			return false;
		}
		if ( v > (limit - d) / base ) {
			return false;
		}
		v = v * base + d;
	}

	if ( negative ) {
		out = static_cast<yaya::int_t>(0 - v);
	}
	else {
		out = static_cast<yaya::int_t>(v);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TextToDouble
 *  機能概要：  ASCIIの数値表記を実数にします（ロケールは呼び出し側で"C"にしておく）
 * -----------------------------------------------------------------------
 */
static bool TextToDouble(const yaya::string_t &text, double &out)
{
	std::string a;
	for ( spos_t i = 0; i < text.size(); ++i ) {
		if ( text[i] == 0 || text[i] >= 0x80 ) {
			return false;
		}
		a += static_cast<char>(text[i]);
	}
	if ( a.empty() ) {
		return false;
	}

	char *end = NULL;
	out = strtod(a.c_str(), &end);
	return end && *end == 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  IsFlowIndicator / IsBlankChar / IsDecDigit
 *  機能概要：  文字の種類の判定
 * -----------------------------------------------------------------------
 */
static bool IsFlowIndicator(yaya::char_t c)
{
	return c == L',' || c == L'[' || c == L']' || c == L'{' || c == L'}';
}

static bool IsBlankChar(yaya::char_t c)
{
	return c == L' ' || c == L'\t';
}

static bool IsDecDigit(yaya::char_t c)
{
	return c >= L'0' && c <= L'9';
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlIsFloatForm
 *  機能概要：  YAML 1.2 Core Schemaの浮動小数点の形式ならtrue
 *
 *  [-+]? ( \. [0-9]+ | [0-9]+ ( \. [0-9]* )? ) ( [eE] [-+]? [0-9]+ )?
 * -----------------------------------------------------------------------
 */
static bool YamlIsFloatForm(const yaya::string_t &t)
{
	spos_t n = t.size();
	spos_t i = 0;

	if ( i < n && (t[i] == L'+' || t[i] == L'-') ) {
		++i;
	}

	spos_t int_start = i;
	while ( i < n && IsDecDigit(t[i]) ) {
		++i;
	}
	bool has_int = i > int_start;

	if ( i < n && t[i] == L'.' ) {
		++i;
		spos_t frac_start = i;
		while ( i < n && IsDecDigit(t[i]) ) {
			++i;
		}
		if ( ! has_int && i == frac_start ) {
			return false;
		}
	}
	else if ( ! has_int ) {
		return false;
	}

	if ( i < n && (t[i] == L'e' || t[i] == L'E') ) {
		++i;
		if ( i < n && (t[i] == L'+' || t[i] == L'-') ) {
			++i;
		}
		spos_t exp_start = i;
		while ( i < n && IsDecDigit(t[i]) ) {
			++i;
		}
		if ( i == exp_start ) {
			return false;
		}
	}

	return i == n;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlResolvePlain
 *  機能概要：  クォートされていないYAMLのスカラーを、YAML 1.2 Core Schemaに従って値にします
 *
 *  null/~/空→空値、true/false→1/0、整数（10進・0o・0x）→整数、実数（.inf .nanを含む）→実数
 *  それ以外は文字列。10進の整数が範囲を超えるときは実数にします
 * -----------------------------------------------------------------------
 */
static CValue YamlResolvePlain(const yaya::string_t &t)
{
	if ( t.empty() || t == L"~" || t == L"null" || t == L"Null" || t == L"NULL" ) {
		return CValue();
	}
	if ( t == L"true" || t == L"True" || t == L"TRUE" ) {
		return CValue(1);
	}
	if ( t == L"false" || t == L"False" || t == L"FALSE" ) {
		return CValue(0);
	}

	spos_t n = t.size();
	bool negative = false;
	spos_t start = 0;
	if ( t[0] == L'+' || t[0] == L'-' ) {
		negative = (t[0] == L'-');
		start = 1;
	}

	// 10進の整数
	if ( start < n ) {
		bool all_digits = true;
		for ( spos_t i = start; i < n; ++i ) {
			if ( ! IsDecDigit(t[i]) ) {
				all_digits = false;
				break;
			}
		}
		if ( all_digits ) {
			yaya::int_t v;
			if ( DigitsToInt(t.substr(start), 10, negative, v) ) {
				return CValue(v);
			}
			double d;
			if ( TextToDouble(t, d) ) {
				return CValue(d);
			}
		}
	}

	// 8進・16進の整数（範囲外や不正な桁なら文字列のまま）
	if ( n > 2 && t[0] == L'0' && (t[1] == L'o' || t[1] == L'x') ) {
		yaya::int_t v;
		if ( DigitsToInt(t.substr(2), (t[1] == L'o') ? 8 : 16, false, v) ) {
			return CValue(v);
		}
		return CValue(t);
	}

	// 実数
	if ( YamlIsFloatForm(t) ) {
		double d;
		if ( TextToDouble(t, d) ) {
			return CValue(d);
		}
	}

	yaya::string_t rest = t.substr(start);
	if ( rest == L".inf" || rest == L".Inf" || rest == L".INF" ) {
		return CValue(MakeInfinity(negative));
	}
	if ( t == L".nan" || t == L".NaN" || t == L".NAN" ) {
		return CValue(MakeNan());
	}

	return CValue(t);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlScalarValue
 *  機能概要：  スカラーの文字列とタグから値を決めます
 *
 *  !!str（と!）は常に文字列、!!int !!float !!bool !!nullはクォートされていても型を解決します
 *  それ以外のタグは無視します
 * -----------------------------------------------------------------------
 */
static CValue YamlScalarValue(const yaya::string_t &text, bool plain, const yaya::string_t &tag, bool has_tag)
{
	if ( has_tag ) {
		if ( tag == L"!" || tag == L"!!str" ) {
			return CValue(text);
		}
		if ( tag == L"!!int" || tag == L"!!float" || tag == L"!!bool" || tag == L"!!null" ) {
			CValue v = YamlResolvePlain(text);
			if ( tag == L"!!float" && v.IsIntReal() ) {
				return CValue(static_cast<double>(v.i_value));
			}
			return v;
		}
	}
	if ( plain ) {
		return YamlResolvePlain(text);
	}
	return CValue(text);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CountNodes
 *  機能概要：  値に含まれる要素の数（自身を含む）を数えます
 * -----------------------------------------------------------------------
 */
static size_t CountNodes(const CValue &v)
{
	size_t count = 1;
	if ( v.IsArray() ) {
		const CValueArray &arr = v.array();
		for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
			count += CountNodes(*it);
		}
	}
	else if ( v.IsHash() ) {
		const CValueHash &hash = v.hash();
		for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
			count += CountNodes(it->second);
		}
	}
	return count;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  MergeHash
 *  機能概要：  srcのキーのうち、dstに無いものだけをdstに加えます（YAMLのマージキー）
 * -----------------------------------------------------------------------
 */
static void MergeHash(CValue &dst, const CValue &src)
{
	const CValueHash &sh = src.hash();
	CValueHash &dh = dst.hash();
	for ( CValueHash::const_iterator it = sh.begin(); it != sh.end(); ++it ) {
		if ( dh.find(it->first) == dh.end() ) {
			dh.insert(*it);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// YAMLの解析
//////////////////////////////////////////////////////////////////////////

enum {
	YAML_CTX_ROOT,			// 文書の最上位
	YAML_CTX_SEQ_ENTRY,		// "- "の後
	YAML_CTX_MAP_VALUE		// "key:"の後
};

// 入力中の位置
struct CYamlMark
{
	spos_t	pos;
	spos_t	line_start;
	size_t	line;
	bool	tab;			// 行頭の字下げにタブが含まれている
};

/* -----------------------------------------------------------------------
 *  クラス名：  CYamlParser
 *  機能概要：  YAMLの解析
 *
 *  ブロック形式は字下げ（桁）で構造を判断する再帰下降の解析です
 *  ノードを1つ解析し終えた時点では、posはそのノードの最後の行の行末（改行かコメントの位置）にあります
 * -----------------------------------------------------------------------
 */
class CYamlParser
{
private:
	const yaya::string_t &s;
	spos_t	n;
	spos_t	pos;
	spos_t	line_start;
	size_t	line;
	int		depth;
	size_t	alias_nodes;

	std::map<yaya::string_t, CValue> anchors;
	std::map<yaya::string_t, size_t> anchor_nodes;

public:
	yaya::string_t errstr;

	CYamlParser(const yaya::string_t &src) :
		s(src), n(src.size()), pos(0), line_start(0), line(1), depth(0), alias_nodes(0) { }

	bool	Parse(CValue &out);

private:
	yaya::char_t Ch(spos_t p) const
	{
		return p < n ? s[p] : 0;
	}
	int Col(void) const
	{
		return static_cast<int>(pos - line_start);
	}
	bool IsWsOrEnd(spos_t p) const
	{
		return p >= n || s[p] == L' ' || s[p] == L'\t' || s[p] == L'\n';
	}
	bool AtLineEnd(void) const
	{
		return pos >= n || s[pos] == L'\n' || s[pos] == L'#';
	}
	void NewLine(void)
	{
		++pos;
		++line;
		line_start = pos;
	}
	void SkipBlanks(void)
	{
		while ( pos < n && IsBlankChar(s[pos]) ) {
			++pos;
		}
	}
	void Seek(const CYamlMark &m)
	{
		pos = m.pos;
		line_start = m.line_start;
		line = m.line;
	}
	static int MarkCol(const CYamlMark &m)
	{
		return static_cast<int>(m.pos - m.line_start);
	}

	bool Fail(const yaya::string_t &msg)
	{
		return FailAt(line, msg);
	}
	bool FailAt(size_t ln, const yaya::string_t &msg)
	{
		if ( errstr.empty() ) {
			errstr = L"line " + yaya::ws_lltoa(static_cast<yaya::int_t>(ln)) + L" : " + msg;
		}
		return false;
	}

	bool	FinishLine(void);
	bool	PeekNext(CYamlMark &m) const;
	bool	IsDocMarker(const CYamlMark &m) const;
	bool	IsDocMarkerAt(spos_t p, spos_t ls) const;
	bool	IsSeqIndicator(spos_t p) const;
	bool	IsPlainStartForbidden(spos_t p, bool flow) const;
	bool	IsImplicitKey(spos_t p) const;

	bool	ParseProperties(yaya::string_t &anchor, bool &has_anchor, yaya::string_t &tag, bool &has_tag, bool flow);
	yaya::string_t	ReadName(void);
	bool	ParseAlias(CValue &out);
	void	RegisterAnchor(const yaya::string_t &name, const CValue &v);
	bool	ApplyMerges(CValue &out, const std::vector<CValue> &merges);

	bool	ParseNode(int parent_indent, int ctx, CValue &out);
	bool	ParseNodeBody(int parent_indent, int ctx, CValue &out);
	bool	ParseBlockSeq(int indent, CValue &out);
	bool	ParseBlockMap(int indent, CValue &out);
	bool	ParseMapKey(yaya::string_t &key, bool &is_merge);
	spos_t	ScanPlainLine(spos_t start);
	void	ParsePlainBlock(int parent_indent, yaya::string_t &text);
	bool	ParseBlockScalar(int parent_indent, yaya::string_t &text);
	bool	ParseQuoted(yaya::string_t &text);
	bool	ParseDoubleEscape(yaya::string_t &text);

	void	SkipFlowSpace(void);
	bool	ParseFlow(CValue &out);
	bool	ParseFlowBody(CValue &out);
	bool	ParseFlowNode(CValue &out);
	bool	ParseFlowKey(yaya::string_t &key, bool &plain);
	void	ScanFlowPlain(yaya::string_t &text);
};

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::FinishLine
 *  機能概要：  値の後が空白とコメントだけで行末になっていることを確かめます
 * -----------------------------------------------------------------------
 */
bool CYamlParser::FinishLine(void)
{
	SkipBlanks();
	if ( pos < n && s[pos] == L'#' ) {
		while ( pos < n && s[pos] != L'\n' ) {
			++pos;
		}
	}
	if ( pos < n && s[pos] != L'\n' ) {
		return Fail(L"unexpected characters after a value");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::PeekNext
 *  機能概要：  次に内容のある位置を探します（行末の空白・コメント、空行、コメント行を飛ばす）
 *
 *  返値　　：　見つかればtrue　ファイルの終わりならfalse　posは動かしません
 * -----------------------------------------------------------------------
 */
bool CYamlParser::PeekNext(CYamlMark &m) const
{
	spos_t q = pos;
	spos_t ls = line_start;
	size_t ln = line;
	bool at_head = (q == ls);
	bool tab = false;

	while ( q < n && IsBlankChar(s[q]) ) {
		if ( s[q] == L'\t' ) {
			tab = true;
		}
		++q;
	}
	if ( q < n && s[q] == L'#' ) {
		while ( q < n && s[q] != L'\n' ) {
			++q;
		}
	}
	if ( q < n && s[q] != L'\n' ) {
		m.pos = q;
		m.line_start = ls;
		m.line = ln;
		m.tab = at_head && tab;
		return true;
	}

	while ( q < n ) {
		// s[q]は改行
		++q;
		++ln;
		ls = q;
		tab = false;
		while ( q < n && IsBlankChar(s[q]) ) {
			if ( s[q] == L'\t' ) {
				tab = true;
			}
			++q;
		}
		if ( q < n && s[q] == L'#' ) {
			while ( q < n && s[q] != L'\n' ) {
				++q;
			}
			continue;
		}
		if ( q < n && s[q] != L'\n' ) {
			m.pos = q;
			m.line_start = ls;
			m.line = ln;
			m.tab = tab;
			return true;
		}
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::IsDocMarker / IsDocMarkerAt
 *  機能概要：  行頭の"---"か"..."（文書の区切り）ならtrue
 * -----------------------------------------------------------------------
 */
bool CYamlParser::IsDocMarker(const CYamlMark &m) const
{
	return IsDocMarkerAt(m.pos, m.line_start);
}

bool CYamlParser::IsDocMarkerAt(spos_t p, spos_t ls) const
{
	if ( p != ls ) {
		return false;
	}
	if ( s.compare(p, 3, L"---") != 0 && s.compare(p, 3, L"...") != 0 ) {
		return false;
	}
	return IsWsOrEnd(p + 3);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::IsSeqIndicator
 *  機能概要：  ブロック形式のシーケンスの"- "ならtrue
 * -----------------------------------------------------------------------
 */
bool CYamlParser::IsSeqIndicator(spos_t p) const
{
	return Ch(p) == L'-' && IsWsOrEnd(p + 1);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::IsPlainStartForbidden
 *  機能概要：  クォートされていないスカラーをこの文字から始められなければtrue
 * -----------------------------------------------------------------------
 */
bool CYamlParser::IsPlainStartForbidden(spos_t p, bool flow) const
{
	if ( p >= n ) {
		return true;
	}
	switch ( s[p] ) {
	case L',': case L'[': case L']': case L'{': case L'}':
	case L'#': case L'&': case L'*': case L'!': case L'|': case L'>':
	case L'\'': case L'"': case L'%': case L'@': case L'`':
		return true;
	case L'-': case L'?': case L':':
		return IsWsOrEnd(p + 1) || (flow && IsFlowIndicator(Ch(p + 1)));
	default:
		return false;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::IsImplicitKey
 *  機能概要：  pから始まる行が"key: "の形（ブロック形式のマッピングのキー）ならtrue
 *
 *  キーはクォートされた文字列か、クォートされていない1行の文字列だけに対応します
 * -----------------------------------------------------------------------
 */
bool CYamlParser::IsImplicitKey(spos_t p) const
{
	yaya::char_t c = Ch(p);

	if ( c == L'"' || c == L'\'' ) {
		spos_t q = p + 1;
		for ( ;; ) {
			if ( q >= n || s[q] == L'\n' ) {
				return false;
			}
			if ( c == L'\'' && s[q] == L'\'' ) {
				if ( Ch(q + 1) == L'\'' ) {
					q += 2;
					continue;
				}
				break;
			}
			if ( c == L'"' && s[q] == L'\\' ) {
				if ( Ch(q + 1) == L'\n' ) {
					return false;
				}
				q += 2;
				continue;
			}
			if ( c == L'"' && s[q] == L'"' ) {
				break;
			}
			++q;
		}
		++q;
		while ( q < n && IsBlankChar(s[q]) ) {
			++q;
		}
		return Ch(q) == L':' && IsWsOrEnd(q + 1);
	}

	if ( IsPlainStartForbidden(p, false) ) {
		return false;
	}
	for ( spos_t r = p; r < n && s[r] != L'\n'; ++r ) {
		if ( s[r] == L':' && IsWsOrEnd(r + 1) ) {
			return true;
		}
		if ( s[r] == L'#' && r > p && IsBlankChar(s[r - 1]) ) {
			return false;
		}
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseProperties
 *  機能概要：  ノードの前のアンカー（&name）とタグ（!tag）を読みます
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseProperties(yaya::string_t &anchor, bool &has_anchor, yaya::string_t &tag, bool &has_tag, bool flow)
{
	for ( ;; ) {
		yaya::char_t c = Ch(pos);
		if ( c == L'&' ) {
			if ( has_anchor ) {
				return Fail(L"duplicate anchor");
			}
			++pos;
			anchor = ReadName();
			if ( anchor.empty() ) {
				return Fail(L"empty anchor name");
			}
			has_anchor = true;
		}
		else if ( c == L'!' ) {
			if ( has_tag ) {
				return Fail(L"duplicate tag");
			}
			spos_t st = pos;
			while ( ! IsWsOrEnd(pos) && ! (flow && IsFlowIndicator(s[pos])) ) {
				++pos;
			}
			tag = s.substr(st, pos - st);
			has_tag = true;
		}
		else {
			break;
		}
		SkipBlanks();
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ReadName
 *  機能概要：  アンカー・エイリアスの名前を読みます
 * -----------------------------------------------------------------------
 */
yaya::string_t CYamlParser::ReadName(void)
{
	spos_t st = pos;
	while ( ! IsWsOrEnd(pos) && ! IsFlowIndicator(s[pos]) ) {
		++pos;
	}
	return s.substr(st, pos - st);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseAlias
 *  機能概要：  エイリアス（*name）を読み、アンカーの値を複製します
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseAlias(CValue &out)
{
	++pos;
	yaya::string_t name = ReadName();
	if ( name.empty() ) {
		return Fail(L"empty alias name");
	}

	std::map<yaya::string_t, CValue>::const_iterator it = anchors.find(name);
	if ( it == anchors.end() ) {
		return Fail(L"unknown alias : " + name);
	}

	alias_nodes += anchor_nodes[name];
	if ( alias_nodes > YAML_MAX_ALIAS_NODES ) {
		return Fail(L"too many alias expansions");
	}

	out = it->second;
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::RegisterAnchor
 *  機能概要：  アンカーの値を登録します（同じ名前は後勝ち）
 * -----------------------------------------------------------------------
 */
void CYamlParser::RegisterAnchor(const yaya::string_t &name, const CValue &v)
{
	anchors[name] = v;
	anchor_nodes[name] = CountNodes(v);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ApplyMerges
 *  機能概要：  マージキー（<<）の値をマッピングに加えます
 *
 *  明示したキーと、先に書いたマージ元が優先されます
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ApplyMerges(CValue &out, const std::vector<CValue> &merges)
{
	for ( size_t i = 0; i < merges.size(); ++i ) {
		const CValue &m = merges[i];
		if ( m.IsHash() ) {
			MergeHash(out, m);
		}
		else if ( m.IsArray() ) {
			const CValueArray &arr = m.array();
			for ( size_t j = 0; j < arr.size(); ++j ) {
				if ( ! arr[j].IsHash() ) {
					return Fail(L"merge key (<<) requires a mapping or a sequence of mappings");
				}
				MergeHash(out, arr[j]);
			}
		}
		else {
			return Fail(L"merge key (<<) requires a mapping or a sequence of mappings");
		}
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseNode
 *  機能概要：  ブロック形式の文脈でノードを1つ解析します
 *
 *  parent_indent : 親のマッピング・シーケンスの桁（最上位は-1）
 *  ctx           : YAML_CTX_*　"key:"の直後の同じ行にはマッピングやシーケンスを書けない
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseNode(int parent_indent, int ctx, CValue &out)
{
	if ( depth >= YAML_MAX_DEPTH ) {
		return Fail(L"nesting too deep");
	}
	++depth;
	bool result = ParseNodeBody(parent_indent, ctx, out);
	--depth;
	return result;
}

bool CYamlParser::ParseNodeBody(int parent_indent, int ctx, CValue &out)
{
	yaya::string_t anchor;
	yaya::string_t tag;
	bool has_anchor = false;
	bool has_tag = false;
	bool compact = (ctx != YAML_CTX_MAP_VALUE);

	SkipBlanks();
	if ( ! ParseProperties(anchor, has_anchor, tag, has_tag, false) ) {
		return false;
	}

	if ( AtLineEnd() ) {
		// 同じ行に何も無い：次の行が親より深ければそれがこのノード、そうでなければ空値
		CYamlMark m;
		bool take = false;
		if ( PeekNext(m) && ! IsDocMarker(m) ) {
			int c = MarkCol(m);
			if ( c > parent_indent ) {
				take = true;
			}
			else if ( c == parent_indent && ctx == YAML_CTX_MAP_VALUE && IsSeqIndicator(m.pos) ) {
				take = true; // key:の下に同じ桁で - を並べる書き方
			}
		}
		if ( ! take ) {
			out = YamlScalarValue(L"", true, tag, has_tag);
			if ( has_anchor ) {
				RegisterAnchor(anchor, out);
			}
			return true;
		}
		if ( m.tab ) {
			return FailAt(m.line, L"tab character used for indentation");
		}
		Seek(m);
		compact = true;

		if ( ! has_anchor && ! has_tag ) {
			if ( ! ParseProperties(anchor, has_anchor, tag, has_tag, false) ) {
				return false;
			}
			if ( (has_anchor || has_tag) && AtLineEnd() ) {
				return Fail(L"an anchor or tag must be followed by a node on the same line");
			}
		}
	}

	yaya::char_t c = Ch(pos);
	int col = Col();

	if ( c == L'*' ) {
		if ( has_anchor || has_tag ) {
			return Fail(L"an alias cannot have an anchor or tag");
		}
		if ( ! ParseAlias(out) ) {
			return false;
		}
		return FinishLine();
	}

	if ( c == L'|' || c == L'>' ) {
		yaya::string_t text;
		if ( ! ParseBlockScalar(parent_indent, text) ) {
			return false;
		}
		out = YamlScalarValue(text, false, tag, has_tag);
	}
	else if ( compact && IsSeqIndicator(pos) ) {
		if ( ! ParseBlockSeq(col, out) ) {
			return false;
		}
	}
	else if ( compact && IsImplicitKey(pos) ) {
		if ( ! ParseBlockMap(col, out) ) {
			return false;
		}
	}
	else if ( c == L'[' || c == L'{' ) {
		if ( ! ParseFlow(out) ) {
			return false;
		}
		if ( ! FinishLine() ) {
			return false;
		}
	}
	else if ( c == L'?' && IsWsOrEnd(pos + 1) ) {
		return Fail(L"complex mapping keys (?) are not supported");
	}
	else if ( c == L'"' || c == L'\'' ) {
		yaya::string_t text;
		if ( ! ParseQuoted(text) ) {
			return false;
		}
		if ( ! FinishLine() ) {
			return false;
		}
		out = YamlScalarValue(text, false, tag, has_tag);
	}
	else {
		if ( IsPlainStartForbidden(pos, false) ) {
			return Fail(yaya::string_t(L"unexpected character '") + c + L"'");
		}
		yaya::string_t text;
		ParsePlainBlock(parent_indent, text);
		if ( ! FinishLine() ) {
			return false;
		}
		out = YamlScalarValue(text, true, tag, has_tag);
	}

	if ( has_anchor ) {
		RegisterAnchor(anchor, out);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseBlockSeq
 *  機能概要：  ブロック形式のシーケンス（- item）を解析します　posは最初の"-"
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseBlockSeq(int indent, CValue &out)
{
	out = CValue(F_TAG_ARRAY, 0/*dmy*/);

	for ( ;; ) {
		++pos; // '-'

		CValue item;
		if ( ! ParseNode(indent, YAML_CTX_SEQ_ENTRY, item) ) {
			return false;
		}
		out.array().push_back(item);

		CYamlMark m;
		if ( ! PeekNext(m) || IsDocMarker(m) ) {
			break;
		}
		int c = MarkCol(m);
		if ( c < indent ) {
			break;
		}
		if ( c > indent ) {
			return FailAt(m.line, L"bad indentation of a sequence entry");
		}
		if ( ! IsSeqIndicator(m.pos) ) {
			break;
		}
		if ( m.tab ) {
			return FailAt(m.line, L"tab character used for indentation");
		}
		Seek(m);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseBlockMap
 *  機能概要：  ブロック形式のマッピング（key: value）を解析します　posは最初のキー
 *
 *  同じキーは後勝ち。キーは常に文字列にします
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseBlockMap(int indent, CValue &out)
{
	out = CValue(F_TAG_HASH, 0/*dmy*/);
	std::vector<CValue> merges;

	for ( ;; ) {
		yaya::string_t key;
		bool is_merge = false;
		if ( ! ParseMapKey(key, is_merge) ) {
			return false;
		}

		CValue value;
		if ( ! ParseNode(indent, YAML_CTX_MAP_VALUE, value) ) {
			return false;
		}
		if ( is_merge ) {
			merges.push_back(value);
		}
		else {
			out.hash()[CValue(key)] = value;
		}

		CYamlMark m;
		if ( ! PeekNext(m) || IsDocMarker(m) ) {
			break;
		}
		int c = MarkCol(m);
		if ( c < indent ) {
			break;
		}
		if ( c > indent ) {
			return FailAt(m.line, L"bad indentation of a mapping entry");
		}
		if ( m.tab ) {
			return FailAt(m.line, L"tab character used for indentation");
		}
		Seek(m);
		if ( Ch(pos) == L'?' && IsWsOrEnd(pos + 1) ) {
			return Fail(L"complex mapping keys (?) are not supported");
		}
		if ( ! IsImplicitKey(pos) ) {
			return Fail(L"expected a mapping key (key: value)");
		}
	}

	return ApplyMerges(out, merges);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseMapKey
 *  機能概要：  ブロック形式のマッピングのキーと":"を読みます
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseMapKey(yaya::string_t &key, bool &is_merge)
{
	is_merge = false;

	yaya::char_t c = Ch(pos);
	if ( c == L'"' || c == L'\'' ) {
		if ( ! ParseQuoted(key) ) {
			return false;
		}
		SkipBlanks();
	}
	else {
		spos_t st = pos;
		while ( pos < n && s[pos] != L'\n' && ! (s[pos] == L':' && IsWsOrEnd(pos + 1)) ) {
			++pos;
		}
		spos_t e = pos;
		while ( e > st && IsBlankChar(s[e - 1]) ) {
			--e;
		}
		key = s.substr(st, e - st);
		is_merge = (key == L"<<");
	}

	if ( Ch(pos) != L':' ) {
		return Fail(L"expected ':' after a mapping key");
	}
	++pos;
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ScanPlainLine
 *  機能概要：  クォートされていないスカラーの1行分を探します
 *
 *  ": "・" #"・行末で止まり、posをその位置にします
 *  返値　　：　末尾の空白を除いた終わりの位置
 * -----------------------------------------------------------------------
 */
spos_t CYamlParser::ScanPlainLine(spos_t start)
{
	spos_t r = start;
	while ( r < n ) {
		yaya::char_t c = s[r];
		if ( c == L'\n' ) {
			break;
		}
		if ( c == L':' && IsWsOrEnd(r + 1) ) {
			break;
		}
		if ( c == L'#' && r > start && IsBlankChar(s[r - 1]) ) {
			break;
		}
		++r;
	}
	pos = r;
	while ( r > start && IsBlankChar(s[r - 1]) ) {
		--r;
	}
	return r;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParsePlainBlock
 *  機能概要：  ブロック形式の文脈で、クォートされていないスカラーを解析します
 *
 *  親より深く字下げされた次の行は続きとみなし、改行1つは空白、空行はその数だけ改行にします
 * -----------------------------------------------------------------------
 */
void CYamlParser::ParsePlainBlock(int parent_indent, yaya::string_t &text)
{
	spos_t st = pos;
	spos_t e = ScanPlainLine(st);
	text = s.substr(st, e - st);

	for ( ;; ) {
		if ( Ch(pos) != L'\n' ) {
			break; // コメント・": "・ファイルの終わり
		}

		spos_t q = pos;
		size_t ln = line;
		spos_t ls = line_start;
		size_t breaks = 0;
		spos_t r = q;
		for ( ;; ) {
			++q;
			++ln;
			ls = q;
			r = q;
			while ( r < n && IsBlankChar(s[r]) ) {
				++r;
			}
			if ( r < n && s[r] == L'\n' ) {
				++breaks;
				q = r;
				continue;
			}
			break;
		}

		if ( r >= n ) {
			break;
		}
		if ( static_cast<int>(r - ls) <= parent_indent ) {
			break;
		}
		if ( s[r] == L'#' ) {
			break;
		}
		if ( IsDocMarkerAt(r, ls) ) {
			break;
		}

		pos = r;
		line = ln;
		line_start = ls;

		spos_t seg_start = pos;
		spos_t seg_end = ScanPlainLine(seg_start);

		if ( breaks == 0 ) {
			text += L' ';
		}
		else {
			text.append(breaks, L'\n');
		}
		text += s.substr(seg_start, seg_end - seg_start);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseBlockScalar
 *  機能概要：  ブロックスカラー（| >）を解析します
 *
 *  字下げは明示（|2など）されていなければ最初の空でない行で決めます
 *  末尾の改行は - で除き、+ で全て残し、指定がなければ1つだけ残します
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseBlockScalar(int parent_indent, yaya::string_t &text)
{
	bool folded = (s[pos] == L'>');
	++pos;

	int chomp = 0; // -1:strip 0:clip 1:keep
	int explicit_indent = 0;
	for ( int k = 0; k < 2; ++k ) {
		yaya::char_t c = Ch(pos);
		if ( (c == L'+' || c == L'-') && chomp == 0 ) {
			chomp = (c == L'+') ? 1 : -1;
			++pos;
		}
		else if ( c >= L'1' && c <= L'9' && explicit_indent == 0 ) {
			explicit_indent = c - L'0';
			++pos;
		}
		else {
			break;
		}
	}
	if ( ! IsWsOrEnd(pos) ) {
		return Fail(L"invalid block scalar header");
	}
	if ( ! FinishLine() ) {
		return false;
	}

	int indent = -1;
	if ( explicit_indent > 0 ) {
		indent = (parent_indent < 0 ? 0 : parent_indent) + explicit_indent;
	}

	// 行を集める（空行は空文字列）
	std::vector<yaya::string_t> lines;
	bool last_break = false;

	while ( pos < n ) {
		// s[pos]は改行
		spos_t ls = pos + 1;
		if ( ls >= n ) {
			break;
		}
		spos_t r = ls;
		while ( r < n && s[r] == L' ' ) {
			++r;
		}
		spos_t e = r;
		while ( e < n && s[e] != L'\n' ) {
			++e;
		}
		int sp = static_cast<int>(r - ls);
		bool blank = (r == e);

		if ( indent < 0 && ! blank ) {
			if ( sp <= parent_indent ) {
				break;
			}
			indent = sp;
		}
		if ( ! blank && sp < indent ) {
			break;
		}
		if ( indent == 0 && IsDocMarkerAt(ls, ls) ) {
			break;
		}

		if ( blank && (indent < 0 || sp <= indent) ) {
			lines.push_back(yaya::string_t());
		}
		else {
			lines.push_back(s.substr(ls + indent, e - ls - indent));
			last_break = (e < n);
		}

		pos = e;
		line_start = ls;
		++line;
	}

	text.erase();
	size_t breaks = 0;
	bool first = true;
	bool prev_more = false;
	for ( size_t li = 0; li < lines.size(); ++li ) {
		const yaya::string_t &ln = lines[li];
		if ( ln.empty() ) {
			++breaks;
			continue;
		}
		bool more = folded && (ln[0] == L' ' || ln[0] == L'\t');
		if ( first ) {
			text.append(breaks, L'\n');
		}
		else if ( ! folded || prev_more || more ) {
			text.append(breaks + 1, L'\n');
		}
		else if ( breaks == 0 ) {
			text += L' ';
		}
		else {
			text.append(breaks, L'\n');
		}
		text += ln;
		first = false;
		prev_more = more;
		breaks = 0;
	}

	if ( first ) {
		if ( chomp > 0 ) {
			text.append(breaks, L'\n');
		}
	}
	else {
		if ( chomp >= 0 && last_break ) {
			text += L'\n';
		}
		if ( chomp > 0 ) {
			text.append(breaks, L'\n');
		}
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseQuoted
 *  機能概要：  クォートされたスカラー（'...' "..."）を解析します　複数行にわたってもよい
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseQuoted(yaya::string_t &text)
{
	yaya::char_t q = s[pos];
	size_t start_line = line;
	++pos;

	text.erase();
	spos_t keep = 0; // これより前はエスケープで作った文字なので、折り返しで空白を削らない

	for ( ;; ) {
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated quoted scalar");
		}
		yaya::char_t c = s[pos];

		if ( q == L'\'' && c == L'\'' ) {
			if ( Ch(pos + 1) == L'\'' ) {
				text += L'\'';
				pos += 2;
				continue;
			}
			++pos;
			break;
		}
		if ( q == L'"' && c == L'"' ) {
			++pos;
			break;
		}
		if ( c == L'\n' ) {
			spos_t e = text.size();
			while ( e > keep && IsBlankChar(text[e - 1]) ) {
				--e;
			}
			text.erase(e);

			size_t breaks = 0;
			NewLine();
			SkipBlanks();
			while ( Ch(pos) == L'\n' ) {
				++breaks;
				NewLine();
				SkipBlanks();
			}
			if ( breaks == 0 ) {
				text += L' ';
			}
			else {
				text.append(breaks, L'\n');
			}
			continue;
		}
		if ( q == L'"' && c == L'\\' ) {
			if ( Ch(pos + 1) == L'\n' ) {
				// 行末の\は改行を消す
				++pos;
				NewLine();
				SkipBlanks();
				keep = text.size();
				continue;
			}
			if ( ! ParseDoubleEscape(text) ) {
				return false;
			}
			keep = text.size();
			continue;
		}

		text += c;
		++pos;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseDoubleEscape
 *  機能概要：  "..."の中のエスケープを1つ解析します　posは\の位置
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseDoubleEscape(yaya::string_t &text)
{
	yaya::char_t e = Ch(pos + 1);
	pos += 2;

	int digits = 0;
	switch ( e ) {
	case L'0':  text += static_cast<yaya::char_t>(0); break;
	case L'a':  text += static_cast<yaya::char_t>(0x07); break;
	case L'b':  text += static_cast<yaya::char_t>(0x08); break;
	case L't':
	case L'\t': text += L'\t'; break;
	case L'n':  text += L'\n'; break;
	case L'v':  text += static_cast<yaya::char_t>(0x0B); break;
	case L'f':  text += static_cast<yaya::char_t>(0x0C); break;
	case L'r':  text += L'\r'; break;
	case L'e':  text += static_cast<yaya::char_t>(0x1B); break;
	case L' ':  text += L' '; break;
	case L'"':  text += L'"'; break;
	case L'/':  text += L'/'; break;
	case L'\\': text += L'\\'; break;
	case L'N':  text += static_cast<yaya::char_t>(0x85); break;
	case L'_':  text += static_cast<yaya::char_t>(0xA0); break;
	case L'L':  text += static_cast<yaya::char_t>(0x2028); break;
	case L'P':  text += static_cast<yaya::char_t>(0x2029); break;
	case L'x':  digits = 2; break;
	case L'u':  digits = 4; break;
	case L'U':  digits = 8; break;
	default:
		return Fail(L"invalid escape sequence");
	}
	if ( digits == 0 ) {
		return true;
	}

	unsigned long cp;
	if ( ! ReadHexDigits(s, pos, digits, cp) ) {
		return Fail(L"invalid escape sequence");
	}
	// \u で書いたサロゲートペア（上位・下位の2つ続き）を1文字にまとめる
	if ( digits == 4 && cp >= 0xD800 && cp <= 0xDBFF && Ch(pos) == L'\\' && Ch(pos + 1) == L'u' ) {
		spos_t q = pos + 2;
		unsigned long lo;
		if ( ReadHexDigits(s, q, 4, lo) && lo >= 0xDC00 && lo <= 0xDFFF ) {
			cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
			pos = q;
		}
	}
	if ( ! AppendCodePoint(text, cp) ) {
		return Fail(L"invalid unicode escape");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::SkipFlowSpace
 *  機能概要：  フロー形式の中の空白・改行・コメントを飛ばします
 * -----------------------------------------------------------------------
 */
void CYamlParser::SkipFlowSpace(void)
{
	while ( pos < n ) {
		yaya::char_t c = s[pos];
		if ( IsBlankChar(c) ) {
			++pos;
		}
		else if ( c == L'\n' ) {
			NewLine();
		}
		else if ( c == L'#' ) {
			while ( pos < n && s[pos] != L'\n' ) {
				++pos;
			}
		}
		else {
			break;
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseFlow
 *  機能概要：  フロー形式のシーケンス（[...]）・マッピング（{...}）を解析します
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseFlow(CValue &out)
{
	if ( depth >= YAML_MAX_DEPTH ) {
		return Fail(L"nesting too deep");
	}
	++depth;
	bool result = ParseFlowBody(out);
	--depth;
	return result;
}

bool CYamlParser::ParseFlowBody(CValue &out)
{
	size_t start_line = line;
	bool is_map = (s[pos] == L'{');
	yaya::char_t close = is_map ? L'}' : L']';
	++pos;

	out = is_map ? CValue(F_TAG_HASH, 0/*dmy*/) : CValue(F_TAG_ARRAY, 0/*dmy*/);
	std::vector<CValue> merges;

	for ( ;; ) {
		SkipFlowSpace();
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated flow collection");
		}
		if ( s[pos] == close ) {
			++pos;
			break;
		}

		if ( is_map ) {
			if ( s[pos] == L'?' && IsWsOrEnd(pos + 1) ) {
				++pos;
				SkipFlowSpace();
			}
			yaya::string_t key;
			bool key_plain = false;
			if ( ! ParseFlowKey(key, key_plain) ) {
				return false;
			}
			SkipFlowSpace();

			CValue value;
			if ( Ch(pos) == L':' ) {
				++pos;
				SkipFlowSpace();
				if ( Ch(pos) != L',' && Ch(pos) != close ) {
					if ( ! ParseFlowNode(value) ) {
						return false;
					}
				}
			}
			if ( key_plain && key == L"<<" ) {
				merges.push_back(value);
			}
			else {
				out.hash()[CValue(key)] = value;
			}
		}
		else {
			CValue item;
			if ( ! ParseFlowNode(item) ) {
				return false;
			}
			SkipFlowSpace();
			if ( Ch(pos) == L':' ) {
				// [a: 1] のような1組だけのマッピング
				if ( item.IsArray() || item.IsHash() ) {
					return Fail(L"complex mapping keys are not supported");
				}
				++pos;
				SkipFlowSpace();
				CValue value;
				if ( Ch(pos) != L',' && Ch(pos) != close ) {
					if ( ! ParseFlowNode(value) ) {
						return false;
					}
				}
				CValue pair(F_TAG_HASH, 0/*dmy*/);
				pair.hash()[CValue(item.GetValueString())] = value;
				item = pair;
			}
			out.array().push_back(item);
		}

		SkipFlowSpace();
		if ( Ch(pos) == L',' ) {
			++pos;
			continue;
		}
		if ( Ch(pos) == close ) {
			++pos;
			break;
		}
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated flow collection");
		}
		return Fail(yaya::string_t(L"expected ',' or '") + close + L"'");
	}

	if ( is_map ) {
		return ApplyMerges(out, merges);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseFlowKey
 *  機能概要：  フロー形式のマッピングのキーを読みます
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseFlowKey(yaya::string_t &key, bool &plain)
{
	plain = false;

	yaya::string_t anchor;
	yaya::string_t tag;
	bool has_anchor = false;
	bool has_tag = false;
	if ( ! ParseProperties(anchor, has_anchor, tag, has_tag, true) ) {
		return false;
	}

	yaya::char_t c = Ch(pos);
	if ( c == L'"' || c == L'\'' ) {
		if ( ! ParseQuoted(key) ) {
			return false;
		}
	}
	else if ( c == L'[' || c == L'{' ) {
		return Fail(L"complex mapping keys are not supported");
	}
	else if ( c == L'*' ) {
		CValue v;
		if ( ! ParseAlias(v) ) {
			return false;
		}
		if ( v.IsArray() || v.IsHash() ) {
			return Fail(L"complex mapping keys are not supported");
		}
		key = v.GetValueString();
	}
	else if ( c == L':' || c == L',' || c == L'}' ) {
		key.erase();
		plain = true;
	}
	else {
		if ( IsPlainStartForbidden(pos, true) ) {
			return Fail(yaya::string_t(L"unexpected character '") + c + L"'");
		}
		ScanFlowPlain(key);
		plain = true;
	}

	if ( has_anchor ) {
		RegisterAnchor(anchor, CValue(key));
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ParseFlowNode
 *  機能概要：  フロー形式の中のノードを1つ解析します
 * -----------------------------------------------------------------------
 */
bool CYamlParser::ParseFlowNode(CValue &out)
{
	yaya::string_t anchor;
	yaya::string_t tag;
	bool has_anchor = false;
	bool has_tag = false;
	if ( ! ParseProperties(anchor, has_anchor, tag, has_tag, true) ) {
		return false;
	}
	SkipFlowSpace();

	yaya::char_t c = Ch(pos);
	if ( c == L'*' ) {
		if ( has_anchor || has_tag ) {
			return Fail(L"an alias cannot have an anchor or tag");
		}
		return ParseAlias(out);
	}

	if ( c == L'[' || c == L'{' ) {
		if ( ! ParseFlow(out) ) {
			return false;
		}
	}
	else if ( c == L'"' || c == L'\'' ) {
		yaya::string_t text;
		if ( ! ParseQuoted(text) ) {
			return false;
		}
		out = YamlScalarValue(text, false, tag, has_tag);
	}
	else if ( pos >= n || c == L',' || c == L']' || c == L'}' || c == L':' ) {
		out = YamlScalarValue(L"", true, tag, has_tag);
	}
	else {
		if ( IsPlainStartForbidden(pos, true) ) {
			return Fail(yaya::string_t(L"unexpected character '") + c + L"'");
		}
		yaya::string_t text;
		ScanFlowPlain(text);
		out = YamlScalarValue(text, true, tag, has_tag);
	}

	if ( has_anchor ) {
		RegisterAnchor(anchor, out);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::ScanFlowPlain
 *  機能概要：  フロー形式の中の、クォートされていないスカラーを読みます
 *
 *  , [ ] { } と ": " " #" で止まります。次の行に続く場合は改行を空白にします
 * -----------------------------------------------------------------------
 */
void CYamlParser::ScanFlowPlain(yaya::string_t &text)
{
	text.erase();

	for ( ;; ) {
		spos_t st = pos;
		spos_t r = pos;
		while ( r < n ) {
			yaya::char_t c = s[r];
			if ( c == L'\n' || IsFlowIndicator(c) ) {
				break;
			}
			if ( c == L':' && (IsWsOrEnd(r + 1) || IsFlowIndicator(Ch(r + 1))) ) {
				break;
			}
			if ( c == L'#' && r > st && IsBlankChar(s[r - 1]) ) {
				break;
			}
			++r;
		}
		spos_t e = r;
		while ( e > st && IsBlankChar(s[e - 1]) ) {
			--e;
		}
		text += s.substr(st, e - st);
		pos = r;

		if ( Ch(pos) != L'\n' ) {
			break;
		}

		// 次の行に続くか
		spos_t q = pos;
		size_t ln = line;
		spos_t ls = line_start;
		size_t breaks = 0;
		spos_t r2 = q;
		for ( ;; ) {
			++q;
			++ln;
			ls = q;
			r2 = q;
			while ( r2 < n && IsBlankChar(s[r2]) ) {
				++r2;
			}
			if ( r2 < n && s[r2] == L'\n' ) {
				++breaks;
				q = r2;
				continue;
			}
			break;
		}
		if ( r2 >= n ) {
			break;
		}
		yaya::char_t c2 = s[r2];
		if ( IsFlowIndicator(c2) || c2 == L'#' || (c2 == L':' && (IsWsOrEnd(r2 + 1) || IsFlowIndicator(Ch(r2 + 1)))) ) {
			break;
		}

		pos = r2;
		line = ln;
		line_start = ls;
		if ( breaks == 0 ) {
			text += L' ';
		}
		else {
			text.append(breaks, L'\n');
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CYamlParser::Parse
 *  機能概要：  文書全体を解析します
 *
 *  先頭の"%"で始まるディレクティブと"---"、末尾の"..."は許します
 *  2つ目の文書があればエラーにします
 * -----------------------------------------------------------------------
 */
bool CYamlParser::Parse(CValue &out)
{
	out = CValue();

	CYamlMark m;
	if ( ! PeekNext(m) ) {
		return true; // 空の文書
	}

	bool directive = false;
	while ( m.pos == m.line_start && s[m.pos] == L'%' ) {
		directive = true;
		Seek(m);
		while ( pos < n && s[pos] != L'\n' ) {
			++pos;
		}
		if ( ! PeekNext(m) ) {
			return Fail(L"'---' is required after directives");
		}
	}

	if ( IsDocMarker(m) && s[m.pos] == L'-' ) {
		Seek(m);
		pos += 3;
		if ( ! ParseNode(-1, YAML_CTX_ROOT, out) ) {
			return false;
		}
	}
	else if ( directive ) {
		return FailAt(m.line, L"'---' is required after directives");
	}
	else if ( ! IsDocMarker(m) ) {
		if ( m.tab ) {
			return FailAt(m.line, L"tab character used for indentation");
		}
		Seek(m);
		if ( ! ParseNode(-1, YAML_CTX_ROOT, out) ) {
			return false;
		}
	}

	if ( ! PeekNext(m) ) {
		return true;
	}
	if ( IsDocMarker(m) && s[m.pos] == L'.' ) {
		Seek(m);
		pos += 3;
		if ( ! FinishLine() ) {
			return false;
		}
		if ( ! PeekNext(m) ) {
			return true;
		}
		return FailAt(m.line, L"multiple documents are not supported");
	}
	if ( IsDocMarker(m) ) {
		return FailAt(m.line, L"multiple documents are not supported");
	}
	return FailAt(m.line, L"unexpected content (bad indentation?)");
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlToValue
 *  機能概要：  UTF-8のYAMLを解析してCValueにします
 *
 *  返値　　：　成功時true　失敗時はerrstrに行番号と内容を入れます
 * -----------------------------------------------------------------------
 */
bool YamlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr)
{
	CNumericLocaleGuard locale_guard;

	yaya::string_t text = PrepareText(utf8);
	CYamlParser parser(text);
	if ( ! parser.Parse(out) ) {
		errstr = parser.errstr;
		out = CValue();
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// TOMLの解析
//////////////////////////////////////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlCollectDigits
 *  機能概要：  t[from, to)が「数字(_数字)*」ならtrueで、_を除いた数字をdigitsに入れます
 * -----------------------------------------------------------------------
 */
static bool TomlCollectDigits(const yaya::string_t &t, spos_t from, spos_t to, int base, yaya::string_t &digits)
{
	digits.erase();
	if ( from >= to ) {
		return false;
	}
	for ( spos_t k = from; k < to; ++k ) {
		yaya::char_t c = t[k];
		if ( c == L'_' ) {
			if ( k == from || k + 1 >= to || t[k + 1] == L'_' ) {
				return false;
			}
			continue;
		}
		int d = HexDigitValue(c);
		if ( d < 0 || d >= base ) {
			return false;
		}
		digits += c;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlTextToNumber
 *  機能概要：  TOMLの整数・実数の表記を値にします
 *
 *  返値　　：　0:成功 1:不正な表記 2:整数の範囲外
 * -----------------------------------------------------------------------
 */
static int TomlTextToNumber(const yaya::string_t &t, CValue &out)
{
	spos_t n = t.size();
	spos_t i = 0;
	bool negative = false;
	bool has_sign = false;

	if ( n == 0 ) {
		return 1;
	}
	if ( t[0] == L'+' || t[0] == L'-' ) {
		negative = (t[0] == L'-');
		has_sign = true;
		i = 1;
	}

	yaya::string_t rest = t.substr(i);
	if ( rest == L"inf" ) {
		out = CValue(MakeInfinity(negative));
		return 0;
	}
	if ( rest == L"nan" ) {
		out = CValue(MakeNan());
		return 0;
	}

	// 0x 0o 0b（符号は付けられない）
	if ( ! has_sign && n > 2 && t[0] == L'0' && (t[1] == L'x' || t[1] == L'o' || t[1] == L'b') ) {
		int base = (t[1] == L'x') ? 16 : ((t[1] == L'o') ? 8 : 2);
		yaya::string_t digits;
		if ( ! TomlCollectDigits(t, 2, n, base, digits) ) {
			return 1;
		}
		yaya::int_t v;
		if ( ! DigitsToInt(digits, base, false, v) ) {
			return 2;
		}
		out = CValue(v);
		return 0;
	}

	// 10進：整数部
	spos_t p = i;
	while ( p < n && (IsDecDigit(t[p]) || t[p] == L'_') ) {
		++p;
	}
	yaya::string_t int_digits;
	if ( ! TomlCollectDigits(t, i, p, 10, int_digits) ) {
		return 1;
	}
	if ( int_digits.size() > 1 && int_digits[0] == L'0' ) {
		return 1; // 先頭の0は不可
	}

	// 小数部・指数部
	bool is_float = false;
	yaya::string_t work;
	if ( p < n && t[p] == L'.' ) {
		++p;
		spos_t frac_start = p;
		while ( p < n && (IsDecDigit(t[p]) || t[p] == L'_') ) {
			++p;
		}
		if ( ! TomlCollectDigits(t, frac_start, p, 10, work) ) {
			return 1;
		}
		is_float = true;
	}
	if ( p < n && (t[p] == L'e' || t[p] == L'E') ) {
		++p;
		if ( p < n && (t[p] == L'+' || t[p] == L'-') ) {
			++p;
		}
		spos_t exp_start = p;
		while ( p < n && (IsDecDigit(t[p]) || t[p] == L'_') ) {
			++p;
		}
		if ( ! TomlCollectDigits(t, exp_start, p, 10, work) ) {
			return 1;
		}
		is_float = true;
	}
	if ( p != n ) {
		return 1;
	}

	if ( ! is_float ) {
		yaya::int_t v;
		if ( ! DigitsToInt(int_digits, 10, negative, v) ) {
			return 2;
		}
		out = CValue(v);
		return 0;
	}

	yaya::string_t plain;
	for ( spos_t k = 0; k < n; ++k ) {
		if ( t[k] != L'_' ) {
			plain += t[k];
		}
	}
	double d;
	if ( ! TextToDouble(plain, d) ) {
		return 1;
	}
	out = CValue(d);
	return 0;
}

static bool IsTomlBareKeyChar(yaya::char_t c)
{
	return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || IsDecDigit(c) || c == L'_' || c == L'-';
}

static bool IsTomlControl(yaya::char_t c)
{
	return (c < 0x20 && c != L'\t') || c == 0x7F;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlChildPath / TomlElementPath / TomlKeysText
 *  機能概要：  表の定義を管理するための内部のパス、エラー表示用のキー
 * -----------------------------------------------------------------------
 */
static yaya::string_t TomlChildPath(const yaya::string_t &path, const yaya::string_t &key)
{
	return path + static_cast<yaya::char_t>(1) + key;
}

static yaya::string_t TomlElementPath(const yaya::string_t &path, size_t index)
{
	return path + static_cast<yaya::char_t>(2) + yaya::ws_lltoa(static_cast<yaya::int_t>(index));
}

static yaya::string_t TomlKeysText(const std::vector<yaya::string_t> &keys)
{
	yaya::string_t result;
	for ( size_t i = 0; i < keys.size(); ++i ) {
		if ( i > 0 ) {
			result += L'.';
		}
		result += keys[i];
	}
	return result;
}

/* -----------------------------------------------------------------------
 *  クラス名：  CTomlParser
 *  機能概要：  TOMLの解析
 *
 *  表の再定義などの禁止事項を調べるため、表ごとの定義のされ方を内部のパスで記録します
 *  （表の配列の要素はパスに添字を含めて区別する）
 * -----------------------------------------------------------------------
 */
class CTomlParser
{
private:
	const yaya::string_t &s;
	spos_t	n;
	spos_t	pos;
	size_t	line;
	int		depth;

	CValue	root;
	CValue	*current;
	yaya::string_t	current_path;

	std::set<yaya::string_t> explicit_tables;	// [a.b]で定義した表
	std::set<yaya::string_t> dotted_tables;		// ドット区切りのキーで作った表
	std::set<yaya::string_t> inline_tables;		// インライン表（後から足せない）
	std::set<yaya::string_t> table_arrays;		// [[a.b]]で作った表の配列

public:
	yaya::string_t errstr;

	CTomlParser(const yaya::string_t &src) :
		s(src), n(src.size()), pos(0), line(1), depth(0), root(F_TAG_HASH, 0/*dmy*/), current(NULL) { }

	bool	Parse(CValue &out);

private:
	yaya::char_t Ch(spos_t p) const
	{
		return p < n ? s[p] : 0;
	}
	bool IsDigitAt(spos_t p) const
	{
		return p < n && IsDecDigit(s[p]);
	}
	void SkipBlanks(void)
	{
		while ( pos < n && IsBlankChar(s[pos]) ) {
			++pos;
		}
	}
	void SkipComment(void)
	{
		while ( pos < n && s[pos] != L'\n' ) {
			++pos;
		}
	}

	bool Fail(const yaya::string_t &msg)
	{
		return FailAt(line, msg);
	}
	bool FailAt(size_t ln, const yaya::string_t &msg)
	{
		if ( errstr.empty() ) {
			errstr = L"line " + yaya::ws_lltoa(static_cast<yaya::int_t>(ln)) + L" : " + msg;
		}
		return false;
	}

	void	SkipSpaceAndComments(void);
	bool	FinishLine(void);
	bool	ParseKey(std::vector<yaya::string_t> &keys);
	bool	ParseTableHeader(bool is_array);
	bool	ParseKeyValue(CValue &table, const yaya::string_t &table_path, bool track);
	bool	ParseValue(CValue &out, bool &is_inline);
	bool	ParseNumber(CValue &out);
	bool	IsDateTimeStart(void) const;
	bool	ScanTime(void);
	bool	ParseDateTime(CValue &out);
	bool	ParseEscape(yaya::string_t &out);
	bool	ParseBasicString(yaya::string_t &out);
	bool	ParseMultiBasicString(yaya::string_t &out);
	bool	ParseLiteralString(yaya::string_t &out);
	bool	ParseMultiLiteralString(yaya::string_t &out);
	bool	ParseArray(CValue &out);
	bool	ParseArrayBody(CValue &out);
	bool	ParseInlineTable(CValue &out);
	bool	ParseInlineTableBody(CValue &out);
};

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::SkipSpaceAndComments
 *  機能概要：  配列・インライン表の中の空白・改行・コメントを飛ばします
 * -----------------------------------------------------------------------
 */
void CTomlParser::SkipSpaceAndComments(void)
{
	while ( pos < n ) {
		yaya::char_t c = s[pos];
		if ( IsBlankChar(c) ) {
			++pos;
		}
		else if ( c == L'\n' ) {
			++pos;
			++line;
		}
		else if ( c == L'#' ) {
			SkipComment();
		}
		else {
			break;
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::FinishLine
 *  機能概要：  行の残りが空白とコメントだけであることを確かめます
 * -----------------------------------------------------------------------
 */
bool CTomlParser::FinishLine(void)
{
	SkipBlanks();
	if ( pos < n && s[pos] == L'#' ) {
		SkipComment();
	}
	if ( pos < n && s[pos] != L'\n' ) {
		return Fail(L"expected the end of the line");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseKey
 *  機能概要：  キー（ドット区切りを含む）を読みます
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseKey(std::vector<yaya::string_t> &keys)
{
	keys.clear();
	for ( ;; ) {
		SkipBlanks();
		yaya::string_t k;
		yaya::char_t c = Ch(pos);
		if ( c == L'"' || c == L'\'' ) {
			if ( Ch(pos + 1) == c && Ch(pos + 2) == c ) {
				return Fail(L"multi-line strings cannot be used as keys");
			}
			if ( c == L'"' ) {
				if ( ! ParseBasicString(k) ) {
					return false;
				}
			}
			else {
				if ( ! ParseLiteralString(k) ) {
					return false;
				}
			}
		}
		else {
			spos_t st = pos;
			while ( pos < n && IsTomlBareKeyChar(s[pos]) ) {
				++pos;
			}
			if ( pos == st ) {
				return Fail(L"invalid key");
			}
			k = s.substr(st, pos - st);
		}
		keys.push_back(k);

		SkipBlanks();
		if ( Ch(pos) != L'.' ) {
			break;
		}
		++pos;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseTableHeader
 *  機能概要：  [table] / [[array of tables]] の見出しを解析し、以降のキーの入れ先を決めます
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseTableHeader(bool is_array)
{
	std::vector<yaya::string_t> keys;
	if ( ! ParseKey(keys) ) {
		return false;
	}
	SkipBlanks();
	if ( is_array ) {
		if ( Ch(pos) != L']' || Ch(pos + 1) != L']' ) {
			return Fail(L"expected ']]'");
		}
		pos += 2;
	}
	else {
		if ( Ch(pos) != L']' ) {
			return Fail(L"expected ']'");
		}
		++pos;
	}

	CValue *t = &root;
	yaya::string_t path;
	for ( size_t i = 0; i < keys.size(); ++i ) {
		bool last = (i + 1 == keys.size());
		path = TomlChildPath(path, keys[i]);

		CValueHash &h = t->hash();
		CValue key(keys[i]);
		CValueHash::iterator it = h.find(key);

		if ( it == h.end() ) {
			CValue &nv = h[key];
			if ( last && is_array ) {
				nv = CValue(F_TAG_ARRAY, 0/*dmy*/);
				table_arrays.insert(path);
				nv.array().push_back(CValue(F_TAG_HASH, 0/*dmy*/));
				path = TomlElementPath(path, 0);
				t = &nv.array().back();
			}
			else {
				nv = CValue(F_TAG_HASH, 0/*dmy*/);
				t = &nv;
			}
			continue;
		}

		CValue &v = it->second;
		if ( v.IsHash() ) {
			if ( inline_tables.count(path) ) {
				return Fail(L"cannot extend an inline table : " + TomlKeysText(keys));
			}
			if ( last ) {
				if ( is_array ) {
					return Fail(L"already defined as a table : " + TomlKeysText(keys));
				}
				if ( explicit_tables.count(path) ) {
					return Fail(L"duplicate table : " + TomlKeysText(keys));
				}
				if ( dotted_tables.count(path) ) {
					return Fail(L"table already defined by dotted keys : " + TomlKeysText(keys));
				}
			}
			t = &v;
		}
		else if ( v.IsArray() && table_arrays.count(path) ) {
			if ( last && ! is_array ) {
				return Fail(L"already defined as an array of tables : " + TomlKeysText(keys));
			}
			CValueArray &arr = v.array();
			if ( last ) {
				arr.push_back(CValue(F_TAG_HASH, 0/*dmy*/));
			}
			path = TomlElementPath(path, arr.size() - 1);
			t = &arr.back();
		}
		else {
			return Fail(L"key already defined as a value : " + TomlKeysText(keys));
		}
	}

	if ( ! is_array ) {
		explicit_tables.insert(path);
	}
	current = t;
	current_path = path;
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseKeyValue
 *  機能概要：  key = value を解析してtableに入れます
 *
 *  trackがtrueなら（インライン表の中でなければ）表の定義のされ方を記録・検査します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseKeyValue(CValue &table, const yaya::string_t &table_path, bool track)
{
	std::vector<yaya::string_t> keys;
	if ( ! ParseKey(keys) ) {
		return false;
	}
	SkipBlanks();
	if ( Ch(pos) != L'=' ) {
		return Fail(L"expected '=' after a key");
	}
	++pos;
	SkipBlanks();

	CValue value;
	bool is_inline = false;
	if ( ! ParseValue(value, is_inline) ) {
		return false;
	}

	CValue *t = &table;
	yaya::string_t path = table_path;
	for ( size_t i = 0; i + 1 < keys.size(); ++i ) {
		path = TomlChildPath(path, keys[i]);

		CValueHash &h = t->hash();
		CValue key(keys[i]);
		CValueHash::iterator it = h.find(key);

		if ( it == h.end() ) {
			CValue &nv = h[key];
			nv = CValue(F_TAG_HASH, 0/*dmy*/);
			if ( track ) {
				dotted_tables.insert(path);
			}
			t = &nv;
		}
		else if ( it->second.IsHash() ) {
			if ( track && (inline_tables.count(path) || explicit_tables.count(path)) ) {
				return Fail(L"cannot extend a table with dotted keys : " + TomlKeysText(keys));
			}
			t = &it->second;
		}
		else {
			return Fail(L"key already defined as a value : " + TomlKeysText(keys));
		}
	}

	CValueHash &h = t->hash();
	CValue last_key(keys.back());
	if ( h.find(last_key) != h.end() ) {
		return Fail(L"duplicate key : " + TomlKeysText(keys));
	}
	h[last_key] = value;
	if ( track && is_inline ) {
		inline_tables.insert(TomlChildPath(path, keys.back()));
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseValue
 *  機能概要：  値を1つ解析します
 *
 *  文字列→文字列、整数→整数、実数→実数、true/false→1/0、日時→書かれたとおりの文字列
 *  配列→配列、インライン表→ハッシュ
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseValue(CValue &out, bool &is_inline)
{
	is_inline = false;

	yaya::char_t c = Ch(pos);
	if ( c == L'"' || c == L'\'' ) {
		yaya::string_t str;
		bool multi = (Ch(pos + 1) == c && Ch(pos + 2) == c);
		bool ok;
		if ( c == L'"' ) {
			ok = multi ? ParseMultiBasicString(str) : ParseBasicString(str);
		}
		else {
			ok = multi ? ParseMultiLiteralString(str) : ParseLiteralString(str);
		}
		if ( ! ok ) {
			return false;
		}
		out = CValue(str);
		return true;
	}
	if ( c == L'[' ) {
		return ParseArray(out);
	}
	if ( c == L'{' ) {
		is_inline = true;
		return ParseInlineTable(out);
	}
	if ( s.compare(pos, 4, L"true") == 0 && ! IsTomlBareKeyChar(Ch(pos + 4)) ) {
		pos += 4;
		out = CValue(1);
		return true;
	}
	if ( s.compare(pos, 5, L"false") == 0 && ! IsTomlBareKeyChar(Ch(pos + 5)) ) {
		pos += 5;
		out = CValue(0);
		return true;
	}
	if ( IsDateTimeStart() ) {
		return ParseDateTime(out);
	}
	return ParseNumber(out);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseNumber
 *  機能概要：  整数・実数を解析します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseNumber(CValue &out)
{
	spos_t st = pos;
	while ( pos < n ) {
		yaya::char_t c = s[pos];
		if ( IsTomlBareKeyChar(c) || c == L'+' || c == L'.' ) {
			++pos;
		}
		else {
			break;
		}
	}
	yaya::string_t t = s.substr(st, pos - st);
	if ( t.empty() ) {
		return Fail(L"invalid value");
	}

	int r = TomlTextToNumber(t, out);
	if ( r == 2 ) {
		return Fail(L"integer out of range : " + t);
	}
	if ( r != 0 ) {
		return Fail(L"invalid value : " + t);
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::IsDateTimeStart / ScanTime / ParseDateTime
 *  機能概要：  日時（1979-05-27T07:32:00Z / 1979-05-27 / 07:32:00 など）を文字列として読みます
 * -----------------------------------------------------------------------
 */
bool CTomlParser::IsDateTimeStart(void) const
{
	if ( IsDigitAt(pos) && IsDigitAt(pos + 1) && IsDigitAt(pos + 2) && IsDigitAt(pos + 3) && Ch(pos + 4) == L'-' ) {
		return true;
	}
	if ( IsDigitAt(pos) && IsDigitAt(pos + 1) && Ch(pos + 2) == L':' ) {
		return true;
	}
	return false;
}

bool CTomlParser::ScanTime(void)
{
	if ( ! (IsDigitAt(pos) && IsDigitAt(pos + 1) && Ch(pos + 2) == L':' && IsDigitAt(pos + 3) && IsDigitAt(pos + 4)) ) {
		return false;
	}
	pos += 5;
	if ( Ch(pos) == L':' && IsDigitAt(pos + 1) && IsDigitAt(pos + 2) ) {
		pos += 3;
		if ( Ch(pos) == L'.' && IsDigitAt(pos + 1) ) {
			++pos;
			while ( IsDigitAt(pos) ) {
				++pos;
			}
		}
	}
	return true;
}

bool CTomlParser::ParseDateTime(CValue &out)
{
	spos_t st = pos;

	if ( Ch(pos + 2) == L':' ) {
		if ( ! ScanTime() ) {
			return Fail(L"invalid time");
		}
	}
	else {
		if ( ! (IsDigitAt(pos + 5) && IsDigitAt(pos + 6) && Ch(pos + 7) == L'-' && IsDigitAt(pos + 8) && IsDigitAt(pos + 9)) ) {
			return Fail(L"invalid date");
		}
		pos += 10;

		yaya::char_t c = Ch(pos);
		if ( c == L'T' || c == L't' || (c == L' ' && IsDigitAt(pos + 1) && IsDigitAt(pos + 2) && Ch(pos + 3) == L':') ) {
			++pos;
			if ( ! ScanTime() ) {
				return Fail(L"invalid date-time");
			}
			c = Ch(pos);
			if ( c == L'Z' || c == L'z' ) {
				++pos;
			}
			else if ( (c == L'+' || c == L'-') && IsDigitAt(pos + 1) && IsDigitAt(pos + 2) && Ch(pos + 3) == L':' && IsDigitAt(pos + 4) && IsDigitAt(pos + 5) ) {
				pos += 6;
			}
		}
	}

	if ( pos < n && (IsTomlBareKeyChar(s[pos]) || s[pos] == L'.' || s[pos] == L':') ) {
		return Fail(L"invalid date-time");
	}
	out = CValue(s.substr(st, pos - st));
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseEscape
 *  機能概要：  "..."の中のエスケープを1つ解析します　posは\の位置
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseEscape(yaya::string_t &out)
{
	yaya::char_t e = Ch(pos + 1);
	pos += 2;

	int digits = 0;
	switch ( e ) {
	case L'b':  out += static_cast<yaya::char_t>(0x08); break;
	case L't':  out += L'\t'; break;
	case L'n':  out += L'\n'; break;
	case L'f':  out += static_cast<yaya::char_t>(0x0C); break;
	case L'r':  out += L'\r'; break;
	case L'e':  out += static_cast<yaya::char_t>(0x1B); break;
	case L'"':  out += L'"'; break;
	case L'\\': out += L'\\'; break;
	case L'x':  digits = 2; break;
	case L'u':  digits = 4; break;
	case L'U':  digits = 8; break;
	default:
		return Fail(L"invalid escape sequence");
	}
	if ( digits == 0 ) {
		return true;
	}

	unsigned long cp;
	if ( ! ReadHexDigits(s, pos, digits, cp) ) {
		return Fail(L"invalid escape sequence");
	}
	if ( ! AppendCodePoint(out, cp) ) {
		return Fail(L"invalid unicode escape");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseBasicString
 *  機能概要：  "..."を解析します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseBasicString(yaya::string_t &out)
{
	++pos;
	out.erase();
	for ( ;; ) {
		if ( pos >= n || s[pos] == L'\n' ) {
			return Fail(L"unterminated string");
		}
		yaya::char_t c = s[pos];
		if ( c == L'"' ) {
			++pos;
			return true;
		}
		if ( c == L'\\' ) {
			if ( ! ParseEscape(out) ) {
				return false;
			}
			continue;
		}
		if ( IsTomlControl(c) ) {
			return Fail(L"control character in a string");
		}
		out += c;
		++pos;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseMultiBasicString
 *  機能概要：  """..."""を解析します
 *
 *  開始直後の改行は除き、行末の\は続く空白と改行をまとめて消します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseMultiBasicString(yaya::string_t &out)
{
	size_t start_line = line;
	pos += 3;
	out.erase();
	if ( Ch(pos) == L'\n' ) {
		++pos;
		++line;
	}

	for ( ;; ) {
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated string");
		}
		yaya::char_t c = s[pos];

		if ( c == L'"' && Ch(pos + 1) == L'"' && Ch(pos + 2) == L'"' ) {
			// 閉じる"""の直前に"を2つまで書ける
			spos_t q = pos + 3;
			int extra = 0;
			while ( extra < 2 && Ch(q) == L'"' ) {
				++q;
				++extra;
			}
			out.append(extra, L'"');
			pos = q;
			return true;
		}
		if ( c == L'\\' ) {
			spos_t q = pos + 1;
			while ( q < n && IsBlankChar(s[q]) ) {
				++q;
			}
			if ( q < n && s[q] == L'\n' ) {
				pos = q;
				while ( pos < n && (IsBlankChar(s[pos]) || s[pos] == L'\n') ) {
					if ( s[pos] == L'\n' ) {
						++line;
					}
					++pos;
				}
				continue;
			}
			if ( ! ParseEscape(out) ) {
				return false;
			}
			continue;
		}
		if ( c == L'\n' ) {
			++line;
		}
		else if ( IsTomlControl(c) ) {
			return Fail(L"control character in a string");
		}
		out += c;
		++pos;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseLiteralString
 *  機能概要：  '...'を解析します（エスケープなし）
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseLiteralString(yaya::string_t &out)
{
	++pos;
	out.erase();
	for ( ;; ) {
		if ( pos >= n || s[pos] == L'\n' ) {
			return Fail(L"unterminated string");
		}
		yaya::char_t c = s[pos];
		if ( c == L'\'' ) {
			++pos;
			return true;
		}
		if ( IsTomlControl(c) ) {
			return Fail(L"control character in a string");
		}
		out += c;
		++pos;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseMultiLiteralString
 *  機能概要：  '''...'''を解析します（エスケープなし、開始直後の改行は除く）
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseMultiLiteralString(yaya::string_t &out)
{
	size_t start_line = line;
	pos += 3;
	out.erase();
	if ( Ch(pos) == L'\n' ) {
		++pos;
		++line;
	}

	for ( ;; ) {
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated string");
		}
		yaya::char_t c = s[pos];

		if ( c == L'\'' && Ch(pos + 1) == L'\'' && Ch(pos + 2) == L'\'' ) {
			spos_t q = pos + 3;
			int extra = 0;
			while ( extra < 2 && Ch(q) == L'\'' ) {
				++q;
				++extra;
			}
			out.append(extra, L'\'');
			pos = q;
			return true;
		}
		if ( c == L'\n' ) {
			++line;
		}
		else if ( IsTomlControl(c) ) {
			return Fail(L"control character in a string");
		}
		out += c;
		++pos;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseArray
 *  機能概要：  配列を解析します（改行・コメント・末尾のカンマを許す）
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseArray(CValue &out)
{
	if ( depth >= TOML_MAX_DEPTH ) {
		return Fail(L"nesting too deep");
	}
	++depth;
	bool result = ParseArrayBody(out);
	--depth;
	return result;
}

bool CTomlParser::ParseArrayBody(CValue &out)
{
	size_t start_line = line;
	++pos;
	out = CValue(F_TAG_ARRAY, 0/*dmy*/);

	for ( ;; ) {
		SkipSpaceAndComments();
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated array");
		}
		if ( s[pos] == L']' ) {
			++pos;
			break;
		}

		CValue v;
		bool is_inline;
		if ( ! ParseValue(v, is_inline) ) {
			return false;
		}
		out.array().push_back(v);

		SkipSpaceAndComments();
		if ( Ch(pos) == L',' ) {
			++pos;
			continue;
		}
		if ( Ch(pos) == L']' ) {
			++pos;
			break;
		}
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated array");
		}
		return Fail(L"expected ',' or ']' in an array");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::ParseInlineTable
 *  機能概要：  インライン表 { key = value, ... } を解析します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::ParseInlineTable(CValue &out)
{
	if ( depth >= TOML_MAX_DEPTH ) {
		return Fail(L"nesting too deep");
	}
	++depth;
	bool result = ParseInlineTableBody(out);
	--depth;
	return result;
}

bool CTomlParser::ParseInlineTableBody(CValue &out)
{
	size_t start_line = line;
	++pos;
	out = CValue(F_TAG_HASH, 0/*dmy*/);

	for ( ;; ) {
		SkipSpaceAndComments();
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated inline table");
		}
		if ( s[pos] == L'}' ) {
			++pos;
			break;
		}

		if ( ! ParseKeyValue(out, yaya::string_t(), false) ) {
			return false;
		}

		SkipSpaceAndComments();
		if ( Ch(pos) == L',' ) {
			++pos;
			continue;
		}
		if ( Ch(pos) == L'}' ) {
			++pos;
			break;
		}
		if ( pos >= n ) {
			return FailAt(start_line, L"unterminated inline table");
		}
		return Fail(L"expected ',' or '}' in an inline table");
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CTomlParser::Parse
 *  機能概要：  文書全体を解析します
 * -----------------------------------------------------------------------
 */
bool CTomlParser::Parse(CValue &out)
{
	current = &root;
	current_path.erase();

	for ( ;; ) {
		SkipBlanks();
		if ( pos >= n ) {
			break;
		}
		yaya::char_t c = s[pos];
		if ( c == L'\n' ) {
			++pos;
			++line;
			continue;
		}
		if ( c == L'#' ) {
			SkipComment();
			continue;
		}

		if ( c == L'[' ) {
			if ( Ch(pos + 1) == L'[' ) {
				pos += 2;
				if ( ! ParseTableHeader(true) ) {
					return false;
				}
			}
			else {
				++pos;
				if ( ! ParseTableHeader(false) ) {
					return false;
				}
			}
		}
		else {
			if ( ! ParseKeyValue(*current, current_path, true) ) {
				return false;
			}
		}

		if ( ! FinishLine() ) {
			return false;
		}
	}

	out = root;
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlToValue
 *  機能概要：  UTF-8のTOMLを解析してハッシュにします
 *
 *  返値　　：　成功時true　失敗時はerrstrに行番号と内容を入れます
 * -----------------------------------------------------------------------
 */
bool TomlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr)
{
	CNumericLocaleGuard locale_guard;

	yaya::string_t text = PrepareText(utf8);
	CTomlParser parser(text);
	if ( ! parser.Parse(out) ) {
		errstr = parser.errstr;
		out = CValue();
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// YAMLの出力
//////////////////////////////////////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlIsSpecialWord
 *  機能概要：  YAML 1.1で真偽値やnullになる語ならtrue（他の処理系のためにクォートする）
 * -----------------------------------------------------------------------
 */
static bool YamlIsSpecialWord(const yaya::string_t &str)
{
	static const yaya::char_t * const words[] = {
		L"y", L"n", L"yes", L"no", L"on", L"off", L"true", L"false", L"null", NULL
	};

	yaya::string_t lower;
	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		if ( c >= L'A' && c <= L'Z' ) {
			c = static_cast<yaya::char_t>(c - L'A' + L'a');
		}
		lower += c;
	}
	for ( int w = 0; words[w]; ++w ) {
		if ( lower == words[w] ) {
			return true;
		}
	}
	return false;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlLooksNumeric
 *  機能概要：  数値や日時に見える文字列ならtrue（他の処理系が数値と解釈しないようクォートする）
 * -----------------------------------------------------------------------
 */
static bool YamlLooksNumeric(const yaya::string_t &str)
{
	yaya::char_t c0 = str[0];
	if ( ! (IsDecDigit(c0) || c0 == L'+' || c0 == L'-' || c0 == L'.') ) {
		return false;
	}
	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		if ( HexDigitValue(c) >= 0 ) {
			continue;
		}
		switch ( c ) {
		case L'x': case L'X': case L'o': case L'O': case L'b': case L'B':
		case L'_': case L'.': case L':': case L'+': case L'-':
			continue;
		default:
			return false;
		}
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlCanBePlain
 *  機能概要：  文字列をクォートせずに書けるならtrue
 *
 *  読み戻したときに文字列以外（数値・真偽値・null）になるものや、
 *  記号で始まるもの、": " " #" を含むものなどはクォートします
 * -----------------------------------------------------------------------
 */
static bool YamlCanBePlain(const yaya::string_t &str, bool flow)
{
	if ( str.empty() ) {
		return false;
	}

	yaya::char_t c0 = str[0];
	yaya::char_t cl = str[str.size() - 1];
	if ( IsBlankChar(c0) || IsBlankChar(cl) ) {
		return false;
	}

	// 記号で始まるものはクォートする（- ? : は続く文字によっては許されるが、処理系による差が大きい）
	switch ( c0 ) {
	case L',': case L'[': case L']': case L'{': case L'}':
	case L'#': case L'&': case L'*': case L'!': case L'|': case L'>':
	case L'\'': case L'"': case L'%': case L'@': case L'`':
	case L'-': case L'?': case L':':
		return false;
	default:
		break;
	}

	if ( str.compare(0, 3, L"...") == 0 ) {
		return false;
	}

	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		if ( c < 0x20 || c == 0x7F || c == 0x85 || c == 0xFEFF || c == 0x2028 || c == 0x2029 ) {
			return false;
		}
		if ( c == L':' ) {
			if ( i + 1 >= str.size() || IsBlankChar(str[i + 1]) || (flow && IsFlowIndicator(str[i + 1])) ) {
				return false;
			}
		}
		if ( c == L'#' && i > 0 && IsBlankChar(str[i - 1]) ) {
			return false;
		}
		if ( flow && IsFlowIndicator(c) ) {
			return false;
		}
	}

	if ( ! YamlResolvePlain(str).IsStringReal() ) {
		return false;
	}
	if ( YamlIsSpecialWord(str) || YamlLooksNumeric(str) ) {
		return false;
	}
	// YAML 1.1の処理系は値の<<もマージキーとみなすため、キー以外でもクォートする
	if ( str == L"<<" ) {
		return false;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendDoubleQuoted
 *  機能概要：  "..."の形で文字列を追加します
 *
 *  encがあれば、出力先の文字コードで表せない文字をエスケープします
 * -----------------------------------------------------------------------
 */
static void YamlAppendDoubleQuoted(yaya::string_t &out, const yaya::string_t &str, const CCharsetEncodable *enc)
{
	static const yaya::char_t hex[] = L"0123456789ABCDEF";

	out += L'"';
	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		switch ( c ) {
		case L'"':   out += L"\\\""; break;
		case L'\\':  out += L"\\\\"; break;
		case L'\n':  out += L"\\n"; break;
		case L'\t':  out += L"\\t"; break;
		case L'\r':  out += L"\\r"; break;
		case 0:      out += L"\\0"; break;
		case 0x85:   out += L"\\N"; break;
		case 0x2028: out += L"\\L"; break;
		case 0x2029: out += L"\\P"; break;
		case 0xFEFF: out += L"\\uFEFF"; break;
		default:
			if ( c < 0x20 || c == 0x7F ) {
				out += L"\\x";
				out += hex[(c >> 4) & 0xF];
				out += hex[c & 0xF];
			}
			else if ( enc ) {
				size_t len;
				unsigned long cp = CCharsetEncodable::CodePointAt(str, i, len);
				if ( enc->CanEncode(str, i, len) ) {
					out.append(str, i, len);
				}
				else {
					AppendUnicodeEscape(out, cp, false);
				}
				i += len - 1;
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
 *  関数名  ：  YamlAppendString
 *  機能概要：  文字列を、クォートせずに書けるならそのまま、そうでなければ"..."で追加します
 *
 *  出力先の文字コードで表せない文字を含むものは、エスケープするために"..."にします
 * -----------------------------------------------------------------------
 */
static void YamlAppendString(yaya::string_t &out, const yaya::string_t &str, bool flow, const CCharsetEncodable *enc)
{
	if ( YamlCanBePlain(str, flow) && CanEncodeAll(enc, str) ) {
		out += str;
	}
	else {
		YamlAppendDoubleQuoted(out, str, enc);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendDouble
 *  機能概要：  実数を追加します
 *
 *  YAML 1.1の処理系でも実数と解釈されるよう、指数表記にも小数点を付けます（1.0e+300）
 * -----------------------------------------------------------------------
 */
static void YamlAppendDouble(yaya::string_t &out, double d)
{
	if ( IsNanDouble(d) ) {
		out += L".nan";
		return;
	}
	if ( ! IsFiniteDouble(d) ) {
		out += (d > 0) ? L".inf" : L"-.inf";
		return;
	}

	yaya::string_t num;
	AppendFiniteDouble(num, d);
	spos_t e = num.find_first_of(L"eE");
	if ( e != yaya::string_t::npos && num.find(L'.') == yaya::string_t::npos ) {
		num.insert(e, L".0");
	}
	out += num;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendScalar
 *  機能概要：  配列・ハッシュ以外の値を追加します（空値はnull）
 * -----------------------------------------------------------------------
 */
static void YamlAppendScalar(yaya::string_t &out, const CValue &v, bool flow, const CCharsetEncodable *enc)
{
	switch ( v.GetType() ) {
	case F_TAG_INT:
		out += yaya::ws_lltoa(v.i_value);
		break;
	case F_TAG_DOUBLE:
		YamlAppendDouble(out, v.d_value);
		break;
	case F_TAG_STRING:
		YamlAppendString(out, v.s_value, flow, enc);
		break;
	default: // F_TAG_VOID
		out += L"null";
		break;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendFlow
 *  機能概要：  値をフロー形式（1行）で追加します
 * -----------------------------------------------------------------------
 */
static void YamlAppendFlow(yaya::string_t &out, const CValue &v, const CCharsetEncodable *enc)
{
	if ( v.IsArray() ) {
		const CValueArray &arr = v.array();
		out += L'[';
		for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
			if ( it != arr.begin() ) {
				out += L", ";
			}
			YamlAppendFlow(out, *it, enc);
		}
		out += L']';
	}
	else if ( v.IsHash() ) {
		const CValueHash &hash = v.hash();
		out += L'{';
		for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
			if ( it != hash.begin() ) {
				out += L", ";
			}
			YamlAppendString(out, it->first.GetValueString(), true, enc);
			out += L": ";
			YamlAppendFlow(out, it->second, enc);
		}
		out += L'}';
	}
	else {
		YamlAppendScalar(out, v, true, enc);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlCanBeLiteral
 *  機能概要：  複数行の文字列をブロックスカラー（|）で書けるならtrue
 * -----------------------------------------------------------------------
 */
static bool YamlCanBeLiteral(const yaya::string_t &str)
{
	if ( str.find(L'\n') == yaya::string_t::npos ) {
		return false;
	}
	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		if ( (c < 0x20 && c != L'\n' && c != L'\t') || c == 0x7F || c == 0x85 || c == 0xFEFF || c == 0x2028 || c == 0x2029 ) {
			return false;
		}
	}

	spos_t end = str.size();
	while ( end > 0 && str[end - 1] == L'\n' ) {
		--end;
	}
	if ( end == 0 ) {
		return false;
	}

	// 空白だけの行があるもの、最初の空でない行が空白で始まるものは"..."にする
	bool first_content = true;
	spos_t ls = 0;
	while ( ls < end ) {
		spos_t le = str.find(L'\n', ls);
		if ( le == yaya::string_t::npos || le > end ) {
			le = end;
		}
		if ( le > ls ) {
			bool all_blank = true;
			for ( spos_t k = ls; k < le; ++k ) {
				if ( ! IsBlankChar(str[k]) ) {
					all_blank = false;
					break;
				}
			}
			if ( all_blank ) {
				return false;
			}
			if ( first_content && IsBlankChar(str[ls]) ) {
				return false;
			}
			first_content = false;
		}
		ls = le + 1;
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendLiteral
 *  機能概要：  複数行の文字列をブロックスカラー（|- | |+）で追加します　行はindent桁で字下げします
 * -----------------------------------------------------------------------
 */
static void YamlAppendLiteral(yaya::string_t &out, const yaya::string_t &str, int indent)
{
	spos_t end = str.size();
	while ( end > 0 && str[end - 1] == L'\n' ) {
		--end;
	}
	spos_t trailing = str.size() - end;

	if ( trailing == 0 ) {
		out += L"|-\n";
	}
	else if ( trailing == 1 ) {
		out += L"|\n";
	}
	else {
		out += L"|+\n";
	}

	spos_t ls = 0;
	while ( ls < end ) {
		spos_t le = str.find(L'\n', ls);
		if ( le == yaya::string_t::npos || le > end ) {
			le = end;
		}
		if ( le > ls ) {
			out.append(indent, L' ');
			out.append(str, ls, le - ls);
		}
		out += L'\n';
		ls = le + 1;
	}
	if ( trailing > 1 ) {
		out.append(trailing - 1, L'\n');
	}
}

static void YamlAppendBlockMap(yaya::string_t &out, const CValue &v, int indent, bool inline_first, const CCharsetEncodable *enc);
static void YamlAppendBlockSeq(yaya::string_t &out, const CValue &v, int indent, bool inline_first, const CCharsetEncodable *enc);

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendBlockValue
 *  機能概要：  "key:"や"-"の直後に続けて値を追加します（行末の改行まで）
 *
 *  ブロックスカラーはエスケープできないので、出力先の文字コードで表せない文字を含むものは"..."にします
 * -----------------------------------------------------------------------
 */
static void YamlAppendBlockValue(yaya::string_t &out, const CValue &v, int indent, const CCharsetEncodable *enc)
{
	if ( v.IsHash() && v.hash_size() > 0 ) {
		out += L'\n';
		YamlAppendBlockMap(out, v, indent + 2, false, enc);
	}
	else if ( v.IsArray() && ! v.array().empty() ) {
		out += L'\n';
		YamlAppendBlockSeq(out, v, indent + 2, false, enc);
	}
	else if ( v.IsStringReal() && YamlCanBeLiteral(v.s_value) && CanEncodeAll(enc, v.s_value) ) {
		out += L' ';
		YamlAppendLiteral(out, v.s_value, indent + 2);
	}
	else if ( v.IsHash() ) {
		out += L" {}\n";
	}
	else if ( v.IsArray() ) {
		out += L" []\n";
	}
	else {
		out += L' ';
		YamlAppendScalar(out, v, false, enc);
		out += L'\n';
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendBlockMap
 *  機能概要：  ハッシュをブロック形式のマッピングで追加します
 *
 *  inline_firstなら最初のキーは字下げせずに書きます（"- "の直後）
 * -----------------------------------------------------------------------
 */
static void YamlAppendBlockMap(yaya::string_t &out, const CValue &v, int indent, bool inline_first, const CCharsetEncodable *enc)
{
	const CValueHash &hash = v.hash();
	for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
		if ( ! (inline_first && it == hash.begin()) ) {
			out.append(indent, L' ');
		}
		YamlAppendString(out, it->first.GetValueString(), false, enc);
		out += L':';
		YamlAppendBlockValue(out, it->second, indent, enc);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  YamlAppendBlockSeq
 *  機能概要：  配列をブロック形式のシーケンスで追加します
 * -----------------------------------------------------------------------
 */
static void YamlAppendBlockSeq(yaya::string_t &out, const CValue &v, int indent, bool inline_first, const CCharsetEncodable *enc)
{
	const CValueArray &arr = v.array();
	for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
		if ( ! (inline_first && it == arr.begin()) ) {
			out.append(indent, L' ');
		}
		out += L'-';
		if ( it->IsHash() && it->hash_size() > 0 ) {
			out += L' ';
			YamlAppendBlockMap(out, *it, indent + 2, true, enc);
		}
		else if ( it->IsArray() && ! it->array().empty() ) {
			out += L' ';
			YamlAppendBlockSeq(out, *it, indent + 2, true, enc);
		}
		else {
			YamlAppendBlockValue(out, *it, indent, enc);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ValueToYaml
 *  機能概要：  CValueをYAMLの文字列にします
 *
 *  prettyならブロック形式（2桁の字下げ、行末は改行）、そうでなければ1行のフロー形式にします
 * -----------------------------------------------------------------------
 */
void ValueToYaml(const CValue &value, bool pretty, yaya::string_t &out, const CCharsetEncodable *enc)
{
	CNumericLocaleGuard locale_guard;

	out.erase();
	if ( ! pretty ) {
		YamlAppendFlow(out, value, enc);
		return;
	}

	if ( value.IsHash() && value.hash_size() > 0 ) {
		YamlAppendBlockMap(out, value, 0, false, enc);
	}
	else if ( value.IsArray() && ! value.array().empty() ) {
		YamlAppendBlockSeq(out, value, 0, false, enc);
	}
	else if ( value.IsStringReal() && YamlCanBeLiteral(value.s_value) && CanEncodeAll(enc, value.s_value) ) {
		YamlAppendLiteral(out, value.s_value, 2);
	}
	else {
		YamlAppendFlow(out, value, enc);
		out += L'\n';
	}
}

//////////////////////////////////////////////////////////////////////////
// TOMLの出力
//////////////////////////////////////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendBasicString
 *  機能概要：  "..."（multilineなら"""..."""）の形で文字列を追加します
 *
 *  encがあれば、出力先の文字コードで表せない文字をエスケープします
 * -----------------------------------------------------------------------
 */
static void TomlAppendBasicString(yaya::string_t &out, const yaya::string_t &str, bool multiline, const CCharsetEncodable *enc)
{
	static const yaya::char_t hex[] = L"0123456789ABCDEF";

	out += multiline ? L"\"\"\"\n" : L"\"";
	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		switch ( c ) {
		case L'"':  out += L"\\\""; break;
		case L'\\': out += L"\\\\"; break;
		case L'\n': out += multiline ? L"\n" : L"\\n"; break;
		case L'\t': out += multiline ? L"\t" : L"\\t"; break;
		case L'\r': out += L"\\r"; break;
		case 0x08:  out += L"\\b"; break;
		case 0x0C:  out += L"\\f"; break;
		default:
			if ( c < 0x20 || c == 0x7F ) {
				out += L"\\u00";
				out += hex[(c >> 4) & 0xF];
				out += hex[c & 0xF];
			}
			else if ( enc ) {
				size_t len;
				unsigned long cp = CCharsetEncodable::CodePointAt(str, i, len);
				if ( enc->CanEncode(str, i, len) ) {
					out.append(str, i, len);
				}
				else {
					AppendUnicodeEscape(out, cp, false);
				}
				i += len - 1;
			}
			else {
				out += c;
			}
			break;
		}
	}
	out += multiline ? L"\"\"\"" : L"\"";
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendString
 *  機能概要：  文字列を追加します
 *
 *  改行を含むもの（allow_multilineのとき）は"""..."""、
 *  \を含み'や改行を含まないもの（Windowsのパスなど）は'...'、それ以外は"..."にします
 *  '...'はエスケープできないので、出力先の文字コードで表せない文字を含むものは"..."にします
 * -----------------------------------------------------------------------
 */
static void TomlAppendString(yaya::string_t &out, const yaya::string_t &str, bool allow_multiline, const CCharsetEncodable *enc)
{
	bool has_ctrl = false;
	bool has_newline = false;
	bool has_backslash = false;
	bool has_quote = false;

	for ( spos_t i = 0; i < str.size(); ++i ) {
		yaya::char_t c = str[i];
		if ( c == L'\n' ) {
			has_newline = true;
		}
		else if ( IsTomlControl(c) ) {
			has_ctrl = true;
		}
		else if ( c == L'\\' ) {
			has_backslash = true;
		}
		else if ( c == L'\'' ) {
			has_quote = true;
		}
	}

	if ( allow_multiline && has_newline && ! has_ctrl ) {
		TomlAppendBasicString(out, str, true, enc);
	}
	else if ( has_backslash && ! has_quote && ! has_newline && ! has_ctrl && CanEncodeAll(enc, str) ) {
		out += L'\'';
		out += str;
		out += L'\'';
	}
	else {
		TomlAppendBasicString(out, str, false, enc);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendKey
 *  機能概要：  キーを追加します（使える文字だけなら裸のまま、そうでなければ"..."）
 * -----------------------------------------------------------------------
 */
static void TomlAppendKey(yaya::string_t &out, const yaya::string_t &key, const CCharsetEncodable *enc)
{
	bool bare = ! key.empty();
	for ( spos_t i = 0; i < key.size(); ++i ) {
		if ( ! IsTomlBareKeyChar(key[i]) ) {
			bare = false;
			break;
		}
	}
	if ( bare ) {
		out += key;
	}
	else {
		TomlAppendBasicString(out, key, false, enc);
	}
}

static yaya::string_t TomlJoinPath(const yaya::string_t &path, const yaya::string_t &key, const CCharsetEncodable *enc)
{
	yaya::string_t result = path;
	if ( ! result.empty() ) {
		result += L'.';
	}
	TomlAppendKey(result, key, enc);
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendDouble
 *  機能概要：  実数を追加します（NaNはnan、無限大はinf/-inf）
 * -----------------------------------------------------------------------
 */
static void TomlAppendDouble(yaya::string_t &out, double d)
{
	if ( IsNanDouble(d) ) {
		out += L"nan";
	}
	else if ( ! IsFiniteDouble(d) ) {
		out += (d > 0) ? L"inf" : L"-inf";
	}
	else {
		AppendFiniteDouble(out, d);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendInline
 *  機能概要：  値を1行の形（配列は[...]、ハッシュはインライン表{...}）で追加します
 *
 *  ハッシュの中の空値のキーは書きません。配列の中の空値はTOMLで表せないのでエラーにします
 * -----------------------------------------------------------------------
 */
static bool TomlAppendInline(yaya::string_t &out, const CValue &v, yaya::string_t &errstr, const CCharsetEncodable *enc)
{
	switch ( v.GetType() ) {
	case F_TAG_INT:
		out += yaya::ws_lltoa(v.i_value);
		return true;
	case F_TAG_DOUBLE:
		TomlAppendDouble(out, v.d_value);
		return true;
	case F_TAG_STRING:
		TomlAppendString(out, v.s_value, false, enc);
		return true;
	case F_TAG_ARRAY:
		{
			const CValueArray &arr = v.array();
			out += L'[';
			for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
				if ( it != arr.begin() ) {
					out += L", ";
				}
				if ( ! TomlAppendInline(out, *it, errstr, enc) ) {
					return false;
				}
			}
			out += L']';
			return true;
		}
	case F_TAG_HASH:
		{
			const CValueHash &hash = v.hash();
			bool first = true;
			out += L'{';
			for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
				if ( it->second.IsVoid() ) {
					continue;
				}
				out += first ? L" " : L", ";
				first = false;
				TomlAppendKey(out, it->first.GetValueString(), enc);
				out += L" = ";
				if ( ! TomlAppendInline(out, it->second, errstr, enc) ) {
					return false;
				}
			}
			out += first ? L"}" : L" }";
			return true;
		}
	default: // F_TAG_VOID
		errstr = L"an array contains an empty value (TOML has no null)";
		return false;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlIsSection / TomlIsTableArray / TomlIsSimpleEntry
 *  機能概要：  整形時の書き分けの判定
 *
 *  中身のあるハッシュは[表]、ハッシュだけの配列は[[表の配列]]、それ以外はkey = valueで書きます
 * -----------------------------------------------------------------------
 */
static bool TomlIsSection(const CValue &v)
{
	return v.IsHash() && v.hash_size() > 0;
}

static bool TomlIsTableArray(const CValue &v)
{
	if ( ! v.IsArray() || v.array().empty() ) {
		return false;
	}
	const CValueArray &arr = v.array();
	for ( CValueArray::const_iterator it = arr.begin(); it != arr.end(); ++it ) {
		if ( ! it->IsHash() ) {
			return false;
		}
	}
	return true;
}

static bool TomlIsSimpleEntry(const CValue &v)
{
	return ! v.IsVoid() && ! TomlIsSection(v) && ! TomlIsTableArray(v);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlNeedsHeader
 *  機能概要：  表の見出しを書く必要があればtrue
 *
 *  キーと値が無く、下の階層の表だけを持つ表は見出しを省きます（[a.b]だけで暗黙に定義される）
 * -----------------------------------------------------------------------
 */
static bool TomlNeedsHeader(const CValue &table)
{
	bool has_child = false;
	const CValueHash &hash = table.hash();
	for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
		if ( TomlIsSimpleEntry(it->second) ) {
			return true;
		}
		if ( TomlIsSection(it->second) || TomlIsTableArray(it->second) ) {
			has_child = true;
		}
	}
	return ! has_child;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendPrettyValue
 *  機能概要：  整形時のkey = valueの値を追加します
 *
 *  改行を含む文字列は"""..."""、長い配列や配列・表を含む配列は1要素1行にします
 * -----------------------------------------------------------------------
 */
static bool TomlAppendPrettyValue(yaya::string_t &out, const CValue &v, yaya::string_t &errstr, const CCharsetEncodable *enc)
{
	if ( v.IsStringReal() ) {
		TomlAppendString(out, v.s_value, true, enc);
		return true;
	}
	if ( ! v.IsArray() ) {
		return TomlAppendInline(out, v, errstr, enc);
	}

	yaya::string_t one_line;
	if ( ! TomlAppendInline(one_line, v, errstr, enc) ) {
		return false;
	}

	const CValueArray &arr = v.array();
	bool nested = false;
	CValueArray::const_iterator it;
	for ( it = arr.begin(); it != arr.end(); ++it ) {
		if ( it->IsArray() || it->IsHash() ) {
			nested = true;
			break;
		}
	}
	if ( ! nested && one_line.size() <= TOML_ARRAY_LINE_WIDTH ) {
		out += one_line;
		return true;
	}

	out += L"[\n";
	for ( it = arr.begin(); it != arr.end(); ++it ) {
		out += L"    ";
		if ( ! TomlAppendInline(out, *it, errstr, enc) ) {
			return false;
		}
		out += L",\n";
	}
	out += L']';
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  TomlAppendTable
 *  機能概要：  表の中身を整形して追加します
 *
 *  key = value → 下の階層の[表] → [[表の配列]] の順に書きます（TOMLの規則上この順が必要）
 * -----------------------------------------------------------------------
 */
static bool TomlAppendTable(yaya::string_t &out, const CValue &table, const yaya::string_t &path, yaya::string_t &errstr, const CCharsetEncodable *enc)
{
	const CValueHash &hash = table.hash();
	CValueHash::const_iterator it;

	for ( it = hash.begin(); it != hash.end(); ++it ) {
		if ( ! TomlIsSimpleEntry(it->second) ) {
			continue;
		}
		TomlAppendKey(out, it->first.GetValueString(), enc);
		out += L" = ";
		if ( ! TomlAppendPrettyValue(out, it->second, errstr, enc) ) {
			return false;
		}
		out += L'\n';
	}

	for ( it = hash.begin(); it != hash.end(); ++it ) {
		if ( ! TomlIsSection(it->second) ) {
			continue;
		}
		yaya::string_t sub = TomlJoinPath(path, it->first.GetValueString(), enc);
		if ( TomlNeedsHeader(it->second) ) {
			if ( ! out.empty() ) {
				out += L'\n';
			}
			out += L"[" + sub + L"]\n";
		}
		if ( ! TomlAppendTable(out, it->second, sub, errstr, enc) ) {
			return false;
		}
	}

	for ( it = hash.begin(); it != hash.end(); ++it ) {
		if ( ! TomlIsTableArray(it->second) ) {
			continue;
		}
		yaya::string_t sub = TomlJoinPath(path, it->first.GetValueString(), enc);
		const CValueArray &arr = it->second.array();
		for ( CValueArray::const_iterator elem = arr.begin(); elem != arr.end(); ++elem ) {
			if ( ! out.empty() ) {
				out += L'\n';
			}
			out += L"[[" + sub + L"]]\n";
			if ( ! TomlAppendTable(out, *elem, sub, errstr, enc) ) {
				return false;
			}
		}
	}
	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  ValueToToml
 *  機能概要：  ハッシュをTOMLの文字列にします
 *
 *  prettyなら[表]の見出しを使って整形し、そうでなければ最上位のキー1つにつき1行
 *  （下の階層はインライン表）にします。空値のキーは書きません
 *  返値　　：　成功時true　最上位がハッシュでない、配列に空値があるときはfalseでerrstrに詳細
 * -----------------------------------------------------------------------
 */
bool ValueToToml(const CValue &value, bool pretty, yaya::string_t &out, yaya::string_t &errstr, const CCharsetEncodable *enc)
{
	CNumericLocaleGuard locale_guard;

	out.erase();
	if ( ! value.IsHash() ) {
		errstr = L"the top-level value must be a hash";
		return false;
	}

	if ( pretty ) {
		return TomlAppendTable(out, value, yaya::string_t(), errstr, enc);
	}

	const CValueHash &hash = value.hash();
	for ( CValueHash::const_iterator it = hash.begin(); it != hash.end(); ++it ) {
		if ( it->second.IsVoid() ) {
			continue;
		}
		TomlAppendKey(out, it->first.GetValueString(), enc);
		out += L" = ";
		if ( ! TomlAppendInline(out, it->second, errstr, enc) ) {
			return false;
		}
		out += L'\n';
	}
	return true;
}
