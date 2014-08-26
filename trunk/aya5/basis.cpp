// 
// AYA version 5
//
// 主な制御を行なうクラス　CBasis
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string.h>

#include <iostream>
#include <string>
#include <vector>
#include <stack>

#if defined(POSIX)
# include <dirent.h>
# include <sys/stat.h>
#endif
/*
#  include <cstdlib>
#  include <sys/types.h>
*/
#include "basis.h"

#include "aya5.h"
#include "ccct.h"
#include "comment.h"
#include "file.h"
#include "function.h"
#include "lib.h"
#include "log.h"
#include "logexcode.h"
#include "messages.h"
#include "misc.h"
#include "parser0.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "wsex.h"
#include "ayavm.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


/* -----------------------------------------------------------------------
 * CBasisコンストラクタ
 * -----------------------------------------------------------------------
 */

CBasis::CBasis(CAyaVM &vmr) : vm(vmr)
{
	ResetSuppress();

	checkparser = 0;
	iolog       = 1;
	msglang     = MSGLANG_JAPANESE;

	dic_charset       = CHARSET_SJIS;
	output_charset    = CHARSET_UTF8;
	file_charset      = CHARSET_SJIS;
	save_charset      = CHARSET_UTF8;
	save_old_charset  = CHARSET_SJIS;
	extension_charset = CHARSET_SJIS;

	encode_savefile = false;
	auto_save = true;

#if defined(WIN32)
	hlogrcvWnd  = NULL;
#endif
	run         = 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetModuleHandle
 *  機能概要：  モジュールハンドルを取得します
 *
 *  ついでにモジュールの主ファイル名取得も行います
 * -----------------------------------------------------------------------
 */
void	CBasis::SetModuleName(const yaya::string_t &s)
{
	modulename = s;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetPath
 *  機能概要：  HGLOBALに格納されたファイルパスを取得し、HGLOBALは開放します
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
void	CBasis::SetPath(yaya::global_t h, int len)
{
	// 取得と領域開放
	std::string	mbpath;
	mbpath.assign((char *)h, 0, len);
	GlobalFree(h);
	h = NULL;
	// 文字コードをUCS-2へ変換（ここでのマルチバイト文字コードはOSデフォルト）
	wchar_t	*wcpath = Ccct::MbcsToUcs2(mbpath, CHARSET_DEFAULT);
	path = wcpath;
	free(wcpath);
	wcpath = NULL;
}
#elif defined(POSIX)
void	CBasis::SetPath(yaya::global_t h, int len)
{
    // 取得と領域開放
    path = widen(std::string(h, static_cast<std::string::size_type>(len)));
    free(h);
	h = NULL;
    // スラッシュで終わってなければ付ける。
    if (path.length() == 0 || path[path.length() - 1] != L'/') {
	path += L'/';
    }
    // モジュールハンドルの取得は出来ないので、力技で位置を知る。
    // このディレクトリにある全ての*.dll(case insensitive)を探し、
    // 中身にyaya.dllという文字列を含んでいたら、それを選ぶ。
    // ただし対応する*.txtが無ければdllの中身は見ずに次へ行く。
    modulename = L"yaya";
    DIR* dh = opendir(narrow(path).c_str());
    if (dh == NULL) {
		std::cerr << narrow(path) << "is not a directory!" << std::endl;
	exit(1);
    }
    while (true) {
	struct dirent* ent = readdir(dh);
	if (ent == NULL) {
	    break; // もう無い
	}
	std::string fname(ent->d_name, strlen(ent->d_name)/*ent->d_namlen*/);	// by umeici. 2005/1/16 5.6.0.232
	if (lc(get_extension(fname)) == "dll") {
		std::string txt_file = narrow(path) + change_extension(fname, "txt");
	    struct stat sb;
	    if (::stat(txt_file.c_str(), &sb) == 0) {
		// txtファイルがあるので、中身を見てみる。
		if (file_content_search(narrow(path) + fname, "yaya.dll") != std::string::npos) {
		    // これはYAYAのDLLである。
		    modulename = widen(drop_extension(fname));
		    break;
		}
	    }
	}
    }
    closedir(dh);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetLogRcvWnd
 *  機能概要：  チェックツールから渡されたhWndを保持します
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CBasis::SetLogRcvWnd(long hwnd)
{
	hlogrcvWnd = (HWND)hwnd;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetLogger
 *  機能概要：  ログ機能を初期化 / 再設定します
 * -----------------------------------------------------------------------
 */
void CBasis::SetLogger(void)
{
	vm.logger().Start(logpath, output_charset, msglang, hlogrcvWnd, iolog);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::Configure
 *  機能概要：  load時に行う初期設定処理
 * -----------------------------------------------------------------------
 */
void	CBasis::Configure(void)
{
	// 基礎設定ファイル（例えばaya.txt）を読み取り
	std::vector<CDic1>	dics;
	LoadBaseConfigureFile(dics);
	// 基礎設定ファイル読み取りで重篤なエラーが発生した場合はここで終了
	if (suppress)
		return;

	// ロギングを開始
	SetLogger();

	// 辞書読み込みと構文解析
	if (vm.parser0().Parse(dic_charset, dics))
		SetSuppress();

	{
		CLogExCode logex(vm);

		if (checkparser)
			logex.OutExecutionCodeForCheck();

		// 前回終了時に保存した変数を復元
		RestoreVariable();

		if (checkparser)
			logex.OutVariableInfoForCheck();
	}

	// ここまでの処理で重篤なエラーが発生した場合はここで終了
	if (suppress)
		return;

	// 外部ライブラリとファイルの文字コードを初期化
	vm.libs().SetCharset(extension_charset);
	vm.files().SetCharset(file_charset);

	// load関数を実行
	ExecuteLoad();

	run = 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::Termination
 *  機能概要：  unload時に行う終了処理
 * -----------------------------------------------------------------------
 */
void	CBasis::Termination(void)
{
	// 動作抑止されていなければ終了時の処理を実行
	if (!suppress) {
		// unload
		ExecuteUnload();
		// ロードしているすべてのライブラリをunload
		vm.libs().DeleteAll();
		// 開いているすべてのファイルを閉じる
		vm.files().DeleteAll();
		// 変数の保存
		if ( auto_save ) {
			SaveVariable();
		}
	}

	// ロギングを終了
	vm.logger().Termination();

	//
	loadindex.Init();
	unloadindex.Init();
	requestindex.Init();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::IsSuppress
 *  機能概要：  現在の自律抑止状態を返します
 *
 *  返値　　：  0/1=非抑止/抑止
 *
 *  基礎設定ファイルの読み取りや辞書ファイルの解析中に、動作継続困難なエラーが発生すると
 *  SetSuppress()によって抑止設定されます。抑止設定されると、load/request/unloadでの動作が
 *  すべてマスクされます。この時、requestの返値は常に空文字列になります。（HGLOBAL=NULL、
 *  len=0で応答します）
 * -----------------------------------------------------------------------
 */
char	CBasis::IsSuppress(void)
{
	return suppress;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetSuppress
 *  機能概要：  自律動作抑止を設定します
 * -----------------------------------------------------------------------
 */
void	CBasis::SetSuppress(void)
{
	suppress = 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::ResetSuppress
 *  機能概要：  自律動作抑止機能をリセットします
 * -----------------------------------------------------------------------
 */
void	CBasis::ResetSuppress(void)
{
	suppress = 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::LoadBaseConfigureFile
 *  機能概要：  基礎設定ファイルを読み取り、各種パラメータをセットします
 *
 *  基礎設定ファイルはDLLと同階層に存在する名前が"DLL主ファイル名.txt"のファイルです。
 *
 *  辞書ファイルの文字コードはShift_JIS以外にもUTF-8とOSデフォルトのコードに対応できますが、
 *  この基礎設定ファイルはOSデフォルトのコードで読み取られることに注意してください。
 *  国際化に関して考慮する場合は、このファイル内の記述にマルチバイト文字を使用するべきでは
 *  ありません（文字コード0x7F以下のASCII文字のみで記述すべきです）。
 * -----------------------------------------------------------------------
 */
void	CBasis::LoadBaseConfigureFile(std::vector<CDic1> &dics)
{
	// 設定ファイル("name".txt)読み取り

	// ファイルを開く
	yaya::string_t	filename = path + modulename + L".txt";
	FILE	*fp = yaya::w_fopen(filename.c_str(), L"r");
	if (fp == NULL) {
		SetSuppress();
		return;
	}

	// 読み取り処理
	CComment	comment;
	yaya::string_t	readline;
	yaya::string_t	cmd, param;

	std::stack<FILE*> fpstack;
	std::stack<yaya::string_t> fnstack;
	fpstack.push(fp);
	fnstack.push(filename);

	for (int i = 1; ; i++) {
		// 1行読み込み
		if (yaya::ws_fgets(readline, fp, dic_charset, 0, i) == yaya::WS_EOF) {
			// ファイルを閉じる
			fclose(fpstack.top());
			fpstack.pop();
			fnstack.pop();
			if ( fpstack.empty() ) {
				break;
			}
			fp = fpstack.top();
			filename = fnstack.top();
		}

		// 改行は消去
		CutCrLf(readline);
		// コメントアウト処理
		comment.Process_Top(readline);
		comment.Process(readline);
		comment.Process_Tail(readline);
		// 空行、もしくは全体がコメント行だった場合は次の行へ
		if (readline.size() == 0)
			continue;
		// パラメータを設定
		if (Split(readline, cmd, param, L",")) {
			if ( cmd.compare(L"include") == 0 ) {
				filename = path + param;
				fp = yaya::w_fopen(filename.c_str(), L"r");

				if (fp == NULL) { //エラーが起きたので復旧
					fp = fpstack.top();
					filename = fnstack.top();
				}
				else {
					fpstack.push(fp);
					fnstack.push(filename);
				}
			}
			else {
				SetParameter(cmd, param, &dics);
			}
		}
		else {
			vm.logger().Error(E_W, 0, filename, i);
		}
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SetParameter
 *  機能概要：  LoadBaseConfigureFileから呼ばれます。各種パラメータを設定します
 * -----------------------------------------------------------------------
 */
bool CBasis::SetParameter(const yaya::string_t &cmd, const yaya::string_t &param, std::vector<CDic1> *dics)
{
	// dic
	if ( cmd.compare(L"dic") == 0 && dics) {
		yaya::string_t param1,param2;
		Split(param, param1, param2, L",");

		yaya::string_t	filename = path + param1;

		char cset = dic_charset;
		if ( param2.size() ) {
			char cx = Ccct::CharsetTextToID(param2.c_str());
			if ( cx != CHARSET_DEFAULT ) {
				cset = cx;
			}
		}
		dics->push_back(CDic1(filename,cset));
		return true;
	}
	// log
	if ( cmd.compare(L"log") == 0 ) {
		if ( param.empty() ) {
			logpath.erase();
		}
		else {
			logpath = path + param;
		}
		return true;
	}
	// iolog
	if ( cmd.compare(L"iolog") == 0 ) {
		iolog = param.compare(L"off") != 0;
		return true;
	}
	// セーブデータ暗号化
	if ( cmd.compare(L"save.encode") == 0 ) {
		encode_savefile = param.compare(L"on") == 0;
		return true;
	}
	// 自動セーブ
	if ( cmd.compare(L"save.auto") == 0 ) {
		auto_save = param.compare(L"off") != 0;
		return true;
	}
	// エラーメッセージ言語
	if ( cmd.compare(L"msglang") == 0 ) {
		if (!param.compare(L"english"))
			msglang = MSGLANG_ENGLISH;
		else
			msglang = MSGLANG_JAPANESE;
		return true;
	}
	// charset
	if ( cmd.compare(L"charset") == 0 ) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
		output_charset    = dic_charset;
		file_charset      = dic_charset;
		save_charset      = dic_charset;
		save_old_charset  = dic_charset;
		extension_charset = dic_charset;
		return true;
	}
	// charset
	if ( cmd.compare(L"charset.dic") == 0 ) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	if ( cmd.compare(L"charset.output") == 0 ) {
		output_charset    = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	if ( cmd.compare(L"charset.file") == 0 ) {
		file_charset      = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	if ( cmd.compare(L"charset.save") == 0 ) {
		save_charset      = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	if ( cmd.compare(L"charset.save.old") == 0 ) {
		save_old_charset  = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	if ( cmd.compare(L"charset.extension") == 0 ) {
		extension_charset = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	// fncdepth
	if ( cmd.compare(L"fncdepth") == 0 ) {
		int	f_depth = yaya::ws_atoi(param, 10);
		vm.calldepth().SetMaxDepth((f_depth < 2) ? 2 : f_depth);
		return true;
	}
	// checkparser closed function
	if ( cmd.compare(L"checkparser") == 0 ) {
		checkparser = param.compare(L"on") == 0;
		return true;
	}
	// ignoreiolog
	if ( cmd.compare(L"ignoreiolog") == 0 ){
		vm.logger().AddIgnoreIologString(param);
	}

	return false;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::GetParameter
 *  機能概要：  各種パラメータを文字列で返します
 * -----------------------------------------------------------------------
 */
yaya::string_t CBasis::GetParameter(const yaya::string_t &cmd)
{
	// log
	if (!cmd.compare(L"log")) {
		return logpath;
	}
	// iolog
	else if (!cmd.compare(L"iolog")) {
		return yaya::string_t(iolog ? L"on" : L"off");
	}
	// save.encode
	else if (!cmd.compare(L"save.encode")) {
		return yaya::string_t(encode_savefile ? L"on" : L"off");
	}
	// save.auto
	else if (!cmd.compare(L"save.auto")) {
		return yaya::string_t(auto_save ? L"on" : L"off");
	}
	// msglang
	else if (!cmd.compare(L"msglang")) {
		return yaya::string_t(msglang == MSGLANG_ENGLISH ? L"english" : L"japanese");
	}
	// charset
	else if (!cmd.compare(L"charset")) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	// charset
	else if (!cmd.compare(L"charset.dic")) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	else if (!cmd.compare(L"charset.output")) {
		return Ccct::CharsetIDToTextW(output_charset);
	}
	else if (!cmd.compare(L"charset.file")) {
		return Ccct::CharsetIDToTextW(file_charset);
	}
	else if (!cmd.compare(L"charset.save")) {
		return Ccct::CharsetIDToTextW(save_charset);
	}
	else if (!cmd.compare(L"charset.save.old")) {
		return Ccct::CharsetIDToTextW(save_old_charset);
	}
	else if (!cmd.compare(L"charset.extension")) {
		return Ccct::CharsetIDToTextW(extension_charset);
	}
	// fncdepth
	else if (!cmd.compare(L"fncdepth")) {
		yaya::string_t str;
		return yaya::ws_itoa(vm.calldepth().GetMaxDepth(),10);
	}
	return L"";
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::SaveVariable
 *  機能概要：  変数値をファイルに保存します
 *
 *  ファイル名は"DLL主ファイル名_variable.cfg"です。
 *  ファイルフォーマットは1行1変数、デリミタ半角カンマで、
 *
 *  変数名,内容,デリミタ
 *
 *  の形式で保存されます。内容は整数/実数の場合はそのまま、文字列ではダブルクォートされます。
 *  配列の場合は各要素間がコロンで分割されます。以下に要素数3、デリミタ"@"での例を示します。
 *
 *  var,1:"TEST":0.3,@
 *
 *  デリミタはダブルクォートされません。
 *
 *  基礎設定ファイルで設定した文字コードで保存されます。
 * -----------------------------------------------------------------------
 */
void	CBasis::SaveVariable(const yaya::char_t* pName)
{
	// 変数の保存

	bool ayc = encode_savefile;

	// ファイルを開く
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = GetSavefilePath();
	}
	else {
		filename = path + pName;
	}

	if ( ayc ) {
		char *s_filestr = Ccct::Ucs2ToMbcs(filename,CHARSET_DEFAULT);
#if defined(WIN32)
		DeleteFile(s_filestr);
#else
		unlink(s_filestr);
#endif
		free(s_filestr);
		s_filestr=0;

		filename += L".ays"; //aycだとかぶるので…
	}
	else {
		filename += L".ays"; //aycだとかぶるので…

		char *s_filestr = Ccct::Ucs2ToMbcs(filename,CHARSET_DEFAULT);
#if defined(WIN32)
		DeleteFile(s_filestr);
#else
		unlink(s_filestr);
#endif
		free(s_filestr);
		s_filestr=0;
		
		filename.erase(filename.size()-4,4);
	}

	vm.logger().Message(7);
	vm.logger().Filename(filename);
	FILE	*fp = yaya::w_fopen((wchar_t *)filename.c_str(), L"w");
	if (fp == NULL) {
		vm.logger().Error(E_E, 57, filename);
		return;
	}
/*
#if defined(WIN32)
	// UTF-8の場合は先頭にBOMを保存
	if (charset == CHARSET_UTF8)
		write_utf8bom(fp);
#endif
	// UTF-8なのにBOMを付けるのはやめた方が宜しいかと…
	// トラブルの原因になるので。

	// 了解です。外してしまいます。
	// メモ
	// UTF-8にはバイトオーダーによるバリエーションが存在しないのでBOMは必要ない。
	// 付与することは出来る。しかし対応していないソフトで読めなくなるので付けないほうが
	// 良い。
*/

	// 文字コード
	yaya::string_t str;
	yaya::string_t wstr;
	str.reserve(1000);

	str = L"//savefile_charset,";
	str += Ccct::CharsetIDToTextW(save_charset);
	str += L"\n";

	yaya::ws_fputs(str,fp,save_charset,ayc);

	// 順次保存
	size_t	var_num = vm.variable().GetNumber();

	for(size_t i = 0; i < var_num; i++) {
		CVariable	*var = vm.variable().GetPtr(i);
		// Voidは放置
		if (var->value_const().IsVoid())
			continue;
		// 内容が空文字列の変数は保存しない
		//if (var->value_const().IsStringReal() && !var->value_const().s_value.size())
		//	continue;
		// 内容が空汎用配列の変数は保存しない
		//if (var->value_const().GetType() == F_TAG_ARRAY && !var->value_const().array_size())
		//	continue;
		// 消去フラグが立っている変数は保存しない
		if (var->IsErased())
			continue;

		// 名前の保存
		str = var->name;
		str += L",";

		// 値の保存
		switch(var->value_const().GetType()) {
		case F_TAG_INT:	
			str += yaya::ws_itoa(var->value_const().i_value);
			str += L",";
			break;
		case F_TAG_DOUBLE:
			str += yaya::ws_ftoa(var->value_const().d_value);
			str += L",";
			break;
		case F_TAG_STRING:
			wstr = var->value_const().s_value;
			EscapeString(wstr);
			str += L"\"";
			str += wstr;
			str += L"\",";
			break;
		case F_TAG_ARRAY:
			if (!var->value_const().array_size()) {
				str += ESC_IARRAY L":" ESC_IARRAY;
			}
			else {
				CValueArray::const_iterator	itv;
				CValueArray::const_iterator	itvbegin = var->value_const().array().begin();

				for(itv = itvbegin; itv != var->value_const().array().end(); itv++) {
					if (itv != itvbegin)
						str += L":";
					wstr = itv->GetValueString();
					EscapeString(wstr);

					if (itv->GetType() == F_TAG_STRING) {
						str += L"\"";
						str += wstr;
						str += L"\"";
					}
					else if (itv->GetType() == F_TAG_VOID) {
						str += ESC_IVOID;
					}
					else {
						str += wstr;
					}
				}
				if (var->value_const().array_size() == 1) {
					str += L":" ESC_IARRAY;
				}
			}
			str += L",";
			break;
		case F_TAG_HASH:
			if (!var->value_const().hash_size()) {
				str += ESC_IHASH L"=" ESC_IHASH L":" ESC_IHASH L"=" ESC_IHASH;
			}
			else {
				CValueHash::const_iterator	itv;
				CValueHash::const_iterator	itvbegin = var->value_const().hash().begin();

				for(itv = itvbegin; itv != var->value_const().hash().end(); itv++) {
					if (itv != itvbegin)
						str += L":";

					wstr = itv->first.GetValueString();
					EscapeString(wstr);

					if (itv->first.GetType() == F_TAG_STRING) {
						str += L"\"";
						str += wstr;
						str += L"\"";
					}
					else if (itv->first.GetType() == F_TAG_VOID) {
						str += ESC_IVOID;
					}
					else {
						str += wstr;
					}

					str += L"=";

					wstr = itv->second.GetValueString();
					EscapeString(wstr);

					if (itv->second.GetType() == F_TAG_STRING) {
						str += L"\"";
						str += wstr;
						str += L"\"";
					}
					else if (itv->second.GetType() == F_TAG_VOID) {
						str += ESC_IVOID;
					}
					else {
						str += wstr;
					}
				}
				if (var->value_const().hash_size() == 1) {
					str += L":" ESC_IHASH L"=" ESC_IHASH;
				}
			}
			str += L",";
			break;
		default:
			vm.logger().Error(E_W, 7, var->name);
			break;
		};
		// デリミタの保存
		str += var->delimiter;
		str += L"\n";

		yaya::ws_fputs(str,fp,save_charset,ayc);
	}

	// ファイルを閉じる
	fclose(fp);

	vm.logger().Message(8);
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::RestoreVariable
 *  機能概要：  前回保存した変数内容を復元します
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreVariable(const yaya::char_t* pName)
{
	bool ayc = encode_savefile;

	// ファイルを開く
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = GetSavefilePath();
	}
	else {
		filename = path + pName;
	}

	vm.logger().Message(6);
	vm.logger().Filename(filename);

	FILE *fp = NULL;

	//暗号化セーブファイル対応
	if ( ayc ) {
		filename += L".ays";
		fp = yaya::w_fopen((wchar_t *)filename.c_str(), L"r");
		if (!fp) {
			filename.erase(filename.size()-4,4);
			fp = yaya::w_fopen((wchar_t *)filename.c_str(), L"r");
			if (!fp) {
				vm.logger().Error(E_N, 0);
				return;
			}
			else {
				ayc = false;
			}
		}
	}
	else {
		fp = yaya::w_fopen((wchar_t *)filename.c_str(), L"r");
		if (!fp) {
			filename += L".ays";
			fp = yaya::w_fopen((wchar_t *)filename.c_str(), L"r");
			if (!fp) {
				vm.logger().Error(E_N, 0);
				return;
			}
			else {
				ayc = true;
				encode_savefile = true; //簡単に戻されてしまうのを防止
			}
		}
	}

	// 内容を読み取り、順次復元していく
	yaya::string_t	linebuffer;
	yaya::string_t	readline;
	yaya::string_t	parseline;
	yaya::string_t	varname, value, delimiter;

	char savefile_charset = save_old_charset;

	for (int i = 1; ; i++) {
		// 1行読み込み
		if (yaya::ws_fgets(readline, fp, savefile_charset, ayc, i, false) == yaya::WS_EOF)
			break;
		// 改行は消去
		CutCrLf(readline);
		// 空行なら次の行へ
		if (readline.size() == 0) {
			linebuffer.erase();
			continue;
		}
		// 既に読み取り済の文字列と結合
		linebuffer += readline;
		// ダブルクォーテーションが閉じているか確認する。閉じていない場合は、
		// 次の行へ値が続いていると思われるので次の行の読み取りへ
		if (IsInDQ(linebuffer, 0, linebuffer.size() - 1) != IsInDQ_notindq)
			continue;

		parseline = linebuffer;
		linebuffer.erase();

		// 変数名を取得
		if (!Split_IgnoreDQ(parseline, varname, value, L",")) {
			vm.logger().Error(E_W, 1, filename, i);
			continue;
		}
		// Charset
		if (varname.compare(L"//savefile_charset") == 0) {
			savefile_charset = Ccct::CharsetTextToID(value.c_str());
			continue;
		}
		// 変数名の正当性を検査
		if (IsLegalVariableName(varname)) {
			vm.logger().Error(E_W, 2, filename, i);
			continue;
		}
		// 値とデリミタを取り出す
		parseline = value;
		if (!Split_IgnoreDQ(parseline, value, delimiter, L",")) {
			vm.logger().Error(E_W, 3, filename, i);
			continue;
		}
		// 値をチェックして型を判定
		int	type;

		if (IsIntString(value)) {
			type = F_TAG_INT;
		}
		else if (IsDoubleButNotIntString(value)) {
			type = F_TAG_DOUBLE;
		}
		else if (!IsLegalStrLiteral(value)) {
			type = F_TAG_STRING;
		}
		else {
			if (Find_IgnoreDQ(value,L":") == yaya::string_t::npos) {
				vm.logger().Error(E_W, 4, filename, i);
				continue;
			}
			else {
				type = F_TAG_ARRAY;
				if (Find_IgnoreDQ(value,L"=") != yaya::string_t::npos) {
					type = F_TAG_HASH;
				}
			}
		}
		// デリミタの正当性を検査
		if (!delimiter.size()) {
			vm.logger().Error(E_W, 5, filename, i);
			continue;
		}
		// 変数を作成
		int	index = vm.variable().Make(varname, 0);
		vm.variable().SetType(index, type);
		
		if (type == F_TAG_INT) {
			// 整数型
			vm.variable().SetValue(index, yaya::ws_atoi(value, 10));
		}
		else if (type == F_TAG_DOUBLE) {
			// 実数型
			vm.variable().SetValue(index, yaya::ws_atof(value));
		}
		else if (type == F_TAG_STRING) {
			// 文字列型
			CutDoubleQuote(value);
			UnescapeString(value);
			vm.variable().SetValue(index, value);
		}
		else if (type == F_TAG_ARRAY) {
			// 配列型
			RestoreArrayVariable(*(vm.variable().GetValuePtr(index)), value);
		}
		else if (type == F_TAG_HASH) {
			// 連想配列型
			RestoreHashVariable(*(vm.variable().GetValuePtr(index)), value);
		}
		else {
			vm.logger().Error(E_W, 6, filename, i);
			continue;
		}
		vm.variable().SetDelimiter(index, delimiter);
	}

	// ファイルを閉じる
	fclose(fp);

	vm.logger().Message(8);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::RestoreArrayVariable
 *  機能概要：  RestoreVariableから呼ばれます。配列変数の内容を復元します
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreArrayVariable(CValue &var, yaya::string_t &value)
{
	var.array().clear();

	yaya::string_t	par, remain;
	char splitResult;

	for( ; ; ) {
		splitResult = Split_IgnoreDQ(value, par, remain, L":");
		if (!splitResult) {
			par = value;
		}

		if (par.compare(ESC_IARRAY) != 0) {
			if (par.compare(ESC_IVOID) == 0) {
				var.array().push_back(CValueSub());
			}
			else if (IsIntString(par)) {
				var.array().push_back(CValueSub( yaya::ws_atoi(par, 10) ));
			}
			else if (IsDoubleButNotIntString(par)) {
				var.array().push_back(CValueSub( yaya::ws_atof(par) ));
			}
			else {
				CutDoubleQuote(par);
				UnescapeString(par);
				var.array().push_back(CValueSub(par));
			}
		}

		if (!splitResult) {
			break;
		}
		value = remain;
	}
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::RestoreHashVariable
 *  機能概要：  RestoreVariableから呼ばれます。配列変数の内容を復元します
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreHashVariable(CValue &var, yaya::string_t &value)
{
	var.hash().clear();

	yaya::string_t	par, remain, key, key_value;
	char splitResult;

	for( ; ; ) {
		splitResult = Split_IgnoreDQ(value, par, remain, L":");
		if (!splitResult) {
			par = value;
		}

		if ( Split_IgnoreDQ(par, key, key_value, L"=") ) {
			if (key.compare(ESC_IHASH) != 0) {
				std::pair<CValueSub,CValueSub> kv;

				if (key.compare(ESC_IVOID) == 0) {
					kv.first = CValueSub();
				}
				else if (IsIntString(key)) {
					kv.first = CValueSub( yaya::ws_atoi(key, 10) );
				}
				else if (IsDoubleButNotIntString(key)) {
					kv.first = CValueSub( yaya::ws_atof(key) );
				}
				else {
					CutDoubleQuote(key);
					UnescapeString(key);
					kv.first = CValueSub(key);
				}

				if (key_value.compare(ESC_IVOID) == 0) {
					kv.second = CValueSub();
				}
				if (IsIntString(key_value)) {
					kv.second = CValueSub( yaya::ws_atoi(key_value, 10) );
				}
				else if (IsDoubleButNotIntString(key_value)) {
					kv.second = CValueSub( yaya::ws_atof(key_value) );
				}
				else {
					CutDoubleQuote(key_value);
					UnescapeString(key_value);
					kv.second = CValueSub(key_value);
				}

				var.hash().insert(kv);
			}
		}

		if (!splitResult) {
			break;
		}
		value = remain;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::ExecuteLoad
 *  機能概要：  load関数を実行します
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteLoad(void)
{
	if (IsSuppress() || loadindex.IsNotFound()) {
		return;
	}

	int funcpos = loadindex.Find(vm,L"load");
	if ( funcpos < 0 ) {
		return;
	}

	// 第一引数（dllのパス）を作成
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	CValueSub	arg0(path);
	arg.array().push_back(arg0);
	// 実行　結果は使用しないのでそのまま捨てる
	vm.calldepth().Init();
	CLocalVariable	lvar;
	vm.logger().Io(0, path);
	CValue	result;
	vm.function()[funcpos].Execute(result, arg, lvar);
	yaya::string_t empty;
	vm.logger().Io(1, empty);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::ExecuteRequest
 *  機能概要：  request関数を実行します
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
yaya::global_t	CBasis::ExecuteRequest(yaya::global_t h, long *len, bool is_debug)
{
	if (IsSuppress() || requestindex.IsNotFound()) {
		GlobalFree(h);
		h = NULL;
		*len = 0;
		return NULL;
	}

	int funcpos = requestindex.Find(vm,L"request");

	if ( funcpos < 0 ) {
		GlobalFree(h);
		h = NULL;
		*len = 0;
		return NULL;
	}

	// 入力文字列を取得
	std::string	istr;
	istr.assign((char *)h, 0, (int)*len);
	
	// 第一引数（入力文字列）を作成　ここで文字コードをUCS-2へ変換
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	wchar_t	*wistr = Ccct::MbcsToUcs2(istr, output_charset);
	if (wistr != NULL) {
		CValueSub	arg0 = wistr;
		vm.logger().Io(0, arg0.s_value);
		arg.array().push_back(arg0);
		free(wistr);
		wistr = NULL;
	}
	else {
		vm.logger().Io(0, L"");
	}

	// 実行
	vm.calldepth().Init();
	CLocalVariable	lvar;
	CValue	result;
	vm.function()[funcpos].Execute(result, arg, lvar);

	// 結果を文字列として取得し、文字コードをMBCSに変換
	yaya::string_t	res = result.GetValueString();
	vm.logger().Io(1, res);
	char	*mostr = Ccct::Ucs2ToMbcs(res, output_charset);
	if (mostr == NULL) {
		// 文字コード変換失敗、NULLを返す
		*len = 0;
		GlobalFree(h);
		h = NULL;
		return NULL;
	}

	// 文字コード変換が成功したので、結果をGMEMへコピーして返す
	unsigned int oldlen = *len;
	*len = (long)strlen(mostr);
	unsigned int copylen = *len + 1;

	//ゼロ終端も含む……互換処理のため
	HGLOBAL r_h = NULL;
	if (oldlen >= copylen) {
		r_h = h;
	}
	else {
		GlobalFree(h);
		h = NULL;
		r_h = ::GlobalAlloc(GMEM_FIXED,copylen);
	}

	if (r_h) {
		memcpy(r_h, mostr, copylen);
	}
	free(mostr);
	mostr = NULL;
	return r_h;
}
#elif defined(POSIX)
yaya::global_t	CBasis::ExecuteRequest(yaya::global_t h, long *len, bool is_debug)
{
    if (IsSuppress() || requestindex.IsNotFound()) {
		free(h);
		h = NULL;
		*len = 0;
		return NULL;
    }

	int funcpos = requestindex.Find(vm,L"request");

	if ( funcpos < 0 ) {
		free(h);
		h = NULL;
		*len = 0;
		return NULL;
	}

    // 入力文字列を取得
	std::string istr(h, *len);
    // 第一引数（入力文字列）を作成　ここで文字コードをUCS-2へ変換
    CValue arg(F_TAG_ARRAY, 0/*dmy*/);
    wchar_t *wistr = Ccct::MbcsToUcs2(istr, output_charset);
	
    if (wistr != NULL) {
		CValueSub arg0 = wistr;
		vm.logger().Io(0, arg0.s_value);
		arg.array().push_back(arg0);
		free(wistr);
		wistr = NULL;
    }
    else {
		yaya::string_t empty;
		vm.logger().Io(0, empty);
    }
    
    // 実行
    vm.calldepth().Init();
    CLocalVariable	lvar;

    CValue	result;
	vm.function()[requestindex].Execute(result, arg, lvar);
    
	// 結果を文字列として取得し、文字コードをMBCSに変換
	yaya::string_t	res = result.GetValueString();
    vm.logger().Io(1, res);
    char *mostr = Ccct::Ucs2ToMbcs(res, output_charset);

    if (mostr == NULL) {
		// 文字コード変換失敗、NULLを返す
		*len = 0;
		return NULL;
    }
    
	// 文字コード変換が成功したので、結果をGMEMへコピーして返す
    *len = (long)strlen(mostr);
    char* r_h = static_cast<char*>(malloc(*len));
    memcpy(r_h, mostr, *len);
    free(mostr);
	mostr = NULL;
    return r_h;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasis::ExecuteUnload
 *  機能概要：  unload関数を実行します
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteUnload(void)
{
	if ( IsSuppress() || unloadindex.IsNotFound() ) {
		return;
	}

	int funcpos = unloadindex.Find(vm,L"unload");
	if ( funcpos < 0 ) {
		return;
	}

	// 実行　引数無し　結果は使用しないのでそのまま捨てる
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	vm.calldepth().Init();
	CLocalVariable	lvar;
	yaya::string_t empty;
	vm.logger().Io(0, empty);
	CValue result;
	vm.function()[funcpos].Execute(result, arg, lvar);
	vm.logger().Io(1, empty);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CBasisFuncPos::CBasisFuncPos
 *  機能概要：  関数位置を探し、位置と「探したかどうか」をキャッシュします
 * -----------------------------------------------------------------------
 */
int CBasisFuncPos::Find(CAyaVM &vm,yaya::char_t *name)
{
	if ( is_try_find ) {
		return pos_saved;
	}

	pos_saved = vm.parser0().GetFunctionIndexFromName(name);
	is_try_find = true;

	if ( pos_saved < 0 ) {
		vm.logger().Error(E_W, 32, name);
	}

	return pos_saved;
}

