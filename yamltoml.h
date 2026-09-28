// 
// AYA version 5
//
// YAML/TOMLの解析と出力　（FREADYAML/FREADTOML/PARSEYAML/PARSETOML/FWRITEYAML/FWRITETOML/DUMPYAML/DUMPTOML）
// どちらも自前で実装しています。
// 

#ifndef	YAMLTOML_H
#define	YAMLTOML_H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>

#include "globaldef.h"
#include "value.h"

//----

// UTF-8のYAMLを解析してCValueにする。成功時true、失敗時はerrstrに詳細
bool	YamlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr);

// UTF-8のTOMLを解析してハッシュにする。成功時true、失敗時はerrstrに詳細
bool	TomlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr);

class CCharsetEncodable;

// CValueをYAMLの文字列にする（prettyならブロック形式、そうでなければ1行のフロー形式）
// encがあれば、出力先の文字コードで表せない文字を含む文字列は"..."にしてエスケープする
void	ValueToYaml(const CValue &value, bool pretty, yaya::string_t &out, const CCharsetEncodable *enc = NULL);

// ハッシュをTOMLの文字列にする（prettyなら[表]の見出しを使い、そうでなければ最上位のキー1つにつき1行）
// encがあれば、出力先の文字コードで表せない文字を含む文字列は"..."にしてエスケープする
// 失敗時はerrstrに詳細
bool	ValueToToml(const CValue &value, bool pretty, yaya::string_t &out, yaya::string_t &errstr, const CCharsetEncodable *enc = NULL);

//----

#endif
