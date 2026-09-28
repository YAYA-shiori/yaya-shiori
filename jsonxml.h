// 
// AYA version 5
//
// JSON/XMLの解析と出力　（FREADJSON/FREADXML/PARSEJSON/PARSEXML/FWRITEJSON/FWRITEXML/DUMPJSON/DUMPXML）
// JSONの解析にはparson、XMLの解析と出力にはtinyxml2を使用しています。
// JSONの出力はYAYAの64bit整数を誤差なく書くため自前で行っています。
// 

#ifndef	JSONXML_H
#define	JSONXML_H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <map>
#include <locale.h>

#include "globaldef.h"
#include "value.h"

//----

// 形式の種類（FREAD系/PARSE系/FWRITE系/DUMP系の共通処理で使う）
enum {
	DATAFMT_JSON,
	DATAFMT_XML,
	DATAFMT_YAML,
	DATAFMT_TOML
};

/* -----------------------------------------------------------------------
 *  クラス名：  CNumericLocaleGuard
 *  機能概要：  生存中だけLC_NUMERICを"C"にします
 *
 *  YAYAは起動時にOSのロケールを設定するため、小数点が","のロケールでは
 *  strtodが"1.5"を読めず、sprintfは"1,5"を書いてしまう
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
 *  クラス名：  CCharsetEncodable
 *  機能概要：  出力先の文字コードで表せない文字を調べます（FWRITE*のエスケープ用）
 *
 *  1文字ずつ出力先の文字コードに変換して読み戻し、元に戻るかで判定します。
 *  ASCIIはどの文字コードでも表せるものとします。結果は文字ごとに覚えておきます
 * -----------------------------------------------------------------------
 */
class CCharsetEncodable
{
private:
	int charset;
	mutable std::map<unsigned long, bool> cache;

public:
	explicit CCharsetEncodable(int cs) : charset(cs) { }

	// str[i]から始まる1文字のコードポイントを返し、lenにその文字の長さ（サロゲートペアなら2）を入れる
	static unsigned long CodePointAt(const yaya::string_t &str, size_t i, size_t &len);

	// str[i]から始まる長さlenの1文字を表せるならtrue
	bool CanEncode(const yaya::string_t &str, size_t i, size_t len) const;

	// strのすべての文字を表せるならtrue
	bool CanEncodeAll(const yaya::string_t &str) const;
};

// encがNULLなら（UTF-8など）すべての文字を表せる
inline bool CanEncodeAll(const CCharsetEncodable *enc, const yaya::string_t &str)
{
	return ! enc || enc->CanEncodeAll(str);
}

// コードポイントを\uXXXX（BMPの外は\UXXXXXXXX、utf16pairなら\uXXXX\uXXXXのサロゲートペア）で追加する
void	AppendUnicodeEscape(yaya::string_t &out, unsigned long cp, bool utf16pair);

// UTF-8文字列と内部文字列の相互変換
yaya::string_t	Utf8ToWide(const char *str);
std::string		WideToUtf8(const yaya::string_t &str);

// NaNでも無限大でもなければtrue
bool	IsFiniteDouble(double d);

// 有限の実数を、読み戻して同じ値になる短い表記で追加する（小数点か指数を必ず付ける）
// CNumericLocaleGuardの生存中に呼ぶこと
void	AppendFiniteDouble(yaya::string_t &out, double d);

// UTF-8のJSONを解析してCValueにする。成功時true
bool	JsonToValue(const std::string &utf8, CValue &out);

// UTF-8のXMLを解析してルート要素をCValueにする。成功時true、失敗時はerrstrに詳細
bool	XmlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr);

// XML宣言のencodingから文字コードを判定する。宣言が無い・不明な場合はCHARSET_UTF8
int		XmlDetectCharset(const std::string &bytes);

// 先頭のUTF-8 BOMを除去する
void	CutUtf8Bom(std::string &str);

// CValueをJSONの文字列にする（prettyなら改行とインデントを入れる）
// encがあれば、出力先の文字コードで表せない文字を\uXXXXにする
void	ValueToJson(const CValue &value, bool pretty, yaya::string_t &out, const CCharsetEncodable *enc = NULL);

// 要素のハッシュをXMLの文字列にする。encodingはXML宣言に書く文字コード名。失敗時はerrstrに詳細
// encがあれば、出力先の文字コードで表せない文字を&#xXXXX;にする（要素名・属性名にあれば失敗）
bool	ValueToXml(const CValue &value, const char *encoding, bool pretty, yaya::string_t &out, yaya::string_t &errstr, const CCharsetEncodable *enc = NULL);

// 文字コードからXML宣言のencodingに書く名前を得る
const char	*XmlCharsetName(int charset);

//----

#endif
