// 
// AYA version 5
//
// ��Ȑ�����s�Ȃ��N���X�@CBasis
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

	yaya::string_t	path;					// ��ʃ��W���[������load���ɓn�����p�X

protected:
	char	msglang;				// ���O�ɋL�^���郁�b�Z�[�W�̌���i��/�p�j
	
	char	dic_charset;				// �����R�[�h�Z�b�g�iShift_JIS/UTF-8/OS�f�t�H���g�j
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
	HWND	hlogrcvWnd;				// �`�F�b�N�c�[����hWnd
#if defined(WIN32) || defined(_WIN32_WCE)
	HMODULE	hmodule;				// ���W���[���̃n���h��
#endif
	yaya::string_t	logpath;				// ���O�t�@�C���̃p�X
	yaya::string_t	modulename;				// ���W���[���̎�t�@�C�����i�Ⴆ��a.dll�̏ꍇ��"a"�j
	char	suppress;				// ��������}�~�i�\����͂ŃG���[���N�����ۂɓ�����}�X�N���邽�߂Ɏg�p���܂��j
	char	checkparser;			// �\����͌��ʂ̃��O�ւ̋L�^���w������t���O
	char	iolog;					// ���o�͂̃��O�ւ̋L�^���w������t���O

	int	loadindex;					// �֐� load �̈ʒu
	int	unloadindex;				// �֐� unload �̈ʒu
	int	requestindex;				// �֐� request �̈ʒu

	char	run;					// load������0��1��

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
