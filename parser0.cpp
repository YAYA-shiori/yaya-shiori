// 
// AYA version 5
//
// 構文解析/中間コードの生成を行うクラス　CParser0
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>
#include <vector>

#include "parser0.h"

#include "ayavm.h"

#include "value.h"
#include "variable.h"

#include "basis.h"
#include "comment.h"
#include "function.h"
#include "log.h"
#include "messages.h"
#include "misc.h"
#include "parser1.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "sysfunc.h"
#include "wsex.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::Parse
 *  機能概要：  指定された辞書ファイル群を読み取り、実行可能な関数群を作成します
 *
 *  返値　　：  0/1=正常/エラー
 * -----------------------------------------------------------------------
 */
char	CParser0::Parse(int charset, const std::vector<CDic1>& dics)
{
	// 読み取り、構文解析、中間コードの生成
	vm.logger().Message(3);
	std::vector<CDefine> &gdefines =vm.gdefines();

	size_t errcount = 0;
	
	// すべて一旦読み込んでから…
	for(std::vector<CDic1>::const_iterator it = dics.begin(); it != dics.end(); it++) {
		vm.logger().Write(L"// ");
		vm.logger().Filename(it->path);

		if ( IsDicFileAlreadyExist(it->path) ) {
			vm.logger().Error(E_E, 95, it->path);
			continue; //skip it
		}

		char err = LoadDictionary1(it->path, gdefines, it->charset);
		if ( err == 2 ) {
			; //NOOP skip it
		}
		else if ( err != 0 ) {
			errcount += 1;
		}
	}

	// チェックコードを仕掛ける (関数がないエラー対策)
	vm.logger().Message(8);
	vm.logger().Message(9);

	for(std::vector<CDic1>::const_iterator it = dics.begin(); it != dics.end(); it++) {
		errcount += ParseAfterLoad(it->path);
	}

	vm.logger().Message(8);
	
	return bool(errcount != 0);
}

