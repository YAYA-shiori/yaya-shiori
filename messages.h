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

#if !defined(POSIX) && !defined(__MINGW32__)
// フェータルエラー文字列（日本語）
const wchar_t	msgfj[][128] = {
	L"",
};

// エラー文字列（日本語）
const wchar_t	msgej[][128] = {
	L"error E0000 : 未知のエラーです.",
	L"error E0001 : 対応する関数名が見つかりません.",
	L"error E0002 : '}' 過多です.",
	L"error E0003 : 不正な関数名です.",
	L"error E0004 : 解析不能. '{' が必要と判断されます.",
	L"error E0005 : ファイルを開けません.",
	L"error E0006 : 簡易配列の変数名がありません.",
	L"error E0007 : 不完全な文字列定数です.",
	L"error E0008 : 不正なダブルクォーテーションです.",
	L"error E0009 : 内部エラー1が発生しました.",
	L"error E0010 : 解析不能な文字列.",
	L"error E0011 : 変数名(?)に使用できない文字が含まれています.",
	L"error E0012 : 変数名が予約語と一致しています.",
	L"error E0013 : 同一名の関数が重複して定義されています.",
	L"error E0014 : 簡易配列の序数が空です.",
	L"error E0015 : 内部エラー2が発生しました.",
	L"error E0016 : 内部エラー3が発生しました.",
	L"error E0017 : 文字列中に埋め込まれた簡易配列の変数名がありません.",
	L"error E0018 : 文字列中に埋め込まれた簡易配列の序数が空です.",
	L"error E0019 : 式の()が閉じていません.",
	L"error E0020 : 簡易配列の[]が不正、もしくは閉じていません.",
	L"error E0021 : 内部エラー4が発生しました.",
	L"error E0022 : 数式の記述に誤りがあります.",
	L"error E0023 : ','で区切られた要素を正しく取得できません.",
	L"error E0024 : ','で区切られた要素を正しく取得できません.",
	L"error E0025 : ','で区切られた引数に対応する関数が見つかりません.",
	L"error E0026 : 内部エラー5が発生しました.",
	L"error E0027 : 有効な要素が何も記述されていません.",
	L"error E0028 : 内部エラー23が発生しました.",
	L"error E0029 : この代入は処理できません.",
	L"error E0030 : 不正な重複回避モードが指定されました.",
	L"error E0031 : 内部エラー6が発生しました.",
	L"error E0032 : 必須の関数がありません.",		// no use
	L"error E0033 : 内部エラー7が発生しました.",
	L"error E0034 : 内部エラー8が発生しました.",
	L"error E0035 : 'if'に続く'{'がありません.",
	L"error E0036 : 'elseif'に続く'{'がありません.",
	L"error E0037 : 'else'に続く'{'がありません.",
	L"error E0038 : 'switch'に続く'{'がありません.",
	L"error E0039 : 'while'に続く'{'がありません.",
	L"error E0040 : 'for'の終了条件式が異常です.",
	L"error E0041 : 'for'のループ式が異常です.",
	L"error E0042 : 'for'に続く'{'がありません.",
	L"error E0043 : 'foreach'の値取得変数が不正です.",
	L"error E0044 : 'foreach'に続く'{'がありません.",
	L"error E0045 : 'when'ラベルに定数でない要素が記述されています.",
	L"error E0046 : 'when'ラベルには、変数/関数を埋め込んだ文字列は使用できません.",
	L"error E0047 : 対応する'if'、'elseif'もしくは'case'が見つかりません.",
	L"error E0048 : 式の()が閉じていません.",
	L"error E0049 : 内部エラー9が発生しました.",
	L"error E0050 : 'when'ラベルに定数でない要素が記述されています.",
	L"error E0051 : 'case'に続く'{'がありません.",
	L"error E0052 : 内部エラー10が発生しました.",
	L"error E0053 : 'when'ラベルの書式が不正です.",
	L"error E0054 : 'when'ラベルの書式が不正です.",
	L"error E0055 : 内部エラー11が発生しました.",
	L"error E0056 : 内部エラー12が発生しました.",
	L"error E0057 : 変数値をファイルに保存できません.",
	L"error E0058 : 内部エラー13が発生しました.",
	L"error E0059 : 数式の記述に誤りがあります.",
	L"error E0060 : 文字列中に\"%()\"で埋め込まれた要素の()が閉じていません.",
	L"error E0061 : 文字列中に\"%()\"で埋め込まれた要素が空です.",
	L"error E0062 : 内部エラー14が発生しました.",
	L"error E0063 : 'when'に対応する'case'がありません.",
	L"error E0064 : 'when'に対応する'case'がありません.",
	L"error E0065 : 'when'ラベルの書式が不正です.",
	L"error E0066 : 'when'ラベルの書式が不正です.",
	L"error E0067 : 内部エラー15が発生しました.",
	L"error E0068 : この'elseif'もしくは'when'を処理できませんでした.",
	L"error E0069 : この'else'もしくは'others'を処理できませんでした.",
	L"error E0070 : 内部エラー16が発生しました.",
	L"error E0071 : 存在しない関数を実行しようとしています.",
	L"error E0072 : 簡易配列の変数名がありません.",
	L"error E0073 : 簡易配列の使用法に不正があります. 変数が見つかりません.",
	L"error E0074 : プリプロセッサの書式に誤りがあります.",
	L"error E0075 : プリプロセッサの書式に誤りがあります.",
	L"error E0076 : 不正なプリプロセス名です.",
	L"error E0077 : 内部エラー17が発生しました.",
	L"error E0078 : 文字列中に'%[]'で埋め込まれた要素の[]が閉じていません.",
	L"error E0079 : 文字列中に'%[]'で埋め込まれた要素が空です.",
	L"error E0080 : 内部エラー18が発生しました.",
	L"error E0081 : '%[]'に先行する関数、もしくは変数の呼び出しがありません.",
	L"error E0082 : 内部エラー19が発生しました.",
	L"error E0083 : 内部エラー20が発生しました.",
	L"error E0084 : 内部エラー21が発生しました.",		// no use
	L"error E0085 : 内部エラー22が発生しました.",		// no use
	L"error E0086 : 書式エラー.",
	L"error E0087 : フィードバック演算子の位置が不正です.",
	L"error E0088 : 内部エラー24が発生しました.",
	L"error E0089 : 内部エラー25が発生しました.",
	L"error E0090 : 内部エラー26が発生しました.",
	L"error E0091 : 内部エラー27が発生しました.",
	L"error E0092 : ()、もしくは[]に続く演算子が見つかりません.",
	L"error E0093 : 不正なシングルクォーテーションです.",
	L"error E0094 : 辞書内部で{}が不正、もしくは閉じていません.",
};

