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
void	ValueToJson(const CValue &value, bool pretty, yaya::string_t &out);

// 要素のハッシュをXMLの文字列にする。encodingはXML宣言に書く文字コード名。失敗時はerrstrに詳細
bool	ValueToXml(const CValue &value, const char *encoding, bool pretty, yaya::string_t &out, yaya::string_t &errstr);

// 文字コードからXML宣言のencodingに書く名前を得る
const char	*XmlCharsetName(int charset);

//----

#endif
