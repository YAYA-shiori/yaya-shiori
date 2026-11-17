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

typedef std::vector<yaya::string_t> MessageArray;

// メッセージ表。実体（CAyaVM）ごとに持つ（複数の実体で言語が違っても混ざらないように）
struct CMessageTables {
	// フェータルエラー文字列
	MessageArray msgf;

	// エラー文字列
	MessageArray msge;

	// ワーニング文字列
	MessageArray msgw;

	// 注記文字列
	MessageArray msgn;

	// その他のログ文字列
	MessageArray msgj;
};

bool LoadMessageFromTxt(CMessageTables &tables,const yaya::string_t &basepath,yaya::string_t &filename,char cset);
bool LoadMessageFromTxtResource(const yaya::string_t &basepath,yaya::string_t &filename,char cset,std::vector<yaya::string_t> &lines);
bool LoadMessageFromTxtFile(const yaya::string_t &basepath,yaya::string_t &filename,char cset,std::vector<yaya::string_t> &lines);

const yaya::string_t GetTextFromTable(const CMessageTables &tables,int mode,int id);
bool IsEmpty(const CMessageTables &tables);

}

#endif