// ワーニング文字列（日本語）
const wchar_t	msgwj[][128] = {
	L"warning W0000 : 文法エラー. この行は無視します.",
	L"warning W0001 : この行の変数の復元に失敗しました. この行を解析できません.",
	L"warning W0002 : この行の変数の復元に失敗しました. 変数名を取得できませんでした.",
	L"warning W0003 : この行の変数の復元に失敗しました. 変数値またはデリミタを取得できませんでした.",
	L"warning W0004 : この行の変数の復元に失敗しました. 変数値が不正です.",
	L"warning W0005 : この行の変数の復元に失敗しました. デリミタを取得できませんでした.",
	L"warning W0006 : この行を処理中に内部エラーが発生しました. この行を解析できません.",
	L"warning W0007 : この変数の値を保存できませんでした.",
	L"warning W0008 : 引数が不足しています.",
	L"warning W0009 : 引数の型が不正です.",
	L"warning W0010 : 空文字は指定できません.",
	L"warning W0011 : 結果を変数に対して設定できません.",
	L"warning W0012 : 指定された値は範囲外、もしくは無効です.",
	L"warning W0013 : 処理に失敗しました.",
	L"warning W0014 : 指定されたライブラリはロードされていません.",
	L"warning W0015 : 指定されたファイルはオープンしていません.",
	L"warning W0016 : 正規表現に誤りがあります. 処理されませんでした.",
	L"warning W0017 : 正規表現処理中に未定義のエラーが発生しました.",
	L"warning W0018 : 変数を指定してください.",
	L"warning W0019 : 割り算にゼロを指定することはできません.",
	L"warning W0020 : 連想配列の生成には偶数個の要素が必要です.",
	L"warning W0021 : 空のヒアドキュメントがあります.",
	L"warning W0022 : ヒアドキュメント開始文字列が(ヒアドキュメント中に)見つかりました.",
};

