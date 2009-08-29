// 
// AYA version 5
//
// システム関数を実行するクラス　CSystemFunction
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
# include <shellapi.h>
#endif

#include <algorithm>
#include <exception>
#if defined(POSIX)
# include <fstream>
#endif
#include <string>
#include <vector>
#include <map>

#include <math.h>
#include <stdio.h>
#if defined(POSIX)
# include <dirent.h>
# include <sys/stat.h>
# include <sys/time.h>
#endif

#include <boost/format.hpp>
#if defined(POSIX)
# include <boost/scoped_array.hpp>
#endif

#include "sysfunc.h"
#include "ayavm.h"

#include "basis.h"
#include "ccct.h"
#include "cell.h"
#include "file.h"
#include "function.h"
#include "lib.h"
#include "log.h"
#include "logexcode.h"
#include "messages.h"
#include "misc.h"
#include "parser0.h"
#if defined(POSIX)
#include "posix_utils.h"
#endif
#include "globaldef.h"
#include "value.h"
#include "variable.h"
#include "wsex.h"

extern "C" {
#define PROTOTYPES 1
#include "md5.h"
#include "sha1.h"
#include "crc32.h"
}

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  システム関数テーブル
 * -----------------------------------------------------------------------
 */
#define	SYSFUNC_NUM					130 //システム関数の全数
#define	SYSFUNC_HIS					61 //EmBeD_HiStOrY の位置（0start）

static const wchar_t sysfunc[SYSFUNC_NUM][32] = {
	// 型取得/変換
	L"TOINT",
	L"TOREAL",
	L"TOSTR",
	L"GETTYPE",
	L"ISFUNC",
	L"ISVAR",
	// デバッグ
	L"LOGGING",
	L"GETLASTERROR",
	// 外部ライブラリ
	L"LOADLIB",
	L"UNLOADLIB",
	L"REQUESTLIB",
	L"CHARSETLIB",
	// 数値
	L"RAND",
	L"FLOOR",
	L"CEIL",
	L"ROUND",
	L"SIN",
	L"COS",
	L"TAN",
	L"LOG",
	L"LOG10",
	L"POW",
	L"SQRT",
	// 文字列操作
	L"STRSTR",
	L"STRLEN",
	L"REPLACE",
	L"SUBSTR",
	L"ERASE",
	L"INSERT",
	L"TOUPPER",
	L"TOLOWER",
	L"CUTSPACE",
	L"TOBINSTR",
	L"TOHEXSTR",
	L"BINSTRTOI",
	L"HEXSTRTOI",
	L"CHR",
	// ファイル操作
	L"FOPEN",
	L"FCLOSE",
	L"FREAD",
	L"FWRITE",
	L"FWRITE2",
	L"FCOPY",
	L"FMOVE",
	L"MKDIR",
	L"RMDIR",
	L"FDEL",
	L"FRENAME",
	L"FSIZE",
	L"FENUM",
	L"FCHARSET",
	// 配列
	L"ARRAYSIZE",
	L"SETDELIM",
	// 特殊
	L"EVAL",
	L"ERASEVAR",
	// システム時刻/メモリ情報
	L"GETTIME",
	L"GETTICKCOUNT",
	L"GETMEMINFO",
	// 正規表現
	L"RE_SEARCH",
	L"RE_MATCH",
	L"RE_GREP",
	// システムで使用
	L"EmBeD_HiStOrY",	// %[n]（置換済の値の再利用）処理用
	// デバッグ用(2)
	L"SETLASTERROR",
	// 正規表現(2)
	L"RE_REPLACE",
	L"RE_SPLIT",
	L"RE_GETSTR",
	L"RE_GETPOS",
	L"RE_GETLEN",
	// 文字列操作(2)
	L"CHRCODE",
	L"ISINTSTR",
	L"ISREALSTR",
	// 配列(2)
	L"IARRAY",
	// 文字列操作(3)
	L"SPLITPATH",
	// 型取得/変換(2)
	L"CVINT",
	L"CVSTR",
	L"CVREAL",
	// 特殊(2)
	L"LETTONAME",
	L"LSO",
	// 文字列操作(4)
	L"STRFORM",
	L"ANY",
	// 特殊(3)
	L"SAVEVAR",
	// 文字列操作(5)
	L"GETSTRBYTES",
	// 配列(3)
	L"ASEARCH",
	L"ASEARCHEX",
	// 配列(2)
	L"GETDELIM",
	// 特殊(4)
	L"GETSETTING",
	// 数値(2)
	L"ASIN",
	L"ACOS",
	L"ATAN",
	// 文字列操作(6)
	L"SPLIT",
	// ファイル操作(2)
	L"FATTRIB",
	// 型取得/変換(3)
	L"GETFUNCLIST",
	L"GETVARLIST",
	// 正規表現(3)
	L"RE_REPLACEEX",
	// 外部ライブラリ(2)
	L"CHARSETLIBEX",
	// 文字コード
	L"CHARSETTEXTTOID",
	L"CHARSETIDTOTEXT",
	// ビット演算
	L"BITWISE_AND",
	L"BITWISE_OR",
	L"BITWISE_XOR",
	L"BITWISE_NOT",
	L"BITWISE_SHIFT",
	// 半角<->全角
	L"ZEN2HAN",
	L"HAN2ZEN",
	// 型取得/変換(3)
	L"CVAUTO",
	L"TOAUTO",
	// ファイル操作(3)
	L"FREADBIN",
	L"FWRITEBIN",
	// 特殊(5)
	L"RESTOREVAR",
	L"GETCALLSTACK",
	// 文字列操作(7) 互換用
	L"GETSTRURLENCODE",
	L"GETSTRURLDECODE",
	// 数値(3)
	L"SINH",
	L"COSH",
	L"TANH",
	// システム時刻/メモリ情報(2)
	L"GETSECCOUNT",
	// FMO(1)
	L"READFMO",
	// ファイル操作(4)
	L"FDIGEST",
	// 特殊(6)
	L"EXECUTE",
	L"SETSETTING",
	// デバッグ用(3)
	L"DUMPVAR",
	// ファイル操作(5)
	L"FSEEK",
	L"FTELL",
	//ライセンス
	L"LICENSE",
	// 文字列操作(8)
	L"STRENCODE",
	L"STRDECODE",
	// 特殊(7)
	L"EXECUTE_WAIT",
	// 正規表現(3)
	L"RE_OPTION",
	// ファイル操作(6)
	L"FREADENCODE",
	// 型取得/変換(4)
	L"GETTYPEEX",
};

//このグローバル変数はマルチインスタンスでも共通
class CSystemFunctionInit {
public:
	int sysfunc_len[SYSFUNC_NUM];
	int sysfunc_len_max;
	int sysfunc_len_min;
	yaya::indexmap sysfunc_map;

	CSystemFunctionInit(void) {
		sysfunc_len_max = 0;
		sysfunc_len_min = 65536;
		for(size_t i = 0; i < SYSFUNC_NUM; i++) {
			sysfunc_len[i] = ::wcslen(sysfunc[i]);
			sysfunc_map.insert(yaya::indexmap::value_type(sysfunc[i],i));

			if ( sysfunc_len_max < sysfunc_len[i] ) {
				sysfunc_len_max = sysfunc_len[i];
			}
			if ( sysfunc_len_min > sysfunc_len[i] ) {
				sysfunc_len_min = sysfunc_len[i];
			}
		}
	}
};

CSystemFunctionInit sysfuncinit;

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CSystemFunction
 * -----------------------------------------------------------------------
 */
