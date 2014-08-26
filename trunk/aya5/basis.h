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

class   CDic1
{
public:
	yaya::string_t path;
	char charset;

	CDic1(const yaya::string_t &p,char c) : path(p), charset(c) {
	}
};

class   CBasisFuncPos
{
private:
	bool is_try_find;
	int pos_saved;

public:
	CBasisFuncPos() : is_try_find(false) , pos_saved(-1) { }
	~CBasisFuncPos() { }

	void Init() { is_try_find = false; pos_saved = -1; }

	bool IsNotFound() { return is_try_find && (pos_saved == -1); }

	int Find(CAyaVM &vm,yaya::char_t *name);
};

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
	char    save_old_charset;
	char	extension_charset;
	bool	encode_savefile;
	bool	auto_save;

#if !defined(WIN32)
	typedef void* HWND;
#endif
	HWND	hlogrcvWnd;				// チェックツールのhWnd

	yaya::string_t	logpath;				// ログファイルのパス
	yaya::string_t	modulename;				// モジュールの主ファイル名（例えばa.dllの場合は"a"）
	char	suppress;				// 自律動作抑止（構文解析でエラーが起きた際に動作をマスクするために使用します）
	char	checkparser;			// 構文解析結果のログへの記録を指示するフラグ
	char	iolog;					// 入出力のログへの記録を指示するフラグ

	CBasisFuncPos loadindex;				// 関数 load の位置
	CBasisFuncPos unloadindex;				// 関数 unload の位置
	CBasisFuncPos requestindex;				// 関数 request の位置

	char	run;					// load完了で0→1へ

private:
	CBasis(void);

public:
	CBasis(CAyaVM &vmr);
	~CBasis(void) {}

	char	IsRun(void) { return run; }

	void	SetModuleName(const yaya::string_t &s);
#if defined(WIN32) || defined(_WIN32_WCE)
	void	SetLogRcvWnd(long hwnd);
#endif
	void	SetPath(yaya::global_t h, int len);

	void	Configure(void);
	void	Termination(void);
	
#if defined(WIN32)
	HWND	GetLogHWnd(void) { return hlogrcvWnd; }
#endif
	const yaya::string_t& GetLogPath(void) const { return logpath; }
	char	GetMsgLang(void)  { return msglang; }
	char	GetDicCharset(void)  { return dic_charset; }
	const yaya::string_t& GetRootPath(void) const { return path;    }
	yaya::string_t GetSavefilePath(void) const { return path + modulename + L"_variable.cfg"; }

	void	ExecuteLoad(void);
	yaya::global_t	ExecuteRequest(yaya::global_t h, long *len, bool is_debug);
	void	ExecuteUnload(void);

	void	SaveVariable(const yaya::char_t* pName = NULL);
	void	RestoreVariable(const yaya::char_t* pName = NULL);

	yaya::string_t GetParameter(const yaya::string_t &cmd);
	bool SetParameter(const yaya::string_t &cmd, const yaya::string_t &param, std::vector<CDic1> *dics = NULL);
	void SetLogger(void);

	void    SetOutputCharset(char cst) { output_charset = cst; }

protected:
	char	IsSuppress(void);
	void	SetSuppress(void);
	void	ResetSuppress(void);

	void	LoadBaseConfigureFile(std::vector<CDic1> &dics);

	void	RestoreArrayVariable(CValue &var, yaya::string_t &value);
	void	RestoreHashVariable(CValue &var, yaya::string_t &value);
};

//----

#endif
