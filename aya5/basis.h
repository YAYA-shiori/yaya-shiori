// 
// AYA version 5
//
// 主な制御を行なうクラス　CBasis
// written by umeici. 2004
// 

#ifndef	BASISH
#define	BASISH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <vector>

#include "aya5.h"
#include "globaldef.h"

class CValue;
class CAyaVM;

class	CBasis
{
public:
	CAyaVM &vm;

	yaya::string_t	path;					// 上位モジュールからload時に渡されるパス

protected:
	char	msglang;				// ログに記録するメッセージの言語（日/英）
	
	char	dic_charset;				// 文字コードセット（Shift_JIS/UTF-8/OSデフォルト）
	char	output_charset;
	char	file_charset;
	char	save_charset;
	char	extension_charset;
	bool	encode_savefile;
	bool	auto_save;

#if !defined(WIN32)
	typedef void* HWND;
#endif
	HWND	hlogrcvWnd;				// チェックツールのhWnd
#if defined(WIN32) || defined(_WIN32_WCE)
	HMODULE	hmodule;				// モジュールのハンドル
#endif
	yaya::string_t	logpath;				// ログファイルのパス
	yaya::string_t	modulename;				// モジュールの主ファイル名（例えばa.dllの場合は"a"）
	char	suppress;				// 自律動作抑止（構文解析でエラーが起きた際に動作をマスクするために使用します）
	char	checkparser;			// 構文解析結果のログへの記録を指示するフラグ
	char	iolog;					// 入出力のログへの記録を指示するフラグ

	int	loadindex;					// 関数 load の位置
	int	unloadindex;				// 関数 unload の位置
	int	requestindex;				// 関数 request の位置

	char	run;					// load完了で0→1へ

private:
	CBasis(void);

public:
	CBasis(CAyaVM &vmr);
	~CBasis(void) {}

	char	IsRun(void) { return run; }

#if defined(WIN32) || defined(_WIN32_WCE)
	void	SetModuleHandle(HANDLE h);
	void	SetLogRcvWnd(long hwnd);
#endif
	void	SetPath(yaya::global_t h, int len);

	void	Configure(void);
	void	Termination(void);
	
#if defined(WIN32)
	HWND	GetLogHWnd(void) { return hlogrcvWnd; }
#endif
	yaya::string_t	GetLogPath(void)  { return logpath; }
	char	GetMsgLang(void)  { return msglang; }
	char	GetDicCharset(void)  { return dic_charset; }
	yaya::string_t	GetRootPath(void) { return path;    }

	void	ExecuteLoad(void);
	yaya::global_t	ExecuteRequest(yaya::global_t h, long *len);
	void	ExecuteUnload(void);

	void	SaveVariable(const yaya::char_t* pName = NULL);
	void	RestoreVariable(const yaya::char_t* pName = NULL);

	yaya::string_t GetParameter(const yaya::string_t &cmd);
	bool SetParameter(yaya::string_t &cmd, yaya::string_t &param, std::vector<yaya::string_t> *dics = NULL);

	void    SetOutputCharset(char cst) { output_charset = cst; }

protected:
	char	IsSuppress(void);
	void	SetSuppress(void);
	void	ResetSuppress(void);

	void	LoadBaseConfigureFile(std::vector<yaya::string_t> *dics);

	void	RestoreArrayVariable(CValue &var, yaya::string_t &value);
	void	RestoreHashVariable(CValue &var, yaya::string_t &value);
};

//----

#endif