CSystemFunction::CSystemFunction(CAyaVM &vmr)
	: vm(vmr), re_str(F_TAG_ARRAY, 0/*dmy*/), re_pos(F_TAG_ARRAY, 0/*dmy*/), re_len(F_TAG_ARRAY, 0/*dmy*/)
{
	lasterror   = 0;
	lso         = -1;

	re_option   = 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GetMaxNameLength
 *  機能概要：  システム関数の名前の最大値を返します
 * -----------------------------------------------------------------------
 */
int CSystemFunction::GetMaxNameLength(void)
{
	return sysfuncinit.sysfunc_len_max;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FindIndex
 *  機能概要：  システム関数を探索します
 * -----------------------------------------------------------------------
 */
int CSystemFunction::FindIndex(const yaya::string_t &str)
{
	yaya::indexmap::const_iterator it = sysfuncinit.sysfunc_map.find(str);
	if ( it == sysfuncinit.sysfunc_map.end() ) { return -1; }

	return it->second;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FindIndexLongestMatch
 *  機能概要：  いちばん長くマッチするシステム関数を探索します
 * -----------------------------------------------------------------------
 */
int CSystemFunction::FindIndexLongestMatch(const yaya::string_t &str,int max_len)
{
	int found_len = 0;
	for(size_t i = 0; i < SYSFUNC_NUM; i++) {
		if ( sysfuncinit.sysfunc_len[i] <= max_len ) { continue; }

		if ( wcsncmp(str.c_str(),sysfunc[i],sysfuncinit.sysfunc_len[i]) == 0 ) {
			found_len = sysfuncinit.sysfunc_len[i];
			max_len = found_len;
		}
	}
	return found_len;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GetNameFromIndex
 *  機能概要：  Index->名前
 * -----------------------------------------------------------------------
 */
const yaya::char_t* CSystemFunction::GetNameFromIndex(int idx)
{
	if ( idx < 0 || idx >= SYSFUNC_NUM ) { return L""; }
	return sysfunc[idx];
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::HistoryIndex / HistoryFunctionName
 *  機能概要：  履歴系の定数を返します
 * -----------------------------------------------------------------------
 */
int CSystemFunction::HistoryIndex(void)
{
	return SYSFUNC_HIS;
}

const yaya::char_t* CSystemFunction::HistoryFunctionName(void)
{
	return sysfunc[SYSFUNC_HIS];
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::Execute
 *  機能概要：  システム関数を実行します
 *
 *  thisfuncはこの関数を実行したCFunctionインスタンスです
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::Execute(int index, const CValue &arg, const std::vector<CCell *> &pcellarg,
			CLocalVariable &lvar, int l, CFunction *thisfunc)
{
	yaya::string_t& d = thisfunc->dicfilename;

	switch(index) {
	case 0:		// TOINT
		return TOINT(arg, d, l);
	case 1:		// TOREAL
		return TOREAL(arg, d, l);
	case 2:		// TOSTR
		return TOSTR(arg, d, l);
	case 3:		// GETTYPE
		return GETTYPE(arg, d, l);
	case 4:		// ISFUNC
		return ISFUNC(arg, d, l);
	case 5:		// ISVAR
		return ISVAR(arg, lvar, d, l);
	case 6:		// LOGGING
		return LOGGING(arg, d, l);
	case 7:		// GETLASTERROR
		return CValue(lasterror);
	case 8:		// LOADLIB
		return LOADLIB(arg, d, l);
	case 9:		// UNLOADLIB
		return UNLOADLIB(arg, d, l);
	case 10:	// REQUESTLIB
		return REQUESTLIB(arg, d, l);
	case 11:	// CHARSETLIB
		return CHARSETLIB(arg, d, l);
	case 12:	// RAND
		return RAND(arg, d, l);
	case 13:	// FLOOR
		return FLOOR(arg, d, l);
	case 14:	// CEIL
		return CEIL(arg, d, l);
	case 15:	// ROUND
		return ROUND(arg, d, l);
	case 16:	// SIN
		return SIN(arg, d, l);
	case 17:	// COS
		return COS(arg, d, l);
	case 18:	// TAN
		return TAN(arg, d, l);
	case 19:	// LOG
		return LOG(arg, d, l);
	case 20:	// LOG10
		return LOG10(arg, d, l);
	case 21:	// POW
		return POW(arg, d, l);
	case 22:	// SQRT
		return SQRT(arg, d, l);
	case 23:	// STRSTR
		return STRSTR(arg, d, l);
	case 24:	// STRLEN
		return STRLEN(arg, d, l);
	case 25:	// REPLACE
		return REPLACE(arg, d, l);
	case 26:	// SUBSTR
		return SUBSTR(arg, d, l);
	case 27:	// ERASE
		return ERASE(arg, d, l);
	case 28:	// INSERT
		return INSERT(arg, d, l);
	case 29:	// TOUPPER
		return TOUPPER(arg, d, l);
	case 30:	// TOLOWER
		return TOLOWER(arg, d, l);
	case 31:	// CUTSPACE
		return CUTSPACE(arg, d, l);
	case 32:	// TOBINSTR
		return TOBINSTR(arg, d, l);
	case 33:	// TOHEXSTR
		return TOHEXSTR(arg, d, l);
	case 34:	// BINSTRTOI
		return BINSTRTOI(arg, d, l);
	case 35:	// HEXSTRTOI
		return HEXSTRTOI(arg, d, l);
	case 36:	// CHR
		return CHR(arg, d, l);
	case 37:	// FOPEN
		return FOPEN(arg, d, l);
	case 38:	// FCLOSE
		return FCLOSE(arg, d, l);
	case 39:	// FREAD
		return FREAD(arg, d, l);
	case 40:	// FWRITE
		return FWRITE(arg, d, l);
	case 41:	// FWRITE2
		return FWRITE2(arg, d, l);
	case 42:	// FCOPY
		return FCOPY(arg, d, l);
	case 43:	// FMOVE
		return FMOVE(arg, d, l);
	case 44:	// MKDIR
		return MKDIR(arg, d, l);
	case 45:	// RMDIR
		return RMDIR(arg, d, l);
	case 46:	// FDEL
		return FDEL(arg, d, l);
	case 47:	// FRENAME
		return FRENAME(arg, d, l);
	case 48:	// FSIZE
		return FSIZE(arg, d, l);
	case 49:	// FENUM
		return FENUM(arg, d, l);
	case 50:	// FCHARSET
		return FCHARSET(arg, d, l);
	case 51:	// ARRAYSIZE
		return ArraySize(arg, pcellarg, lvar, d, l);
	case 52:	// SETDELIM
		return SETDELIM(pcellarg, lvar, d, l);
	case 53:	// EVAL
		return EVAL(arg, d, l, lvar, thisfunc);
	case 54:	// ERASEVAR
		return ERASEVAR(arg, lvar, d, l);
	case 55:	// GETTIME
		return GETTIME(arg, d, l);
	case 56:	// GETTICKCOUNT
		return GETTICKCOUNT(arg, lvar, d, l);
	case 57:	// GETMEMINFO
		return GETMEMINFO();
	case 58:	// RE_SEARCH
		return RE_SEARCH(arg, d, l);
	case 59:	// RE_MATCH
		return RE_MATCH(arg, d, l);
	case 60:	// RE_GREP
		return RE_GREP(arg, d, l);
	case 61:	// %[n]（置換済の値の再利用）処理用関数 → これのみCFunctionで処理するのでここへは来ない
		vm.logger().Error(E_E, 49, d, l);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	case 62:	// SETLASTERROR
		return SETLASTERROR(arg, d, l);
	case 63:	// RE_REPLACE
		return RE_REPLACE(arg, d, l);
	case 64:	// RE_SPLIT
		return RE_SPLIT(arg, d, l);
	case 65:	// RE_GETSTR
		return re_str;
	case 66:	// RE_GETPOS
		return re_pos;
	case 67:	// RE_GETLEN
		return re_len;
	case 68:	// CHRCODE
		return CHRCODE(arg, d, l);
	case 69:	// ISINTSTR
		return ISINTSTR(arg, d, l);
	case 70:	// ISREALSTR
		return ISREALSTR(arg, d, l);
	case 71:	// IARRAY
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	case 72:	// SPLITPATH
		return SPLITPATH(arg, d, l);
	case 73:	// CVINT
		return CVINT(arg, pcellarg, lvar, d, l);
	case 74:	// CVSTR
		return CVSTR(arg, pcellarg, lvar, d, l);
	case 75:	// CVREAL
		return CVREAL(arg, pcellarg, lvar, d, l);
	case 76:	// LETTONAME
		return LETTONAME(arg, d, l, lvar, thisfunc);
	case 77:	// LSO
		return CValue(lso);
	case 78:	// STRFORM
		return STRFORM(arg, d, l);
	case 79:	// ANY
		return ANY(arg, pcellarg, lvar, d, l);
	case 80:	// SAVEVAR
		return SAVEVAR(arg, d, l);
	case 81:	// GETSTRBYTES
		return GETSTRBYTES(arg, d, l);
	case 82:	// ASEARCH
		return ASEARCH(arg, d, l);
	case 83:	// ASEARCHEX
		return ASEARCHEX(arg, d, l);
	case 84:	// GETDELIM
		return GETDELIM(pcellarg, lvar, d, l);
	case 85:	// GETSETTING
		return GETSETTING(arg, d, l);
	case 86:	// ASIN
		return ASIN(arg, d, l);
	case 87:	// ACOS
		return ACOS(arg, d, l);
	case 88:	// ATAN
		return ATAN(arg, d, l);
	case 89:	// SPLIT
		return SPLIT(arg, d, l);
	case 90:	// FATTRIB
		return FATTRIB(arg, d, l);
	case 91:	// GETFUNCLIST
		return GETFUNCLIST(arg, d, l);
	case 92:	// GETVARLIST
		return GETVARLIST(arg, lvar, d, l);
	case 93:	// RE_REPLACEEX
		return RE_REPLACEEX(arg, d, l);
	case 94:	// CHARSETLIBEX
		return CHARSETLIBEX(arg, d, l);
	case 95:	// CHARSETTEXTTOID
		return CHARSETTEXTTOID(arg, d, l);
	case 96:	// CHARSETIDTOTEXT
		return CHARSETIDTOTEXT(arg, d, l);
	case 97:
		return BITWISE_AND(arg, d, l);
	case 98:
		return BITWISE_OR(arg, d, l);
	case 99:
		return BITWISE_XOR(arg, d, l);
	case 100:
		return BITWISE_NOT(arg, d, l);
	case 101:
		return BITWISE_SHIFT(arg, d, l);
	case 102:
		return ZEN2HAN(arg, d, l);
	case 103:
		return HAN2ZEN(arg, d, l);
	case 104:
		return CVAUTO(arg, pcellarg, lvar, d, l);
	case 105:
		return TOAUTO(arg, d, l);
	case 106:
		return FREADBIN(arg, d, l);
	case 107:
		return FWRITEBIN(arg, d, l);
	case 108:
		return RESTOREVAR(arg, d, l);
	case 109:
		return GETCALLSTACK(arg, d, l);
	case 110:
		return STRENCODE(arg, d, l);
	case 111:
		return STRDECODE(arg, d, l);
	case 112:
		return SINH(arg, d, l);
	case 113:
		return COSH(arg, d, l);
	case 114:
		return TANH(arg, d, l);
	case 115:
		return GETSECCOUNT(arg, d, l);
	case 116:
		return READFMO(arg, d, l);
	case 117:
		return FDIGEST(arg, d, l);
	case 118:
		return EXECUTE(arg, d, l);
	case 119:
		return SETSETTING(arg, d, l);
	case 120:
		return DUMPVAR(arg, d, l);
	case 121:
		return FSEEK(arg, d, l);
	case 122:
		return FTELL(arg, d, l);
	case 123:
		return LICENSE();
	case 124:
		return STRENCODE(arg, d, l);
	case 125:
		return STRDECODE(arg, d, l);
	case 126:
		return EXECUTE_WAIT(arg, d, l);
	case 127:
		return RE_OPTION(arg, d, l);
	case 128:
		return FREADENCODE(arg, d, l);
	case 129:
		return GETTYPEEX(arg, lvar, d, l);
	default:
		vm.logger().Error(E_E, 49, d, l);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	};

	return CValue(F_TAG_NOP, 0/*dmy*/);
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOINT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOINT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOINT", d, l);
		SetError(8);
		return CValue(0);
	}

	return CValue(arg.array()[0].GetValueInt());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOREAL
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOREAL(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOREAL", d, l);
		SetError(8);
		return CValue(0.0);
	}

	return CValue(arg.array()[0].GetValueDouble());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOSTR(const CValue &args, yaya::string_t &d, int &l)
{
	int	sz = args.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"TOSTR", d, l);
		SetError(8);
		return CValue();
	}

	if (sz == 1)
		return CValue(args.array()[0].GetValueString());

	return CValue(args.GetValueString());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOAUTO
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOAUTO(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOAUTO", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		return CValue(arg.array()[0]);
	}

	yaya::string_t str = arg.array()[0].GetValueString();

	if ( IsIntString(str) ) {
		return CValue(arg.array()[0].GetValueInt());
	}
	else if ( IsDoubleButNotIntString(str) ) {
		return CValue(arg.array()[0].GetValueDouble());
	}
	else {
		return CValue(str);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETTYPE
 *  返値　　：  0/1/2/3/4=エラー/整数/実数/文字列/配列
 *
 *  GETTYPE(1,2)のように複数の引数を与えた場合、これらは1つの配列値ですから
 *  返値は4になることに注意してください。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETTYPE(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"GETTYPE", d, l);
		SetError(8);
		return CValue(0);
	}

	if (sz > 1)
		return CValue(4);

	switch(arg.array()[0].GetType()) {
	case F_TAG_INT:
		return CValue(1);
	case F_TAG_DOUBLE:
		return CValue(2);
	case F_TAG_STRING:
		return CValue(3);
	case F_TAG_VOID:
		return CValue(0);
	default:
		vm.logger().Error(E_E, 88, L"GETTYPE", d, l);
		return CValue(0);
	};
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETTYPEEX
 *  返値　　：  0/1/2/3/4=エラー/整数/実数/文字列/配列
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETTYPEEX(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	size_t arg_size = arg.array_size();

	if (!arg_size) {
		vm.logger().Error(E_W, 8, L"GETTYPEEX", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	//文字列かどうかチェック - 警告は吐くが処理続行
	if ( ! arg.array()[0].IsString() ) {
		vm.logger().Error(E_W, 9, L"GETTYPEEX", d, l);
		SetError(9);
	}

	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	if (!arg0.size()) {
		return CValue(0);
	}

	int type = 0;
	if (arg0[0] == L'_') {
		const CValue *v = lvar.GetValuePtr(arg0);
		if ( v ) {
			type = v->GetType();
		}
	}
	else {
		int gidx = vm.variable().GetIndex(arg0);
		if ( gidx >= 0 ) {
			const CValue &v = vm.variable().GetValue(gidx);
			type = v.GetType();
		}
	}

	switch(type) {
	case F_TAG_INT:
		return CValue(1);
	case F_TAG_DOUBLE:
		return CValue(2);
	case F_TAG_STRING:
		return CValue(3);
	case F_TAG_ARRAY:
		return CValue(4);
	default:
		return CValue(0);
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ISFUNC
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ISFUNC(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ISFUNC", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"ISFUNC", d, l);
		SetError(9);
		return CValue(0);
	}

	int	i = vm.parser0().GetFunctionIndexFromName(arg.array()[0].s_value);
	if(i != -1)
		return CValue(1);

	if ( FindIndex(arg.array()[0].s_value) >= 0 ) {
		return CValue(2);
	}

	return CValue(0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ISVAR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ISVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ISVAR", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"ISVAR", d, l);
		SetError(9);
		return CValue(0);
	}

	int	index = vm.variable().GetIndex(arg.array()[0].s_value);
	if (index >= 0)
		return CValue(1);

	int	depth = -1;
	lvar.GetIndex(arg.array()[0].s_value, index, depth);
	if (index >= 0)
		return CValue(2);

	return CValue(0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LOGGING
 *
 *  文字列の場合はダブルクォートします。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LOGGING(const CValue &arg, yaya::string_t &/*d*/, int &/*l*/)
{
	if (arg.array_size())
		vm.logger().Write(arg.GetValueStringForLogging());

	vm.logger().Write(L"\n");

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LOADLIB
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LOADLIB(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"LOADLIB", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"LOADLIB", d, l);
		SetError(9);
		return CValue(0);
	}

	int	excode = vm.libs().Add(ToFullPath(arg.array()[0].s_value));
	if (!excode) {
		vm.logger().Error(E_W, 13, L"LOADLIB", d, l);
		SetError(13);
	}

	return CValue(excode);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::UNLOADLIB
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::UNLOADLIB(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"UNLOADLIB", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"UNLOADLIB", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int	result = vm.libs().Delete(ToFullPath(arg.array()[0].s_value));

	if (!result) {
		vm.logger().Error(E_W, 13, L"UNLOADLIB", d, l);
		SetError(13);
	}
	else if (result == 2) {
		vm.logger().Error(E_W, 14, d, l);
		SetError(14);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::REQUESTLIB
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::REQUESTLIB(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"REQUESTLIB", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"REQUESTLIB", d, l);
		SetError(9);
		return CValue();
	}

	yaya::string_t	result;
	if (!vm.libs().Request(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value, result)) {
		vm.logger().Error(E_W, 13, L"REQUESTLIB", d, l);
		SetError(13);
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHARSETTEXTTOID
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHARSETTEXTTOID(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CHARSETTEXTTOID", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"CHARSETTEXTTOID", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(Ccct::CharsetTextToID(arg.array()[0].s_value.c_str()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHARSETIDTOTEXT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHARSETIDTOTEXT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CHARSETIDTOTEXT", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"CHARSETIDTOTEXT", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(Ccct::CharsetIDToTextW(arg.array()[0].GetValueInt()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::BITWISE_***
 *  ビット演算関連です
 * -----------------------------------------------------------------------
 */
CValue CSystemFunction::BITWISE_AND(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"BITWISE_AND", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(arg.array()[0].GetValueInt() & arg.array()[1].GetValueInt());
}

CValue CSystemFunction::BITWISE_OR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"BITWISE_OR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(arg.array()[0].GetValueInt() | arg.array()[1].GetValueInt());
}

CValue CSystemFunction::BITWISE_XOR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"BITWISE_XOR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(arg.array()[0].GetValueInt() ^ arg.array()[1].GetValueInt());
}

CValue CSystemFunction::BITWISE_NOT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"BITWISE_NOT", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	return CValue(~arg.array()[0].GetValueInt());
}

CValue CSystemFunction::BITWISE_SHIFT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"BITWISE_SHIFT", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int shiftValue = arg.array()[1].GetValueInt();
	if ( shiftValue > 0 ) {
		return CValue(arg.array()[0].GetValueInt() << shiftValue );
	}
	else {
		return CValue(arg.array()[0].GetValueInt() >> abs(shiftValue) );
	}
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ZEN2HAN / HAN2ZEN
 *
 *  半角<->全角
 * -----------------------------------------------------------------------
 */

static const yaya::char_t zen_support_symbol[] = 
	L"　！“”＃＄％＆‘’（）＝｜‘｛＋＊｝＜＞？＿ー＾￥＠；：・．［］";
static const yaya::char_t han_support_symbol[] = 
	L" !\"\"#$%&''()=|`{+*}<>?_-^\\@;:･.[]";

static const yaya::char_t zen_support_kana[] = 
	L"アイウエオカキクケコサシスセ\x30bdタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォャュョ゛゜、。";
static const yaya::char_t han_support_kana[] = 
	L"ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝｧｨｩｪｫｬｭｮﾞﾟ､｡";

static const yaya::char_t zen_support_kana2[] = 
	L"ガギグゲゴザジズゼゾダヂヅデドバビブベボ";
static const yaya::char_t han_support_kana2[] = 
	L"ｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾊﾋﾌﾍﾎ";


#define ZH_FLAG_NUMBER   0x1U
#define ZH_FLAG_ALPHABET 0x2U
#define ZH_FLAG_SYMBOL   0x4U
#define ZH_FLAG_KANA     0x8U

static unsigned int CSystemFunction_ZHFlag(const yaya::string_t &str);

static unsigned int CSystemFunction_ZHFlag(const yaya::string_t &str)
{
	unsigned int flag = 0;

	if ( str.find(L"num") != yaya::string_t::npos ) {
		flag |= ZH_FLAG_NUMBER;
	}
	if ( str.find(L"alpha") != yaya::string_t::npos ) {
		flag |= ZH_FLAG_ALPHABET;
	}
	if ( str.find(L"sym") != yaya::string_t::npos ) {
		flag |= ZH_FLAG_SYMBOL;
	}
	if ( str.find(L"kana") != yaya::string_t::npos ) {
		flag |= ZH_FLAG_KANA;
	}
	return flag;
}

static const yaya::char_t char_zen_0 = 0xff10;
static const yaya::char_t char_zen_9 = 0xff19;

static const yaya::char_t char_zen_upper_a = 0xff21;
static const yaya::char_t char_zen_upper_z = 0xff3a;

static const yaya::char_t char_zen_lower_a = 0xff41;
static const yaya::char_t char_zen_lower_z = 0xff5a;

static const yaya::char_t char_dakuten = 0xff9e;

CValue CSystemFunction::ZEN2HAN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ZEN2HAN", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	yaya::string_t str = arg.array()[0].GetValueString();

	unsigned int flag = 0xFFFFFFFFU;
	if ( arg.array_size() >= 2 ) {
		flag = CSystemFunction_ZHFlag(arg.array()[1].GetValueString());
	}

	for ( yaya::string_t::iterator it = str.begin() ; it < str.end(); ++it ) {
		if ( *it >= char_zen_0 && *it <= char_zen_9 ) {
			if ( flag & ZH_FLAG_NUMBER ) {
				*it = *it - char_zen_0 + L'0';
			}
		}
		else if ( *it >= char_zen_upper_a && *it <= char_zen_upper_z ) {
			if ( flag & ZH_FLAG_ALPHABET ) {
				*it = *it - char_zen_upper_a + L'A';
			}
		}
		else if ( *it >= char_zen_lower_a && *it <= char_zen_lower_z ) {
			if ( flag & ZH_FLAG_ALPHABET ) {
				*it = *it - char_zen_lower_a + L'a';
			}
		}
		else {
			if ( flag & ZH_FLAG_SYMBOL ) {
				const yaya::char_t *found = wcschr(zen_support_symbol,*it);
				if ( found ) {
					*it = han_support_symbol[found - zen_support_symbol];
				}
			}
			if ( flag & ZH_FLAG_KANA ) {
				const yaya::char_t *found = wcschr(zen_support_kana,*it);
				if ( found ) {
					*it = han_support_kana[found - zen_support_kana];
				}

				found = wcschr(zen_support_kana2,*it);
				if ( found ) {
					*it = han_support_kana2[found - zen_support_kana2];
					it = str.insert(it+1,char_dakuten);
				}
			}
		}
	}

	return CValue(str);
}

CValue CSystemFunction::HAN2ZEN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"HAN2ZEN", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	yaya::string_t str = arg.array()[0].GetValueString();

	unsigned int flag = 0xFFFFFFFFU;
	if ( arg.array_size() >= 2 ) {
		flag = CSystemFunction_ZHFlag(arg.array()[1].GetValueString());
	}

	for ( yaya::string_t::iterator it = str.begin() ; it < str.end(); ++it ) {
		if ( *it >= L'0' && *it <= L'9' ) {
			if ( flag & ZH_FLAG_NUMBER ) {
				*it = *it - L'0' + char_zen_0;
			}
		}
		else if ( *it >= L'A' && *it <= L'Z' ) {
			if ( flag & ZH_FLAG_ALPHABET ) {
				*it = *it - L'A' + char_zen_upper_a;
			}
		}
		else if ( *it >= L'a' && *it <= L'z' ) {
			if ( flag & ZH_FLAG_ALPHABET ) {
				*it = *it - L'a' + char_zen_lower_a;
			}
		}
		else {
			if ( flag & ZH_FLAG_SYMBOL ) {
				const yaya::char_t *found = wcschr(han_support_symbol,*it);
				if ( found ) {
					*it = zen_support_symbol[found - han_support_symbol];
				}
			}
			if ( flag & ZH_FLAG_KANA ) {
				const yaya::char_t *found = wcschr(han_support_kana,*it);
				if ( found ) {
					if ( it < str.end()-1 && *(it+1) == char_dakuten ) {
						found = wcschr(han_support_kana2,*it);
						if ( found ) {
							*it = zen_support_kana2[found - han_support_kana2];
							it = str.erase(it+1) - 1;
						}
					}
					else {
						*it = zen_support_kana[found - han_support_kana];
					}
				}
			}
		}
	}
	return CValue(str);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHARSETLIB
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHARSETLIB(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		return CValue(Ccct::CharsetIDToTextW(vm.libs().GetCharset()));
	}
	else {
		int	charset = GetCharset(arg.array()[0],L"CHARSETLIB",d,l);
		if ( charset < 0 ) {
			return CValue();
		}

		vm.libs().SetCharset(charset);
		return CValue();
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHARSETLIBEX
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHARSETLIBEX(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 1) {
		vm.logger().Error(E_W, 8, L"CHARSETLIBEX", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"CHARSETLIBEX", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if ( arg.array_size() >= 2 ) {
		int	charset = GetCharset(arg.array()[1],L"CHARSETLIBEX",d,l);
		if ( charset < 0 ) {
			return CValue(0);
		}
		int result = vm.libs().SetCharsetDynamic(ToFullPath(arg.array()[0].s_value),charset);

		if (!result) {
			vm.logger().Error(E_W, 13, L"CHARSETLIBEX", d, l);
			SetError(13);
		}

		return CValue(result);
	}
	else {
		int result = vm.libs().GetCharsetDynamic(ToFullPath(arg.array()[0].s_value));
		if ( result < 0 ) {
			return CValue();
		}
		return CValue(Ccct::CharsetIDToTextW(result));
	}
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RAND
 *
 *  引数なしはRAND(100)扱いです。
 *  引数エラーでは0を返します。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RAND(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size())
	        return vm.genrand_int(100);

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"RAND", d, l);
		SetError(9);
	}

	int num = arg.array()[0].GetValueInt();

	if ( num == 0 ) {
		vm.logger().Error(E_W, 19, L"RAND", d, l);
		SetError(19);
		return CValue(0);
	}

	return vm.genrand_int(num);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FLOOR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FLOOR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FLOOR", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"FLOOR", d, l);
		SetError(9);
	}
	return CValue(floor(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CEIL
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CEIL(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CEIL", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"CEIL", d, l);
		SetError(9);
	}
	return CValue(ceil(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ROUND
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ROUND(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ROUND", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (arg.array()[0].IsInt()) {
		return CValue((double)arg.array()[0].GetValueInt());
	}
	else {
		if (!arg.array()[0].IsDouble()) {
			vm.logger().Error(E_W, 9, L"ROUND", d, l);
			SetError(9);
		}
		double	value = arg.array()[0].GetValueDouble();
		if (value - floor(value) >= 0.5)
			return CValue(ceil(value));
		else
			return CValue(floor(value));
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SIN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SIN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SIN", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"SIN", d, l);
		SetError(9);
	}
	return CValue(sin(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::COS
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::COS(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"COS", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"COS", d, l);
		SetError(9);
	}
	return CValue(cos(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TAN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TAN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TAN", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"TAN", d, l);
		SetError(9);
	}
	return CValue(tan(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SINH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SINH(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SINH", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"SINH", d, l);
		SetError(9);
	}
	return CValue(sinh(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::COSH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::COSH(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"COSH", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"COSH", d, l);
		SetError(9);
	}
	return CValue(cosh(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TANH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TANH(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TANH", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"TANH", d, l);
		SetError(9);
	}
	return CValue(tanh(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ASIN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ASIN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ASIN", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"ASIN", d, l);
		SetError(9);
	}
	return CValue(asin(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ACOS
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ACOS(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ACOS", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"ACOS", d, l);
		SetError(9);
	}
	return CValue(acos(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ATAN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ATAN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ATAN", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"ATAN", d, l);
		SetError(9);
	}
	return CValue(atan(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LOG
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LOG(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"LOG", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"LOG", d, l);
		SetError(9);
	}
	return CValue(log(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LOG10
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LOG10(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"LOG10", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"LOG10", d, l);
		SetError(9);
	}
	return CValue(log10(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::POW
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::POW(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"POW", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum() || !arg.array()[1].IsNum()) {
		vm.logger().Error(E_W, 9, L"POW", d, l);
		SetError(9);
	}
	return CValue(pow(arg.array()[0].GetValueDouble(), arg.array()[1].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SQRT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SQRT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SQRT", d, l);
		SetError(8);
		return CValue(0.0);
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"SQRT", d, l);
		SetError(9);
	}

	double	value = arg.array()[0].GetValueDouble();

	if (value < 0.0)
		return CValue(-1);

	return CValue(sqrt(arg.array()[0].GetValueDouble()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::STRSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"STRSTR", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[2].IsNum()) {
		vm.logger().Error(E_W, 9, L"STRSTR", d, l);
		SetError(9);
	}

	return CValue((int)arg.array()[0].GetValueString().find(arg.array()[1].GetValueString(), arg.array()[2].GetValueInt()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRLEN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::STRLEN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"STRLEN", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"STRLEN", d, l);
		SetError(9);
	}

	return CValue((int)arg.array()[0].GetValueString().size());
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::REPLACE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::REPLACE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"REPLACE", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString() ||
		!arg.array()[2].IsString()) {
		vm.logger().Error(E_W, 9, L"REPLACE", d, l);
		SetError(9);
	}

	yaya::string_t	result = arg.array()[0].GetValueString();
	yaya::string_t  before = arg.array()[1].GetValueString();
	yaya::string_t  after  = arg.array()[2].GetValueString();
	//int	sz_before = before->size();
	//int	sz_after  = after->size();

	if (!before.empty()) {
		yaya::ws_replace(result, before.c_str(), after.c_str());
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SUBSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SUBSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"SUBSTR", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[1].IsNum() ||
		!arg.array()[2].IsNum()) {
		vm.logger().Error(E_W, 9, L"SUBSTR", d, l);
		SetError(9);
	}

	const yaya::string_t& src = arg.array()[0].GetValueString();
	int pos = arg.array()[1].GetValueInt();
	int len = arg.array()[2].GetValueInt();

	if ( pos < 0 ) {
		pos += src.length();
		if ( pos < 0 ) { //まだ負なら強制補正
			len += pos; //負値なのでたしざんで引かれる
			pos = 0;
			if ( len <= 0 ) {
				return CValue(L"");
			}
		}
	}

	if ( pos >= static_cast<int>(src.length()) || len <= 0 ) {
	    return CValue(L"");
	}
	if ( pos + len >= static_cast<int>(src.length()) ) {
	    len = src.length() - pos;
	}

	return CValue(src.substr(pos, len));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ERASE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ERASE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"ERASE", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsNum() ||
		!arg.array()[2].IsNum()) {
		vm.logger().Error(E_W, 9, L"ERASE", d, l);
		SetError(9);
	}
	
	yaya::string_t str = arg.array()[0].GetValueString();
	return CValue(str.erase(arg.array()[1].GetValueInt(), arg.array()[2].GetValueInt()));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::INSERT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::INSERT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"INSERT", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsNum() ||
		!arg.array()[2].IsString()) {
		vm.logger().Error(E_W, 9, L"ERASE", d, l);
		SetError(9);
	}

	yaya::string_t str = arg.array()[0].GetValueString();
	return CValue(str.insert(arg.array()[1].GetValueInt(), arg.array()[2].s_value));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOUPPER
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOUPPER(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOUPPER", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"TOUPPER", d, l);
		SetError(9);
	}

	yaya::string_t	result = arg.array()[0].GetValueString();
	int	len = result.size();
	for(int i = 0; i < len; i++)
		if (result[i] >= L'a' && result[i] <= L'z')
			result[i] += static_cast<yaya::string_t::value_type>(L'A' - L'a');
	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOLOWER
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOLOWER(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOLOWER", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"TOLOWER", d, l);
		SetError(9);
	}

	yaya::string_t	result = arg.array()[0].GetValueString();
	int	len = result.size();
	for(int i = 0; i < len; i++)
		if (result[i] >= L'A' && result[i] <= L'Z')
			result[i] += (L'a' - L'A');

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CUTSPACE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CUTSPACE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CUTSPACE", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"CUTSPACE", d, l);
		SetError(9);
	}

	yaya::string_t	result = arg.array()[0].GetValueString();
	CutSpace(result);

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOBINSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOBINSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOBINSTR", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"TOBINSTR", d, l);
		SetError(9);
	}

	return CValue(yaya::ws_itoa(arg.array()[0].GetValueInt(), 2));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TOHEXSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TOHEXSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"TOHEXSTR", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsInt()) {
		vm.logger().Error(E_W, 9, L"TOHEXSTR", d, l);
		SetError(9);
	}

	return CValue(yaya::ws_itoa(arg.array()[0].GetValueInt(), 16));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::BINSTRTOI
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::BINSTRTOI(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"BINSTRTOI", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"BINSTRTOI", d, l);
		SetError(9);
	}

	if (!IsIntBinString(arg.array()[0].GetValueString(), 0)) {
		vm.logger().Error(E_W, 12, L"BINSTRTOI", d, l);
		SetError(12);
		return CValue(0);
	}

	return CValue(yaya::ws_atoi(arg.array()[0].GetValueString(), 2));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::HEXSTRTOI
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::HEXSTRTOI(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"HEXSTRTOI", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"HEXSTRTOI", d, l);
		SetError(9);
	}

	if (!IsIntHexString(arg.array()[0].GetValueString(), 0)) {
		vm.logger().Error(E_W, 12, L"HEXSTRTOI", d, l);
		SetError(12);
		return CValue(0);
	}

	return CValue(yaya::ws_atoi(arg.array()[0].GetValueString(), 16));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CHR", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsNum()) {
		vm.logger().Error(E_W, 9, L"CHR", d, l);
		SetError(9);
	}

	yaya::string_t r_value(1, static_cast<yaya::char_t>(arg.array()[0].GetValueInt()));
	
	for ( std::vector<CValueSub>::const_iterator i = arg.array().begin() + 1 ;
		i < arg.array().end() ; ++i ) {
		r_value.append(1, static_cast<yaya::char_t>(i->GetValueInt()) );
	}

	return CValue(r_value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FOPEN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FOPEN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FOPEN", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FOPEN", d, l);
		SetError(9);
		return CValue(0);
	}

	return CValue(vm.files().Add(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FCLOSE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FCLOSE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FCLOSE", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FCLOSE", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int	result = vm.files().Delete(ToFullPath(arg.array()[0].s_value));

	if (!result) {
		vm.logger().Error(E_W, 13, L"FCLOSE", d, l);
		SetError(13);
	}
	else if (result == 2) {
		vm.logger().Error(E_W, 15, d, l);
		SetError(15);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FREAD
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FREAD(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FREAD", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FREAD", d, l);
		SetError(9);
		return CValue();
	}

	yaya::string_t	r_value;
	int	result = vm.files().Read(ToFullPath(arg.array()[0].s_value), r_value);
	CutCrLf(r_value);

	if (!result) {
		vm.logger().Error(E_W, 13, L"FREAD", d, l);
		SetError(13);
		return CValue(-1);
	}
	if (result == -1) {
		return CValue(-1);
	}

	return CValue(r_value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FREADBIN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FREADBIN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FREADBIN", d, l);
		SetError(8);
		return CValue();
	}

    if (!arg.array()[0].IsString() || (arg.array_size() >= 2 && !arg.array()[1].IsInt()) ) {
		vm.logger().Error(E_W, 9, L"FREADBIN", d, l);
		SetError(9);
		return CValue();
	}

	size_t readsize = 0;
	if ( arg.array_size() >= 2 ) {
		readsize = arg.array()[1].GetValueInt();
	}

	yaya::char_t alt = L' ';
	if (arg.array_size() >= 3) {
		if (!arg.array()[2].IsString()) {
			vm.logger().Error(E_W, 9, L"FREADBIN", d, l);
			SetError(9);
			return CValue(F_TAG_NOP, 0/*dmy*/);
		}
		alt = arg.array()[2].GetValueString()[0];
	}

	yaya::string_t	r_value;
	int	result = vm.files().ReadBin(ToFullPath(arg.array()[0].GetValueString()), r_value, readsize, alt);

	if (!result) {
		vm.logger().Error(E_W, 13, L"FREADBIN", d, l);
		SetError(13);
	}
	else if (result == -1)
		return CValue(-1);

	return CValue(r_value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FREADENCODE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FREADENCODE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FREADENCODE", d, l);
		SetError(8);
		return CValue();
	}

    if (!arg.array()[0].IsString() || (arg.array_size() >= 2 && !arg.array()[1].IsInt()) ) {
		vm.logger().Error(E_W, 9, L"FREADENCODE", d, l);
		SetError(9);
		return CValue();
	}

	size_t readsize = 0;
	if ( arg.array_size() >= 2 ) {
		readsize = arg.array()[1].GetValueInt();
	}

	yaya::string_t type = L"";
	if ( arg.array_size() >= 3 ) {
		type = arg.array()[2].GetValueString();
	}

	yaya::string_t	r_value;
	int	result = vm.files().ReadEncode(ToFullPath(arg.array()[0].GetValueString()), r_value, readsize, type);

	if (!result) {
		vm.logger().Error(E_W, 13, L"FREADENCODE", d, l);
		SetError(13);
	}
	else if (result == -1)
		return CValue(-1);

	return CValue(r_value);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FWRITE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FWRITE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FWRITE", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FWRITE", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!vm.files().Write(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value + yaya::string_t(L"\n"))) {
		vm.logger().Error(E_W, 13, L"FWRITE", d, l);
		SetError(13);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FWRITEBIN
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FWRITEBIN(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FWRITEBIN", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FWRITEBIN", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	yaya::char_t alt = L' ';

	if (arg.array_size() >= 3) {
		if (!arg.array()[2].IsString()) {
			vm.logger().Error(E_W, 9, L"FWRITEBIN", d, l);
			SetError(9);
			return CValue(F_TAG_NOP, 0/*dmy*/);
		}
		alt = arg.array()[2].s_value[0];
	}

	if (!vm.files().WriteBin(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value, alt) ) {
		vm.logger().Error(E_W, 13, L"FWRITEBIN", d, l);
		SetError(13);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FWRITE2
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FWRITE2(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FWRITE2", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FWRITE2", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!vm.files().Write(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value)) {
		vm.logger().Error(E_W, 13, L"FWRITE2", d, l);
		SetError(13);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FSEEK
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FSEEK(const CValue &arg, yaya::string_t &d, int &l){
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"FSEEK", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsInt()    ||
		!arg.array()[2].IsString()
		) {
		vm.logger().Error(E_W, 9, L"FSEEK", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int result=vm.files().FSeek(ToFullPath(arg.array()[0].s_value), arg.array()[1].i_value,arg.array()[2].s_value);
	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FTELL
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FTELL(const CValue &arg, yaya::string_t &d, int &l){
	if (arg.array_size() < 1) {
		vm.logger().Error(E_W, 8, L"FTELL", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FTELL", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int result=vm.files().FTell(ToFullPath(arg.array()[0].s_value));
	return CValue(result);
}




/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FCOPY
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FCOPY(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FCOPY", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FCOPY", d, l);
		SetError(9);
		return CValue(0);
	}

	// 絶対パス化
	yaya::char_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_wsplitpath(arg.array()[0].s_value.c_str(), drive, dir, fname, ext);
	yaya::string_t	s_path = ((::wcslen(drive)) ? yaya::string_t(L"") : vm.basis().path) + arg.array()[0].s_value;

	yaya::char_t	fname2[_MAX_FNAME], ext2[_MAX_EXT];
	_wsplitpath(arg.array()[1].s_value.c_str(), drive, dir, fname2, ext2);
	yaya::string_t	d_path = ((::wcslen(drive)) ?
						yaya::string_t(L"") : vm.basis().path) + arg.array()[1].s_value + L"\\" + fname + ext;

	// パスをMBCSに変換
	char	*s_pstr = Ccct::Ucs2ToMbcs(s_path, CHARSET_DEFAULT);
	if (s_pstr == NULL) {
		vm.logger().Error(E_E, 89, L"FCOPY", d, l);
		return CValue(0);
	}
	char	*d_pstr = Ccct::Ucs2ToMbcs(d_path, CHARSET_DEFAULT);
	if (d_pstr == NULL) {
		free(s_pstr);
		vm.logger().Error(E_E, 89, L"FCOPY", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (CopyFile(s_pstr, d_pstr, FALSE) ? 1 : 0);
	free(s_pstr);
	free(d_pstr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::FCOPY(const CValue &arg, yaya::string_t &d, int &l) {
    if (arg.array_size() < 2) {
	vm.logger().Error(E_W, 8, L"FCOPY", d, l);
	SetError(8);
	return CValue(0);
    }
    
    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FCOPY", d, l);
		SetError(9);
		return CValue(0);
    }

    // 絶対パス化
	std::string src = narrow(ToFullPath(arg.array()[0].s_value));
	std::string dest = narrow(ToFullPath(arg.array()[1].s_value));
    fix_filepath(src);
    fix_filepath(dest);

    // srcは通常ファイルでなければならない。
    // FCOPY("/dev/zero", "/tmp/hoge") とかやられたら嫌過ぎ。
    struct stat sb;
    if (stat(src.c_str(), &sb) != 0) {
	return CValue(0);
    }
    if ((sb.st_mode & S_IFREG) == 0) {
	return CValue(0);
    }

    // 実行
    unlink(dest.c_str()); // コピー先がシンボリックリンクとかだと嫌。
	std::ifstream is(src.c_str());
    int result = 0;
    if (is.good()) {
		std::ofstream os(dest.c_str());
	if (os.good()) {
		boost::scoped_array<char> buf(new char[512]);
	    while (is.good()) {
		is.read(buf.get(), 512);
		int len = is.gcount();
		if (len == 0) {
		    break;
		}
		os.write(buf.get(), len);
	    }
	    result = 1;
	}
    }
    
    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FMOVE
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FMOVE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FMOVE", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FMOVE", d, l);
		SetError(9);
		return CValue(0);
	}

	// 絶対パス化
	yaya::char_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_wsplitpath(arg.array()[0].s_value.c_str(), drive, dir, fname, ext);
	yaya::string_t	s_path = ((::wcslen(drive)) ? yaya::string_t(L"") : vm.basis().path) + arg.array()[0].s_value;

	yaya::char_t	fname2[_MAX_FNAME], ext2[_MAX_EXT];
	_wsplitpath(arg.array()[1].s_value.c_str(), drive, dir, fname2, ext2);
	yaya::string_t	d_path = ((::wcslen(drive)) ?
						yaya::string_t(L"") : vm.basis().path) + arg.array()[1].s_value + L"\\" + fname + ext;

	// パスをMBCSに変換
	char	*s_pstr = Ccct::Ucs2ToMbcs(s_path, CHARSET_DEFAULT);
	if (s_pstr == NULL) {
		vm.logger().Error(E_E, 89, L"FMOVE", d, l);
		return CValue(0);
	}
	char	*d_pstr = Ccct::Ucs2ToMbcs(d_path, CHARSET_DEFAULT);
	if (d_pstr == NULL) {
		free(s_pstr);
		vm.logger().Error(E_E, 89, L"FMOVE", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (MoveFile(s_pstr, d_pstr) ? 1 : 0);
	free(s_pstr);
	free(d_pstr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::FMOVE(const CValue &arg, yaya::string_t &d, int &l) {
    if (arg.array_size() < 2) {
	vm.logger().Error(E_W, 8, L"FMOVE", d, l);
	SetError(8);
	return CValue(0);
    }
    
    if (!arg.array()[0].IsString() ||
	!arg.array()[1].IsString()) {
	vm.logger().Error(E_W, 9, L"FMOVE", d, l);
	SetError(9);
	return CValue(0);
    }

    // 絶対パス化
	std::string src = narrow(ToFullPath(arg.array()[0].s_value));
	std::string dest = narrow(ToFullPath(arg.array()[1].s_value));
    fix_filepath(src);
    fix_filepath(dest);
    
    // 実行
    int result = rename(src.c_str(), dest.c_str()) ? 0 : 1;

    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::MKDIR
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::MKDIR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"MKDIR", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"MKDIR", d, l);
		SetError(9);
		return CValue(0);
	}

	// パスをMBCSに変換
	char	*s_dirstr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_dirstr == NULL) {
		vm.logger().Error(E_E, 89, L"MKDIR", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (::CreateDirectory(s_dirstr,NULL) ? 0 : 1);
	free(s_dirstr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::MKDIR(const CValue &arg, yaya::string_t &d, int &l) {
    if (!arg.array_size()) {
	vm.logger().Error(E_W, 8, L"MKDIR", d, l);
	SetError(8);
	return CValue(0);
    }
    
    if (!arg.array()[0].IsString()) {
	vm.logger().Error(E_W, 9, L"MKDIR", d, l);
	SetError(9);
	return CValue(0);
    }

	std::string dirstr = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(dirstr);

    // 実行
    int result = (mkdir(dirstr.c_str(), 0644) ? 0 : 1);

    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RMDIR
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::RMDIR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"RMDIR", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"RMDIR", d, l);
		SetError(9);
		return CValue(0);
	}

	// パスをMBCSに変換
	char	*s_dirstr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_dirstr == NULL) {
		vm.logger().Error(E_E, 89, L"RMDIR", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (::RemoveDirectory(s_dirstr) == 0 ? 0 : 1);
	free(s_dirstr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::RMDIR(const CValue &arg, yaya::string_t &d, int &l) {
    if (!arg.array_size()) {
	vm.logger().Error(E_W, 8, L"RMDIR", d, l);
	SetError(8);
	return CValue(0);
    }
    
    if (!arg.array()[0].IsString()) {
	vm.logger().Error(E_W, 9, L"RMDIR", d, l);
	SetError(9);
	return CValue(0);
    }

	std::string dirstr = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(dirstr);

    // 実行。
    int result = (rmdir(dirstr.c_str()) ? 0 : 1);

    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FDEL
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FDEL(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FDEL", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FDEL", d, l);
		SetError(9);
		return CValue(0);
	}

	// パスをMBCSに変換
	char	*s_filestr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FDEL", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (DeleteFile(s_filestr) ? 1 : 0);
	free(s_filestr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::FDEL(const CValue &arg, yaya::string_t &d, int &l) {
    if (!arg.array_size()) {
	vm.logger().Error(E_W, 8, L"FDEL", d, l);
	SetError(8);
	return CValue(0);
    }
    
    if (!arg.array()[0].IsString()) {
	vm.logger().Error(E_W, 9, L"FDEL", d, l);
	SetError(9);
	return CValue(0);
    }

	std::string filestr = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(filestr);

    // 実行
    int result = (unlink(filestr.c_str()) ? 0 : 1);

    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FRENAME
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FRENAME(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FRENAME", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FRENAME", d, l);
		SetError(9);
		return CValue(0);
	}

	// パスをMBCSに変換
	char	*s_filestr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FRENAME", d, l);
		return CValue(0);
	}
	char	*d_filestr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[1].s_value), CHARSET_DEFAULT);
	if (d_filestr == NULL) {
		free(s_filestr);
		vm.logger().Error(E_E, 89, L"FRENAME", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (MoveFile(s_filestr, d_filestr) ? 1 : 0);
	free(s_filestr);
	free(d_filestr);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::FRENAME(const CValue &arg, yaya::string_t &d, int &l) {
    if (arg.array_size() < 2) {
	vm.logger().Error(E_W, 8, L"FRENAME", d, l);
	SetError(8);
	return CValue(0);
    }
    
	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FRENAME", d, l);
		SetError(9);
		return CValue(0);
	}

    // 絶対パス化
	std::string src = narrow(ToFullPath(arg.array()[0].s_value));
	std::string dest = narrow(ToFullPath(arg.array()[1].s_value));
    fix_filepath(src);
    fix_filepath(dest);
    
    // 実行
    int result = rename(src.c_str(), dest.c_str()) ? 0 : 1;

    return CValue(result);
}
#endif


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FDIGEST
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FDIGEST(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FDIGEST", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FDIGEST", d, l);
		SetError(9);
		return CValue(-1);
	}

	yaya::string_t digest_type = L"md5";
	if (arg.array_size()>=2) {
		digest_type = arg.array()[1].GetValueString();
	}

	// パスをMBCSに変換
	const char *s_filestr;

#if defined(WIN32)	
	s_filestr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FDIGEST", d, l);
		return CValue(-1);
	}
#elif defined(POSIX)
	std::string path = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(path);
	s_filestr = path.c_str();
#endif

	// 実行
	FILE *pF = fopen(s_filestr,"rb");
	if ( ! pF ) { return CValue(-1); }

#if defined(WIN32)	
	free((void*)s_filestr);
#endif

	unsigned char digest_result[32];
	size_t digest_len;
	unsigned char buf[32768];

	if ( wcsicmp(digest_type.c_str(),L"sha1") == 0 || wcsicmp(digest_type.c_str(),L"sha-1") == 0 ) {
		SHA1Context sha1ctx;
		SHA1Reset(&sha1ctx);

		while ( TRUE ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			SHA1Input(&sha1ctx,buf,readsize);
			if ( readsize <= sizeof(buf) ) { break; }
		}

		SHA1Result(&sha1ctx,digest_result);
		digest_len = SHA1HashSize;
	}
	else if ( wcsicmp(digest_type.c_str(),L"crc32") == 0 ) {
		unsigned long crc = 0;
		while ( TRUE ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			crc = update_crc32(buf,readsize,crc);;
			if ( readsize <= sizeof(buf) ) { break; }
		}

		digest_result[0] = crc & 0xFFU;
		digest_result[1] = (crc >> 8) & 0xFFU;
		digest_result[2] = (crc >> 16) & 0xFFU;
		digest_result[3] = (crc >> 24) & 0xFFU;
		digest_len = 4;
	}
	else { //md5
		MD5_CTX md5ctx;
		MD5Init(&md5ctx);

		while ( TRUE ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			MD5Update(&md5ctx,buf,readsize);
			if ( readsize <= sizeof(buf) ) { break; }
		}

		MD5Final(digest_result,&md5ctx);
		digest_len = 16;
	}

	fclose(pF);

	yaya::char_t md5str[65];
	md5str[digest_len*2] = 0; //ゼロ終端

	for ( unsigned int i = 0 ; i < digest_len ; ++i ) {
		swprintf(md5str+i*2,L"%02X",digest_result[i]);
	}

	return CValue(yaya::string_t(md5str));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FSIZE
 *
 *  4GB以上のサイズは取得できません
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FSIZE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FSIZE", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FSIZE", d, l);
		SetError(9);
		return CValue(-1);
	}

	//すでに開いているファイルならそっちから情報をパクる
	yaya::string_t fullpath = ToFullPath(arg.array()[0].s_value);
	long size = vm.files().Size(fullpath);
	if ( size >= 0 ) { return CValue((int)size); }

	// パスをMBCSに変換
	char *s_filestr = Ccct::Ucs2ToMbcs(fullpath, CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FSIZE", d, l);
		return CValue(-1);
	}

	// 実行
	HANDLE	hFile = CreateFile(s_filestr, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	free(s_filestr);
	if (hFile == INVALID_HANDLE_VALUE)
		return CValue(-1);
	unsigned long	result = GetFileSize(hFile, NULL);
	CloseHandle(hFile);

	if (result == 0xFFFFFFFF)
		return CValue(-1);

	return CValue((int)result);
}
#elif defined(POSIX)
CValue CSystemFunction::FSIZE(const CValue &arg, yaya::string_t &d, int &l) {
    if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FSIZE", d, l);
		SetError(8);
		return CValue(-1);
    }
    
	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FSIZE", d, l);
		SetError(9);
		return CValue(-1);
    }

	yaya::string_t fullpath = ToFullPath(arg.array()[0].s_value);
	long size = vm.files().Size(fullpath);
	if ( size >= 0 ) { return CValue((int)size); }

	std::string path = narrow(fullpath);
    fix_filepath(path);

    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) {
	return CValue(-1);
    }
    return CValue(static_cast<int>(sb.st_size));
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FENUM
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::FENUM(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"FENUM", d, l);
		SetError(8);
		return CValue();
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"FENUM", d, l);
		SetError(9);
		return CValue();
	}

	// デリミタ取得
	yaya::string_t	delimiter = VAR_DELIMITER;
	if (sz >= 2) {
		if (arg.array()[1].IsString() &&
			arg.array()[1].s_value.size())
			delimiter = arg.array()[1].s_value;
		else {
			vm.logger().Error(E_W, 9, L"FENUM", d, l);
			SetError(9);
			return CValue();
		}
	}

	// パスをMBCSに変換
	yaya::string_t tmp_str = arg.array()[0].s_value + L"\\*.*";
	char	*s_filestr = Ccct::Ucs2ToMbcs(ToFullPath(tmp_str), CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FENUM", d, l);
		return CValue();
	}

	// 実行
	CValue result(F_TAG_STRING,0);
	HANDLE hFile;
	WIN32_FIND_DATA	w32FindData;
	hFile = FindFirstFile(s_filestr, &w32FindData);
	free(s_filestr);
	if(hFile != INVALID_HANDLE_VALUE) {
		int i = 0;
		do {
			// 1つ取得
			std::string	t_file =w32FindData.cFileName;
			// もし"."or".."なら飛ばす
			if (!t_file.compare(".") || !t_file.compare(".."))
				continue;
			// UCS2へ変換
			yaya::char_t	*t_wfile = Ccct::MbcsToUcs2(t_file, CHARSET_DEFAULT);
			if (t_wfile == NULL)
				continue;
			// 追加
			if (i)
				result.s_value += delimiter;
			if (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				result.s_value +=  L"\\";
			result.s_value +=  t_wfile;
			free(t_wfile);
			i++;
		}
		while(FindNextFile(hFile, &w32FindData));

		FindClose(hFile);
	}

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::FENUM(const CValue &arg, yaya::string_t &d, int &l) {
    int	sz = arg.array_size();
    
    if (!sz) {
	vm.logger().Error(E_W, 8, L"FENUM", d, l);
	SetError(8);
	return CValue();
    }
    
    if (!arg.array()[0].IsString()) {
	vm.logger().Error(E_W, 9, L"FENUM", d, l);
	SetError(9);
	return CValue();
    }
    
    // デリミタ取得
    yaya::string_t delimiter = VAR_DELIMITER;
    if (sz >= 2) {
	if (arg.array()[1].IsString() &&
	    arg.array()[1].s_value.size()) {
	    delimiter = arg.array()[1].s_value;
	}
	else {
	    vm.logger().Error(E_W, 9, L"FENUM", d, l);
	    SetError(9);
	    return CValue();
	}
    }

	std::string path = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(path);

    // 実行
	CValue result(F_TAG_STRING,0);
    DIR* dh = opendir(path.c_str());
    if (dh == NULL) {
	return result;
    }
    bool first_entry = true;
    while (true) {
	struct dirent* ent = readdir(dh);
	if (ent == NULL) {
	    break; // もう無い。
	}
	
	std::string name(ent->d_name, strlen(ent->d_name)/*ent->d_namlen*/);	// by umeici. 2005/1/16 5.6.0.232
	if (name == "." || name == "..") {
	    continue; // .と..は飛ばす
	}
	if (!first_entry) {
	    result.s_value += delimiter;
	}
	first_entry = false;

	struct stat sb;
	if (stat((path+'/'+name).c_str(), &sb) == 0) {
	    if (sb.st_mode & S_IFDIR) {
		name.insert(name.begin(), '\\');
	    }
	}
	result.s_value += widen(name);
    }
    closedir(dh);

    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FCHARSET
 *
 *  有効な値は、0/1/127=Shift_JIS/UTF-8/OSデフォルト　です。
 *  これ以外の値を与えた場合は無効で、warningとなります。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FCHARSET(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FCHARSET", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	int	charset = GetCharset(arg.array()[0],L"FCHARSET",d,l);
	if ( charset < 0 ) {
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	vm.files().SetCharset(charset);

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ARRAYSIZE
 *
 *  文字列の場合は簡易配列の、汎用配列の場合はその要素数を返します。
 *
 *  winnt.h とマクロが被った。減点２。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ArraySize(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &/*d*/, int &/*l*/)
{
	// 引数無しなら0
	int	sz = arg.array_size();
	if (!sz)
		return CValue(0);

	if (sz == 1) {
		// 引数1つで文字列でないなら1
		if (!arg.array()[0].IsString())
			return CValue(1);

		// 引数1つで文字列なら簡易配列の要素数を返す　変数の場合はそのデリミタで分割する
		yaya::string_t	delimiter = VAR_DELIMITER;
		if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
			delimiter = vm.variable().GetDelimiter(pcellarg[0]->index);
		else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
			delimiter = lvar.GetDelimiter(pcellarg[0]->name);

		return CValue((int)SplitToMultiString(arg.array()[0].s_value, NULL, delimiter));
	}

	// 汎用配列なら要素数をそのまま返す
	return CValue((int)sz);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SETDELIM
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	if (pcellarg.size() < 2) {
		vm.logger().Error(E_W, 8, L"SETDELIM", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	const CValue *delimiter;
	if (pcellarg[1]->value_GetType() >= F_TAG_ORIGIN_VALUE &&
		pcellarg[1]->value_GetType() <= F_TAG_STRING)
		delimiter = &(pcellarg[1]->value_const());
	else
		delimiter = &(pcellarg[1]->ansv_const());

	if (!delimiter->IsString()) {
		vm.logger().Error(E_W, 9, L"SETDELIM", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!delimiter->s_value.size()) {
		vm.logger().Error(E_W, 10, L"SETDELIM", d, l);
		SetError(10);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		vm.variable().SetDelimiter(pcellarg[0]->index, delimiter->s_value);
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		lvar.SetDelimiter(pcellarg[0]->name, delimiter->s_value);
	else {
		vm.logger().Error(E_W, 11, L"SETDELIM", d, l);
		SetError(11);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::EVAL
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::EVAL(const CValue &arg, yaya::string_t &d, int &l, CLocalVariable &lvar,
			CFunction *thisfunc)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"EVAL", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"EVAL", d, l);
		SetError(9);
	}

	// 数式へ展開
	yaya::string_t	str = arg.array()[0].GetValueString();
	CStatement	t_state(ST_FORMULA, l);
	if (vm.parser0().ParseEmbedString(str, t_state, d, l))
		return CValue(arg.array()[0].GetValueString());

	// 実行して結果を返す
	CValue	result = thisfunc->GetFormulaAnswer(lvar, t_state);
	if (t_state.type == ST_FORMULA_SUBST)
		return CValue(F_TAG_NOP, 0/*dmy*/);
	else
		return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ERASEVAR
 *
 *  ローカル変数では消去フラグを立てるだけです。
 *  グローバル変数では消去フラグを立て、さらにunload時にファイルへ値を保存しなくなります。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ERASEVAR(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	size_t arg_size = arg.array_size();

	if (!arg_size) {
		vm.logger().Error(E_W, 8, L"ERASEVAR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	for ( size_t i = 0 ; i < arg_size ; ++i ) {

		//文字列かどうかチェック - 警告は吐くが処理続行
		if ( ! arg.array()[i].IsString() ) {
			vm.logger().Error(E_W, 9, L"ERASEVAR", d, l);
			SetError(9);
		}

		const yaya::string_t &arg0 = arg.array()[i].GetValueString();
		if (!arg0.size()) {
			continue;
		}

		if (arg0[0] == L'_') {
			lvar.Erase(arg0);
		}
		else {
			vm.variable().Erase(arg0);
		}
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETTIME
 *  
 *  引数なしか、Epochからの秒数(GETSECCOUNT)
 *
 *  返値　　：  year,month,day,week(0-6),hour,minute,secondの汎用配列
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETTIME(const CValue &arg, yaya::string_t &d, int &l)
{
	time_t	ltime;

	if (!arg.array_size()) {
		time(&ltime);
	}
	else {
		ltime = arg.array()[0].GetValueInt();
	}

	struct tm *today = localtime(&ltime);

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

	if ( today ) {
		result.array().push_back(CValueSub(static_cast<int>(today->tm_year) + 1900));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_mon) + 1));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_mday)));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_wday)));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_hour)));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_min)));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_sec)));
	}
	else {
		vm.logger().Error(E_W, 12, L"GETTIME", d, l);
		SetError(12);
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETSECCOUNT
 *  
 *  引数なしか、year,month,day,week(0-6),hour,minute,secondの配列
 *
 *  返値　　：  EPOCHからの秒数
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETSECCOUNT(const CValue &arg, yaya::string_t &d, int &l)
{
	time_t	ltime;

	if (!arg.array_size()) {
		time(&ltime);
		return CValue((int)ltime);
	}

	struct tm input_time;
	time(&ltime);
	struct tm *today = localtime(&ltime);
	if ( today ) {
		input_time = *today;
		input_time.tm_yday = 0;
		input_time.tm_wday = 0;
	}

	unsigned int asize = arg.array_size();
	if ( asize > 7 ) { asize = 7; }

	switch ( asize ) {
	case 7:
		input_time.tm_sec = arg.array()[6].GetValueInt();
	case 6:
		input_time.tm_min = arg.array()[5].GetValueInt();
	case 5:
		input_time.tm_hour = arg.array()[4].GetValueInt();
	/*case 4:
		input_time.tm_wday = arg.array()[3].GetValueInt();*/ //代入禁止
	case 3:
		input_time.tm_mday = arg.array()[2].GetValueInt();
	case 2:
		input_time.tm_mon = arg.array()[1].GetValueInt()-1;
	case 1:
		input_time.tm_year = arg.array()[0].GetValueInt()-1900;
	}

	return CValue((int)mktime(&input_time));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETTICKCOUNT
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::GETTICKCOUNT(const CValue &arg, CLocalVariable &/*lvar*/, yaya::string_t &/*d*/, int &/*l*/)
{
	DWORD	tickcount = GetTickCount();
	int	highc = (int)(tickcount >> 31);
	int	lowc  = (int)(tickcount & 0x7fffffff);

	if (!arg.array_size())
		return lowc;

	if (arg.array()[0].IsInt() && arg.array()[0].GetValueInt() == 0)
		return lowc;

	return highc;
}
#elif defined(POSIX)
CValue CSystemFunction::GETTICKCOUNT(const CValue &arg, CLocalVariable &lvar, yaya::string_t &d, int &l) {
    struct timeval tv;
	struct timezone tz;
    gettimeofday(&tv, &tz);
    
    return static_cast<int>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETMEMINFO
 *
 *  返値　　：  memoryload,memorytotalphys,memoryavailphys,memorytotalvirtual,memoryavailvirtual
 *  　　　　　  の汎用配列
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::GETMEMINFO(void)
{
	MEMORYSTATUS	meminfo;
	GlobalMemoryStatus(&meminfo);

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

	result.array().push_back(CValueSub((int)meminfo.dwMemoryLoad)  );
	result.array().push_back(CValueSub((int)meminfo.dwTotalPhys)   );
	result.array().push_back(CValueSub((int)meminfo.dwAvailPhys)   );
	result.array().push_back(CValueSub((int)meminfo.dwTotalVirtual));
	result.array().push_back(CValueSub((int)meminfo.dwAvailVirtual));

	return result;
}
#elif defined(POSIX)
CValue CSystemFunction::GETMEMINFO(void) {
    // メモリの状態を取得するポータブルな方法は無いので…
    CValue result(F_TAG_ARRAY, 0/*dmy*/);
    result.array().push_back(CValueSub(0)); // dwMemoryLoad
    result.array().push_back(CValueSub(0)); // dwTotalPhys
    result.array().push_back(CValueSub(0)); // dwAvailPhys
    result.array().push_back(CValueSub(0)); // dwTotalVirtual
    result.array().push_back(CValueSub(0)); // dwAvailVirtual
    return result;
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_SEARCH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_SEARCH(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"RE_SEARCH", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_SEARCH", d, l);
		SetError(9);
	}
	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(0);

	// 実行
	int	t_result;
	try {
		boost::basic_regex<yaya::char_t> regex(arg1.c_str(),boost::regex::perl | boost::regex::collate | re_option);
		boost::match_results<yaya::string_t::const_iterator>	result;
		t_result = (int)boost::regex_search(arg0, result, regex);
		if (t_result)
			StoreReResultDetails(result);
	}
	catch(const boost::bad_expression &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_SEARCH", d, l);
		SetError(16);
	}
	catch(const std::runtime_error &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_SEARCH", d, l);
		SetError(16);
	}
	catch(...) {
		t_result = 0;
		vm.logger().Error(E_W, 17, L"RE_SEARCH", d, l);
		SetError(17);
	}

	return CValue(t_result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_MATCH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_MATCH(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"RE_MATCH", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_MATCH", d, l);
		SetError(9);
		return CValue(0);
	}

	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(0);

	// 実行
	int	t_result;
	try {
		boost::basic_regex<yaya::char_t> regex(arg1.c_str(),boost::regex::perl | boost::regex::collate | re_option);
		boost::match_results<yaya::string_t::const_iterator>	result;
		t_result = (int)boost::regex_match(arg0, result, regex);
		if (t_result)
			StoreReResultDetails(result);
	}
	catch(const boost::bad_expression &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_MATCH", d, l);
		SetError(16);
	}
	catch(const std::runtime_error &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_MATCH", d, l);
		SetError(16);
	}
	catch(...) {
		t_result = 0;
		vm.logger().Error(E_W, 17, L"RE_MATCH", d, l);
		SetError(17);
	}

	return CValue(t_result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_GREP
 *
 *  regex_grepは使用せず、regex_searchを繰り返し実行することで同等の機能としています。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_GREP(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"RE_GREP", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_GREP", d, l);
		SetError(9);
	}

	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(0);

	// 実行
	yaya::string_t::const_iterator str = arg0.begin();
	yaya::string_t::const_iterator search_end = arg0.end();
	yaya::string_t::const_iterator search_point = str;
	int	t_pos    = 0;
	int	t_result = 0;

	try {
		boost::basic_regex<yaya::char_t> regex(arg1.c_str(),boost::regex::perl | boost::regex::collate | re_option);
		boost::match_results<yaya::string_t::const_iterator>	result;
		for( ; ; ) {
			if (!boost::regex_search(search_point, search_end, result, regex))
				break;
			t_result++;
			AppendReResultDetail(result.str(0), result.position(0) + t_pos, result.length(0));
			search_point = str + (t_pos += (result.position(0) + result.length(0)));
		}
	}
	catch(const boost::bad_expression &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_GREP", d, l);
		SetError(16);
	}
	catch(const std::runtime_error &) {
		t_result = 0;
		vm.logger().Error(E_W, 16, L"RE_GREP", d, l);
		SetError(16);
	}
	catch(...) {
		t_result = 0;
		vm.logger().Error(E_W, 17, L"RE_GREP", d, l);
		SetError(17);
	}

	return CValue(t_result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SETLASTERROR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SETLASTERROR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SETLASTERROR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (arg.array()[0].IsNum()) {
		lasterror = arg.array()[0].GetValueInt();
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	vm.logger().Error(E_W, 9, L"SETLASTERROR", d, l);
	SetError(9);
	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_OPTION
 *
 *  引数1個：Perlスタイルの正規表現オプション
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_OPTION(const CValue &arg, yaya::string_t &d, int &l)
{
	// 引数の数/型チェック
	if (arg.array_size() >= 1) {
		yaya::string_t opt = arg.array()[0].GetValueString();

		re_option = 0;
		if ( opt.find(L"m") == yaya::string_t::npos ) {
			re_option |= boost::regex::no_mod_m;
		}
		if ( opt.find(L"s") != yaya::string_t::npos ) {
			re_option |= boost::regex::mod_s;
		}
		if ( opt.find(L"x") != yaya::string_t::npos ) {
			re_option |= boost::regex::mod_x;
		}
		if ( opt.find(L"i") != yaya::string_t::npos ) {
			re_option |= boost::regex::icase;
		}
	}

	yaya::string_t result = L"";

	if ( (re_option & boost::regex::no_mod_m) == 0 ) {
		result += L"m";
	}
	if ( (re_option & boost::regex::mod_s) != 0 ) {
		result += L"s";
	}
	if ( (re_option & boost::regex::mod_x) != 0 ) {
		result += L"x";
	}
	if ( (re_option & boost::regex::icase) != 0 ) {
		result += L"i";
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_SPLIT
 *
 *  regex_splitは使用せず、regex_searchを繰り返し実行することで同等の機能としています。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_SPLIT(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	int sz = arg.array_size();
	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"RE_SPLIT", d, l);
		SetError(8);
		return CValue(0);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_SPLIT", d, l);
		SetError(9);
	}

	yaya::string_t::size_type nums = 0;
	if (sz > 2) {
		if (!arg.array()[2].IsNum()) {
			vm.logger().Error(E_W, 9, L"RE_SPLIT", d, l);
			SetError(9);
			return CValue(F_TAG_ARRAY, 0/*dmy*/);
		}
		nums = static_cast<yaya::string_t::size_type>(arg.array()[2].GetValueInt());

		if ( nums <= 1 ) {
			return CValue(arg.array()[0]);
		}
	}

	return RE_SPLIT_CORE(arg, d, l, L"RE_SPLIT", NULL, nums);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_REPLACE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_REPLACE(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"RE_REPLACE", d, l);
		SetError(8);
		return CValue(arg.array()[0].GetValueString());
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString() ||
		!arg.array()[2].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_REPLACE", d, l);
		SetError(9);
	}

	// まずsplitする
	CValue	splits = RE_SPLIT_CORE(arg, d, l, L"RE_REPLACE", NULL, 0);
	int	num = splits.array_size();
	if (!num || num == 1)
		return CValue(arg.array()[0].GetValueString());

	// 置換後文字列の作成
	yaya::string_t	result;
	int	i = 0;
	for(i = 0; i < num; i++) {
		if (i)
			result += arg.array()[2].GetValueString();
		result += splits.array()[i].GetValueString();
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_REPLACEEX
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_REPLACEEX(const CValue &arg, yaya::string_t &d, int &l)
{
	ClearReResultDetails();

	// 引数の数/型チェック
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"RE_REPLACE", d, l);
		SetError(8);
		return CValue(arg.array()[0].GetValueString());
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString() ||
		!arg.array()[2].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_REPLACEEX", d, l);
		SetError(9);
	}

	// 置換後文字列の用意
	std::vector<yaya::string_t> replace_array;

	// まずsplitする
	CValue	splits = RE_SPLIT_CORE(arg, d, l, L"RE_REPLACEEX", &replace_array, 0);
	int	num = splits.array_size();
	if (!num || num == 1)
		return CValue(arg.array()[0].GetValueString());

	// 置換後文字列の作成
	yaya::string_t	result;
	int	i = 0;
	for(i = 0; i < num; i++) {
		if (i)
			result += replace_array[i-1];
		result += splits.array()[i].GetValueString();
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_SPLIT_CORE
 *
 *  RE_SPLITの主処理部分です。RE_REPLACEでも使用します。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_SPLIT_CORE(const CValue &arg, yaya::string_t &d, int &l, const yaya::char_t *fncname, std::vector<yaya::string_t> *replace_array, size_t num)
{
	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(arg0);

	yaya::string_t::const_iterator str = arg0.begin();
	yaya::string_t::const_iterator search_point = str;
	yaya::string_t::const_iterator search_end = arg0.end();

	int	t_pos = 0;
	size_t count = 1;
	CValue	splits(F_TAG_ARRAY, 0/*dmy*/);

	try {
		boost::basic_regex<yaya::char_t> regex(arg1.c_str(),boost::regex::perl | boost::regex::collate | re_option);
		boost::match_results<yaya::string_t::const_iterator>	result;
		for( ; ; ) {
			if (!boost::regex_search(search_point, search_end, result, regex))
				break;

			splits.array().push_back(arg0.substr(t_pos, result.position(0)));

			AppendReResultDetail(result.str(0), result.position(0) + t_pos, result.length(0));
			
			if ( replace_array ) {
				replace_array->push_back(result.format(arg.array()[2].GetValueString(),boost::format_perl));
			}

			search_point = str + (t_pos += (result.position(0) + result.length(0)));
			++count;

			if ( num != 0 && (count >= num) ) {
				break;
			}
		}
        int len = std::distance(search_point, search_end);
//		if (len)
			splits.array().push_back(arg0.substr(t_pos, len));
	}
	catch(const boost::bad_expression &) {
		splits = CValue(F_TAG_ARRAY, 0/*dmy*/);
		vm.logger().Error(E_W, 16, fncname, d, l);
		SetError(16);
	}
	catch(const std::runtime_error &) {
		splits = CValue(F_TAG_ARRAY, 0/*dmy*/);
		vm.logger().Error(E_W, 16, fncname, d, l);
		SetError(16);
	}
	catch(...) {
		splits = CValue(F_TAG_ARRAY, 0/*dmy*/);
		vm.logger().Error(E_W, 17, fncname, d, l);
		SetError(17);
	}

	return splits;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CHRCODE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CHRCODE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CHRCODE", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"CHRCODE", d, l);
		SetError(9);
		return CValue(0);
	}

	if (!arg.array()[0].s_value.size()) {
		vm.logger().Error(E_W, 10, L"CHRCODE", d, l);
		SetError(10);
		return CValue(0);
	}

	size_t getpos = 0;

	if (arg.array_size() >= 2) {
		if (!arg.array()[1].IsInt()) {
			vm.logger().Error(E_W, 9, L"CHRCODE", d, l);
			SetError(9);
			return CValue(0);
		}
		getpos = arg.array()[1].GetValueInt();
		if ( getpos >= arg.array()[0].s_value.length() ) {
			getpos = arg.array()[0].s_value.length() - 1;
		}
	}

	return CValue(static_cast<int>(arg.array()[0].s_value[getpos]));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ISINTSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ISINTSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ISINTSTR", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"ISINTSTR", d, l);
		SetError(9);
		return CValue(0);
	}

	return CValue((int)IsIntString(arg.array()[0].s_value));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ISREALSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ISREALSTR(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"ISREALSTR", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"ISREALSTR", d, l);
		SetError(9);
		return CValue(0);
	}

	return CValue( static_cast<int>(IsIntString(arg.array()[0].s_value) || IsDoubleButNotIntString(arg.array()[0].s_value)) );
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SPLITPATH
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue	CSystemFunction::SPLITPATH(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SPLITPATH", d, l);
		SetError(8);
		return CValue(0);
	}

    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"SPLITPATH", d, l);
		SetError(9);
	}

	yaya::char_t drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	yaya::string_t path = arg.array()[0].GetValueString();

	_wsplitpath(path.c_str(), drive, dir, fname, ext);

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);
	result.array().push_back(drive);
	result.array().push_back(dir);
	result.array().push_back(fname);
	result.array().push_back(ext);

	return CValue(result);
}
#elif defined(POSIX)
CValue CSystemFunction::SPLITPATH(const CValue &arg, yaya::string_t &d, int &l) {
    if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"SPLITPATH", d, l);
		SetError(8);
		return CValue(0);
    }
    
    if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"SPLITPATH", d, l);
		SetError(9);
    }

    yaya::string_t path = arg.array()[0].GetValueString();
    fix_filepath(path);

    CValue result(F_TAG_ARRAY, 0/*dmy*/);
    result.array().push_back(L""); // driveは常に空文字列
	
    yaya::string_t::size_type pos_slash = path.rfind(L'/');
    yaya::string_t fname;
    if (pos_slash == yaya::string_t::npos) {
		result.array().push_back(L""); // dirも空
		fname = path;
    }
    else {
		result.array().push_back(path.substr(0, pos_slash+1));
		fname = path.substr(pos_slash+1);
    }
	
    yaya::string_t::size_type pos_period = fname.rfind(L'.');
    if (pos_period == yaya::string_t::npos) {
		result.array().push_back(fname);
		result.array().push_back(L""); // extは空
    }
    else {
		result.array().push_back(fname.substr(0, pos_period));
		result.array().push_back(fname.substr(pos_period+1));
    }
	
    return CValue(result);
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CVINT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CVINT(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CVINT", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		vm.variable().SetValue(pcellarg[0]->index, arg.array()[0].GetValueInt());
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		lvar.SetValue(pcellarg[0]->name, CValue(arg.array()[0].GetValueInt()));
	else {
		vm.logger().Error(E_W, 11, L"CVINT", d, l);
		SetError(11);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CVSTR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CVSTR(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CVSTR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		vm.variable().SetValue(pcellarg[0]->index, arg.array()[0].GetValueString());
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		lvar.SetValue(pcellarg[0]->name, CValue(arg.array()[0].GetValueString()));
	else {
		vm.logger().Error(E_W, 11, L"CVSTR", d, l);
		SetError(11);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CVREAL
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CVREAL(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CVREAL", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		vm.variable().SetValue(pcellarg[0]->index, arg.array()[0].GetValueDouble());
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		lvar.SetValue(pcellarg[0]->name, CValue(arg.array()[0].GetValueDouble()));
	else {
		vm.logger().Error(E_W, 11, L"CVREAL", d, l);
		SetError(11);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::CVAUTO
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::CVAUTO(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"CVAUTO", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString()) {
		return CValue(arg.array()[0]);
	}

	if ( IsIntString(arg.array()[0].GetValueString()) ) {
		if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE) {
			vm.variable().SetValue(pcellarg[0]->index, arg.array()[0].GetValueInt());
		}
		else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE) {
			lvar.SetValue(pcellarg[0]->name, CValue(arg.array()[0].GetValueInt()));
		}
		else {
			vm.logger().Error(E_W, 11, L"CVAUTO", d, l);
			SetError(11);
		}
	}
	else if ( IsDoubleButNotIntString(arg.array()[0].GetValueString()) ) {
		if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE) {
			vm.variable().SetValue(pcellarg[0]->index, arg.array()[0].GetValueDouble());
		}
		else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE) {
			lvar.SetValue(pcellarg[0]->name, CValue(arg.array()[0].GetValueDouble()));
		}
		else {
			vm.logger().Error(E_W, 11, L"CVAUTO", d, l);
			SetError(11);
		}
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LETTONAME
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LETTONAME(const CValue &arg, yaya::string_t &d, int &l, CLocalVariable &lvar,
			CFunction *thisfunc)
{
	size_t sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"LETTONAME", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"LETTONAME", d, l);
		SetError(9);
	}

	yaya::string_t	vname = arg.array()[0].GetValueString();

	CValue val;
	if ( sz == 2 ) {
		val = arg.array()[1];
	}
	else {
		val.SetType(F_TAG_ARRAY);
		for ( size_t i = 1 ; i < sz ; ++i ) {
			val.array().push_back(arg.array()[i]);
		}
	}

	if ( vname[0] == L'_' ) {
		lvar.SetValue(vname,val);
	}
	else {
		int	index = vm.variable().Make(vname, 0);
		vm.variable().SetValue(index,val);
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRFORM
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::STRFORM(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz)
		return CValue();

	if (sz == 1)
		return CValue(arg.array()[0]);

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"STRFORM", d, l);
		SetError(9);
		return CValue(arg.array()[0]);
	}

	// '$'でsplitする
	std::vector<yaya::string_t>	vargs;
	int	vargs_sz = SplitToMultiString(arg.array()[0].GetValueString(), &vargs, yaya::string_t(L"$"));
	if (!vargs_sz)
		return CValue();

	// 各要素ごとに_snwprintfで書式化して結合していく
	yaya::string_t	left, right;
	yaya::string_t	result = vargs[0];
	yaya::char_t	t_str[128];
	for(int i = 1; i < vargs_sz; i++) {
		yaya::string_t	format = L"%" + vargs[i];
		if (i < sz) {
			switch(arg.array()[i].GetType()) {
			case F_TAG_INT:
				yaya::snprintf(t_str,128,format.c_str(),arg.array()[i].i_value);
				break;
			case F_TAG_DOUBLE:
				yaya::snprintf(t_str,128,format.c_str(),arg.array()[i].d_value);
				break;
			case F_TAG_STRING:
				yaya::snprintf(t_str,128,format.c_str(),arg.array()[i].s_value.c_str());
				break;
			case F_TAG_VOID:
				t_str[0] = 0;
				break;
			default:
				vm.logger().Error(E_E, 91, d, l);
				break;
			};
			result += t_str;
		}
		else
			result += L"$" + vargs[i];
	}

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ANY
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ANY(const CValue &arg, const std::vector<CCell *> &pcellarg, CLocalVariable &lvar,
			yaya::string_t &/*d*/, int &/*l*/)
{
	// 引数無しなら空文字列
	int	sz = arg.array_size();
	if (!sz) {
		SetLso(-1);
		return CValue();
	}

	if (sz == 1) {
		// 引数1つで文字列でないなら引数がそのまま返る
		if (!arg.array()[0].IsString()) {
			SetLso(0);
			return CValue(arg.array()[0]);
		}

		// 引数1つで文字列なら簡易配列として処理　変数の場合はそのデリミタで分割する
		yaya::string_t	delimiter = VAR_DELIMITER;
		if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
			delimiter = vm.variable().GetDelimiter(pcellarg[0]->index);
		else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
			delimiter = lvar.GetDelimiter(pcellarg[0]->name);

		std::vector<yaya::string_t>	s_array;
		int	a_sz = SplitToMultiString(arg.array()[0].GetValueString(), &s_array, delimiter);
		if (!a_sz) {
			SetLso(-1);
			return CValue();
		}

		int s_pos = vm.genrand_int(a_sz);
		SetLso(s_pos);
		return CValue(s_array[s_pos]);
	}

	// 引数が複数なら汎用配列として処理
	int s_pos = vm.genrand_int(sz);
	SetLso(s_pos);
	return CValue(arg.array()[s_pos]);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SAVEVAR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SAVEVAR(const CValue &arg, yaya::string_t &d, int &l)
{
	if ( arg.array_size() ) {
		yaya::string_t path = arg.array()[0].GetValueString();
		vm.basis().SaveVariable(path.c_str());
	}
	else {
		vm.basis().SaveVariable();
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RESTOREVAR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RESTOREVAR(const CValue &arg, yaya::string_t &d, int &l)
{
	if ( arg.array_size() ) {
		yaya::string_t path = arg.array()[0].GetValueString();
		vm.basis().RestoreVariable(path.c_str());
	}
	else {
		vm.basis().RestoreVariable();
	}

	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETSTRBYTES
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETSTRBYTES(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"GETSTRBYTES", d, l);
		SetError(8);
		return CValue(0);
	}

	// 文字コード取得
	int	charset = CHARSET_SJIS;
	if (sz > 1) {
		charset = GetCharset(arg.array()[1],L"GETSTRBYTES",d,l);
		if ( charset < 0 ) {
			return CValue(0);
		}
	}
	
	// 主処理
	char	*t_str = Ccct::Ucs2ToMbcs(arg.array()[0].GetValueString(), charset);
	if (t_str == NULL) {
		vm.logger().Error(E_E, 89, L"GETSTRBYTES", d, l);
		return CValue(0);
	}
	int	result = ::strlen(t_str);
	free(t_str);

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRENCODE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::STRENCODE(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"STRENCODE", d, l);
		SetError(8);
		return CValue(0);
	}

	// 文字コード取得
	int	charset = CHARSET_SJIS;
	if (sz > 1) {
		charset = GetCharset(arg.array()[1],L"STRENCODE",d,l);
		if ( charset < 0 ) {
			return CValue(0);
		}
	}
	
	//変換タイプ
	yaya::string_t type = L"url";
	if ( sz > 2 ) {
		type = arg.array()[2].GetValueString();
		std::transform(type.begin(), type.end(), type.begin(), (int (*)(int))std::tolower);
	}
	
	// 主処理
	char *t_str = Ccct::Ucs2ToMbcs(arg.array()[0].GetValueString(), charset);
	if (t_str == NULL) {
		vm.logger().Error(E_E, 89, L"STRENCODE", d, l);
		return CValue(0);
	}
	
	yaya::native_signed len = strlen(t_str);

	yaya::string_t result;
	result.reserve(len);

	if ( wcsicmp(type.c_str(),L"base64") == 0 ) {
		EncodeBase64(result,t_str,strlen(t_str));
	}
	else {
		EncodeURL(result,t_str,strlen(t_str));
	}

	free(t_str);

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRDECODE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::STRDECODE(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"STRDECODE", d, l);
		SetError(8);
		return CValue(0);
	}

	// 文字コード取得
	int	charset = CHARSET_SJIS;
	if (sz > 1) {
		charset = GetCharset(arg.array()[1],L"STRDECODE",d,l);
		if ( charset < 0 ) {
			return CValue(0);
		}
	}

	//変換タイプ
	yaya::string_t type = L"url";
	if ( sz > 2 ) {
		type = arg.array()[2].GetValueString();
		std::transform(type.begin(), type.end(), type.begin(), (int (*)(int))std::tolower);
	}
	
	// 主処理
	yaya::string_t src = arg.array()[0].GetValueString();
	yaya::string_t::iterator end = src.end();

	std::string str;
	str.reserve(src.size());

	if ( wcsicmp(type.c_str(),L"base64") == 0 ) {
		static const unsigned char reverse_64[] = {
			//0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00 - 0x0F
			  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10 - 0x1F
			  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,   // 0x20 - 0x2F
			 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,   // 0x30 - 0x3F
			  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   // 0x40 - 0x4F
			 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,   // 0x50 - 0x5F
			  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   // 0x60 - 0x6F
			 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0    // 0x70 - 0x7F
		};

		const yaya::char_t* p = src.c_str();
		while (*p!='=')
		{
			//11111122:22223333:33444444
			if (*p=='\0' || *(p+1)=='=') break;
			str.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<2) & 0xFC | (reverse_64[*(p+1)&0x7f] >>4) & 0x03));
			++p;

			if (*p=='\0' || *(p+1)=='=') break;
			str.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<4) & 0xF0 | (reverse_64[*(p+1)&0x7f] >>2) & 0x0F));
			++p;

			if (*p=='\0' || *(p+1)=='=') break;
			str.append(1,static_cast<unsigned char>((reverse_64[*p&0x7f] <<6) & 0xC0 | reverse_64[*(p+1)&0x7f] & 0x3f ));
			++p;

			if (*p=='\0' || *(p+1)=='=') break;
			++p;
		}
	}
	else { //if url
		char ch[3] = {0,0,0};

		for ( yaya::string_t::iterator it = src.begin() ; it < end ; ++it ) {
			if ( *it == L'%' && (end - it) >= 3) {
				ch[0] = static_cast<char>(*(it+1));
				ch[1] = static_cast<char>(*(it+2));
				str.append(1,static_cast<char>(strtol(ch,NULL,16)));
				it += 2;
			}
			else if ( *it == L'+' ) {
				str.append(1,' ');
			}
			else {
				str.append(1,static_cast<char>(*it));
			}
		}
	}

	yaya::char_t *t_str = Ccct::MbcsToUcs2(str, charset);
	if (t_str == NULL) {
		vm.logger().Error(E_E, 89, L"STRDECODE", d, l);
		return CValue(0);
	}

	CValue result(t_str);
	free(t_str);

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ASEARCH
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ASEARCH(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"ASEARCH", d, l);
		SetError(8);
		return CValue(-1);
	}

	const CValueSub &key = arg.array()[0];
	for(int i = 1; i < sz; i++)
		if (key.Compare(arg.array()[i]))
			return CValue(i - 1);

	return CValue(-1);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ASEARCHEX
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"ASEARCHEX", d, l);
		SetError(8);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);
	const CValueSub &key = arg.array()[0];
	for(int i = 1; i < sz; i++)
		if (key.Compare(arg.array()[i]))
			result.array().push_back(CValueSub(i - 1));

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETDELIM
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETDELIM(const std::vector<CCell *> &pcellarg, CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	if (!pcellarg.size()) {
		vm.logger().Error(E_W, 8, L"GETDELIM", d, l);
		SetError(8);
		return CValue(L"");
	}

	CValue	delimiter(L"");
	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		delimiter = vm.variable().GetDelimiter(pcellarg[0]->index);
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		delimiter = lvar.GetDelimiter(pcellarg[0]->name);
	else {
		vm.logger().Error(E_W, 18, L"GETDELIM", d, l);
		SetError(18);
	}

	return delimiter;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SETSETTING
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SETSETTING(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"SETSETTING", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if ( vm.basis().SetParameter(arg.array()[0].GetValueString(),arg.array()[1].GetValueString()) ) {
		return CValue(1);
	}
	else {
		return CValue(0);
	}
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETSETTING
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETSETTING(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (!sz) {
		vm.logger().Error(E_W, 8, L"GETSETTING", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (arg.array()[0].IsString()) {
		const yaya::string_t str = arg.array()[0].GetValueString();
		
		if ( !str.compare(L"coreinfo.version") ) {
			return CValue(yaya::string_t(aya_version));
		}
		if ( !str.compare(L"coreinfo.path") ) {
			return CValue(vm.basis().GetRootPath());
		}
		if ( !str.compare(L"coreinfo.name") ) {
			return CValue(yaya::string_t(aya_name));
		}
		if ( !str.compare(L"coreinfo.author") ) {
			return CValue(yaya::string_t(aya_author));
		}

		return vm.basis().GetParameter(str);
	}
	else {
		switch(arg.array()[0].GetValueInt()) {
		case 0:	// AYAINFO_VERSION
			return CValue(yaya::string_t(aya_version));
		case 1:	// AYAINFO_CHARSET
			return CValue(static_cast<int>(vm.basis().GetDicCharset()));
		case 2:	// AYAINFO_PATH
			return CValue(vm.basis().GetRootPath());
		case 3:	// AYAINFO_NAME
			return CValue(yaya::string_t(aya_name));
		case 4:	// AYAINFO_AUTHOR
			return CValue(yaya::string_t(aya_author));
		default:
			break;
		};
	}

	vm.logger().Error(E_W, 12, L"GETSETTING", d, l);
	SetError(12);
	return CValue(F_TAG_NOP, 0/*dmy*/);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SPLIT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SPLIT(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"SPLIT", d, l);
		SetError(8);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"SPLIT", d, l);
		SetError(9);
	}

	yaya::string_t::size_type nums = 0;
	if (sz > 2) {
		if (!arg.array()[2].IsNum()) {
			vm.logger().Error(E_W, 9, L"SPLIT", d, l);
			SetError(9);
			return CValue(F_TAG_ARRAY, 0/*dmy*/);
		}
		nums = static_cast<yaya::string_t::size_type>(arg.array()[2].GetValueInt());
	}

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

	const yaya::string_t &tgt_str = arg.array()[0].GetValueString();
	const yaya::string_t &sep_str = arg.array()[1].GetValueString();

	if (nums == 1 || sep_str.length() == 0) {
		result.array().push_back(CValueSub(arg.array()[0].GetValueString()));
		return result;
	}
	
	const yaya::string_t::size_type sep_strlen = sep_str.size();
	const yaya::string_t::size_type tgt_strlen = tgt_str.size();
	yaya::string_t::size_type seppoint = 0;
	yaya::string_t::size_type spoint;

	for(yaya::string_t::size_type i = 1; ; i++) {
		spoint = tgt_str.find(sep_str,seppoint);
		if (spoint == yaya::string_t::npos || i == nums) {
			result.array().push_back(CValueSub(tgt_str.substr(seppoint,tgt_strlen - seppoint)));
			break;
		}
		result.array().push_back(CValueSub(tgt_str.substr(seppoint, spoint-seppoint)));
		seppoint = spoint + sep_strlen;
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::FATTRIB
 * -----------------------------------------------------------------------
 */

#if defined(WIN32)
static time_t FileTimeToUnixTime(FILETIME &filetime)
{
    FILETIME localfiletime;
    SYSTEMTIME systime;
    struct tm utime;
    FileTimeToLocalFileTime(&filetime, &localfiletime);
    FileTimeToSystemTime(&localfiletime, &systime);
    utime.tm_sec=systime.wSecond;
    utime.tm_min=systime.wMinute;
    utime.tm_hour=systime.wHour;
    utime.tm_mday=systime.wDay;
    utime.tm_mon=systime.wMonth-1;
    utime.tm_year=systime.wYear-1900;
    utime.tm_isdst=-1;
    return(mktime(&utime));
}
#endif

CValue	CSystemFunction::FATTRIB(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"FATTRIB", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsStringReal()) {
		vm.logger().Error(E_W, 9, L"FATTRIB", d, l);
		SetError(9);
		return CValue(-1);
	}

#if defined(WIN32)
	// パスをMBCSに変換
	char	*s_filestr = Ccct::Ucs2ToMbcs(ToFullPath(arg.array()[0].s_value), CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"FATTRIB", d, l);
		return CValue(-1);
	}

	// 取得
	DWORD	attrib = GetFileAttributes(s_filestr);
	if (attrib == 0xFFFFFFFF) {
		return CValue(-1);
	}

	// 返値生成
	CValue	result(F_TAG_ARRAY, 0/*dmy*/);

	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_ARCHIVE   ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_COMPRESSED) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_DIRECTORY ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_HIDDEN    ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib == FILE_ATTRIBUTE_NORMAL   ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_OFFLINE   ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_READONLY  ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_SYSTEM    ) ? 1 : 0));
	result.array().push_back(CValueSub((attrib & FILE_ATTRIBUTE_TEMPORARY ) ? 1 : 0));

	if ( attrib & FILE_ATTRIBUTE_DIRECTORY ) { //ディレクトリ
		//GetFileAttributesExつかいたい、けどWin95蹴るので却下
		size_t len = strlen(s_filestr);
		char *s_newstr = (char*)malloc(len+3);
		strcpy(s_newstr,s_filestr);

		if ( s_filestr[len-1] != '\\' && s_filestr[len-1] != '/' ) {
			strcat(s_newstr,"\\*");
		}
		else {
			strcat(s_newstr,"*");
		}
		free(s_filestr);
		s_filestr = s_newstr;

		WIN32_FIND_DATA ffdata;
		HANDLE hFind = ::FindFirstFile(s_filestr,&ffdata);
		if ( hFind == INVALID_HANDLE_VALUE ) { return CValue(-1); }
		
		bool found = false;
		do {
			if ( strcmp(ffdata.cFileName,".") == 0 ) {
				found = true;
				break;
			}
		} while ( ::FindNextFile(hFind,&ffdata) );
		::FindClose(hFind);
		
		if ( ! found ) {
			result.array().push_back(CValueSub(0));
			result.array().push_back(CValueSub(0));
		}
		else {
			result.array().push_back(CValueSub((int)FileTimeToUnixTime(ffdata.ftCreationTime)));
			result.array().push_back(CValueSub((int)FileTimeToUnixTime(ffdata.ftLastWriteTime)));
		}
	}
	else { //ただのファイル
		HANDLE hFile = ::CreateFile(s_filestr , GENERIC_READ , FILE_SHARE_READ | FILE_SHARE_WRITE , NULL ,OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			result.array().push_back(CValueSub(0));
			result.array().push_back(CValueSub(0));
		}
		else {
			FILETIME ftctime,ftmtime;
			::GetFileTime(hFile , &ftctime , NULL , &ftmtime);

			result.array().push_back(CValueSub((int)FileTimeToUnixTime(ftctime)));
			result.array().push_back(CValueSub((int)FileTimeToUnixTime(ftmtime)));

			::CloseHandle(hFile);
		}
	}
	free(s_filestr);

#elif defined(POSIX)
	std::string path = narrow(ToFullPath(arg.array()[0].s_value));
    fix_filepath(path);

    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) {
		return CValue(-1);
    }

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(S_ISDIR(sb.st_mode) ? 1 : 0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(S_ISREG(sb.st_mode) ? 1 : 0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(0));
	result.array().push_back(CValueSub(sb.st_ctime);
	result.array().push_back(CValueSub(sb.st_mtime);
#endif

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETFUNCLIST
 *  引数　　：　_argv[0] = 絞りこみ文字列
 *  　　　　　　指定した文字列が頭についてるもののみ抽出して配列で返す
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETFUNCLIST(const CValue &arg, yaya::string_t &/*d*/, int &/*l*/)
{
	yaya::string_t name;

	//STRINGの場合のみ絞りこみ文字列として認識
	if ( arg.array_size() ) {
		if (arg.array()[0].IsString()) {
			name = arg.array()[0].GetValueString();
		}
	}

	CValue result(F_TAG_ARRAY, 0/*dmy*/);

	//絞りこみ文字列がない場合
	if ( name.empty() ) {
		for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
			result.array().push_back(CValueSub(it->name));
		}
	}
	//ある場合
	else {
		yaya::string_t::size_type len = name.length();

		for(std::vector<CFunction>::iterator it = vm.function().begin(); it != vm.function().end(); it++) {
			if(name.compare(0,len,it->name,0,len) == 0) {
				result.array().push_back(CValueSub(it->name));
			}
		}
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETVARLIST
 *  引数　　：　_argv[0] = 絞りこみ文字列
 *  　　　　　　指定した文字列が頭についてるもののみ抽出して配列で返す
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETVARLIST(const CValue &arg, CLocalVariable &lvar, yaya::string_t &/*d*/, int &/*l*/)
{
	yaya::string_t name;

	//STRINGの場合のみ絞りこみ文字列として認識
	if ( arg.array_size() ) {
		if (arg.array()[0].IsString()) {
			name = arg.array()[0].GetValueString();
		}
	}

	CValue result(F_TAG_ARRAY, 0/*dmy*/);

	//絞りこみ文字列がない場合
	if ( name.empty() ) {
		//グローバル変数
		size_t n = vm.variable().GetNumber();

		for(size_t i = 0; i < n; ++i) {
			CVariable *pVal = vm.variable().GetPtr(i);
			if (pVal && !pVal->IsErased()) {
				result.array().push_back(CValueSub(pVal->name));
			}
		}

		unsigned int depthmax = lvar.GetDepth();
		
		//ローカル変数
		for(unsigned int depth = 0; depth < depthmax; ++depth) {
			size_t n = lvar.GetNumber(depth);
			for(size_t i = 0; i < n; ++i) {
				CVariable *pVal = lvar.GetPtr(depth,i);
				if (pVal && !pVal->IsErased()) {
					result.array().push_back(CValueSub(pVal->name));
				}
			}
		}
	}
	//ある場合
	else {
		yaya::string_t::size_type len = name.length();

		if (name[0] != L'_') {
			//グローバル変数
			size_t n = vm.variable().GetNumber();

			for(size_t i = 0; i < n; ++i) {
				CVariable *pVal = vm.variable().GetPtr(i);
				if (pVal && !pVal->IsErased()) {
					if(name.compare(0,len,pVal->name,0,len) == 0) {
						result.array().push_back(CValueSub(pVal->name));
					}
				}
			}
		}
		else {
			//ローカル変数
			unsigned int depthmax = lvar.GetDepth();
			
			for(unsigned int depth = 0; depth < depthmax; ++depth) {
				size_t n = lvar.GetNumber(depth);
				for(size_t i = 0; i < n; ++i) {
					CVariable *pVal = lvar.GetPtr(depth,i);
					if (pVal && !pVal->IsErased()) {
						if(name.compare(0,len,pVal->name,0,len) == 0) {
							result.array().push_back(CValueSub(pVal->name));
						}
					}
				}
			}
		}
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETCALLSTACK（呼び出し履歴）
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETCALLSTACK(const CValue &arg, yaya::string_t &/*d*/, int &/*l*/)
{
	CValue result(F_TAG_ARRAY, 0/*dmy*/);

	std::vector<yaya::string_t> &stack = vm.calldepth().Stack();

	size_t n = stack.size();

	for(size_t i = 0; i < n; ++i) {
		result.array().push_back(CValueSub(stack[i]));
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::MakeReResultDetails
 *  機能概要：  正規表現系関数の処理結果詳細を蓄積します
 * -----------------------------------------------------------------------
 */
void	CSystemFunction::StoreReResultDetails(boost::match_results<yaya::string_t::const_iterator> &result)
{
	int	sz = result.size();
	for(int i = 0; i < sz; i++)
		AppendReResultDetail(result.str(i), result.position(i), result.length(i));
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ClearReResultDetails
 *  機能概要：  正規表現系関数の処理結果詳細をクリアします
 * -----------------------------------------------------------------------
 */
void	CSystemFunction::ClearReResultDetails(void)
{
	re_str.array().clear();
	re_pos.array().clear();
	re_len.array().clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::AppendReResultDetail
 *  機能概要：  正規表現系関数の処理結果詳細を1つ蓄積します
 * -----------------------------------------------------------------------
 */
void	CSystemFunction::AppendReResultDetail(const yaya::string_t &str, int pos, int len)
{
	re_str.array().push_back(str);
	re_pos.array().push_back(pos);
	re_len.array().push_back(len);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SetError
 *  機能概要：  GETLASTERRORが返す値を設定します
 * -----------------------------------------------------------------------
 */
void	CSystemFunction::SetError(int code)
{
	lasterror = code;
}

int CSystemFunction::GetCharset(const CValueSub &var,const wchar_t *fname, yaya::string_t &d, int &l)
{
	if (var.IsNum()) {
		int	charset = var.GetValueInt();
		if (Ccct::CheckInvalidCharset(charset)) {
			vm.logger().Error(E_W, 12, fname, d, l);
			SetError(12);
			return -1;
		}
		return charset;
	}

	if (var.IsString()) {
		yaya::string_t cset = var.GetValueString();
		int	charset = Ccct::CharsetTextToID(cset.c_str());
		return charset;
	}

	vm.logger().Error(E_W, 9, fname, d, l);
	SetError(9);
	return -1;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ToFullPath
 *  機能概要：  渡された文字列が相対パス表記なら絶対パスに書き換えます
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
yaya::string_t	CSystemFunction::ToFullPath(const yaya::string_t &str)
{
	yaya::char_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_wsplitpath(str.c_str(), drive, dir, fname, ext);

	if (!::wcslen(drive))
		return vm.basis().path + str;

	return str;
}
#elif defined(POSIX)
yaya::string_t CSystemFunction::ToFullPath(const yaya::string_t &str)
{
    if (str.length() > 0 && str[0] == L'/') {
	return str;
    }
    else {
	return vm.basis().path + str;
    }
}
#endif

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::READFMO
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
CValue CSystemFunction::READFMO(const CValue &arg, yaya::string_t &d, int &l)
{
	yaya::string_t fmoname;
	if (arg.array_size()==0) {
		fmoname=L"Sakura";
	}else{
		fmoname=arg.array()[0].GetValueString();
	}

	HANDLE hFMO;
	void* pData;
	CValue result=CValue(F_TAG_NOP, 0/*dmy*/);

	//UNICODE→SJISにして呼ぶ（for win95）
	char* tmpstr=Ccct::Ucs2ToMbcs(fmoname.c_str(),CHARSET_SJIS);

	hFMO=OpenFileMapping(FILE_MAP_READ,false,tmpstr);
	if(hFMO == NULL){
		vm.logger().Error(E_W, 13, L"READFMO(" + fmoname + L").OpenFileMapping Failed", d, l);
		SetError(13);
		return result;
	}

	free(tmpstr);


	pData=MapViewOfFile(hFMO,FILE_MAP_READ,0,0,0);
	if(pData == NULL){
		CloseHandle(hFMO);
		vm.logger().Error(E_W, 13, L"READFMO(" + fmoname + L").MapViewOfFile Failed" , d, l);
		SetError(13);
		return result;
	}

	unsigned int size=*(unsigned int*)(pData);
	if(size<=4){
		UnmapViewOfFile(pData);
		CloseHandle(hFMO);
		vm.logger().Error(E_W, 13, L"READFMO(" + fmoname + L").FMO size less than 4 bytes" , d, l);
		SetError(13);
		return result;
	}

	char* pBuf=new char[size];
	strncpy( pBuf , (const char*) pData+4, size-4 );
	UnmapViewOfFile(pData);
	CloseHandle(hFMO);

	yaya::char_t *t_str = Ccct::MbcsToUcs2(pBuf,CHARSET_DEFAULT);
	if (t_str == NULL) {
		vm.logger().Error(E_E, 13, L"READFMO(" + fmoname + L").MbcsToUcs2 Failed", d, l);
		SetError(13);
		return result;
	}
	free(pBuf);

	result=CValue(t_str);
	free(t_str);

	return result;
}
#else
CValue CSystemFunction::READFMO(const CValue &arg, yaya::string_t &d, int &l)
{
	vm.logger().Error(E_W, 13, L"READFMO not implemented for non-win32 system.", d, l);
	return CValue(F_TAG_NOP, 0/*dmy*/);
}
#endif


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::EXECUTE_WAIT
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::EXECUTE_WAIT(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"EXECUTE_WAIT", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"EXECUTE_WAIT", d, l);
		SetError(9);
		return CValue(-1);
	}

	// パスをMBCSに変換
	int result;

#if defined(WIN32)	
	
	char *s_filestr = Ccct::Ucs2ToMbcs(arg.array()[0].s_value, CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"EXECUTE_WAIT", d, l);
		return CValue(-1);
	}

	char *s_parameter = NULL;
	if ( arg.array_size() >= 2 ) {
		if ( arg.array()[1].s_value.size() ) {
			s_parameter = Ccct::Ucs2ToMbcs(arg.array()[1].s_value, CHARSET_DEFAULT);
		}
	}

	SHELLEXECUTEINFO inf;
	ZeroMemory(&inf,sizeof(inf));
	inf.cbSize = sizeof(inf);
	inf.fMask = SEE_MASK_NOCLOSEPROCESS;
	inf.lpVerb = "open";
	inf.lpFile = s_filestr;
	inf.lpParameters = s_parameter;
	inf.nShow = SW_SHOWNORMAL;

	if ( ::ShellExecuteEx(&inf) ) {
		::WaitForSingleObject(inf.hProcess,INFINITE);
		DWORD status;
		result = ::GetExitCodeProcess(inf.hProcess,&status);
		::CloseHandle(inf.hProcess);
	}
	else {
		result = -1;
	}

	free(s_filestr);
	if ( s_parameter ) { free(s_parameter); }

#elif defined(POSIX)

	std::string path = narrow(arg.array()[0].s_value);
    fix_filepath(path);

	if ( arg.array_size() >= 2 ) {
		if ( arg.array()[1].s_value.size() ) {
			path += L" ";
			path += arg.array()[1].s_value;
		}
	}

	result = system(path.c_str());

#endif

	// 実行
	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::EXECUTE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::EXECUTE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"EXECUTE", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"EXECUTE", d, l);
		SetError(9);
		return CValue(-1);
	}

	// パスをMBCSに変換
	int result;

#if defined(WIN32)	
	
	char *s_filestr = Ccct::Ucs2ToMbcs(arg.array()[0].s_value, CHARSET_DEFAULT);
	if (s_filestr == NULL) {
		vm.logger().Error(E_E, 89, L"EXECUTE", d, l);
		return CValue(-1);
	}

	char *s_parameter = NULL;
	if ( arg.array_size() >= 2 ) {
		if ( arg.array()[1].s_value.size() ) {
			s_parameter = Ccct::Ucs2ToMbcs(arg.array()[1].s_value, CHARSET_DEFAULT);
		}
	}

	result = (int)::ShellExecute(NULL,"open",s_filestr,s_parameter,NULL,SW_SHOWNORMAL);
	if ( result <= 32 ) { result = -1; }

	free(s_filestr);
	if ( s_parameter ) { free(s_parameter); }

#elif defined(POSIX)

	//TODO: Implement

#endif

	// 実行
	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::DUMPVAR
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::DUMPVAR(const CValue &arg, yaya::string_t &d, int &l)
{
	CLogExCode logex(vm);
	logex.OutVariableInfoForCheck();
	return CValue(F_TAG_NOP, 0/*dmy*/);
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::LICENSE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::LICENSE()
{
	CValue v(F_TAG_ARRAY, 0/*dmy*/);

	v.array().push_back(yaya::string_t(aya_name) + aya_version);
	v.array().push_back(yaya::string_t(L"Copyright (C) 2007 - 2008, ") + aya_author);
	v.array().push_back(L"All rights reserved.");
	v.array().push_back(L"");
	v.array().push_back(L"Redistribution and use in source and binary forms, with or without");
	v.array().push_back(L"modification, are permitted provided that the following conditions");
	v.array().push_back(L"are met:");
	v.array().push_back(L"");
	v.array().push_back(L" 1. Redistributions of source code must retain the above copyright");
	v.array().push_back(L"    notice, this list of conditions and the following disclaimer.");
	v.array().push_back(L"");
	v.array().push_back(L" 2. Redistributions in binary form must reproduce the above copyright");
	v.array().push_back(L"    notice, this list of conditions and the following disclaimer in the");
	v.array().push_back(L"    documentation and/or other materials provided with the distribution.");
	v.array().push_back(L"");
	v.array().push_back(L" 3. The names of its contributors may not be used to endorse or promote ");
	v.array().push_back(L"    products derived from this software without specific prior written ");
	v.array().push_back(L"    permission.");
	v.array().push_back(L"");
	v.array().push_back(L"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS");
	v.array().push_back(L"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT");
	v.array().push_back(L"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR");
	v.array().push_back(L"A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR");
	v.array().push_back(L"CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,");
	v.array().push_back(L"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,");
	v.array().push_back(L"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR");
	v.array().push_back(L"PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF");
	v.array().push_back(L"LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING");
	v.array().push_back(L"NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS");
	v.array().push_back(L"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.");
	v.array().push_back(L"");

	v.array().push_back(L"---MT19937---");
	v.array().push_back(L"Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,");
	v.array().push_back(L"All rights reserved.");
	v.array().push_back(L"");
	v.array().push_back(L"Redistribution and use in source and binary forms, with or without");
	v.array().push_back(L"modification, are permitted provided that the following conditions");
	v.array().push_back(L"are met:");
	v.array().push_back(L"");
	v.array().push_back(L" 1. Redistributions of source code must retain the above copyright");
	v.array().push_back(L"    notice, this list of conditions and the following disclaimer.");
	v.array().push_back(L"");
	v.array().push_back(L" 2. Redistributions in binary form must reproduce the above copyright");
	v.array().push_back(L"    notice, this list of conditions and the following disclaimer in the");
	v.array().push_back(L"    documentation and/or other materials provided with the distribution.");
	v.array().push_back(L"");
	v.array().push_back(L" 3. The names of its contributors may not be used to endorse or promote ");
	v.array().push_back(L"    products derived from this software without specific prior written ");
	v.array().push_back(L"    permission.");
	v.array().push_back(L"");
	v.array().push_back(L"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS");
	v.array().push_back(L"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT");
	v.array().push_back(L"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR");
	v.array().push_back(L"A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR");
	v.array().push_back(L"CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,");
	v.array().push_back(L"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,");
	v.array().push_back(L"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR");
	v.array().push_back(L"PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF");
	v.array().push_back(L"LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING");
	v.array().push_back(L"NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS");
	v.array().push_back(L"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.");
	v.array().push_back(L"");

	v.array().push_back(L"---zlib---");
	v.array().push_back(L"(C) 1995-2002 Jean-loup Gailly and Mark Adler");
	v.array().push_back(L"");
	v.array().push_back(L"This software is provided 'as-is', without any express or implied");
	v.array().push_back(L"warranty.  In no event will the authors be held liable for any damages");
	v.array().push_back(L"arising from the use of this software.");
	v.array().push_back(L"");
	v.array().push_back(L"Permission is granted to anyone to use this software for any purpose,");
	v.array().push_back(L"including commercial applications, and to alter it and redistribute it");
	v.array().push_back(L"freely, subject to the following restrictions:");
	v.array().push_back(L"");
	v.array().push_back(L"1. The origin of this software must not be misrepresented; you must not");
	v.array().push_back(L" claim that you wrote the original software. If you use this software");
	v.array().push_back(L" in a product, an acknowledgment in the product documentation would be");
	v.array().push_back(L" appreciated but is not required.");
	v.array().push_back(L"2. Altered source versions must be plainly marked as such, and must not be");
	v.array().push_back(L" misrepresented as being the original software.");
	v.array().push_back(L"3. This notice may not be removed or altered from any source distribution.");
	v.array().push_back(L"");

	return v;
}