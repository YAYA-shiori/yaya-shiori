// 
// AYA version 5
//
// ログメッセージ
// written by umeici. 2004
// 
// 英語のメッセージは基本的に以下のサイト等で自動翻訳したものです。
// excite翻訳
// http://www.excite.co.jp/world/
//

#ifndef	MESSAGESH
#define	MESSAGESH

//----
#include <vector>
#include <string>
#include "globaldef.h"
// メッセージ種別
#define	E_I			0	/* info */
#define	E_F			1	/* fatal */
#define	E_E			2	/* error */
#define	E_W			3	/* warning */
#define	E_N			4	/* note */
#define E_J			5	/* other(j) */
#define	E_END		6	/* ログの終了 */
#define	E_SJIS		16	/* マルチバイト文字コード＝SJIS */
#define	E_UTF8		17	/* マルチバイト文字コード＝UTF-8 */
#define	E_DEFAULT	32	/* マルチバイト文字コード＝OSデフォルトのコード */

namespace yayamsg {

bool LoadMessageFromTxt(const yaya::string_t &file,char cset);
const yaya::string_t GetTextFromTable(int mode,int id);
bool IsEmpty(void);

typedef std::vector<yaya::string_t> MessageArray;

// フェータルエラー文字列
extern MessageArray msgf;

// エラー文字列
extern MessageArray msge;

// ワーニング文字列
extern MessageArray msgw;

// 注記文字列
extern MessageArray msgn;

// その他のログ文字列
extern MessageArray msgj;

}

#endif
