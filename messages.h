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

/* 残念ながら、ワイド文字の埋め込みは文字コードを明示的に指定する方法が無いため
   移植性がありません(埋め込む代わりにメッセージファイルを別に持っておく、
   というのが現在の主流)。なので日本語メッセージモードでも英語を出します…
 */
/*
#define msgf msgfe
#define msge msgee
#define msgw msgwe
#define msgn msgne
#define msg  msge

// フェータルエラー文字列（英語）
const wchar_t	msgfe[][128] = {
	L"",
};

// エラー文字列（英語）
const wchar_t	msgee[][256] = {
	L"error E0000 : Unknown error.",
	L"error E0001 : Function name is required.",
	L"error E0002 : Too much '}' exist.",
	L"error E0003 : Invalid name is used for the function.",
	L"error E0004 : Unanalyzable. Probably, '{' is required here.",
	L"error E0005 : Failed to open the file.",
	L"error E0006 : Missing name of the array.",
	L"error E0007 : Incomplete string exists.",
	L"error E0008 : Invalid double quote exists.",
	L"error E0009 : Internal error (1).",
	L"error E0010 : Unanalyzable string.",
	L"error E0011 : Some invalid characters are used in the identifier.",
	L"error E0012 : Invalid name is used for the variable.",
	L"error E0013 : Duplicated function names exist.",
	L"error E0014 : Ordinal number is required for the array.",
	L"error E0015 : Internal error (2).",
	L"error E0016 : Internal error (3).",
	L"error E0017 : Missing name of array embedded into the string.",
	L"error E0018 : Missing ordinal number of array embedded into the string.",
	L"error E0019 : The parenthesis () has not been closed.",
	L"error E0020 : The parenthesis [] is invalid or has not been closed correctly.",
	L"error E0021 : Internal error (4).",
	L"error E0022 : Invalid expression.",
	L"error E0023 : Operator ',' is not used correctly.",
	L"error E0024 : Operator ',' is not used correctly.",
	L"error E0025 : Function not found.",
	L"error E0026 : Internal error (5).",
	L"error E0027 : No significant elements exist.",
	L"error E0028 : Internal error (23).",
	L"error E0029 : Invalid substitution exists.",
	L"error E0030 : Undefined function flag (anti-duplication) is specified.",
	L"error E0031 : Internal error (6).",
	L"error E0032 : Indispensable functions are not found.",		// no use
	L"error E0033 : Internal error (7).",
	L"error E0034 : Internal error (8).",
	L"error E0035 : Missing '{' after 'if'.",
	L"error E0036 : Missing '{' after 'elseif'.",
	L"error E0037 : Missing '{' after 'else'.",
	L"error E0038 : Missing '{' after 'switch'.",
	L"error E0039 : Missing '{' after 'while'.",
	L"error E0040 : Invalid expression is used for termination condition of the syntax 'for'.",
	L"error E0041 : Invalid expression is used for continual condition of the syntax 'for'.",
	L"error E0042 : Missing '{' after 'for'.",
	L"error E0043 : The container variable of 'foreach' is incorrectly placed.",
	L"error E0044 : Missing '{' after 'foreach'.",
	L"error E0045 : The label of 'when' must be a constant.",
	L"error E0046 : The label of 'when' must be a constant.",
	L"error E0047 : 'if' 'elseif' 'case' (corresponding to this line) was not found.",
	L"error E0048 : The parenthesis () has not been closed.",
	L"error E0049 : Internal error (9).",
	L"error E0050 : The label of 'when' must be a constant.",
	L"error E0051 : Missing '{' after 'case'.",
	L"error E0052 : Internal error (10).",
	L"error E0053 : Syntax error.",
	L"error E0054 : Syntax error.",
	L"error E0055 : Internal error (11).",
	L"error E0056 : Internal error (12).",
	L"error E0057 : The value of variable has not been saved.",
	L"error E0058 : Internal error (13).",
	L"error E0059 : Invalid expression.",
	L"error E0060 : The parenthesis '%()' has not been closed in embedded string.",
	L"error E0061 : The parenthesis '%()' is empty in embedded string.",
	L"error E0062 : Internal error (14).",
	L"error E0063 : There was no 'case' corresponding to 'when'.",
	L"error E0064 : There was no 'case' corresponding to 'when'.",
	L"error E0065 : Syntax error.",
	L"error E0066 : Syntax error.",
	L"error E0067 : Internal error (15).",
	L"error E0068 : This 'elseif'(or 'when') was not able to processed.",
	L"error E0069 : This 'else'(or 'others') was not able to processed.",
	L"error E0070 : Internal error (16).",
	L"error E0071 : Undefined function is being called.",
	L"error E0072 : Missing name of variable.",
	L"error E0073 : Invalid use of array: array variable was not found.",
	L"error E0074 : Syntax error of preprocessor.",
	L"error E0075 : Syntax error of preprocessor.",
	L"error E0076 : Invalid preprocess directive.",
	L"error E0077 : Internal error (17).",
	L"error E0078 : The parenthesis '%[]' has not been closed in embedded string.",
	L"error E0079 : The parenthesis '%[]' is empty in embedded string.",
	L"error E0080 : Internal error (18).",
	L"error E0081 : This '%[]' is unusual.",
	L"error E0082 : Internal error (19).",
	L"error E0083 : Internal error (20).",
	L"error E0084 : Internal error (21).",		// no use
	L"error E0085 : Internal error (22).",		// no use
	L"error E0086 : Syntax error.",
	L"error E0087 : The position of operator '&' is wrong.",
	L"error E0088 : Internal error (24).",
	L"error E0089 : Internal error (25).",
	L"error E0090 : Internal error (26).",
	L"error E0091 : Internal error (27).",
	L"error E0092 : There was no operator which should follow () or [].",
	L"error E0093 : Invalid single quote exists.",
	L"error E0094 : The parenthesis {} is invalid or has not been closed correctly.",
	L"error E0095 : Tried to load duplicate dictionary.",
};

// ワーニング文字列（英語）
const wchar_t	msgwe[][128] = {
	L"warning W0000 : Syntax error. This line will be ignored.",
	L"warning W0001 : Restoration failed for the variable of this line: not parsable line.",
	L"warning W0002 : Restoration failed for the variable of this line: invalid variable name.",
	L"warning W0003 : Restoration failed for the variable of this line: invalid value or delimiter",
	L"warning W0004 : Restoration failed for the variable of this line: invalid value",
	L"warning W0005 : Restoration failed for the variable of this line: invalid delimiter",
	L"warning W0006 : While processing this line, the internal error occurred. This line is unanalyzable.",
	L"warning W0007 : The value of this variable has not been saved.",
	L"warning W0008 : Missing arguments.",
	L"warning W0009 : Mismatched type of argument(s).",
	L"warning W0010 : Null string is invalid.",
	L"warning W0011 : The result can't be set to the variable.",
	L"warning W0012 : The numeric value is out of range or invalid.",
	L"warning W0013 : Processing failed.",
	L"warning W0014 : Requested library isn't loaded.",
	L"warning W0015 : Requested file isn't opened.",
	L"warning W0016 : Syntax error in regular expression.",
	L"warning W0017 : Unknown error occurred at regular expression.",
	L"warning W0018 : It is necessary to specify the variable.",
	L"warning W0019 : Integer/Double divided by zero.",
	L"warning W0020 : Requires even number of elements to create hash.",
	L"warning W0021 : Empty here document found.",
	L"warning W0022 : Here document start sequence found.",
};

// 注記文字列（英語）
const wchar_t	msgne[][128] = {
	L"note N0000 : Restoration of a variable value was not performed.",
	L"note N0001 : Invalid character code set was specified. Using the default setting of OS instead.",
};

// その他のログ文字列（英語）
const wchar_t	msge[][96] = {
	L"// AYA request log\n// load time: ",
	L"// unload time: ",
	L"",	// 欠番
	L"// Loading of dictionary file\n",
	L"// Parser report (@:function,$:system function,#:array)\n\n",
	L"// Variable definition report\n",
	L"// Loading of variable value ",
	L"// Saving of variable value ",
	L"...Done.\n\n",
	L"// Parsing of dictionary\n",
	L"// Dictionary error found : Emergency mode loading...\n",
	L"// Dictionary error found : Emergency mode loaded.\n",
};
*/
//----

#endif
