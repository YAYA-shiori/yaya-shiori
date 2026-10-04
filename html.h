//
// AYA version 5
//
// HTMLの解析　（FREADHTML/PARSEHTML）
// 解析にはGumbo（HTML5準拠のパーサ）を使用しています。
//

#ifndef	HTML_H
#define	HTML_H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>

#include "globaldef.h"
#include "value.h"

//----

// UTF-8のHTMLを解析して<html>要素をCValueにする。成功時true、失敗時はerrstrに詳細
// HTMLは壊れていても解析できるので、失敗するのは入れ子が深すぎるときなど
bool	HtmlToValue(const std::string &utf8, CValue &out, yaya::string_t &errstr);

// 先頭のBOMと<meta charset=...>（http-equiv="Content-Type"のcontent内も）から文字コードを判定する
// 見つからない・不明な文字コードの場合はCHARSET_UTF8
int		HtmlDetectCharset(const std::string &bytes);

//----

#endif
