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
// メッセージ種別
#define	E_I			0	/* 標準のメッセージ */
#define	E_F			1	/* フェータルエラー */
#define	E_E			2	/* エラー */
#define	E_W			3	/* ワーニング */
#define	E_N			4	/* 注記 */
#define	E_END		5	/* ログの終了 */
#define	E_SJIS		16	/* マルチバイト文字コード＝SJIS */
#define	E_UTF8		17	/* マルチバイト文字コード＝UTF-8 */
#define	E_DEFAULT	32	/* マルチバイト文字コード＝OSデフォルトのコード */

void LoadMassageFromTxt(const yaya::string_t &file,char cset);
typedef std::vector<std::wstring> MassageArray;
// フェータルエラー文字列（日本語）
extern MassageArray msgf;

// エラー文字列（日本語）
extern MassageArray msg;

// ワーニング文字列（日本語）
extern MassageArray msgw;

// 注記文字列（日本語）
extern MassageArray msgn;

// その他のログ文字列（日本語）
extern MassageArray msgj;

#endif