// 注記文字列（日本語）
const wchar_t	msgnj[][128] = {
	L"note N0000 : 前回実行時の変数値の復元は行われませんでした.",
	L"note N0001 : 無効な文字コードセットが指定されました. OSのデフォルト値を使用します.",
};

// その他のログ文字列（日本語）
const wchar_t	msgj[][96] = {
	L"// 文　リクエストログ\n// load 時刻 : ",
	L"// unload 時刻 : ",
	L"",	// 欠番
	L"// 辞書ファイル読み込み\n",
	L"// 構文解析結果のレポート (@:関数、$:システム関数、#:配列)\n\n",
	L"// 変数定義状態レポート\n",
	L"// 変数値の復元 ",
	L"// 変数値の保存 ",
	L"...完了.\n\n",
	L"// 辞書の構文解析と中間コード生成\n",
	L"// 辞書エラーを検出：緊急モードで再読み込み中...\n",
	L"// 辞書エラーを検出：緊急モードで再読み込みしました\n",
};
#else
/* 残念ながら、ワイド文字の埋め込みは文字コードを明示的に指定する方法が無いため
   移植性がありません(埋め込む代わりにメッセージファイルを別に持っておく、
   というのが現在の主流)。なので日本語メッセージモードでも英語を出します…
 */
#define msgfj msgfe
#define msgej msgee
#define msgwj msgwe
#define msgnj msgne
#define msgj  msge
#endif

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
	L"error E0008 : Invalid doublequote exists.",
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
	L"error E0048 : The parenthesis () has not brrn closed.",
	L"error E0049 : Internal error (9).",
	L"error E0050 : The label of 'when' must be a constant.",
	L"error E0051 : Missing '{' after 'case'.",
	L"error E0052 : Internal error (10).",
	L"error E0053 : Syntex error.",
	L"error E0054 : Syntex error.",
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
	L"error E0065 : Syntex error.",
	L"error E0066 : Syntex error.",
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
	L"error E0093 : Invalid singlequote exists.",
	L"error E0094 : The parenthesis {} is invalid or has not been closed correctly.",
};

// ワーニング文字列（英語）
const wchar_t	msgwe[][128] = {
	L"warning W0000 : Syntax error. This line will be ignored.",
	L"warning W0001 : Restoration failed for the variable of this line: unparsable line.",
	L"warning W0002 : Restoration failed for the variable of this line: invalid variable name.",
	L"warning W0003 : Restoration failed for the variable of this line: invalid value or delimiter",
	L"warning W0004 : Restoration failed for the variable of this line: invalid value",
	L"warning W0005 : Restoration failed for the variable of this line: invalid delimiter",
	L"warning W0006 : While processing this line, the internal error occurred. This line is unanalyzable.",
	L"warning W0007 : The value of this variable has not been saved.",
	L"warning W0008 : Missing arguments.",
	L"warning W0009 : Mismatced type of argument(s).",
	L"warning W0010 : Null string is invalid.",
	L"warinig W0011 : The result can't be set to the variable.",
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

//----

#endif
