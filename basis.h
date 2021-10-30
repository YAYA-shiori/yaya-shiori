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
	ptrdiff_t pos_saved;

public:
	CBasisFuncPos() : is_try_find(false) , pos_saved(-1) { }
	~CBasisFuncPos() { }

	void Init() { is_try_find = false; pos_saved = -1; }

	bool IsNotFound() { return is_try_find && (pos_saved == -1); }

	ptrdiff_t Find(CAyaVM &vm,const yaya::char_t *name);
};

class	CBasis
{
public:
	CAyaVM &vm;

	yaya::string_t	base_path;					// 上位モジュールからload時に渡されるパス
	yaya::string_t	load_path;					// messagetxt\savefile\yaya.txt etc.
	yaya::string_t  messagetxt_path;			// エラーメッセージを読んだパス

protected:
	char	msglang_for_compat;

	char	dic_charset;				// 文字コードセット（Shift_JIS/UTF-8/OSデフォルト）
	char    setting_charset;
	char	output_charset;
	char	file_charset;
	char	save_charset;
	char    save_old_charset;
	char	extension_charset;
	char    log_charset;

	bool	encode_savefile;
	bool	auto_save;

#if !defined(WIN32)
	typedef void* HWND;
#endif
	HWND	hlogrcvWnd;				// チェックツールのhWnd

	yaya::string_t	logpath;				// ログファイルのパス
	yaya::string_t	modulename;				// モジュールの主ファイル名（例えばa.dllの場合は"a"）
	yaya::string_t  config_file_name_trailer;  // 主設定ファイル名生成時のトレイラー(通常は空)
	yaya::string_t	modename;				// 現在のモード (emergency or normal)
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

	void	SetModuleName(const yaya::string_t &s,const yaya::char_t *trailer,const yaya::char_t *mode);
#if defined(WIN32) || defined(_WIN32_WCE)
	void	SetLogRcvWnd(long hwnd);
#endif
	void	SetPath(yaya::global_t h, int len);

	void	Configure(void);
	void	Termination(void);

#if defined(WIN32)
	HWND	GetLogHWnd(void) { return hlogrcvWnd; }
#endif
	const yaya::string_t& GetLogPath(void) const  { return logpath; }
	char	GetDicCharset(void)  { return dic_charset; }
	const yaya::string_t& GetRootPath(void) const { return base_path;    }
	yaya::string_t GetSavefilePath(void) const { return load_path + modulename + L"_variable.cfg"; }
	const yaya::char_t* GetModeName(void) const { return modename.c_str(); }

	void	ExecuteLoad(void);
	yaya::global_t	ExecuteRequest(yaya::global_t h, long *len, bool is_debug);
	void	CallOnMemoryLimit();
	void	CallOnMemoryError();
	void	ExecuteUnload(void);

	void	SaveVariable(const yaya::char_t* pName = NULL);
	void	RestoreVariable(const yaya::char_t* pName = NULL);

	CValue GetParameter(const yaya::string_t &cmd);
	bool SetParameter(const yaya::string_t &cmd, const yaya::string_t &param, std::vector<CDic1> *dics = NULL);
	void SetLogger(void);

	void    SetOutputCharset(char cst) { output_charset = cst; }

	char	IsSuppress(void);

	yaya::string_t	ToFullPath(const yaya::string_t& str);
protected:
	void	SetSuppress(void);
	void	ResetSuppress(void);

	void LoadBaseConfigureFile(std::vector<CDic1> &dics);

	void LoadBaseConfigureFile_Base(yaya::string_t filename, std::vector<CDic1> &dics,char cset);

	void	RestoreArrayVariable(CValue &var, yaya::string_t &value);
	void	RestoreHashVariable(CValue &var, yaya::string_t &value);
};

//----

#endif