bool	CParser0::ParseAfterLoad(const yaya::string_t &dicfilename)
{
	int aret=0;

	aret += AddSimpleIfBrace(dicfilename);

	aret += SetCellType(dicfilename);
	aret += MakeCompleteFormula(dicfilename);

	// 中間コード生成の後処理と検査
	aret += vm.parser1().CheckExecutionCode(dicfilename);

	return aret != 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ParseEmbedString
 *  機能概要：  文字列を演算可能な数式に変換します
 *
 *  返値　　：  0/1=正常/エラー
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbedString(yaya::string_t& str, CStatement& st, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	// 文字列を数式に成形
	if (!StructFormula(str, st.cell(), dicfilename, linecount))
		return 1;

	// 数式の項の種類を判定
	if ( st.cell_size() ) { //高速化用
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++) {
			if (it->value_GetType() != F_TAG_NOP)
				continue;

			if (SetCellType1(*it, 0, dicfilename, linecount))
				return 1;
		}
	}

	// ()、[]の正当性判定
	if (CheckDepth1(st, dicfilename))
		return 1;

	// 埋め込み要素の分解
	if (ParseEmbeddedFactor1(st, dicfilename))
		return 1;

	// シングルクォート文字列を通常文字列へ置換
	ConvertPlainString1(st, dicfilename);

	// 演算順序決定
	if (CheckDepthAndSerialize1(st, dicfilename))
		return 1;

	// 後処理と検査
	if (vm.parser1().CheckExecutionCode1(st, dicfilename))
		return 1;

	return 0;
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::DynamicLoadDictionary
 *  機能概要：  DICLOADの実装本体
 *
 *  返値　　：  0=正常 1=文法エラー 5=ファイルなし 95=重複
 * -----------------------------------------------------------------------
 */
int CParser0::DynamicLoadDictionary(const yaya::string_t& dicfilename, int charset)
{
	if ( IsDicFileAlreadyExist(dicfilename) ) {
		vm.logger().Error(E_E, 95, dicfilename);
		return 95;
	}

	vm.func_parse_new();

	std::vector<CDefine> old_define = vm.gdefines();

	char t = LoadDictionary1(dicfilename,vm.gdefines(),charset);
	if ( t == 2 ) {
		vm.func_parse_destruct();
		vm.gdefines() = old_define;
		return 5;
	}

	if ( t == 0 ) {
		t += ParseAfterLoad(dicfilename);
	}

	if ( t == 0 ) { //success
		vm.func_parse_to_exec();
	}
	else { //error
		vm.func_parse_destruct();
		vm.gdefines() = old_define;
	}

	return t != 0;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::DynamicAppendRuntimeDictionary
 *  機能概要：  APPEND_RUNTIME_DICの実装本体
 *
 *  返値　　：  0=正常 1=文法エラー
 * -----------------------------------------------------------------------
 */
int CParser0::DynamicAppendRuntimeDictionary(const yaya::string_t& codes)
{
	vm.func_parse_new();

	bool isnoterror = 1;
	{
		std::vector<yaya::string_t>	factors;
		size_t depth = 0;
		ptrdiff_t targetfunction = -1;
		// {、}、;で分割
		SeparateFactor(factors, yaya::string_t(codes));
		// 分割された文字列を解析して関数を作成し、内部のステートメントを蓄積していく
		if(DefineFunctions(factors, L"_RUNTIME_DIC_", 0, depth, targetfunction)) {
			isnoterror = 0;
		}
		if( depth != 0 ) {
			vm.logger().Error(E_E, 94, L"APPEND_RUNTIME_DIC", -1);
			isnoterror = 0;
		}
	}

	if(isnoterror) {
		isnoterror &= !ParseAfterLoad(L"_RUNTIME_DIC_");
	}

	if(isnoterror) { //success
		vm.func_parse_to_exec();
	}
	else { //error
		vm.func_parse_destruct();
	}

	return !isnoterror;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::DynamicUnloadDictionary
 *  機能概要：  特定のファイル名に関係する関数とdefineを削除します
 *
 *  返値　　：  0=正常 71=関数行方不明 96=ファイルなし
 * -----------------------------------------------------------------------
 */
int	CParser0::DynamicUnloadDictionary(yaya::string_t dicfilename)
{
	dicfilename = vm.basis().ToFullPath(dicfilename);
	if ( ! IsDicFileAlreadyExist(dicfilename) ) {
		vm.logger().Error(E_E, 96, dicfilename);
		return 96;
	}

	vm.func_parse_new();

	std::vector<CFunction> &functions = vm.function_parse().func;
	std::vector<CFunction>::iterator itf = functions.begin();

	//remove function
	while (itf != functions.end()) {
		if ( itf->dicfilename_fullpath == dicfilename ) {
			itf = functions.erase(itf);
		}
		else {
			++itf;
		}
	}

	vm.function_parse().RebuildFunctionMap();

	int error = 0;

	itf = functions.begin();
	while (itf != functions.end()) {
		error += itf->ReindexUserFunctions();
		++itf;
	}

	if ( error ) {
		vm.func_parse_destruct();
		return 71; //function not found
	}

	//remove globaldefine
	std::vector<CDefine> &gdefines = vm.gdefines();
	std::vector<CDefine>::iterator itg = gdefines.begin();

	while (itg != gdefines.end()) {
		if ( itg->dicfilename_fullpath == dicfilename ) {
			itg = gdefines.erase(itg);
		}
		else {
			++itg;
		}
	}

	vm.func_parse_to_exec();
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::DynamicUndefFunc
 *  機能概要：  特定の関数を削除します
 *
 *  返値　　：  0=正常 1=対象関数なし 71=関数行方不明
 * -----------------------------------------------------------------------
 */
int	CParser0::DynamicUndefFunc(const yaya::string_t& funcname)
{
	vm.func_parse_new();

	std::vector<CFunction> &functions = vm.function_parse().func;
	std::vector<CFunction>::iterator itf = functions.begin();

	//remove function
	int count = 0;

	while (itf != functions.end()) {
		if ( itf->name == funcname ) {
			itf = functions.erase(itf);
			count += 1;
		}
		else {
			++itf;
		}
	}

	if ( count == 0 ) {
		vm.logger().Error(E_E, 1, funcname);
		vm.func_parse_destruct();
		return 1; //function not found
	}

	vm.function_parse().RebuildFunctionMap();

	int error = 0;

	itf = functions.begin();
	while (itf != functions.end()) {
		error += itf->ReindexUserFunctions();
		++itf;
	}

	if ( error ) {
		vm.func_parse_destruct();
		return 71; //function not found
	}

	vm.func_parse_to_exec();
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::IsDicFileAlreadyExist
 *  機能概要：  すでに当該辞書が読み込まれているかの判定
 *
 *  返値　　：  0/1=ない/あった
 * -----------------------------------------------------------------------
 */
char CParser0::IsDicFileAlreadyExist(yaya::string_t dicfilename)
{
	dicfilename = vm.basis().ToFullPath(dicfilename);
	std::vector<CFunction> &functions = vm.function_parse().func;
	std::vector<CFunction>::iterator itf = functions.begin();

	while (itf != functions.end()) {
		if ( itf->dicfilename_fullpath == dicfilename ) {
			return 1;
		}
		++itf;
	}

	std::vector<CDefine> &gdefines = vm.gdefines();
	std::vector<CDefine>::iterator itg = gdefines.begin();

	while (itg != gdefines.end()) {
		if ( itg->dicfilename_fullpath == dicfilename ) {
			return 1;
		}
		++itg;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::LoadDictionary1
 *  機能概要：  一つの辞書ファイルを読み取り、大雑把に構文を解釈して蓄積していきます
 *
 *  返値　　：  2/1/0=ファイルがないエラー/エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::LoadDictionary1(const yaya::string_t& filename, std::vector<CDefine>& gdefines, int charset)
{
	yaya::string_t file = filename;
#if defined(POSIX)
	fix_filepath(file);
#endif
	// ファイルを開く
	FILE	*fp = yaya::w_fopen(file.c_str(), L"r");
	if (fp == NULL) {
		vm.logger().Error(E_E, 5, file);
		return 2;
	}

	// 読み取り
	CComment	comment;
	char	ciphered = IsCipheredDic(file);
	size_t depth = 0;
	ptrdiff_t targetfunction = -1;
	std::vector<CDefine>	defines;
	char	errcount = 0;

	int	 isInHereDocument = 0; //2 = ダブルクオート 1 = シングルクオート
	bool isHereDocumentFirstLine = true;

	yaya::string_t	readline;
	readline.reserve(1000);

	yaya::string_t	linebuffer;
	linebuffer.reserve(2000);

	std::vector<yaya::string_t>	factors;
	int	ret;
	std::string buf;
	buf.reserve(1000);

	for (size_t i = 1; ; i++) {
		// 1行読み込み　暗号化ファイルの場合は復号も行なう
		ret = yaya::ws_fgets(buf, readline, fp, charset, ciphered, i);
		if (ret == yaya::WS_EOF)
			break;

		// 終端の改行を消す
		CutCrLf(readline);
		if ( ! isInHereDocument ) {
			// 不要な空白（インデント等）を消す
			CutSpace(readline);
			// コメント処理
			comment.Process_Top(readline);
			comment.Process(readline);
			// 空行（もしくは全体がコメント行だった）なら次へ
			if (readline.size() == 0)
				continue;
		}
		else {
			// 不要な空白（インデント等）を消す
			CutStartSpace(readline);
		}

		/*--------------------------------------------------------
			ヒアドキュメントのエスケープ
			\xFFFF\x0001 -> 改行
			\xFFFF\x0002 -> "
			\xFFFF\x0003 -> '
			\xFFFFはUnicodeとして不適切、現れることはない…たぶん
		--------------------------------------------------------*/
		
		// 読み取り済バッファへ結合
		if ( isInHereDocument ) {
			//ヒアドキュメント解除部
			if ( isInHereDocument == 1 ) {
				if (readline.compare(0,3,L"'>>") == 0) {
					readline.erase(1,3);
					isInHereDocument = 0;
					
					if ( isHereDocumentFirstLine ) {
						linebuffer.append(L"'' ");
						vm.logger().Error(E_W, 21, filename, i);
					}

					linebuffer.append(readline);
				}
			}
			else {
				if (readline.compare(0,3,L"\">>") == 0) {
					readline.erase(1,3);
					isInHereDocument = 0;
					
					if ( isHereDocumentFirstLine ) {
						linebuffer.append(L"'' ");
						vm.logger().Error(E_W, 21, filename, i);
					}

					linebuffer.append(readline);
				}
			}

			//解除されていない（ヒアドキュメント内＝テキストをそのまんま結合）
			if ( isInHereDocument ) {
				if ( isHereDocumentFirstLine ) {
					isHereDocumentFirstLine = false;
				}
				else {
					linebuffer.append(L"\xFFFF\x0001");
				}

				yaya::string_t::size_type it1 = find_last_str(readline,L"<<'");
				yaya::string_t::size_type it2 = find_last_str(readline,L"<<\"");

				if ( (it1 != yaya::string_t::npos) || (it2 != yaya::string_t::npos) ) {
					yaya::string_t::size_type it = it1;
					if ( it == yaya::string_t::npos ) {
						it = it2;
					}
					else {
						if ( (it2 != yaya::string_t::npos) && (it1 < it2) ) {
							it = it2;
						}
					}

					it += 3;

					bool is_not_space = false;
					yaya::string_t::size_type itend = readline.size();

					while ( it < itend ) {
						if ( ! IsSpace(readline[it]) ) {
							is_not_space = true;
							break;
						}
						it += 1;
					}

					if ( ! is_not_space ) {
						vm.logger().Error(E_W, 22, filename, i);
					}
				}

				if ( isInHereDocument == 1 ) {
					yaya::ws_replace(readline, L"\'", L"\xFFFF\x0003");
				}
				else {
					yaya::ws_replace(readline, L"\"", L"\xFFFF\x0002");
				}
				linebuffer.append(readline);

				continue;
			}
		}
		else {
			linebuffer.append(readline);
			// 終端が"/"なら結合なので"/"を消して次を読む
			if (readline[readline.size() - 1] == L'/') {
				linebuffer.erase(linebuffer.end() - 1);
				continue;
			}
			//ヒアドキュメント開始判定
			else if ( readline.size() >= 3 ) {
				if ( readline.compare(readline.size()-3,3,L"<<'") == 0 ) {
					isInHereDocument = 1;
					isHereDocumentFirstLine = true;
					
					linebuffer.erase(linebuffer.size() - 3,2);
					continue;
				}
				else if ( readline.compare(readline.size()-3,3,L"<<\"") == 0 ) {
					isInHereDocument = 2;
					isHereDocumentFirstLine = true;

					linebuffer.erase(linebuffer.size() - 3,2);
					continue;
				}
			}
		}

		// プリプロセッサの場合は取得
		int	pp = GetPreProcess(linebuffer, defines, gdefines, file, i);
		// プリプロセッサであったらこの行の処理は終わり、次へ
		// 異常なプリプロセス行はエラー
		if (pp == 1)
			continue;
		else if (pp == 2) {
			errcount = 1;
			continue;
		}
		// プリプロセッサ#define、#globaldefine処理
		ExecDefinePreProcess(linebuffer, defines);	// #define
		ExecDefinePreProcess(linebuffer, gdefines);	// #globaldefine
		ExecInternalPreProcess(linebuffer,file,i);
		// コメント処理(2)
		comment.Process_Tail(linebuffer);
		// {、}、;で分割
		factors.clear();
		SeparateFactor(factors, linebuffer);
		// 分割された文字列を解析して関数を作成し、内部のステートメントを蓄積していく
		if (DefineFunctions(factors, file, i, depth, targetfunction)) {
			errcount = 1;
		}
	}

	// ファイルを閉じる
	::fclose(fp);

	if ( depth != 0 ) {
		vm.logger().Error(E_E, 94, filename, -1);
		errcount = 1;
	}

	return errcount != 0 ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::GetPreProcess
 *  機能概要：  入力が#define/#globaldefineプリプロセッサであった場合、情報を取得します
 *
 *  返値　　：  0/1/2=プリプロセスではない/プリプロセス/エラー
 * -----------------------------------------------------------------------
 */
char	CParser0::GetPreProcess(yaya::string_t &str, std::vector<CDefine>& defines, std::vector<CDefine>& gdefines, const yaya::string_t& dicfilename,
		ptrdiff_t linecount)
{
#if !defined(POSIX) && !defined(__MINGW32__)
	static const yaya::string_t space_delim(L" \t　");
#else
	static const yaya::string_t space_delim(L" \t\u3000");
#endif

	// 先頭1バイトが"#"であるかを確認
	// （この関数に来るまでに空文字列は除外されているので、いきなり[0]を参照しても問題ない）
	if (str[0] != L'#')
		return 0;

	// デリミタである空白もしくはタブを探す
	ptrdiff_t sep_pos = str.find_first_of(space_delim);
	if (sep_pos == -1) {
		vm.logger().Error(E_E, 74, dicfilename, linecount);
		return 2;
	}
	ptrdiff_t sep_pos_end = sep_pos + 1;
	while (IsSpace(str[sep_pos_end])) { ++sep_pos_end; }

	// こっちは名前と値の区切り
	ptrdiff_t rep_pos = str.find_first_of(space_delim,sep_pos_end);
	if (rep_pos == -1) {
		vm.logger().Error(E_E, 75, dicfilename, linecount);
		return 2;
	}
	ptrdiff_t rep_pos_end = rep_pos + 1;
	while (IsSpace(str[rep_pos_end])) { ++rep_pos_end; }

	// プリプロセス名称、変換前文字列、変換後文字列を取得
	// 取得できなければエラー
	yaya::string_t	pname, bef, aft;

	pname.assign(str, 0, sep_pos);
	CutSpace(pname);

	bef.assign(str, sep_pos_end, rep_pos - sep_pos_end);
	CutSpace(bef);

	aft.assign(str, rep_pos_end, str.size() - rep_pos_end);
	CutSpace(aft);

	//aftはカラでもよい
	if (!pname.size() || !bef.size()) {
		vm.logger().Error(E_E, 75, dicfilename, linecount);
		return 2;
	}

	str.erase(); //行全体が前処理対象だったので消す

	// 種別の判定と情報の保持
	if (pname == L"#define") {
		defines.emplace_back(CDefine(vm, bef, aft, dicfilename));
	}
	else if (pname == L"#globaldefine") {
		gdefines.emplace_back(CDefine(vm, bef, aft, dicfilename));
	}
	else {
		vm.logger().Error(E_E, 76, pname, dicfilename, linecount);
		return 2;
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ExecDefinePreProcess
 *  機能概要：  #define/#globaldefine処理。文字列を置換します
 * -----------------------------------------------------------------------
 */
void	CParser0::ExecDefinePreProcess(yaya::string_t &str, const std::vector<CDefine>& defines)
{
	for(std::vector<CDefine>::const_iterator it = defines.begin(); it != defines.end(); it++) {
		if (str.size() >= it->before.size()) {
			yaya::ws_replace(str, it->before.c_str(), it->after.c_str());
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ExecInternalPreProcess
 *  機能概要：  組み込み定義文字列を置換します。
 * -----------------------------------------------------------------------
 */
void	CParser0::ExecInternalPreProcess(yaya::string_t &str,const yaya::string_t &file, ptrdiff_t line)
{
	if ( str.find_first_of(L"__AYA_SYSTEM_FILE__") != yaya::string_t::npos ) {

		yaya::string_t file_str = file;
		yaya::ws_replace(file_str,vm.basis().GetRootPath().c_str(),L"");
		yaya::ws_replace(file_str,L"\\",L"/");

		yaya::ws_replace(str, L"__AYA_SYSTEM_FILE__", file_str.c_str());
	}

	if ( str.find_first_of(L"__AYA_SYSTEM_LINE__") != yaya::string_t::npos ) {
		yaya::char_t line_str[32];

		yaya::snprintf(line_str,31,L"%d",line);

		yaya::ws_replace(str, L"__AYA_SYSTEM_LINE__", line_str);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::IsCipheredDic
 *  機能概要：  ファイルが暗号化ファイルかをファイル拡張子を見て判断します
 *
 *  返値　　：  1/0=拡張子は.aycである/.aycではない
 * -----------------------------------------------------------------------
 */
char	CParser0::IsCipheredDic(const yaya::string_t& filename)
{
	size_t len = filename.size();
	if (len < 5)
		return 0;

	return ((filename[len - 3] == L'a' || filename[len - 3] == L'A') &&
		    (filename[len - 2] == L'y' || filename[len - 2] == L'Y') &&
		    (filename[len - 1] == L'c' || filename[len - 1] == L'C'))
			? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::SeparateFactor
 *  機能概要：  与えられた文字列を"{"、"}"、";"の位置で分割します。";"は以降不要なので消し込みます
 * -----------------------------------------------------------------------
 */
void	CParser0::SeparateFactor(std::vector<yaya::string_t> &s, yaya::string_t &line)
{
	yaya::string_t::size_type separatepoint = 0;
	yaya::string_t::size_type apoint        = 0;
	yaya::string_t::size_type len           = line.size();
	yaya::string_t::size_type nextdq        = 0;
	char	executeflg    = 0;

	for( ; ; ) {
		// { } ; 発見
		yaya::string_t::size_type newseparatepoint = line.find_first_of(L"{};",separatepoint);
		// 何も見つからなければ終わり
		if (newseparatepoint == yaya::string_t::npos)
			break;

		// 発見位置がダブルクォート内なら無視して先へ進む
		nextdq = IsInDQ(line, separatepoint, newseparatepoint);
		if ( nextdq < IsInDQ_npos) {
			separatepoint = nextdq;
			continue;
		}
		//クオートが閉じてない＝これ以上はみつからない
		if ( nextdq == IsInDQ_runaway ) {
			break;
		}

		// 発見位置の手前の文字列を取得
		if (newseparatepoint > apoint) {
			yaya::string_t	tmpstr;
			tmpstr.assign(line, apoint, newseparatepoint - apoint);
			CutSpace(tmpstr);
			s.emplace_back(tmpstr);
		}
		// 発見したのが"{"もしくは"}"ならこれも取得
		yaya::char_t c = line[newseparatepoint];
		if (c == L'{') {
			s.emplace_back(L"{");
		}
		else if (c == L'}') {
			s.emplace_back(L"}");
		}
		// 検索開始位置を更新
		apoint = separatepoint = newseparatepoint + 1;
		if (separatepoint >= len) {
			executeflg = 1;
			break;
		}
	}

	// まだ文字列が残っているならそれも追加
	if (executeflg == 0) {
		yaya::string_t	tmpstr;
		tmpstr.assign(line, apoint, len - apoint);
		CutSpace(tmpstr);
		s.emplace_back(tmpstr);
	}

	// 元の文字列はクリアする
	line.erase();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::DefineFunctions
 *  機能概要：  文字列群を解析して関数の原型を定義する
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::DefineFunctions(std::vector<yaya::string_t>& s, const yaya::string_t& dicfilename, ptrdiff_t linecount, size_t& depth, ptrdiff_t& targetfunction)
{
	char	retcode = 0;

	for(std::vector<yaya::string_t>::iterator it = s.begin(); it != s.end(); it++) {
		// 空行はスキップ
		if (!(it->size()))
			continue;

		// {}入れ子の深さを見て関数名を検索する
		// 深さが0なら{}の外なので関数名を取得すべき位置である
		if (!depth) {
			// 関数の作成
			if (targetfunction == -1) {
				// 関数名と重複回避オプションを取得
				yaya::string_t	d0, d1;
				if (!Split(*it, d0, d1, L":"))
					d0 = *it;
				// 関数名の正当性検査
				if (IsLegalFunctionName(d0)) {
					if (!it->compare(L"{"))
						vm.logger().Error(E_E, 1, L"{", dicfilename, linecount);
					else if (!it->compare(L"}"))
						vm.logger().Error(E_E, 2, dicfilename, linecount);
					else
						vm.logger().Error(E_E, 3, d0, dicfilename, linecount);
					return 1;
				}
				// 重複回避オプションの判定
				choicetype_t	chtype = CHOICETYPE_RANDOM;
				if (d1.size()) {
					chtype = CSelecter::StringToChoiceType(d1, vm, dicfilename, linecount);
				}
				// 作成
				targetfunction = MakeFunction(d0, chtype, dicfilename, linecount);
				if (targetfunction == -1) {
					vm.logger().Error(E_E, 13, *it, dicfilename, linecount);
					return 1;
				}
				continue;
			}
			// 関数名の次のステートメント　これは"{"でなければならない
			else {
				if (it->compare(L"{")) {
					vm.logger().Error(E_E, 4, dicfilename, linecount);
					return 1;
				}
				// 入れ子の深さを加算
				depth++;
			}
		}
		else {
			if ((*it)[it->size()-1]==L':' && *(it+1)==L"{"){
				*(it+1)=L"";
				*it+=L"{";
			}
			// 関数内のステートメントの定義　{}入れ子の計算もここで行う
			if (!StoreInternalStatement(targetfunction, *it, depth, dicfilename, linecount))
				retcode = 1;
			// 入れ子深さが0になったらこの関数定義から脱出する
			if (!depth)
				targetfunction = -1;
		}
	}

	return retcode;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::MakeFunction
 *  機能概要：  名前を指定して関数を作成します
 *
 *  返値　　：  作成された位置を返します
 *  　　　　　  指定された名前の関数が既に作成済の場合はエラーで、-1を返します
 * -----------------------------------------------------------------------
 */
ptrdiff_t	CParser0::MakeFunction(const yaya::string_t& name, choicetype_t chtype, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	ptrdiff_t i = vm.function_parse().GetFunctionIndexFromName(name);
	if(i != -1)
		return -1;
/*	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++)
		if (name == it->name)
			return -1;
*/

	vm.function_parse().func.emplace_back(CFunction(vm, name, dicfilename, linecount));
	size_t index = vm.function_parse().func.size()-1;
	vm.function_parse().AddFunctionIndex(name, index);
	
	CFunction& targetfunction = vm.function_parse().func[index];
	targetfunction.statement.emplace_back( CStatement(ST_OPEN, linecount, std_make_shared<CDuplEvInfo>(chtype)) );
	
	m_BlockhHeaderOfProcessingIndexStack.clear();
	m_BlockhHeaderOfProcessingIndexStack.emplace_back(0);
	m_defaultBlockChoicetypeStack.clear();
	m_defaultBlockChoicetypeStack.emplace_back(CSelecter::GetDefaultBlockChoicetype(chtype));

	return index;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::StoreInternalStatement
 *  機能概要：  関数内のステートメントを種類を判定して蓄積します
 *
 *  返値　　：  0/1=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::StoreInternalStatement(size_t targetfunc, yaya::string_t &str, size_t& depth, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	// パラメータのないステートメント
	CFunction& targetfunction = vm.function_parse().func[targetfunc];

	if(!str.size())
		return 1;
	// {
	if(str[str.length()-1] == L'{') {
		// blockと重複回避オプションを取得
		choicetype_t	chtype = CSelecter::GetDefaultBlockChoicetype(m_defaultBlockChoicetypeStack.back());
		yaya::string_t	d0, d1;
		if (Split(str, d0, d1, L":")){
			chtype = CSelecter::StringToChoiceType(d0, vm, dicfilename, linecount);
		}
		m_defaultBlockChoicetypeStack.emplace_back(chtype);
		depth++;
		targetfunction.statement.emplace_back(CStatement(ST_OPEN, linecount, std_make_shared<CDuplEvInfo>(chtype)));
		m_BlockhHeaderOfProcessingIndexStack.emplace_back(targetfunction.statement.size()-1);
		return 1;
	}
	// }
	else if (str == L"}") {
		m_BlockhHeaderOfProcessingIndexStack.pop_back();
		m_defaultBlockChoicetypeStack.pop_back();
		depth--;
		targetfunction.statement.emplace_back(CStatement(ST_CLOSE, linecount));
		return 1;
	}
	// others　elseへ書き換えてしまう
	else if (str == L"others") {
		targetfunction.statement.emplace_back(CStatement(ST_ELSE, linecount));
		return 1;
	}
	// else
	else if (str == L"else") {
		targetfunction.statement.emplace_back(CStatement(ST_ELSE, linecount));
		return 1;
	}
	// break
	else if (str == L"break") {
		targetfunction.statement.emplace_back(CStatement(ST_BREAK, linecount));
		return 1;
	}
	// continue
	else if (str == L"continue") {
		targetfunction.statement.emplace_back(CStatement(ST_CONTINUE, linecount));
		return 1;
	}
	// return
	else if (str == L"return") {
		targetfunction.statement.emplace_back(CStatement(ST_RETURN, linecount));
		return 1;
	}
	// --
	else if (str == L"--") {
		targetfunction.statement[m_BlockhHeaderOfProcessingIndexStack.back()].ismutiarea = true;
		targetfunction.statement.emplace_back(CStatement(ST_COMBINE, linecount));
		return 1;
	}

	// パラメータのあるステートメント（制御文）
	yaya::string_t	st, par;
	if (!Split(str, st, par, L" "))
		st = str;
	yaya::string_t	t_st, t_par;
	if (!Split(str, t_st, t_par, L"\t"))
		t_st = str;
	if (st.size() > t_st.size()) {
		st  = t_st;
		par = t_par;
	}
	// if
	if (st == L"if") {
		str = par;
		return MakeStatement(ST_IF, targetfunc, str, dicfilename, linecount);
	}
	// elseif
	else if (st == L"elseif") {
		str = par;
		return MakeStatement(ST_ELSEIF, targetfunc, str, dicfilename, linecount);
	}
	// while
	else if (st == L"while") {
		str = par;
		return MakeStatement(ST_WHILE, targetfunc, str, dicfilename, linecount);
	}
	// switch
	else if (st == L"switch") {
		str = par;
		return MakeStatement(ST_SWITCH, targetfunc, str, dicfilename, linecount);
	}
	// for
	else if (st == L"for") {
		str = par;
		return MakeStatement(ST_FOR, targetfunc, str, dicfilename, linecount);
	}
	// foreach
	else if (st == L"foreach") {
		str = par;
		return MakeStatement(ST_FOREACH, targetfunc, str, dicfilename, linecount);
	}
	// case　特殊な名前のローカル変数への代入に書き換えてしまう
	else if (st == L"case") {
		str = PREFIX_CASE_VAR + vm.function_parse().func[targetfunc].name;
		str += yaya::ws_itoa(linecount);
		str += L'=';
		str += par;
		return MakeStatement(ST_FORMULA, targetfunc, str, dicfilename, linecount);
	}
	// when
	else if (st == L"when") {
		str = par;
		return MakeStatement(ST_WHEN, targetfunc, str, dicfilename, linecount);
	}
	// parallel
	else if (st == L"parallel") {
		str = par;
		return MakeStatement(ST_PARALLEL, targetfunc, str, dicfilename, linecount);
	}
	// void
	else if (st == L"void") {
		str = par;
		return MakeStatement(ST_VOID, targetfunc, str, dicfilename, linecount);
	}

	// これまでのすべてにマッチしない文字列は数式と認識される
	return MakeStatement(ST_FORMULA, targetfunc, str, dicfilename, linecount);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::MakeStatement
 *  機能概要：  関数内のステートメントを作成、蓄積します
 *
 *  返値　　：  0/1=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeStatement(int type, size_t targetfunc, yaya::string_t& str, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	if (!str.size()) {
		vm.logger().Error(E_E, 27, dicfilename, linecount);
		return 0;
	}

	CStatement	addstatement(type, linecount);
	if (type == ST_WHEN) {
		if (!StructWhen(str, addstatement.cell(), dicfilename, linecount))
			return 0;
	}
	else {
		if (!StructFormula(str, addstatement.cell(), dicfilename, linecount))
			return 0;
	}

	vm.function_parse().func[targetfunc].statement.emplace_back(addstatement);
	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::StructFormula
 *  機能概要：  文字列を数式の項と演算子に分解し、基本的な整形を行います
 *
 *  返値　　：  0/1=エラー/正常
 *
 *  渡された文字列はこの関数で破壊されます
 * -----------------------------------------------------------------------
 */
char	CParser0::StructFormula(yaya::string_t& str, std::vector<CCell>& cells, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	// 演算子と項に分解　項の種別はこの時点では調べていない
	StructFormulaCell(str, cells);

	// 整形と書式エラーの検出
	char	bracket = 0;
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); ) {
		// 直前が")""]"の場合、次は演算子が来なければならない
		if (bracket) {
			bracket = 0;
			if (it->value_GetType() == F_TAG_NOP) {
				vm.logger().Error(E_E, 92, dicfilename, linecount);
				return 0;
			}
		}
		// 正符号（+の直前の項が無いかあるいは")""]"以外の演算子　この項は消してよい）
		if (it->value_GetType() == F_TAG_PLUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (!F_TAG_ISOUT_OR_NOP(itm->value_GetType())) {
				it = cells.erase(it);
				continue;
			}
		}
		// 負符号（-の直前の項が無いかあるいは")"以外の演算子　"-1*"に整形する）
		if (it->value_GetType() == F_TAG_MINUS) {
			if (it == cells.begin()) {
				it->value_SetType(F_TAG_NOP);
				it->value().s_value = L"-1";
				it++;
				CCell	addcell2(F_TAG_MUL);
				it = cells.insert(it, addcell2);
				it++;
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (!F_TAG_ISOUT_OR_NOP(itm->value_GetType()))
			{
				it->value_SetType(F_TAG_NOP);
				it->value().s_value = L"-1";
				it++;
				CCell	addcell2(F_TAG_MUL);
				it = cells.insert(it, addcell2);
				it++;
				continue;
			}
		}
		// インクリメント（"+=1"に整形する）
		if (it->value_GetType() == F_TAG_INCREMENT) {
			it->value_SetType(F_TAG_PLUSEQUAL);
			it++;
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"1";
			it = cells.insert(it, addcell);
			it++;
			continue;
		}
		// デクリメント（"-=1"に整形する）
		if (it->value_GetType() == F_TAG_DECREMENT) {
			it->value_SetType(F_TAG_MINUSEQUAL);
			it++;
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"1";
			it = cells.insert(it, addcell);
			it++;
			continue;
		}
		// !、&演算子（手前にダミー項0を追加）
		if (it->value_GetType() == F_TAG_FEEDBACK) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 87, dicfilename, linecount);
				return 0;
			}
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"0";
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		if (it->value_GetType() == F_TAG_EXC) {
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = L"0";
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		// 簡易配列序数アクセス演算子（"["直前に付与）
		if (it->value_GetType() == F_TAG_HOOKBRACKETIN) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 6, dicfilename, linecount);
				return 0;
			}
			CCell	addcell(F_TAG_ARRAYORDER);
			it = cells.insert(it, addcell);
			it += 2;
			continue;
		}
		// 関数要素指定演算子（"("直前が演算子でなければ付与）
		if (it->value_GetType() == F_TAG_BRACKETIN) {
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_NOP) {
					CCell	addcell(F_TAG_FUNCPARAM);
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// 空のカッコ（"()"　消す　関数要素指定演算子があった場合はそれも消す）
		// 関数引数2個以上で空の引数（",)"の場合、""を補完）
		if (it->value_GetType() == F_TAG_BRACKETOUT) {
			bracket = 1;
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_BRACKETIN) {
					it = cells.erase(itm);
					it = cells.erase(it);
					if (it != cells.begin()) {
						itm = it;
						itm--;
						if (itm->value_GetType() == F_TAG_FUNCPARAM)
							it = cells.erase(itm);
					}
					continue;
				}
				else if (itm->value_GetType() == F_TAG_COMMA) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = L"\"\"";
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// 空の鉤カッコ（"[]"　エラー）
		if (it->value_GetType() == F_TAG_HOOKBRACKETOUT) {
			bracket = 1;
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_HOOKBRACKETIN) {
					vm.logger().Error(E_E, 14, dicfilename, linecount);
					return 0;
				}
			}
		}
		// 関数引数2個以上で空の引数（"(,"および",,"の場合、""を補完）
		if (it->value_GetType() == F_TAG_COMMA) {
			if (it != cells.begin()) {
				std::vector<CCell>::iterator	itm = it;
				itm--;
				if (itm->value_GetType() == F_TAG_BRACKETIN || itm->value_GetType() == F_TAG_COMMA) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = L"\"\"";
					it = cells.insert(it, addcell);
					it += 2;
					continue;
				}
			}
		}
		// 次の項へ
		it++;
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::StructWhen
 *  機能概要：  文字列を数式の項と演算子に分解し、基本的な整形を行います（when構文用）
 *
 *  返値　　：  0/1=エラー/正常
 *
 *  渡された文字列はこの関数で破壊されます
 * -----------------------------------------------------------------------
 */
char	CParser0::StructWhen(yaya::string_t& str, std::vector<CCell>& cells, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	// 演算子と項に分解　項の種別はこの時点では調べていない
	StructFormulaCell(str, cells);

	// 整形と書式エラーの検出
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); ) {
		// 正符号（+の直前の項が無いかあるいは")"以外の演算子　この項は消してよい）
		if (it->value_GetType() == F_TAG_PLUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP && itm->value_GetType() != F_TAG_BRACKETOUT) {
				it = cells.erase(it);
				continue;
			}
		}
		// 負符号（-の直前の項が無い　次の項に単純に-を付加する）
		if (it->value_GetType() == F_TAG_MINUS) {
			if (it == cells.begin()) {
				it = cells.erase(it);
				it->value().s_value.insert(0, L"-");
				continue;
			}
			std::vector<CCell>::iterator	itm = it;
			itm--;
			if (itm->value_GetType() != F_TAG_NOP && itm->value_GetType() != F_TAG_BRACKETOUT) {
				it = cells.erase(it);
				it->value().s_value.insert(0, L"-");
				continue;
			}
		}
		// デクリメント 正当な形状かを検査の上、- と次項への-付与として処理する
		if (it->value_GetType() == F_TAG_DECREMENT) {
			if (it == cells.begin()) {
				vm.logger().Error(E_E, 65, dicfilename, linecount);
				return 0;
			}
			it->value_SetType(F_TAG_MINUS);
			it++;
			if (it == cells.end()) {
				vm.logger().Error(E_E, 66, dicfilename, linecount);
				return 0;
			}
			it->value().s_value.insert(0, L"-");
			continue;
		}
		// 次の項へ
		it++;
	}
	// ","を"||"へ、"-"を"&&"へ変換する　無効な演算子ではエラー
	for(std::vector<CCell>::iterator it = cells.begin(); it != cells.end(); it++) {
		if (it->value_GetType() == F_TAG_COMMA)
			it->value_SetType(F_TAG_OR);
		else if (it->value_GetType() == F_TAG_MINUS)
			it->value_SetType(F_TAG_AND);
		else if (it->value_GetType() >= F_TAG_ORIGIN && it->value_GetType() < F_TAG_ORIGIN_VALUE) {
			vm.logger().Error(E_E, 50, dicfilename, linecount);
			return 0;
		}
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::StructFormulaCell
 *  機能概要：  文字列を数式の項と演算子に分解します
 *
 *  返値　　：  0/1=エラー/正常
 *
 *  渡された文字列はこの関数で破壊されます
 * -----------------------------------------------------------------------
 */
//#include <iostream>
void	CParser0::StructFormulaCell(yaya::string_t &str, std::vector<CCell> &cells)
{
	yaya::string_t cell_name;
	yaya::string_t bstr;

	for( ; ; ) {
		// 分割位置を取得　最も手前で最も名前が長く、クォートされていない演算子を探す
		ptrdiff_t tagpoint = -1;
		int	tagtype  = 0;
		size_t taglen   = 0;
		
//		wcout << endl << "str: " << str << endl;
//		wcout << "WordMatch:" << endl;

		bool in_dq = 0;
		bool in_sq = 0;
		size_t strlen = str.size();
		for(size_t i = 0; i < strlen; ++i) {
			if (str[i] == L'\"') {
				if (!in_sq)
					in_dq = !in_dq;
				continue;
			}
			if (str[i] == L'\'') {
				if (!in_dq)
					in_sq = !in_sq;
				continue;
			}
			if (in_dq || in_sq)
				continue;

			ptrdiff_t result = -1;
			size_t maxlen = 0;
			for ( size_t r = 0 ; r < FORMULATAG_NUM ; ++r ) {
				if ( formulatag_len[r] <= strlen - i ) {
					if ( str.compare(i,formulatag_len[r],formulatag[r]) == 0 ) {
						if ( maxlen < formulatag_len[r] ) {
							result = r;
							maxlen = formulatag_len[r];
						}
					}
				}
			}

			if ( result >= 0 ) {
				tagtype = result;
				taglen  = formulatag_len[tagtype];
				
				bstr.assign(str, i + taglen, str.size());
				CutSpace(bstr);

				tagpoint = i;

//				wcout << "d0: " << d0 << endl;
//				wcout << "d1: " << d1 << endl;
				break;
			}
		}
/*		wcout << "  tagpoint: " << tagpoint << endl;
		wcout << "  tagtype: " << tagtype << endl;
		wcout << "  taglen: " << taglen << endl;
		wcout << "  bstr: " << bstr << endl;

		wcout << "Aya5:" << endl;
		tagpoint = -1;
		tagtype  = 0;
		taglen   = 0;

		for(size_t i = 0; i < FORMULATAG_NUM; i++) {
		yaya::string_t	d0, d1;
			if (!Split_IgnoreDQ(str, d0, d1, (wchar_t *)formulatag[i]))
				continue;
			int	d_point = d0.size();
			if (tagpoint == -1 ||
				(tagpoint != -1 && ((tagpoint > d_point) || tagpoint == d_point && taglen < formulatag_len[i]))) {
				tagpoint = d_point;
				tagtype  = i;
				taglen   = formulatag_len[i];
				bstr = d1;
//				wcout << "d0: " << d0 << endl;
//				wcout << "d1: " << d1 << endl;
			}
		}

		wcout << "  tagpoint: " << tagpoint << endl;
		wcout << "  tagtype: " << tagtype << endl;
		wcout << "  taglen: " << taglen << endl;
		wcout << "  bstr: " << bstr << endl;
*/
		// 見つからない場合は最後の項を登録して抜ける
		if (tagpoint == -1) {
			CCell	addcell(F_TAG_NOP);
			addcell.value().s_value = str;
			CutSpace(addcell.value().s_value);
			if (addcell.value_const().s_value.size())
				cells.emplace_back(addcell);
			break;
		}
		// 見つかったので登録する
		else {
			// 項の登録　空文字列は登録しない
			if (tagpoint > 0) {
				cell_name.assign(str, 0, tagpoint);
				CutSpace(cell_name);
				if (cell_name.length()) {
					CCell	addcell(F_TAG_NOP);
					addcell.value().s_value = cell_name;
					cells.emplace_back(addcell);
				}
			}
			// 演算子の登録
			cells.emplace_back(CCell(tagtype + F_TAG_ORIGIN));
			// 元の文字列から取り出し済の要素を削る
			str = bstr;
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::AddSimpleIfBrace
 *  機能概要：  if/elseif/else/when/othersの直後に'{'が無い場合、次の行を{}で囲みます
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::AddSimpleIfBrace(const yaya::string_t &dicfilename)
{
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		int	beftype = ST_UNKNOWN;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (beftype == ST_IF ||
				beftype == ST_ELSEIF ||
				beftype == ST_ELSE ||
				beftype == ST_WHEN) {
				if (it2->type != ST_OPEN) {
					// { 追加
					it2 = it->statement.insert(it2, CStatement(ST_OPEN, it2->linecount));
					it2 += 2;
					// } 追加
					it2 = it->statement.insert(it2, CStatement(ST_CLOSE, it2->linecount));
				}
			}
			beftype = it2->type;
		}
	}
			
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::SetCellType
 *  機能概要：  数式の項の種別（リテラル、関数、変数...など）を調べて格納していきます
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::SetCellType(const yaya::string_t &dicfilename)
{
	int	errorflg = 0;

	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			// 数式以外は飛ばす
			if (it2->type < ST_FORMULA)
				continue;

			if ( it2->cell_size() ) { //高速化用
				for(std::vector<CCell>::iterator it3 = it2->cell().begin(); it3 != it2->cell().end(); it3++) {
					// 演算子は飛ばす
					if (it3->value_GetType() != F_TAG_NOP)
						continue;

					// 項種別取得
					errorflg += SetCellType1(*it3, 0, it->dicfilename, it2->linecount);
					// whenの場合、項はリテラルしかあり得ない
					if (it2->type == ST_WHEN) {
						if (it3->value_GetType() != F_TAG_INT && 
							it3->value_GetType() != F_TAG_DOUBLE && 
							it3->value_GetType() != F_TAG_STRING && 
							it3->value_GetType() != F_TAG_STRING_PLAIN && 
							it3->value_GetType() != F_TAG_DOUBLE) {
							vm.logger().Error(E_E, 45, it->dicfilename, it2->linecount);
							errorflg++;
						}
					}
				}
			}
		}
	}
	
	return (errorflg) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::SetCellType1
 *  機能概要：  渡された項の種別（リテラル、関数、変数...など）を設定します
 *  引数　　：  emb 0/1=通常の数式の項/文字列に埋め込まれていた数式の項
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::SetCellType1(CCell& scell, char emb, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	// 関数
	ptrdiff_t i = vm.function_parse().GetFunctionIndexFromName(scell.value_const().s_value);
	if(i >= 0) {
		scell.name  = scell.value_const().s_value;
		scell.value_SetType(F_TAG_USERFUNC);
		scell.index = i;
		scell.value_Delete();
		return 0;
	}

/*
	size_t i = 0;
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++, i++)
		if (!scell.value_const().s_value != it->name) {
			scell.value_SetType(F_TAG_USERFUNC);
			scell.index     = i;
			scell.value_Delete();
//			wcout << "Aya5:" << endl;
//			wcout << "  result: " << i << endl;
			return 0;
		}
*/

	// システム関数
	ptrdiff_t sysidx = CSystemFunction::FindIndex(scell.value_const().s_value);
	if ( sysidx >= 0 ) {
		scell.value_SetType(F_TAG_SYSFUNC);
		scell.index = sysidx;
		scell.name  = scell.value_const().s_value;
		scell.value_Delete();
		return 0;
	}

	// 整数リテラル(DEC)
	if (IsIntString(scell.value_const().s_value)) {
		scell.value() = yaya::ws_atoll(scell.value_const().s_value);
		return 0;
	}
	// 整数リテラル(BIN)
	if (IsIntBinString(scell.value_const().s_value, 1)) {
		scell.value() = yaya::ws_atoll(scell.value_const().s_value, 2);
		return 0;
	}
	// 整数リテラル(HEX)
	if (IsIntHexString(scell.value_const().s_value, 1)) {
		scell.value() = yaya::ws_atoll(scell.value_const().s_value, 16);
		return 0;
	}
	// 実数リテラル
	if (IsDoubleButNotIntString(scell.value_const().s_value)) {
		scell.value() = yaya::ws_atof(scell.value_const().s_value);
		return 0;
	}
	// 文字列リテラル(ダブルクォート)
	i = IsLegalStrLiteral(scell.value_const().s_value);
	if (!i) {
		CutDoubleQuote(scell.value().s_value);
		EscapingInsideDoubleDoubleQuote(scell.value().s_value);
		UnescapeSpecialString(scell.value().s_value);
		
		if (!emb) {
			scell.value_SetType(F_TAG_STRING);
		}
		else {
			if ( scell.value_const().s_value.size() <= 1 ) { //1文字以下ならそもそも埋め込みすらない
				scell.value_SetType(F_TAG_STRING_PLAIN);
			}
			else {
				if (scell.value_const().s_value[0] == L'%') {
					scell.value().s_value.erase(0, 1);
					scell.value_SetType(F_TAG_STRING_EMBED);
				}
				else {
					scell.value_SetType(F_TAG_STRING);
				}
			}
		}
		return 0;
	}
	else if (i == 1) {
		scell.value_Delete();
		vm.logger().Error(E_E, 7, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	else if (i == 2) {
		scell.value_Delete();
		vm.logger().Error(E_E, 8, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	// 文字列リテラル(シングルクォート)
	i = IsLegalPlainStrLiteral(scell.value_const().s_value);
	if (!i) {
		CutSingleQuote(scell.value().s_value);
		EscapingInsideDoubleSingleQuote(scell.value().s_value);
		UnescapeSpecialString(scell.value().s_value);
		scell.value_SetType(F_TAG_STRING_PLAIN);
		return 0;
	}
	else if (i == 1) {
		scell.value_Delete();
		vm.logger().Error(E_E, 7, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	else if (i == 2) {
		scell.value_Delete();
		vm.logger().Error(E_E, 93, scell.value_const().s_value, dicfilename, linecount);
		return 1;
	}
	// ここまで残ったものは変数の候補
	CVariable	addvariable;
	std::string	errstr;
	switch(IsLegalVariableName(scell.value_const().s_value)) {
	case 0:
		// グローバル変数
		scell.value_SetType(F_TAG_VARIABLE);
		scell.index  = vm.variable().Make(scell.value_const().s_value, 1);
		scell.name   = scell.value_const().s_value;
		scell.value_Delete();
		return 0;
	case 16:
		// ローカル変数
		scell.value_SetType(F_TAG_LOCALVARIABLE);
		scell.name   = scell.value_const().s_value;
		scell.value_Delete();
		return 0;
	case 3:
		// エラー　変数名の誤り
		vm.logger().Error(E_E, 10, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	case 4:
		// エラー　使用できない文字を含む
		vm.logger().Error(E_E, 11, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	case 6:
		// エラー　予約語と衝突
		vm.logger().Error(E_E, 12, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	default:
		// 内部エラー
		vm.logger().Error(E_E, 9, scell.value_const().s_value, dicfilename, linecount);
		scell.value_Delete();
		return 1;
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::MakeCompleteFormula
 *  機能概要：  埋め込み要素の式への展開と演算順序の決定を行ないます
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeCompleteFormula(const yaya::string_t &dicfilename)
{
	int	errcount = 0;

	errcount += ParseEmbeddedFactor(dicfilename);
	ConvertPlainString(dicfilename);
	errcount += CheckDepthAndSerialize(dicfilename);

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ParseEmbeddedFactor
 *  機能概要：  "%"で埋め込まれた要素を持つ文字列を分解して数式を作り、元の式と結合します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbeddedFactor(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
		    errcount += ParseEmbeddedFactor1(*it2, it->dicfilename);
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ParseEmbeddedFactor1
 *  機能概要：  "%"で埋め込まれた要素を持つ文字列を分解して数式を作り、元の式と結合します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::ParseEmbeddedFactor1(CStatement& st, const yaya::string_t& dicfilename)
{
	if (st.type < ST_FORMULA)
		return 0;

	int	errcount = 0;

	// 演算順序を崩さないようにするため、%を含む要素を()で囲む
	if ( st.cell_size() ) { //高速化用
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); ) {
			if (it->value_GetType() == F_TAG_STRING) {
				if (it->value_const().s_value.find(L'%') != yaya::string_t::npos) {
					
					it = st.cell().insert(it, CCell(F_TAG_BRACKETIN) );
					it += 2;
					
					it = st.cell().insert(it, CCell(F_TAG_BRACKETOUT) );
					it++;

					if (st.type == ST_WHEN) {
						vm.logger().Error(E_E, 46, dicfilename, st.linecount);
						errcount++;
					}
					continue;
				}
			}
			// 次へ
			it++;
		}
	}

	// 埋め込み要素を加算多項式に分解して元の式の該当位置へ挿入
	if ( st.cell_size() ) { //高速化用
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); ) {
			if (it->value_GetType() == F_TAG_STRING) {
				if (it->value_const().s_value.find(L'%') != yaya::string_t::npos) {
					// 加算多項式へ変換
					int	t_errcount = 0;
					yaya::string_t	linedata = it->value_const().s_value;
					int	res = ConvertEmbedStringToFormula(linedata, dicfilename, st.linecount);
					t_errcount += res;
					if (res) {
						it++;
						continue;
					}
					// 数式の項分解と整形
					std::vector<CCell>	addcells;
					res = 1 - StructFormula(linedata, addcells, dicfilename, st.linecount);
					t_errcount += res;
					if (res) {
						it++;
						continue;
					}
					// 項の種別を設定
					for(std::vector<CCell>::iterator it2 = addcells.begin(); it2 != addcells.end(); it2++) {
						if (it2->value_GetType() != F_TAG_NOP)
							continue;

						t_errcount += SetCellType1(*it2, 1, dicfilename, st.linecount);
					}
					// 元の式の該当位置へ挿入
					if (!t_errcount) {
						it = st.cell().erase(it);
						size_t c_num = addcells.size();
						for(ptrdiff_t i = c_num - 1; i >= 0; i--)
							it = st.cell().insert(it, addcells[i]);
						it += c_num;
						continue;
					}
					errcount += t_errcount;
				}
			}
			// 次へ
			it++;
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ConvertPlainString
 *  機能概要：  シングルクォート文字列を通常文字列へ置換します
 * -----------------------------------------------------------------------
 */
void	CParser0::ConvertPlainString(const yaya::string_t& dicfilename)
{
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
		    ConvertPlainString1(*it2, it->dicfilename);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ConvertPlainString1
 *  機能概要：  シングルクォート文字列を通常文字列へ置換します
 * -----------------------------------------------------------------------
 */
void	CParser0::ConvertPlainString1(CStatement& st, const yaya::string_t& /*dicfilename*/)
{
	if (st.type < ST_FORMULA)
		return;

	if ( st.cell_size() ) { //高速化用
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++)
			if (it->value_GetType() == F_TAG_STRING_PLAIN)
				it->value_SetType(F_TAG_STRING);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::ConvertEmbedStringToFormula
 *  機能概要：  "%"で埋め込まれた要素を持つ文字列を加算多項式へ変換します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */

static void AddDoubleQuoteAndEscape(yaya::string_t &str)
{
	yaya::ws_replace(str, L"\r\n", L"\xFFFF\x0001");
	yaya::ws_replace(str, L"\"", L"\xFFFF\x0002");
	AddDoubleQuote(str);
}

char	CParser0::ConvertEmbedStringToFormula(yaya::string_t& str, const yaya::string_t& dicfilename, ptrdiff_t linecount)
{
	yaya::string_t	resstr;
	ptrdiff_t nindex = -1;
	for(size_t nfirst = 0; ; nfirst++) {
		// "%"の発見
		ptrdiff_t p_pers = str.find(L'%', 0);
		if (p_pers == -1) {
			vm.logger().Error(E_E, 55, dicfilename, linecount);
			return 1;
		}
		// 加算演算子を追加
		if (nfirst)
			resstr += L'+';
		// 先行する文字列項を追加
		if (p_pers > 0) {
			yaya::string_t	prestr;
			prestr.assign(str, 0, p_pers);
			AddDoubleQuoteAndEscape(prestr);
			resstr += prestr;
			str.erase(0, p_pers);
			resstr += L'+';
		}
		// "%"しか残らなかったらそれで終わり
		if (str.size() == 1) {
			resstr += L"\"%\"";
			break;
		}
		// "%"の次が"("なら長さ指定付きの埋め込みなのでそれを抜き出す
		if (str[1] == L'(') {
			// 抜き出し位置検索
			size_t bdepth = 1;
			size_t len = str.size();
			size_t spos = 0;
			size_t validlen = 0;
			for(spos = 2; spos < len; spos++) {
				bdepth += ((str[spos] == L'(') - (str[spos] == L')'));
				if (!bdepth)
					break;
				if (str[spos] != L' ' && str[spos] != L'\t')
					validlen++;
			}
			if (spos < len) {
				spos++;
			}
			// エラー処理
			if (bdepth != 0) {
				vm.logger().Error(E_E, 60, dicfilename, linecount);
				return 1;
			}
			if (validlen == 0) {
				vm.logger().Error(E_E, 61, dicfilename, linecount);
				return 1;
			}
			if (spos < 2) {
				vm.logger().Error(E_E, 62, dicfilename, linecount);
				return 1;
			}
			// 埋め込み要素を取り出して追加
			yaya::string_t	embedstr;
			embedstr.assign(str, 1, spos - 1);
			resstr += L"TOSTR";
			resstr += embedstr;
			str.erase(0, spos);
//			nindex++;
			// 次の"%"を探してみる
			p_pers = str.find(L'%', 0);
			// 見つからなければこれが最後の文字列定数項
			if (p_pers == -1) {
				embedstr = str;
				if (embedstr.size()) {
					AddDoubleQuoteAndEscape(embedstr);
					resstr += L'+';
					resstr += embedstr;
				}
				break;
			}
			continue;
		}
		// "%"の次が"["なら結果の再利用（%[n]）なのでそれを抜き出す
		if (str[1] == L'[') {
			// まだ先行する項が無いならエラー
			if (nindex == -1) {
				vm.logger().Error(E_E, 81, dicfilename, linecount);
				return 1;
			}
			// 抜き出し位置検索
			size_t bdepth = 1;
			size_t len = str.size();
			size_t spos = 0;
			size_t validlen = 0;
			for(spos = 2; spos < len; spos++) {
				bdepth += ((str[spos] == L'[') - (str[spos] == L']'));
				if (!bdepth)
					break;
				if (str[spos] != L' ' && str[spos] != L'\t')
					validlen++;
			}
			if (spos < len) {
				spos++;
			}
			// エラー処理
			if (bdepth != 0) {
				vm.logger().Error(E_E, 78, dicfilename, linecount);
				return 1;
			}
			if (validlen == 0) {
				vm.logger().Error(E_E, 79, dicfilename, linecount);
				return 1;
			}
			if (spos < 2) {
				vm.logger().Error(E_E, 80, dicfilename, linecount);
				return 1;
			}
			// 埋め込み要素を取り出し、"結果の再利用処理を行う関数"として追加
			resstr += CSystemFunction::HistoryFunctionName();
			resstr += L'(';
			
			resstr += yaya::ws_itoa(nindex, 10);
			resstr += L"-(";

			resstr.append(str, 2, spos - 3);
			resstr += L"))";

			str.erase(0, spos);
			// 次の"%"を探してみる
			p_pers = str.find(L'%', 0);
			// 見つからなければこれが最後の文字列定数項
			if (p_pers == -1) {
				yaya::string_t embedstr;
				embedstr = str;
				if (embedstr.size()) {
					AddDoubleQuoteAndEscape(embedstr);
					resstr += L'+';
					resstr += embedstr;
				}
				break;
			}
			continue;
		}
		// 通常の"%"
		else {
			// 次の"%"を発見
			p_pers = str.find(L'%', 1);
			nindex++;
			// 見つからなければこれが最後の項
			if (p_pers == -1) {
				yaya::string_t	embedstr = str;
				AddDoubleQuoteAndEscape(embedstr);
				resstr += embedstr;
				break;
			}
			// 見つかったので追加
			yaya::string_t	embedstr;
			embedstr.assign(str, 0, p_pers);
			AddDoubleQuoteAndEscape(embedstr);
			resstr += embedstr;
			str.erase(0, p_pers);
		}
	}

	// 返る
	str = resstr;
	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::CheckDepthAndSerialize
 *  機能概要：  数式のカッコが正しく閉じているか検査、および式の演算順序を求めます
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepthAndSerialize(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	// 数式のカッコ検査
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type < ST_FORMULA) {
				continue;
			}

			errcount += CheckDepth1(*it2, it->dicfilename);
		}
	}

	// whenのif変換の最終処理　仮の数式をifで処理可能な判定式に整形する
	errcount += MakeCompleteConvertionWhenToIf(dicfilename);

	// 演算順序の決定
	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (it2->type < ST_FORMULA) {
				continue;
			}

			errcount += CheckDepthAndSerialize1(*it2, it->dicfilename);
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::MakeCompleteConvertionWhenToIf
 *  機能概要：  whenのif変換の最終処理を行います。仮の数式をifで処理可能な判定式に整形します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::MakeCompleteConvertionWhenToIf(const yaya::string_t& dicfilename)
{
	int	errcount = 0;

	for(std::vector<CFunction>::iterator it = vm.function_parse().func.begin(); it != vm.function_parse().func.end(); it++) {
		if ( it->dicfilename != dicfilename ) { continue; }

		std::vector<yaya::string_t>	caseary;
		yaya::string_t	dmystr;
		caseary.emplace_back(dmystr);
		std::vector<size_t> whencnt;
		whencnt.emplace_back(0);
		ptrdiff_t depth = 0;
		for(std::vector<CStatement>::iterator it2 = it->statement.begin(); it2 != it->statement.end(); it2++) {
			if (depth == -1) {
				vm.logger().Error(E_E, 52, it->dicfilename, it2->linecount);
				errcount++;
				break;
			}
			// {
			if (it2->type == ST_OPEN) {
				depth++;
				yaya::string_t	dmystr;
				caseary.emplace_back(dmystr);
				whencnt.emplace_back(0);
				continue;
			}
			// }
			if (it2->type == ST_CLOSE) {
				caseary[depth] = L"";
				whencnt[depth] = 0;
				depth--;
				continue;
			}
			// case
			if (it2->type == ST_FORMULA) {
				if (it2->cell_size() >= 2) {
					if (it2->cell()[0].value_GetType() == F_TAG_LOCALVARIABLE &&
						it2->cell()[1].value_GetType() == F_TAG_EQUAL) {
						if (!::wcsncmp(PREFIX_CASE_VAR,
							it2->cell()[0].name.c_str(),PREFIX_CASE_VAR_SIZE)) {
							caseary[depth] = it2->cell()[0].name;
							whencnt[depth] = 0;
							continue;
						}
					}
				}
			}
			// when
			if (it2->type == ST_WHEN) {
				ptrdiff_t depthm1 = depth - 1;
				if (depthm1 < 0) {
					vm.logger().Error(E_E, 64, it->dicfilename, it2->linecount);
					errcount++;
					break;
				}
				if (!caseary[depthm1].size()) {
					vm.logger().Error(E_E, 63, it->dicfilename, it2->linecount);
					errcount++;
					break;
				}
				// if/elseifへ変換
				if (!whencnt[depthm1])
					it2->type = ST_IF;
				else
					it2->type = ST_ELSEIF;
				(whencnt[depthm1])++;
				// 仮の数式を判定式に書き換える
				bool i = 0;

				if ( it2->cell_size() ) { //高速化用
					for(std::vector<CCell>::iterator it3 = it2->cell().begin(); it3 != it2->cell().end(); ) {
						// ラベル
						if (!i) {
							if (it3->value_GetType() != F_TAG_INT && 
								it3->value_GetType() != F_TAG_DOUBLE && 
								it3->value_GetType() != F_TAG_STRING && 
								it3->value_GetType() != F_TAG_DOUBLE) {
								vm.logger().Error(E_E, 53, it->dicfilename, it2->linecount);
								errcount++;
								break;
							}
							i = 1;
							it3++;
							// 最後の項　これは必ず==判定
							if (it3 == it2->cell().end()) {
								CCell	addcell1(F_TAG_LOCALVARIABLE);
								addcell1.name    = caseary[depthm1];
								addcell1.value_Delete();
								it3 = it2->cell().insert(it3, addcell1);
								CCell	addcell2(F_TAG_IFEQUAL);
								it3 = it2->cell().insert(it3, addcell2);
								break;
							}
							continue;
						}
						// or/and
						i = 0;
						if (it3->value_GetType() == F_TAG_OR) {
							// or
							CCell	addcell1(F_TAG_LOCALVARIABLE);
							addcell1.name      = caseary[depthm1];
							addcell1.value_Delete();
							it3 = it2->cell().insert(it3, addcell1);
							CCell	addcell2(F_TAG_IFEQUAL);
							it3 = it2->cell().insert(it3, addcell2);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
						}
						else if (it3->value_GetType() == F_TAG_AND) {
							// and
							CCell	addcell1(F_TAG_LOCALVARIABLE);
							addcell1.name      = caseary[depthm1];
							addcell1.value_Delete();
							it3 = it2->cell().insert(it3, addcell1);
							CCell	addcell2(F_TAG_IFLTEQUAL);
							it3 = it2->cell().insert(it3, addcell2);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							CCell	addcell3(F_TAG_IFLTEQUAL);
							it3 = it2->cell().insert(it3, addcell3);
							CCell	addcell4(F_TAG_LOCALVARIABLE);
							addcell4.name      = caseary[depthm1];
							addcell4.value_Delete();
							it3 = it2->cell().insert(it3, addcell4);
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
							if (it3 == it2->cell().end())
								break;
							it3++;
						}
						else {
							vm.logger().Error(E_E, 54, it->dicfilename, it2->linecount);
							errcount++;
							break;
						}
					}
				}
			}
		}
	}

	return (errcount) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::CheckDepth1
 *  機能概要：  数式のカッコが正しく閉じているか検査します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepth1(CStatement& st, const yaya::string_t& dicfilename)
{
	// ()/[]の対応づけを検査
	std::vector<size_t>	hb_depth;
	ptrdiff_t depth = 0;
	if ( st.cell_size() ) { //高速化用
		for(std::vector<CCell>::iterator it = st.cell().begin(); it != st.cell().end(); it++) {
			if (it->value_GetType() == F_TAG_BRACKETIN)
				depth++;
			else if (it->value_GetType() == F_TAG_BRACKETOUT)
				depth--;
			else if (it->value_GetType() == F_TAG_HOOKBRACKETIN)
				hb_depth.emplace_back(depth);
			else if (it->value_GetType() == F_TAG_HOOKBRACKETOUT) {
				size_t gb_depth_size = hb_depth.size();
				if (!gb_depth_size) {
					vm.logger().Error(E_E, 20, dicfilename, st.linecount);
					return 1;
				}
				else if (hb_depth[gb_depth_size - 1] != depth) {
					vm.logger().Error(E_E, 20, dicfilename, st.linecount);
					return 1;
				}
				hb_depth.erase(hb_depth.end() - 1);
			}
		}
	}
	if (depth) {
		vm.logger().Error(E_E, 19, dicfilename, st.linecount);
		return 1;
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CParser0::CheckDepthAndSerialize1
 *  機能概要：  数式の演算順序を決定します
 *
 *  返値　　：  1/0=エラー/正常
 * -----------------------------------------------------------------------
 */
char	CParser0::CheckDepthAndSerialize1(CStatement& st, const yaya::string_t& dicfilename)
{
	// 再度()入れ子の対応を検査しつつ、演算順序算出用のフラグを作成する
	// フラグdepthvecは対応する項の処理状態を示している。
	// -1    処理済もしくは処理対象ではない項
	// -2    処理待ちの項
	// 0以上 演算子、数字が大きいほど優先度が高い

	//ここはintでないとだめ……i--のループで負（＝0xFFFFFFFFになってunsignedだと巨大な値）になる可能性がある
	ptrdiff_t sz = st.cell_size();
	ptrdiff_t i = 0;

	std::vector<ptrdiff_t> depthvec;
	depthvec.reserve(sz);

	size_t depth = 0;
	for(i = 0; i < sz; i++) {
		// 演算子
		int	type = st.cell()[i].value_GetType();
		if (type >= F_TAG_ORIGIN && type < F_TAG_ORIGIN_VALUE) {
			if (F_TAG_ISIN_OR_OUT(type)) {
				depth += formulatag_depth[type];
				depthvec.emplace_back(-1);
			}
			else
				depthvec.emplace_back(depth + formulatag_depth[type]);
			continue;
		}
		// 演算子以外
		depthvec.emplace_back(-2);
	}
	if (depth) {
		vm.logger().Error(E_E, 48, dicfilename, st.linecount);
		return 1;
	}
	if (sz != depthvec.size()) {
		vm.logger().Error(E_E, 21, dicfilename, st.linecount);
		return 1;
	}

	// 演算順序の決定
	for( ; ; ) {
		// 演算対象の演算子を発見
		// 同一深さの演算子の並列は最初のものが選ばれる。つまりAYA5では演算子は常に左から右へ結合される
		// したがって i=j=1 は i=j; j=1 と等価である。i に 1 は代入されない。右から結合されるC/C++とは
		// ここは異なっている。
		ptrdiff_t	t_index = -1;
		ptrdiff_t	t_depth = -1;
		for(i = 0; i < sz; i++)
			if (depthvec[i] > t_depth) {
				t_depth = depthvec[i];
				t_index = i;
			}
		// 対象が無くなったら抜ける
		if (t_depth == -1)
			break;

		// 定義の開始　演算子の登録
		int	t_type = st.cell()[t_index].value_GetType();
		CSerial	addserial(t_index);
		depthvec[t_index] = -2;
		// 左辺の項を取得
		ptrdiff_t f_depth = 1;
		bool out_of_bracket = false;
		for(i = t_index - 1; i >= 0; i--) {
			// カッコ深さ検査
			if (F_TAG_ISIN(st.cell()[i].value_GetType()))
				f_depth--;
			else if (F_TAG_ISOUT(st.cell()[i].value_GetType()))
				f_depth++;
			if (!f_depth) {
				out_of_bracket = true;
				break;
			}
			// 取得
			if (depthvec[i] == -2) {
				addserial.index.emplace_back(i);
				depthvec[i] = -1;
				break;
			}
		}
		if (out_of_bracket) {
			if (t_type == F_TAG_COMMA)
				vm.logger().Error(E_E, 23, dicfilename, st.linecount);
			else
				vm.logger().Error(E_E, 22, dicfilename, st.linecount);
			return 1;
		}
		// 演算子が","の場合、左項のさらに左へ検索を進め、もし引数を関数に渡すための演算子と関数が
		// 見つかった場合は引数つき関数扱いに書き換える。
		// 関数が見つからない場合は通常の配列ということになる
		if (t_type == F_TAG_COMMA) {
			f_depth = 1;
			for( ; i >= 0; i--) {
				// カッコ深さ検査
				if (F_TAG_ISIN(st.cell()[i].value_GetType()))
					f_depth--;
				else if (F_TAG_ISOUT(st.cell()[i].value_GetType()))
					f_depth++;
				if (!f_depth) {
					i--;
					break;
				}
			}
			if (i > 0) {
				if (st.cell()[i].value_GetType() == F_TAG_FUNCPARAM) {
					// 関数
					depthvec[t_index] = -1;
					addserial.tindex = i;
					depthvec[i] = -2;
					i--;
					if (i < 0) {
						vm.logger().Error(E_E, 25, dicfilename, st.linecount);
						return 1;
					}
					if (F_TAG_ISFUNC(st.cell()[i].value_GetType()) && depthvec[i] == -2) {
						addserial.index.insert(addserial.index.begin(), i);
						depthvec[i] = -1;
					}
				}
			}
			//}
			//
			// 右辺の項を取得　演算子が","の場合は列挙されたすべてを一括して取得する
			//if (t_type == F_TAG_COMMA) {
			// ","
			bool gflg = 0;
			f_depth = 1;
			for(i = t_index + 1; i < sz; i++) {
				// カッコ深さ検査
				if (F_TAG_ISIN(st.cell()[i].value_GetType()))
					f_depth++;
				else if (F_TAG_ISOUT(st.cell()[i].value_GetType()))
					f_depth--;
				if (!f_depth)
					break;
				// 取得
				if (depthvec[i] == -2) {
					addserial.index.emplace_back(i);
					depthvec[i] = -1;
					gflg = 1;
					continue;
				}
				else if (depthvec[i] == -1)
					continue;
				else if (st.cell()[i].value_GetType() == F_TAG_COMMA) {
					depthvec[i] = -1;
					continue;
				}

				break;
			}
			if (!gflg) {
				vm.logger().Error(E_E, 24, dicfilename, st.linecount);
				return 1;
			}
		}
		else {
			// ","以外
			for(i = t_index + 1; i < sz; i++) {
				if (depthvec[i] == -2) {
					addserial.index.emplace_back(i);
					depthvec[i] = -1;
					break;
				}
			}
			if (i == sz) {
				vm.logger().Error(E_E, 22, dicfilename, st.linecount);
				return 1;
			}
		}

		// 演算定義を登録
		st.serial().emplace_back(addserial);
	}

	// 演算順序が決定すると、未処理項がひとつだけ残ることになる（これが結果）。これを確認する
	//
	// もし未処理項が演算子でない場合は、有効な項が1つしかないため演算が無かったことを意味している。
	// そのままでは結果が得られないので、「残った項から結果を得る」ことを指示するフラグを追加する
	size_t scount = 0;
	for(i = 0; i < sz; i++) {
		if (depthvec[i] == -2) {
			scount++;
			if (st.cell()[i].value_GetType() >= F_TAG_ORIGIN_VALUE) {
				CSerial	addserial(i);
				addserial.index.emplace_back(0);	// dmy
				addserial.index.emplace_back(0);	// dmy
				st.serial().emplace_back(addserial);
			}
		}
	}
	if (scount != 1) {
		vm.logger().Error(E_E, 86, dicfilename, st.linecount);
		return 1;
	}

	return 0;
}

inline CDefine::CDefine(CAyaVM& vm, const yaya::string_t& bef, const yaya::string_t& aft, const yaya::string_t& df) :
	before(bef), after(aft), dicfilename(df), dicfilename_fullpath(vm.basis().ToFullPath(df))
{
}
