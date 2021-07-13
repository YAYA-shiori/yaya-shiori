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
#include <functional>
#include <exception>
#if defined(POSIX)
# include <fstream>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define wcsnicmp wcsncasecmp
#endif
#include <string>
#include <vector>
#include <map>
#include <ctime>

#include <cstring>
#include <stdexcept>

#include "fix_unistd.h"

#include <math.h>
#include <stdio.h>
#if defined(POSIX)
# include <sys/stat.h>
# include <sys/time.h>
#endif

#if defined(POSIX)
# include <memory>
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
#include "dir_enum.h"

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

#ifdef POSIX
#define wcsicmp wcscasecmp
#ifndef TRUE
#define TRUE	1
#endif
#endif

#define	SYSFUNC_NUM					141 //システム関数の全数
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
	// 配列(4)
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
	// 正規表現(4)
	L"RE_OPTION",
	// ファイル操作(6)
	L"FREADENCODE",
	// 型取得/変換(4)
	L"GETTYPEEX",
	// 正規表現(5)
	L"RE_ASEARCHEX",
	L"RE_ASEARCH",
	// 配列(5)
	L"ASORT",
	// 文字列操作(9)
	L"TRANSLATE",
	// 数値(4)
	L"SRAND",
	// 特殊(8)
	L"GETENV",
	// ファイル操作(7)
	L"FWRITEDECODE",
	// デバッグ用(4)
	L"GETERRORLOG",
	// 特殊(9)
	L"DICLOAD",
	L"GETSYSTEMFUNCLIST",
	L"GETFUNCINFO",
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
	if ( str.size() == 0 ) { return -1; }

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
			CValueArgArray &valuearg, CLocalVariable &lvar, int l, CFunction *thisfunc)
{
	yaya::string_t& d = thisfunc->dicfilename;

	switch(index) {
	case 0:		// TOINT
		return TOINT(arg, d, l);
	case 1:		// TOREAL
		return TOREAL(arg, d, l);
	case 2:		// TOSTR
		return TOSTR(valuearg, d, l);
	case 3:		// GETTYPE
		return GETTYPE(valuearg, d, l);
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
		return ArraySize(valuearg, pcellarg, lvar, d, l);
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
	case SYSFUNC_HIS:	// %[n]（置換済の値の再利用）処理用関数 → これのみCFunctionで処理するのでここへは来ない
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
		return CVSTR(valuearg, pcellarg, lvar, d, l);
	case 75:	// CVREAL
		return CVREAL(arg, pcellarg, lvar, d, l);
	case 76:	// LETTONAME
		return LETTONAME(valuearg, d, l, lvar, thisfunc);
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
	case 130:
		return RE_ASEARCHEX(arg, d, l);
	case 131:
		return RE_ASEARCH(arg, d, l);
	case 132:
		return ASORT(arg, d, l);
	case 133:
		return TRANSLATE(arg, d, l);
	case 134:
		return SRAND(arg, d, l);
	case 135:
		return GETENV(arg, d, l);
	case 136:
		return FWRITEDECODE(arg, d, l);
	case 137:
		return GETERRORLOG(arg, d, l);
	case 138:
		return DICLOAD(arg, d, l);
	case 139:
		return GETSYSTEMFUNCLIST(arg, d, l);
	case 140:
		return GETFUNCINFO(arg, d, l);
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
CValue	CSystemFunction::TOSTR(CValueArgArray &valuearg, yaya::string_t &d, int &l)
{
	if (valuearg.empty()) {
		vm.logger().Error(E_W, 8, L"TOSTR", d, l);
		SetError(8);
		return CValue();
	}

	yaya::string_t str;
	for ( CValueArgArray::const_iterator it = valuearg.begin() ; it != valuearg.end() ; ++it ) {
		str += it->GetValueString();
	}
	return CValue(str);
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
 *  返値　　：  0/1/2/3/4/5=エラー/整数/実数/文字列/配列/連想配列
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETTYPE(CValueArgArray &valuearg, yaya::string_t &d, int &l)
{
	if (valuearg.empty()) {
		vm.logger().Error(E_W, 8, L"GETTYPE", d, l);
		SetError(8);
		return CValue(0);
	}

	switch(valuearg[0].GetType()) {
	case F_TAG_VOID:
		return CValue(0);
	case F_TAG_INT:
		return CValue(1);
	case F_TAG_DOUBLE:
		return CValue(2);
	case F_TAG_STRING:
		return CValue(3);
	case F_TAG_ARRAY: //互換処理
		{
			if ( valuearg[0].array_size() == 1 ) {
				int t = valuearg[0].array()[0].GetType();	
				if ( t == F_TAG_INT ) {
					return CValue(1);
				}
				else if ( t == F_TAG_DOUBLE ) {
					return CValue(2);
				}
				else if ( t == F_TAG_STRING ) {
					return CValue(3);
				}
				else {
					return CValue(0);
				}
			}
			else {
				return CValue(4);
			}
		}
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
 *  関数名  ：  CSystemFunction::GETERRORLOG
 *
 *  エラーログを配列で返します
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETERRORLOG(const CValue &arg, yaya::string_t &d, int &l)
{

	CValue result(F_TAG_ARRAY, 0/*dmy*/);

	//絞りこみ文字列がない場合
	std::deque<yaya::string_t> &log = vm.logger().GetErrorLogHistory();

	for(std::deque<yaya::string_t>::iterator it = log.begin(); it != log.end(); it++) {
		result.array().push_back(CValueSub(*it));
	}

	return result;
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
	L"アイウエオカキクケコサシスセ\x30bdタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォャュョッ゛゜、。";
static const yaya::char_t han_support_kana[] = 
	L"ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝｧｨｩｪｫｬｭｮｯﾞﾟ､｡";

static const yaya::char_t zen_support_kana2[] = 
	L"ガギグゲゴザジズゼゾダヂヅデドバビブベボヴ";
static const yaya::char_t han_support_kana2[] = 
	L"ｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾊﾋﾌﾍﾎｳ";

static const yaya::char_t zen_support_kana3[] = 
	L"パピプペポ";
static const yaya::char_t han_support_kana3[] = 
	L"ﾊﾋﾌﾍﾎ";


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
static const yaya::char_t char_handakuten = 0xff9f;

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
				
				found = wcschr(zen_support_kana3,*it);
				if ( found ) {
					*it = han_support_kana3[found - zen_support_kana3];
					it = str.insert(it+1,char_handakuten);
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
					if ( it < str.end()-1 && ((*(it+1) == char_dakuten) || (*(it+1) == char_handakuten)) ) {
						if ( *(it+1) == char_dakuten ) {
							found = wcschr(han_support_kana2,*it);
							if ( found ) {
								*it = zen_support_kana2[found - han_support_kana2];
								it = str.erase(it+1) - 1;
							}
						}
						else /*if char_handakuten*/ {
							found = wcschr(han_support_kana3,*it);
							if ( found ) {
								*it = zen_support_kana3[found - han_support_kana3];
								it = str.erase(it+1) - 1;
							}
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
	        return vm.genrand_sysfunc_int(100);

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

	return vm.genrand_sysfunc_int(num);
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::SRAND
 *
 *  RANDのseed。パラメータは文字列も可。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::SRAND(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 19, L"SRAND", d, l);
		SetError(19);
		return CValue(0);
	}

	if (arg.array()[0].IsInt()) {
		int num = arg.array()[0].GetValueInt();
		vm.genrand_sysfunc_srand(num);
	}
	else if (arg.array()[0].IsDouble()) {
		union {
			double d;
			unsigned long i[2];
		} num;

		num.d = arg.array()[0].GetValueDouble();

		vm.genrand_sysfunc_srand_array(num.i,2);
	}
	else if (arg.array()[0].IsString()) {
		std::vector<unsigned long> num;

		yaya::string_t str = arg.array()[0].GetValueString();

		int nlen = str.length();
		int n = nlen / 2;

		for ( int i = 0 ; i < n ; ++i ) {
			num.push_back( static_cast<unsigned long>(str[i]) | (static_cast<unsigned long>(str[i+1]) << 16) );
		}
		if ( (n*2) != nlen ) { //奇数
			num.push_back( static_cast<unsigned long>(str[nlen-1]) );
		}

		vm.genrand_sysfunc_srand_array(&(num[0]),num.size());
	}
	else {
		vm.logger().Error(E_W, 9, L"SRAND", d, l);
		SetError(9);
		return CValue(0);
	}

	return CValue(1);
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

	int count = 0;
	if ( arg.array_size() >= 4 ) {
		if (!arg.array()[3].IsInt()) {
			vm.logger().Error(E_W, 9, L"REPLACE", d, l);
			SetError(9);
		}
		count = arg.array()[3].GetValueInt();
	}

	yaya::string_t	result = arg.array()[0].GetValueString();
	yaya::string_t  before = arg.array()[1].GetValueString();
	yaya::string_t  after  = arg.array()[2].GetValueString();
	//int	sz_before = before->size();
	//int	sz_after  = after->size();

	if (!before.empty()) {
		yaya::ws_replace(result, before.c_str(), after.c_str(), count);
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
	
	yaya::string_t src = arg.array()[0].GetValueString();
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

	return CValue(src.erase(pos, len));
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
		vm.logger().Error(E_W, 9, L"INSERT", d, l);
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

	yaya::string_t str = arg.array()[0].GetValueString();

	if ( wcsnicmp(str.c_str(),L"0x",2) == 0 ) {
		str.erase(0,2);
	}

	if (!IsIntHexString(str, 0)) {
		vm.logger().Error(E_W, 12, L"HEXSTRTOI", d, l);
		SetError(12);
		return CValue(0);
	}

	return CValue(yaya::ws_atoi(str, 16));
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
	
	for ( CValueArray::const_iterator i = arg.array().begin() + 1 ;
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

	yaya::string_t type = L"base64";
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
 *  関数名  ：  CSystemFunction::FWRITEDECODE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::FWRITEDECODE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 2) {
		vm.logger().Error(E_W, 8, L"FWRITEDECODE", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

    if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString()) {
		vm.logger().Error(E_W, 9, L"FWRITEDECODE", d, l);
		SetError(9);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	yaya::string_t type = L"base64";

	if (arg.array_size() >= 3) {
		if (!arg.array()[2].IsString()) {
			vm.logger().Error(E_W, 9, L"FWRITEDECODE", d, l);
			SetError(9);
			return CValue(F_TAG_NOP, 0/*dmy*/);
		}
		type = arg.array()[2].s_value;
	}

	if (!vm.files().WriteDecode(ToFullPath(arg.array()[0].s_value), arg.array()[1].s_value, type) ) {
		vm.logger().Error(E_W, 13, L"FWRITEDECODE", d, l);
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
	yaya::string_t	s_path = ((::wcslen(drive)) ? yaya::string_t(L"") : vm.basis().base_path) + arg.array()[0].s_value;

	yaya::char_t	fname2[_MAX_FNAME], ext2[_MAX_EXT];
	_wsplitpath(arg.array()[1].s_value.c_str(), drive, dir, fname2, ext2);
	yaya::string_t	d_path = ((::wcslen(drive)) ?
						yaya::string_t(L"") : vm.basis().base_path) + arg.array()[1].s_value + L"\\" + fname + ext;

	// パスをMBCSに変換
	char	*s_pstr = Ccct::Ucs2ToMbcs(s_path, CHARSET_DEFAULT);
	if (s_pstr == NULL) {
		vm.logger().Error(E_E, 89, L"FCOPY", d, l);
		return CValue(0);
	}
	char	*d_pstr = Ccct::Ucs2ToMbcs(d_path, CHARSET_DEFAULT);
	if (d_pstr == NULL) {
		free(s_pstr);
		s_pstr = NULL;
		vm.logger().Error(E_E, 89, L"FCOPY", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (CopyFile(s_pstr, d_pstr, FALSE) ? 1 : 0);
	free(s_pstr);
	s_pstr = NULL;
	free(d_pstr);
	d_pstr = NULL;

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
    std::remove(dest.c_str()); // コピー先がシンボリックリンクとかだと嫌。
	std::ifstream is(src.c_str());
    int result = 0;
    if (is.good()) {
		std::ofstream os(dest.c_str());
	if (os.good()) {
        std::unique_ptr<char[]> buf(new char[512]);
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
	yaya::string_t	s_path = ((::wcslen(drive)) ? yaya::string_t(L"") : vm.basis().base_path) + arg.array()[0].s_value;

	yaya::char_t	fname2[_MAX_FNAME], ext2[_MAX_EXT];
	_wsplitpath(arg.array()[1].s_value.c_str(), drive, dir, fname2, ext2);
	yaya::string_t	d_path = ((::wcslen(drive)) ?
						yaya::string_t(L"") : vm.basis().base_path) + arg.array()[1].s_value + L"\\" + fname + ext;

	// パスをMBCSに変換
	char	*s_pstr = Ccct::Ucs2ToMbcs(s_path, CHARSET_DEFAULT);
	if (s_pstr == NULL) {
		vm.logger().Error(E_E, 89, L"FMOVE", d, l);
		return CValue(0);
	}
	char	*d_pstr = Ccct::Ucs2ToMbcs(d_path, CHARSET_DEFAULT);
	if (d_pstr == NULL) {
		free(s_pstr);
		s_pstr = NULL;
		vm.logger().Error(E_E, 89, L"FMOVE", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (MoveFile(s_pstr, d_pstr) ? 1 : 0);
	free(s_pstr);
	s_pstr = NULL;
	free(d_pstr);
	d_pstr = NULL;

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
	int	result = (::CreateDirectory(s_dirstr,NULL) ? 1 : 0); //mkdirと論理が逆
	free(s_dirstr);
	s_dirstr = NULL;

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
    int result = (mkdir(dirstr.c_str(), 0644) == 0 ? 1 : 0);

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
	s_dirstr = NULL;

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
    int result = (std::remove(dirstr.c_str()) ? 0 : 1);

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
	s_filestr = NULL;

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
    int result = (std::remove(filestr.c_str()) ? 0 : 1);

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
		s_filestr = NULL;
		vm.logger().Error(E_E, 89, L"FRENAME", d, l);
		return CValue(0);
	}

	// 実行
	int	result = (MoveFile(s_filestr, d_filestr) ? 1 : 0);
	free(s_filestr);
	s_filestr = NULL;
	free(d_filestr);
	d_filestr = NULL;

	
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
	s_filestr = NULL;
	
#endif

	unsigned char digest_result[32];
	size_t digest_len;
	unsigned char buf[32768];

	if ( wcsicmp(digest_type.c_str(),L"sha1") == 0 || wcsicmp(digest_type.c_str(),L"sha-1") == 0 ) {
		SHA1Context sha1ctx;
		SHA1Reset(&sha1ctx);

		while ( ! feof(pF) ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			SHA1Input(&sha1ctx,buf,readsize);
			if ( readsize < sizeof(buf) ) { break; }
		}

		SHA1Result(&sha1ctx,digest_result);
		digest_len = SHA1HashSize;
	}
	else if ( wcsicmp(digest_type.c_str(),L"crc32") == 0 ) {
		unsigned long crc = 0;

		while ( ! feof(pF) ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			crc = update_crc32(buf,readsize,crc);;
			if ( readsize < sizeof(buf) ) { break; }
		}

		digest_result[0] = static_cast<unsigned char>(crc & 0xFFU);
		digest_result[1] = static_cast<unsigned char>((crc >> 8) & 0xFFU);
		digest_result[2] = static_cast<unsigned char>((crc >> 16) & 0xFFU);
		digest_result[3] = static_cast<unsigned char>((crc >> 24) & 0xFFU);
		digest_len = 4;
	}
	else { //md5
		MD5_CTX md5ctx;
		MD5Init(&md5ctx);

		while ( ! feof(pF) ) {
			size_t readsize = fread(buf,sizeof(buf[0]),sizeof(buf),pF);
			MD5Update(&md5ctx,buf,readsize);
			if ( readsize < sizeof(buf) ) { break; }
		}

		MD5Final(digest_result,&md5ctx);
		digest_len = 16;
	}

	fclose(pF);

	yaya::char_t md5str[65];
	md5str[digest_len*2] = 0; //ゼロ終端

	for ( unsigned int i = 0 ; i < digest_len ; ++i ) {
		yaya::snprintf(md5str+i*2,sizeof(md5str)/sizeof(md5str[0]), L"%02X",digest_result[i]);
	}

	return CValue(yaya::string_t(md5str));
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::DICLOAD
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::DICLOAD(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"DICLOAD", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"DICLOAD", d, l);
		SetError(9);
		return CValue(-1);
	}

	yaya::string_t fullpath = ToFullPath(arg.array()[0].s_value);
	char cset = vm.basis().GetDicCharset();

	if ( arg.array_size() >= 2 && arg.array()[1].s_value.size() ) {
		char cx = Ccct::CharsetTextToID(arg.array()[1].s_value.c_str());
		if ( cx != CHARSET_DEFAULT ) {
			cset = cx;
		}
	}

	int err = vm.parser0().DynamicLoadDictionary(fullpath,cset);

	if ( err > 1 ) {
		SetError(err);
	}

	return CValue(err != 0 ? 1 : 0);
}
/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETFUNCINFO
 * -----------------------------------------------------------------------
 */
CValue CSystemFunction::GETFUNCINFO(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"GETFUNCINFO", d, l);
		SetError(8);
		return CValue(-1);
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"GETFUNCINFO", d, l);
		SetError(9);
		return CValue(-1);
	}

	yaya::string_t name = arg.array()[0].GetValueString();

	int index = vm.parser0().GetFunctionIndexFromName(name);

	if ( index < 0 ) {
		vm.logger().Error(E_W, 12, L"GETFUNCINFO", d, l);
		SetError(12);
		return CValue(-1);
	}

	CValue result(F_TAG_ARRAY, 0/*dmy*/);
	const CFunction *it = &vm.function()[size_t(index)];

	result.array().push_back(CValueSub(it->GetFileName()));
	result.array().push_back(CValueSub((int)it->GetLineNumBegin()));
	result.array().push_back(CValueSub((int)it->GetLineNumEnd()));

	return result;
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
	s_filestr = NULL;
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

	CDirEnum ef(ToFullPath(arg.array()[0].s_value));
	CDirEnumEntry entry;
	int count = 0;
	CValue result(F_TAG_STRING,0);

	while ( ef.next(entry) ) {
		if ( count ) { result.s_value += delimiter; }
		if ( entry.isdir ) { result.s_value +=  L"\\"; }
		result.s_value += entry.name;

		count += 1;
	}

	return result;
}

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
CValue	CSystemFunction::ArraySize(CValueArgArray &valuearg, const std::vector<CCell *> &pcellarg,
								   CLocalVariable &lvar, yaya::string_t &d, int &l)
{
	// 引数無しなら0
	size_t sz = valuearg.size();
	if (!sz) {
		return CValue(0);
	}

	if ( valuearg[0].IsArray() ) {
		return CValue(static_cast<int>(valuearg[0].array_size()));
	}
	else if ( valuearg[0].IsString() ) {
		if ( valuearg[0].GetValueString().size() == 0 ) {
			return CValue(0);
		}
		// 引数1つで文字列なら簡易配列の要素数を返す　変数の場合はそのデリミタで分割する
		yaya::string_t	delimiter = VAR_DELIMITER;
		if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
			delimiter = vm.variable().GetDelimiter(pcellarg[0]->index);
		else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
			delimiter = lvar.GetDelimiter(pcellarg[0]->name);

		return CValue((int)SplitToMultiString(valuearg[0].s_value, NULL, delimiter));
	}
	else if ( valuearg[0].IsVoid() ) {
		return CValue(0);
	}
	else {
		return CValue(1);
	}
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
		result.array().push_back(CValueSub(static_cast<int>(today->tm_yday)));
		result.array().push_back(CValueSub(static_cast<int>(today->tm_isdst)));
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
 *  引数なしか、year,month,day,week(0-6),hour,minute,secondの配列、または
 *  日時を表すテキスト
 *
 *  返値　　：  EPOCHからの秒数
 * -----------------------------------------------------------------------
 */
/*-----------------------------------------------------
	HTTP Date Conversion
------------------------------------------------------*/
#define HTTP_DATE_TOKEN " \t,:-;"

static const char * const g_pWDayArray[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char * const g_pMonthArray[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static unsigned int Utils_HTTPToSystemTime_MonthConv(char *pMon)
{
	static const unsigned int n = sizeof(g_pMonthArray) / sizeof(g_pMonthArray[0]);
	unsigned int i = 0;
	for ( i = 0 ; i < n ; ++i ) {
		if ( strnicmp(pMon,g_pMonthArray[i],3) == 0 ) {
			break;
		}
	}
	return i + 1;
}

static int Utils_TimeZoneConvert(char *pTZ)
{
	if ( ! pTZ || ! *pTZ ) { return 0; }

	int tzdiff = atoi(pTZ);
	if ( ! tzdiff ) {
		if ( stricmp(pTZ,"ut") == 0 ) {
			return 0;
		}
		if ( stricmp(pTZ,"gmt") == 0 ) {
			return 0;
		}
		if ( stricmp(pTZ,"est") == 0 ) {
			return -5*60;
		}
		if ( stricmp(pTZ,"edt") == 0 ) {
			return -4*60;
		}
		if ( stricmp(pTZ,"cst") == 0 ) {
			return -6*60;
		}
		if ( stricmp(pTZ,"cdt") == 0 ) {
			return -5*60;
		}
		if ( stricmp(pTZ,"mst") == 0 ) {
			return -7*60;
		}
		if ( stricmp(pTZ,"mdt") == 0 ) {
			return -6*60;
		}
		if ( stricmp(pTZ,"pst") == 0 ) {
			return -8*60;
		}
		if ( stricmp(pTZ,"pdt") == 0 ) {
			return -7*60;
		}

		if ( pTZ[1] == 0 ) { //1文字アルファベット？
			int c = pTZ[0];
			if ( c >= 'A' && c <= 'Z' ) {
				c = c - 'A' + 'a';
			}
			if ( c >= 'a' && c <= 'm' ) {
				return static_cast<int>(c - 'n' + 1)*-60;
			}
			if ( c >= 'n' && c <= 'y' ) {
				return static_cast<int>(c - 'n' + 1)*60;
			}
			if ( c == 'z' ) {
				return 0;
			}
		}
	}

	if ( labs(tzdiff) <= 24 ) { //2けた+時間
		return tzdiff * 60;
	}
	
	int h = tzdiff / 100;
	return (h * 60) + (tzdiff - (h*100));
}

// Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
// Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
// Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format

static bool Utils_HTTPToTM(const char *pText,struct tm &outTime)
{
	if ( ! pText ) { return false; }

	memset(&outTime,0,sizeof(outTime));

	unsigned int len = strlen(pText) + 1;
	char *pData = (char*)malloc(len);
	memcpy(pData,pText,len);

	char *pTok = strtok(pData,HTTP_DATE_TOKEN);
	unsigned int num = 0;

	char *pTokArray[8];
	memset(&pTokArray,0,sizeof(pTokArray));

	while ( pTok && (num <= 7) ) {
		pTokArray[num] = pTok;
		++num;
		pTok = strtok(NULL,HTTP_DATE_TOKEN);
	}

	if ( num < 6 ) {
		free(pData);
		return FALSE;
	}

	unsigned int n,i = 0;
	bool isDayOfWeekFound = false;
	n = sizeof(g_pWDayArray) / sizeof(g_pWDayArray[0]);
	for ( i = 0 ; i < n ; ++i ) {
		if ( strnicmp(pTokArray[0],g_pWDayArray[i],3) == 0 ) {
			isDayOfWeekFound = true;
			break;
		}
	}

	if ( ! isDayOfWeekFound ) { //曜日省略形
		for ( int j = 7 ; j > 0 ; --j ) {
			pTokArray[j] = pTokArray[j-1];
		}
		pTokArray[0] = "";
		outTime.tm_wday = 0;
	}
	else {
		outTime.tm_wday = static_cast<unsigned short>(i);
	}

	if ( isdigit(pTokArray[1][0]) ) { //RFC Format
		outTime.tm_mday = static_cast<unsigned short>(strtoul(pTokArray[1],NULL,10));
		outTime.tm_mon = Utils_HTTPToSystemTime_MonthConv(pTokArray[2]) - 1;
		outTime.tm_year = static_cast<unsigned short>(strtoul(pTokArray[3],NULL,10)) - 1900;
		outTime.tm_hour = static_cast<unsigned short>(strtoul(pTokArray[4],NULL,10));
		outTime.tm_min = static_cast<unsigned short>(strtoul(pTokArray[5],NULL,10));
		outTime.tm_sec = static_cast<unsigned short>(strtoul(pTokArray[6],NULL,10));

		if ( outTime.tm_year < 100 ) { //2桁だった
			if ( outTime.tm_year < 70 ) {
				outTime.tm_year += 100;
			}
		}

		if ( pTokArray[7] ) { //補正
			int diff = Utils_TimeZoneConvert(pTokArray[7]);
			if ( diff ) {
				outTime.tm_min -= diff;
			}
		}
	}
	else { //C asctime
		outTime.tm_mon = Utils_HTTPToSystemTime_MonthConv(pTokArray[1]) - 1;
		outTime.tm_mday = static_cast<unsigned short>(strtoul(pTokArray[2],NULL,10));
		outTime.tm_hour = static_cast<unsigned short>(strtoul(pTokArray[3],NULL,10));
		outTime.tm_min = static_cast<unsigned short>(strtoul(pTokArray[4],NULL,10));
		outTime.tm_sec = static_cast<unsigned short>(strtoul(pTokArray[5],NULL,10));
		outTime.tm_year = static_cast<unsigned short>(strtoul(pTokArray[6],NULL,10)) - 1900;

		if ( outTime.tm_year < 100 ) { //2桁だった
			if ( outTime.tm_year < 70 ) {
				outTime.tm_year += 100;
			}
		}
	}

	free(pData);
	return true;
}

CValue	CSystemFunction::GETSECCOUNT(const CValue &arg, yaya::string_t &d, int &l)
{
	time_t	ltime;

	if (!arg.array_size()) {
		time(&ltime);
		return CValue((int)ltime);
	}

	struct tm input_time = {0};

	time(&ltime);
	struct tm *today = localtime(&ltime);

	if ( today ) {
		input_time = *today;
		input_time.tm_yday = 0;
		input_time.tm_wday = 0;
	}

	unsigned int asize = arg.array_size();
	if ( asize > 7 ) { asize = 7; }

	if ( asize == 1 && arg.array()[0].IsString() ) { //文字列日付の可能性
		char* text = Ccct::Ucs2ToMbcs(arg.array()[0].GetValueString().c_str(),CHARSET_DEFAULT);
		Utils_HTTPToTM(text,input_time);
		free(text);
		time_t gmt_local = mktime(&input_time);

		time_t now;
		time(&now);
		struct tm* gmt_tm = gmtime(&now);
		time_t local_gmt = now - mktime(gmt_tm);

		return CValue((int)(gmt_local + local_gmt));
	}
	else {
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
	MatchResult	t_result;
	try {
		CRegexpT<yaya::char_t> regex(arg1.c_str(),re_option);
		t_result = regex.Match(arg0.c_str());
		if (t_result.IsMatched()) {
			StoreReResultDetails(arg0,t_result);
		}
	}
	catch(const std::runtime_error &) {
		vm.logger().Error(E_W, 16, L"RE_SEARCH", d, l);
		SetError(16);
	}
	catch(...) {
		vm.logger().Error(E_W, 17, L"RE_SEARCH", d, l);
		SetError(17);
	}

	return CValue(t_result.IsMatched() ? 1 : 0);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_ASEARCH / RE_ASEARCHEX
 * -----------------------------------------------------------------------
 */

CValue	CSystemFunction::RE_ASEARCH(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"RE_ASEARCHEX", d, l);
		SetError(8);
		return CValue(-1);
	}

	const CValueSub &key = arg.array()[0];

	try {
		CRegexpT<yaya::char_t> regex(key.GetValueString().c_str(),re_option);

		for(int i = 1; i < sz; i++) {
			try {
				MatchResult t_result = regex.Match(arg.array()[i].GetValueString().c_str());
				if (t_result.IsMatched()) {
					return CValue(i-1);
				}
			}
			catch(const std::runtime_error &) {
				vm.logger().Error(E_W, 16, L"RE_ASEARCHEX", d, l);
				SetError(16);
			}
			catch(...) {
				vm.logger().Error(E_W, 17, L"RE_ASEARCHEX", d, l);
				SetError(17);
			}
		}

	}
	catch(...) {
		vm.logger().Error(E_W, 17, L"RE_ASEARCHEX", d, l);
		SetError(17);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	return CValue(-1);
}

CValue	CSystemFunction::RE_ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		vm.logger().Error(E_W, 8, L"RE_ASEARCHEX", d, l);
		SetError(8);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	const CValueSub &key = arg.array()[0];
	CValue res(F_TAG_ARRAY, 0/*dmy*/);

	try {
		CRegexpT<yaya::char_t> regex(key.GetValueString().c_str(),re_option);

		for(int i = 1; i < sz; i++) {
			try {
				MatchResult t_result = regex.Match(arg.array()[i].GetValueString().c_str());
				if (t_result.IsMatched()) {
					res.array().push_back(CValueSub(i-1));
				}
			}
			catch(const std::runtime_error &) {
				vm.logger().Error(E_W, 16, L"RE_ASEARCHEX", d, l);
				SetError(16);
			}
			catch(...) {
				vm.logger().Error(E_W, 17, L"RE_ASEARCHEX", d, l);
				SetError(17);
			}
		}

	}
	catch(...) {
		vm.logger().Error(E_W, 17, L"RE_ASEARCHEX", d, l);
		SetError(17);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	return res;
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
	MatchResult	t_result;
	try {
		CRegexpT<yaya::char_t> regex(arg1.c_str(),re_option);
		t_result = regex.MatchExact(arg0.c_str());
		if (t_result.IsMatched()) {
			StoreReResultDetails(arg0,t_result);
		}
	}
	catch(const std::runtime_error &) {
		vm.logger().Error(E_W, 16, L"RE_MATCH", d, l);
		SetError(16);
	}
	catch(...) {
		vm.logger().Error(E_W, 17, L"RE_MATCH", d, l);
		SetError(17);
	}

	return CValue(t_result.IsMatched() ? 1 : 0);
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
	int	match_count = 0;

	try {
		CRegexpT<yaya::char_t> regex(arg1.c_str(),re_option);
		CContext *pCtx = regex.PrepareMatch(arg0.c_str());

		for( ; ; ) {
			MatchResult result = regex.Match(pCtx);
			if ( ! result.IsMatched() ) {
				break;
			}
			match_count++;

			AppendReResultDetail(
				arg0.substr(result.GetStart(),result.GetEnd()-result.GetStart()),
				result.GetStart(),
				result.GetEnd()-result.GetStart());
		}

		regex.ReleaseContext(pCtx);
	}
	catch(const std::runtime_error &) {
		match_count = 0;
		vm.logger().Error(E_W, 16, L"RE_GREP", d, l);
		SetError(16);
	}
	catch(...) {
		match_count = 0;
		vm.logger().Error(E_W, 17, L"RE_GREP", d, l);
		SetError(17);
	}

	return CValue(match_count);
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
		if ( opt.find(L"m") != yaya::string_t::npos ) {
			re_option |= MULTILINE;
		}
		if ( opt.find(L"s") != yaya::string_t::npos ) {
			re_option |= SINGLELINE;
		}
		if ( opt.find(L"x") != yaya::string_t::npos ) {
			re_option |= EXTENDED;
		}
		if ( opt.find(L"i") != yaya::string_t::npos ) {
			re_option |= IGNORECASE;
		}
	}

	yaya::string_t result = L"";

	if ( (re_option & MULTILINE) != 0 ) {
		result += L"m";
	}
	if ( (re_option & SINGLELINE) != 0 ) {
		result += L"s";
	}
	if ( (re_option & EXTENDED) != 0 ) {
		result += L"x";
	}
	if ( (re_option & IGNORECASE) != 0 ) {
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

	return RE_SPLIT_CORE(arg, d, l, L"RE_SPLIT", nums);
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

	int count = 0;
	if ( arg.array_size() >= 4 ) {
		if (!arg.array()[3].IsInt()) {
			vm.logger().Error(E_W, 9, L"RE_REPLACE", d, l);
			SetError(9);
		}
		count = arg.array()[3].GetValueInt();
		if ( count <= 0 ) { count = 0; }
		else { count += 1; }
	}

	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();
	const yaya::string_t &arg2 = arg.array()[2].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(arg0);

	// まずsplitする
	CValue	splits = RE_SPLIT_CORE(arg, d, l, L"RE_REPLACE", (size_t)count);
	int	num = splits.array_size();
	if (!num || num == 1)
		return CValue(arg0);

	// 置換後文字列の作成
	yaya::string_t	result;
	int	i = 0;
	for(i = 0; i < num; i++) {
		if (i) {
			result += arg2;
		}
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
		vm.logger().Error(E_W, 8, L"RE_REPLACEEX", d, l);
		SetError(8);
		return CValue(arg.array()[0].GetValueString());
	}

	if (!arg.array()[0].IsString() ||
		!arg.array()[1].IsString() ||
		!arg.array()[2].IsString()) {
		vm.logger().Error(E_W, 9, L"RE_REPLACEEX", d, l);
		SetError(9);
	}

	int count = -1;
	if ( arg.array_size() >= 4 ) {
		if (!arg.array()[3].IsInt()) {
			vm.logger().Error(E_W, 9, L"RE_REPLACEEX", d, l);
			SetError(9);
		}
		count = arg.array()[3].GetValueInt();
		if ( count <= 0 ) { count = -1; }
	}

	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();
	const yaya::string_t &arg2_orig = arg.array()[2].GetValueString();

	if (!arg0.size() || !arg1.size())
		return CValue(arg0);

	yaya::string_t arg2 = arg2_orig;

	//最後から1文字手前まで
	if ( arg2.size() > 0 ) {
		for ( yaya::string_t::iterator it = arg2.begin() ; it < (arg2.end()-1) ; ++it ) {
			if ( *it == L'\\' ) {
				yaya::char_t c = *(it+1);
				
				if ( c == L'\\' ) {
					arg2.replace(it,it+2,L"\\");
				}
				else if ( c == L'a' ) {
					arg2.replace(it,it+2,L"\a");
				}
				else if ( c == L'e' ) {
					arg2.replace(it,it+2,L"\x1B");
				}
				else if ( c == L'f' ) {
					arg2.replace(it,it+2,L"\f");
				}
				else if ( c == L'n' ) {
					arg2.replace(it,it+2,L"\n");
				}
				else if ( c == L'r' ) {
					arg2.replace(it,it+2,L"\r");
				}
				else if ( c == L't' ) {
					arg2.replace(it,it+2,L"\t");
				}
				else if ( c == L'v' ) {
					arg2.replace(it,it+2,L"\v");
				}
				else if ( c >= L'0' && c <= L'9' ) {
					yaya::char_t rep[3] = L"$0";
					rep[1] = c;
					arg2.replace(it,it+2,rep);
					it += 1; //次の文字は読み飛ばして良い
				}
			}
		}
	}

	// 実行
	yaya::string_t str_result;

	try {
		CRegexpT<yaya::char_t> regex(arg1.c_str(),re_option);

		MatchResult t_result;
		yaya::char_t *result;

		if ( arg2.size() > 0 ) {
			result = regex.Replace(arg0.c_str(),arg2.c_str(),0,count,&t_result);
		}
		else {
			result = regex.Replace(arg0.c_str(),L"",0,count,&t_result);
		}

		str_result = result;

		if (t_result.IsMatched()) {
			StoreReResultDetails(str_result,t_result);
		}

		regex.ReleaseString(result);
	}
	catch(const std::runtime_error &) {
		vm.logger().Error(E_W, 16, L"RE_GREP", d, l);
		SetError(16);
	}
	catch(...) {
		vm.logger().Error(E_W, 17, L"RE_GREP", d, l);
		SetError(17);
	}
	return CValue(str_result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::RE_SPLIT_CORE
 *
 *  RE_SPLITの主処理部分です。RE_REPLACEでも使用します。
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::RE_SPLIT_CORE(const CValue &arg, yaya::string_t &d, int &l, const yaya::char_t *fncname, size_t num)
{
	const yaya::string_t &arg0 = arg.array()[0].GetValueString();
	const yaya::string_t &arg1 = arg.array()[1].GetValueString();

	int	t_pos = 0;
	size_t count = 1;
	CValue	splits(F_TAG_ARRAY, 0/*dmy*/);

	try {
		CRegexpT<yaya::char_t> regex(arg1.c_str(),re_option);
		CContext *pCtx = regex.PrepareMatch(arg0.c_str());

		for( ; ; ) {
			MatchResult result = regex.Match(pCtx);

			if ( ! result.IsMatched() )
				break;

			count += 1;

			splits.array().push_back(arg0.substr(t_pos, result.GetStart()-t_pos));
			t_pos = result.GetEnd();

			AppendReResultDetail(
				arg0.substr(result.GetStart(),result.GetEnd()-result.GetStart()),
				result.GetStart(),
				result.GetEnd()-result.GetStart());
			
			if ( num != 0 && (count >= num) ) {
				break;
			}
		}
		
		regex.ReleaseContext(pCtx);

		int len = arg0.size() - t_pos;
		if ( len > 0 ) {
			splits.array().push_back(arg0.substr(t_pos, len));
		}
		else {
			splits.array().push_back(L"");
		}
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
CValue	CSystemFunction::CVSTR(CValueArgArray &valuearg, const std::vector<CCell *> &pcellarg,
							   CLocalVariable &lvar,yaya::string_t &d, int &l)
{
	if (!valuearg.size()) {
		vm.logger().Error(E_W, 8, L"CVSTR", d, l);
		SetError(8);
		return CValue(F_TAG_NOP, 0/*dmy*/);
	}

	if (pcellarg[0]->value_GetType() == F_TAG_VARIABLE)
		vm.variable().SetValue(pcellarg[0]->index, TOSTR(valuearg,d,l));
	else if (pcellarg[0]->value_GetType() == F_TAG_LOCALVARIABLE)
		lvar.SetValue(pcellarg[0]->name, TOSTR(valuearg,d,l));
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
		return CValue(F_TAG_NOP, 0/*dmy*/);
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
CValue	CSystemFunction::LETTONAME(CValueArgArray &valuearg, yaya::string_t &d, int &l, CLocalVariable &lvar,
			CFunction *thisfunc)
{
	int	sz = valuearg.size();

	if (sz < 2) {
		if ( valuearg[0].IsArray() && valuearg[0].array_size() >= 2 ) {
			yaya::string_t	vname = valuearg[0].array()[0].GetValueString();

			if ( vname[0] == L'_' ) {
				lvar.SetValue(vname,valuearg[0].array()[1]);
			}
			else {
				int	index = vm.variable().Make(vname, 0);
				vm.variable().SetValue(index,valuearg[0].array()[1]);
			}

			return CValue(F_TAG_NOP, 0/*dmy*/);
		}
		else {
			vm.logger().Error(E_W, 8, L"LETTONAME", d, l);
			SetError(8);

			return CValue(F_TAG_NOP, 0/*dmy*/);
		}
	}

	if (!valuearg[0].IsString()) {
		vm.logger().Error(E_W, 9, L"LETTONAME", d, l);
		SetError(9);
	}

	yaya::string_t	vname = valuearg[0].GetValueString();

	if ( vname[0] == L'_' ) {
		lvar.SetValue(vname,valuearg[1]);
	}
	else {
		int	index = vm.variable().Make(vname, 0);
		vm.variable().SetValue(index,valuearg[1]);
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
	yaya::string_t	t_format;

	for(int i = 1; i < vargs_sz; i++) {
		t_format = L"%";

		const yaya::char_t *arg_str = vargs[i].c_str();
		if ( (*arg_str == L'-') || (*arg_str == L'+') || (*arg_str == L'0') || (*arg_str == L' ') || (*arg_str == L'#') ) { //flag
			t_format += *arg_str;
			arg_str += 1;
		}
		while ( (*arg_str >= L'0') && (*arg_str <= L'9') ) { //width
			t_format += *arg_str;
			arg_str += 1;
		}
		if ( *arg_str == L'.' ) { //precision
			t_format += *arg_str;
			arg_str += 1;
			while ( (*arg_str >= L'0') && (*arg_str <= L'9') ) {
				t_format += *arg_str;
				arg_str += 1;
			}
		}
		if ( (*arg_str == L'h') || (*arg_str == L'l') || (*arg_str == L'L') ) { //extension
			//ここは読み飛ばす
			arg_str += 1;
		}
		if ( wcsncmp(arg_str,L"I64",3) == 0 ) {
			//ここは読み飛ばす
			arg_str += 3;
		}
		int type = F_TAG_VOID;
		if ( (*arg_str == L'c') || (*arg_str == L'C') || (*arg_str == L'd') || (*arg_str == L'i') || (*arg_str == L'o') || (*arg_str == L'u') || (*arg_str == L'x') || (*arg_str == L'X') ) {
			type = F_TAG_INT;
			t_format += *arg_str;
			arg_str += 1;
		}
		else if ( (*arg_str == L'e') || (*arg_str == L'E') || (*arg_str == L'f') || (*arg_str == L'g') || (*arg_str == L'G') ) {
			type = F_TAG_DOUBLE;
			t_format += *arg_str;
			arg_str += 1;
		}
		else if ( *arg_str == L's' ) {
			type = F_TAG_STRING;
			t_format += *arg_str;
			arg_str += 1;
		}
		else if ( *arg_str == L'S' ) {
			type = F_TAG_STRING;
			t_format += L's'; //ワイド文字列しか使わないので調整
			arg_str += 1;
		}
		else if ( (*arg_str == L'n') || (*arg_str == L'p') ) {
			//nとpは無視
			arg_str += 1;
		}

		yaya::string_t	format = L"%" + vargs[i];
		if (i < sz) {
			switch ( type ) {
			case F_TAG_INT:
				yaya::snprintf(t_str,128,t_format.c_str(),arg.array()[i].GetValueInt());
				break;
			case F_TAG_DOUBLE:
				yaya::snprintf(t_str,128,t_format.c_str(),arg.array()[i].GetValueDouble());
				break;
			case F_TAG_STRING:
				yaya::snprintf(t_str,128,t_format.c_str(),arg.array()[i].GetValueString().c_str());
				break;
			case F_TAG_VOID:
				t_str[0] = 0;
				break;
			default:
				vm.logger().Error(E_E, 91, d, l);
				t_str[0] = 0;
				break;
			};
			result += t_str;
			result += arg_str;
		}
		else {
			result += L"$" + vargs[i];
		}
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
	t_str = NULL;

	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::STRENCODE
 * -----------------------------------------------------------------------
 */

//std::tolowerの定義がへちょい処理系対策
struct ToLower {
	yaya::char_t operator()(yaya::char_t c) { return ::tolower(c); }
};

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
		std::transform(type.begin(), type.end(), type.begin(), ToLower());
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
	else if ( wcsicmp(type.c_str(),L"form") == 0 ) {
		EncodeURL(result,t_str,strlen(t_str),true);
	}
	else {
		EncodeURL(result,t_str,strlen(t_str),false);
	}

	free(t_str);
	t_str = NULL;

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
		std::transform(type.begin(), type.end(), type.begin(), ToLower());
	}
	
	// 主処理
	yaya::string_t src = arg.array()[0].GetValueString();

	std::string str;
	str.reserve(src.size());

	if ( wcsicmp(type.c_str(),L"base64") == 0 ) {
		DecodeBase64(str,src.c_str(),src.length());
	}
	else if ( wcsicmp(type.c_str(),L"form") == 0 ) {
		DecodeURL(str,src.c_str(),src.length(),true);
	}
	else { //if url
		DecodeURL(str,src.c_str(),src.length(),false);
	}

	yaya::char_t *t_str = Ccct::MbcsToUcs2(str, charset);
	if (t_str == NULL) {
		vm.logger().Error(E_E, 89, L"STRDECODE", d, l);
		return CValue(0);
	}

	CValue result(t_str);
	free(t_str);
	t_str = NULL;
	
	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ASEARCH / ASEARCHEX
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::ASEARCH(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		//要素1コ＝空配列の探索である。正常。
		if (sz < 1) {
			vm.logger().Error(E_W, 8, L"ASEARCHEX", d, l);
			SetError(8);
		}
		return CValue(-1);
	}

	const CValueSub &key = arg.array()[0];
	for (int i = 1; i < sz; i++) {
		if (key.Compare(arg.array()[i])) {
			return CValue(i - 1);
		}
	}

	return CValue(-1);
}

CValue	CSystemFunction::ASEARCHEX(const CValue &arg, yaya::string_t &d, int &l)
{
	int	sz = arg.array_size();

	if (sz < 2) {
		//要素1コ＝空配列の探索である。正常。
		if (sz < 1) {
			vm.logger().Error(E_W, 8, L"ASEARCHEX", d, l);
			SetError(8);
		}
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	CValue	result(F_TAG_ARRAY, 0/*dmy*/);
	const CValueSub &key = arg.array()[0];
	for(int i = 1; i < sz; i++) {
		if (key.Compare(arg.array()[i])) {
			result.array().push_back(CValueSub(i - 1));
		}
	}

	return result;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::ASORT
 * -----------------------------------------------------------------------
 */

//string,ascent
template <class TTYPE>
class CSFSORT_StringAscent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringAscent(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringAscent(const CSFSORT_StringAscent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return wcscmp(a[x].GetValueString().c_str(),a[y].GetValueString().c_str()) < 0;
	}
};

//string,descent
template <class TTYPE>
class CSFSORT_StringDescent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringDescent(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringDescent(const CSFSORT_StringDescent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return wcscmp(a[x].GetValueString().c_str(),a[y].GetValueString().c_str()) > 0;
	}
};

//string,ascent,case insensitive
template <class TTYPE>
class CSFSORT_StringAscentI : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringAscentI(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringAscentI(const CSFSORT_StringAscentI &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return wcsicmp(a[x].GetValueString().c_str(),a[y].GetValueString().c_str()) < 0;
	}
};

//string,descent,case insensitive
template <class TTYPE>
class CSFSORT_StringDescentI : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringDescentI(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringDescentI(const CSFSORT_StringDescentI &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return wcsicmp(a[x].GetValueString().c_str(),a[y].GetValueString().c_str()) > 0;
	}
};

//string,ascent,length
template <class TTYPE>
class CSFSORT_StringAscentL : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringAscentL(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringAscentL(const CSFSORT_StringAscentL &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueString().size() < a[y].GetValueString().size();
	}
};

//string,descent,length
template <class TTYPE>
class CSFSORT_StringDescentL : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_StringDescentL(const CValueArray &ain) : a(ain) { }
	CSFSORT_StringDescentL(const CSFSORT_StringDescentL &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueString().size() > a[y].GetValueString().size();
	}
};

//int,ascent
template <class TTYPE>
class CSFSORT_IntAscent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_IntAscent(const CValueArray &ain) : a(ain) { }
	CSFSORT_IntAscent(const CSFSORT_IntAscent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueInt() < a[y].GetValueInt();
	}
};

//int,descent
template <class TTYPE>
class CSFSORT_IntDescent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_IntDescent(const CValueArray &ain) : a(ain) { }
	CSFSORT_IntDescent(const CSFSORT_IntDescent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueInt() > a[y].GetValueInt();
	}
};

//double,ascent
template <class TTYPE>
class CSFSORT_DoubleAscent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_DoubleAscent(const CValueArray &ain) : a(ain) { }
	CSFSORT_DoubleAscent(const CSFSORT_DoubleAscent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueDouble() < a[y].GetValueDouble();
	}
};

//double,descent
template <class TTYPE>
class CSFSORT_DoubleDescent : public std::binary_function<TTYPE, TTYPE, bool> {
private:
	const CValueArray &a;

public:
	CSFSORT_DoubleDescent(const CValueArray &ain) : a(ain) { }
	CSFSORT_DoubleDescent(const CSFSORT_DoubleDescent &ain) : a(ain.a) { }

    inline bool operator()(const TTYPE& x, const TTYPE& y) const {
		return a[x].GetValueDouble() > a[y].GetValueDouble();
	}
};

CValue	CSystemFunction::ASORT(const CValue &arg, yaya::string_t &d, int &l)
{
	unsigned int sz = arg.array_size();
	if (sz <= 0) {
		vm.logger().Error(E_W, 8, L"ASORT", d, l);
		SetError(8);
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}
	if (sz <= 1) {
		return CValue(F_TAG_ARRAY, 0/*dmy*/);
	}

	yaya::string_t option = arg.array()[0].GetValueString();
	if ( ! arg.array()[0].IsString() || option.size() == 0 ) {
		option = L"string,ascent";
	}

	if (sz <= 2) {
		CValue rval(F_TAG_ARRAY, 0/*dmy*/);
		if ( option.find(L"index") != yaya::string_t::npos ) {
			rval.array().push_back(CValueSub(0));
		}
		else {
			rval.array().push_back(arg.array()[1]);
		}
		return rval;
	}

	std::vector<unsigned int> sort_vector;

	for ( unsigned int i = 1 ; i < sz ; ++i ) {
		sort_vector.push_back(i);
	}

	bool isDescent = (option.find(L"des") != yaya::string_t::npos);

	if ( option.find(L"int") != yaya::string_t::npos ) { //int
		if ( isDescent ) {
			std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_IntDescent<unsigned int>(arg.array()));
		}
		else {
			std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_IntAscent<unsigned int>(arg.array()));
		}
	}
	else if ( option.find(L"double") != yaya::string_t::npos ) { //double
		if ( isDescent ) {
			std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_DoubleDescent<unsigned int>(arg.array()));
		}
		else {
			std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_DoubleAscent<unsigned int>(arg.array()));
		}
	}
	else /*if ( option.find(L"str") != yaya::string_t::npos )*/ {
		if ( option.find(L"len") != yaya::string_t::npos ) { //strlen
			if ( isDescent ) {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringDescentL<unsigned int>(arg.array()));
			}
			else {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringAscentL<unsigned int>(arg.array()));
			}
		}
		else if ( option.find(L"case") != yaya::string_t::npos ) { //string,case
			if ( isDescent ) {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringDescent<unsigned int>(arg.array()));
			}
			else {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringAscent<unsigned int>(arg.array()));
			}
		}
		else /*if ( option.find(L"case") != yaya::string_t::npos )*/ { //string
			if ( isDescent ) {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringDescentI<unsigned int>(arg.array()));
			}
			else {
				std::sort(sort_vector.begin(),sort_vector.end(),CSFSORT_StringAscentI<unsigned int>(arg.array()));
			}
		}
	}

	CValue rval(F_TAG_ARRAY, 0/*dmy*/);

	if ( option.find(L"index") != yaya::string_t::npos ) {
		unsigned int n = sort_vector.size();
		for ( unsigned int i = 0 ; i < n ; ++i ) {
			rval.array().push_back(CValueSub((int)sort_vector[i]-1));
		}
	}
	else {
		unsigned int n = sort_vector.size();
		for ( unsigned int i = 0 ; i < n ; ++i ) {
			rval.array().push_back(arg.array()[sort_vector[i]]);
		}
	}

	return rval;
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

	bool result = vm.basis().SetParameter(arg.array()[0].GetValueString(),arg.array()[1].GetValueString());
	if ( result ) {
		vm.basis().SetLogger();
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
		
		if ( str.compare(L"coreinfo.version") == 0 ) {
			return CValue(yaya::string_t(aya_version));
		}
		if ( str.compare(L"coreinfo.path") == 0 ) {
			return CValue(vm.basis().GetRootPath());
		}
		if ( str.compare(L"coreinfo.name") == 0 ) {
			return CValue(yaya::string_t(aya_name));
		}
		if ( str.compare(L"coreinfo.author") == 0 ) {
			return CValue(yaya::string_t(aya_author));
		}
		if ( str.compare(L"coreinfo.savefile") == 0 ) {
			return CValue(vm.basis().GetSavefilePath());
		}
		if ( str.compare(L"coreinfo.mode") == 0 ) {
			return CValue(vm.basis().GetModeName());
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
	s_filestr = NULL;

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
	result.array().push_back(CValueSub((int)sb.st_ctime));
	result.array().push_back(CValueSub((int)sb.st_mtime));
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
 *  関数名  ：  CSystemFunction::GETSYSTEMFUNCLIST
 *  引数　　：　_argv[0] = 絞りこみ文字列
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETSYSTEMFUNCLIST(const CValue &arg, yaya::string_t &/*d*/, int &/*l*/)
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
		for ( int i = 0 ; i < sizeof(sysfunc) / sizeof(sysfunc[0]) ; ++i ) {
			result.array().push_back(CValueSub(sysfunc[i]));
		}
	}
	//ある場合
	else {
		yaya::string_t::size_type len = name.length();

		for ( int i = 0 ; i < sizeof(sysfunc) / sizeof(sysfunc[0]) ; ++i ) {
			if ( name.compare(0,len,sysfunc[i],0,len) == 0 && sysfunc[i][0] ) {
				result.array().push_back(CValueSub(sysfunc[i]));
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
void	CSystemFunction::StoreReResultDetails(const yaya::string_t &str,MatchResult &result)
{
	int	sz = result.MaxGroupNumber();
	for(int i = 0; i <= sz; i++) {
		if ( result.GetGroupStart(i) >= 0 ) {
			AppendReResultDetail(
				str.substr(result.GetGroupStart(i),result.GetGroupEnd(i)-result.GetGroupStart(i)),
				result.GetGroupStart(i),
				result.GetGroupEnd(i)-result.GetGroupStart(i));
		}
	}
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
		return vm.basis().base_path + str;

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
	tmpstr = NULL;

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
	delete[](pBuf);
	pBuf = NULL;
	result=CValue(t_str);
	free(t_str);
	t_str= NULL;

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
	s_filestr = NULL;
	if ( s_parameter ) { free(s_parameter); s_parameter = NULL;}

#elif defined(POSIX)

	std::string path = narrow(arg.array()[0].s_value);
    fix_filepath(path);

	if ( arg.array_size() >= 2 ) {
		if ( arg.array()[1].s_value.size() ) {
			path += " ";
			std::string tmp(arg.array()[1].s_value.begin(), arg.array()[1].s_value.end()); 
			path += tmp;
		}
	}

	result = system(path.c_str());

#endif

	// 実行
	return CValue(result);
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::GETENV
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::GETENV(const CValue &arg, yaya::string_t &d, int &l)
{
	if (!arg.array_size()) {
		vm.logger().Error(E_W, 8, L"GETENV", d, l);
		SetError(8);
		return CValue(L"");
	}

	if (!arg.array()[0].IsString()) {
		vm.logger().Error(E_W, 9, L"GETENV", d, l);
		SetError(9);
		return CValue(L"");
	}

	char *s_name = Ccct::Ucs2ToMbcs(arg.array()[0].s_value, CHARSET_DEFAULT);
	if (s_name == NULL) {
		vm.logger().Error(E_E, 89, L"GETENV", d, l);
		SetError(89);
		return CValue(L"");
	}

	const char *s_env = getenv(s_name);

	if (s_env == NULL) {
		vm.logger().Error(E_W, 12, L"GETENV", d, l);
		SetError(12);
		return CValue(L"");
	}

	yaya::char_t	*t_env = Ccct::MbcsToUcs2(s_env, CHARSET_DEFAULT);
	if (t_env == NULL) {
		vm.logger().Error(E_E, 89, L"GETENV", d, l);
		SetError(89);
		return CValue(L"");
	}

	return CValue(t_env);
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
#if defined(WIN32)
	int result;
	
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
	s_filestr = NULL;
	if ( s_parameter ) { free(s_parameter); s_parameter = NULL;}
	
	// 実行
	return CValue(result);
	
#elif defined(POSIX)
	//TODO: Implement
	return CValue(-1);
	
#endif

}

/* -----------------------------------------------------------------------
 *  関数名  ：  CSystemFunction::TRANSLATE
 * -----------------------------------------------------------------------
 */
CValue	CSystemFunction::TRANSLATE(const CValue &arg, yaya::string_t &d, int &l)
{
	if (arg.array_size() < 3) {
		vm.logger().Error(E_W, 8, L"TRANSLATE", d, l);
		SetError(8);
		return CValue(-1);
	}

	yaya::string_t str          = arg.array()[0].GetValueString();
	yaya::string_t rep_from_str = arg.array()[1].GetValueString();
	yaya::string_t rep_to_str   = arg.array()[2].GetValueString();

	std::vector<yaya::char_t> rep_from;
	std::vector<yaya::char_t> rep_to;

	if ( ! ProcessTranslateSyntax(rep_from,rep_from_str,d,l) ) {
		return CValue(-1);
	}
	if ( ! ProcessTranslateSyntax(rep_to,rep_to_str,d,l) ) {
		return CValue(-1);
	}

	if ( rep_from.size() > rep_to.size() ) {
		if ( rep_to.size() > 0 ) {
			//置き換え先のほうが小さいっぽい？
			vm.logger().Error(E_W, 12, L"TRANSLATE", d, l);
			SetError(12);
			//警告を吐いた後で、一番最後の文字で埋めておく
			yaya::char_t cx = *(rep_to.end()-1);
			while ( rep_from.size() > rep_to.size() ) {
				rep_to.push_back(cx);
			}
		}
	}

	bool is_delete = (rep_to.size() == 0);

	//signedでカウントしないと erase時にひどいことになる
	int n = str.length();
	int rep_size = rep_from.size();

	for ( int i = 0 ; i < n ; ++i ) {
		yaya::char_t cx = str[i];
		for ( int r = 0 ; r < rep_size ; ++r ) {
			if ( cx == rep_from[r] ) {
				if ( is_delete ) {
					str.erase(i,1);
					n -= 1;
					i -= 1;
				}
				else {
					str[i] = rep_to[r];
				}
				break;
			}
		}
	}

	return CValue(str);
}

bool CSystemFunction::ProcessTranslateSyntax(std::vector<yaya::char_t> &array,yaya::string_t &str, yaya::string_t &d, int &l)
{
	size_t n = str.length();

	for ( size_t i = 0 ; i < n ; ++i ) {
		if ( str[i] == L'-' ) {
			if ( (i >= (n-1)) || (array.size() == 0) ) {
				//-が閉じてない、もしくは開いてない
				vm.logger().Error(E_W, 12, L"TRANSLATE", d, l);
				SetError(12);
				array.push_back(L'-');
				continue;
			}
			i += 1;
			yaya::char_t start = *(array.end()-1);
			array.erase(array.end()-1,array.end());
			yaya::char_t end = str[i];
			if ( start > end ) {
				//startのほうがでかい：ゼロ要素として処理可能なので続行
				vm.logger().Error(E_W, 12, L"TRANSLATE", d, l);
				SetError(12);
			}
			else if ( start == end ) {
				array.push_back(start);
			}
			else {
				if ( (end - start) >= 256 ) {
					//範囲がでかすぎる：ちょん切って続行
					vm.logger().Error(E_W, 12, L"TRANSLATE", d, l);
					SetError(12);
					end = start + 255;
				}

				for ( yaya::char_t cx = start ; cx <= end ; ++cx ) {
					array.push_back(cx);
				}
			}
		}
		else if ( str[i] == L'\\' ) {
			if ( i >= (n-1) ) {
				//エスケープ後の文字がない
				vm.logger().Error(E_W, 12, L"TRANSLATE", d, l);
				SetError(12);
				array.push_back(L'-');
				continue;
			}
			i += 1;
			yaya::char_t esc_char = str[i];

			if ( esc_char == L'a' ) {
				array.push_back(L'\a');
			}
			else if ( esc_char == L'b' ) {
				array.push_back(L'\b');
			}
			else if ( esc_char == L'e' ) {
				array.push_back(0x1bU);
			}
			else if ( esc_char == L'f' ) {
				array.push_back(L'\f');
			}
			else if ( esc_char == L'n' ) {
				array.push_back(L'\n');
			}
			else if ( esc_char == L'r' ) {
				array.push_back(L'\r');
			}
			else if ( esc_char == L't' ) {
				array.push_back(L'\t');
			}
			else {
				array.push_back(esc_char);
			}
		}
		else {
			array.push_back(str[i]);
		}
	}
	return true;
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
	v.array().push_back(yaya::string_t(L"Copyright (C) 2007 - 2013, ") + aya_author);
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
