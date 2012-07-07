// 
// AYA version 5
//
// ���M���O�p�N���X�@CLog
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <boost/lexical_cast.hpp>

#include "ccct.h"
#include "log.h"
#include "manifest.h"
#include "messages.h"
#include "misc.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "timer.h"
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
 *  �֐���  �F  CLog::Start
 *  �@�\�T�v�F  ���M���O���J�n���܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Start(const yaya::string_t &p, int cs, int ml, HWND hw, char il)
{
	iolog   = il;

	if ( open ) {
		if ( path != p || charset != cs || msglang != ml ) {
			Termination();
		}
		else {
			if ( ! iolog ) {
				Termination();
			}
			return;
		}
	}

	path    = p;
	charset = cs;
	msglang = ml;
	
	if ( hw ) { //hw�����遁�ʂ���̌Ăяo���Ȃ̂ŋ���ON�A�t�@�C������
		path = L"";
	}
	else if ( ! il ) {
		enable = 0;
		return;
	}

#if defined(WIN32)
	// ����hWnd��NULL�Ȃ�N�����̃`�F�b�N�c�[����T���Ď擾����
	hWnd    = hw != NULL ? hw : GetCheckerWnd();
#endif

#if defined(POSIX)
	fix_filepath(path);
#endif

	// ���M���O�L��/�����̔���
	if ( path.size() ) {
		fileen = 1;
		enable = 1;
	}
	else {
		fileen = 0;
#if defined(WIN32)
		if (hWnd == NULL) {
			enable = 0;
			return;
		}
#else
		enable = 0;
		return;
#endif
	}

	// ������쐬
	yaya::string_t	str = (msglang) ? msge[0] : msgj[0];
	str += GetDateString();
	str += L"\n\n";

	// �t�@�C���֏�������
	if (fileen) {
		char	*tmpstr = Ccct::Ucs2ToMbcs(str, charset);
		if (tmpstr != NULL) {
			FILE	*fp = yaya::w_fopen((yaya::char_t *)path.c_str(), L"w");
			if (fp != NULL) {
/*				if (charset == CHARSET_UTF8)
					write_utf8bom(fp);*/
				fprintf(fp, "%s", tmpstr);
				fclose(fp);
			}
			free(tmpstr);
			tmpstr = NULL;
		}
	}
	open = 1;

#if defined(WIN32)
	// �`�F�b�N�c�[���֑��o�@�ŏ��ɕ����R�[�h��ݒ肵�Ă��當����𑗏o
	if (charset == CHARSET_SJIS)
		SendLogToWnd(L"", E_SJIS);
	else if (charset == CHARSET_UTF8)
		SendLogToWnd(L"", E_UTF8);
	else	// CHARSET_DEFAULT
		SendLogToWnd(L"", E_DEFAULT);

	SendLogToWnd(str, E_I);
#endif
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Termination
 *  �@�\�T�v�F  ���M���O���I�����܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Termination(void)
{
	if (!enable)
		return;

	Message(1);

	yaya::string_t	str = GetDateString();
	str += L"\n\n";

	Write(str);

	open = 0;

#if defined(WIN32)
	SendLogToWnd(L"", E_END);
#endif
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Write
 *  �@�\�T�v�F  ���O�ɕ�������������݂܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Write(const yaya::char_t *str, int mode)
{
	if (!enable)
		return;
	if (str == NULL)
		return;
	if (!wcslen(str))
		return;

	// �����񒆂�\r�͏���
	yaya::string_t	cstr = str;
	int	len = cstr.size();
	for(int i = 0; i < len; ) {
		if (cstr[i] == L'\r') {
			cstr.erase(i, 1);
			len--;
			continue;
		}
		i++;
	}

	// �t�@�C���֏�������
	if (fileen) {
		if (! path.empty()) {
			char	*tmpstr = Ccct::Ucs2ToMbcs(cstr, charset);
			if (tmpstr != NULL) {
				FILE	*fp = yaya::w_fopen((yaya::char_t *)path.c_str(), L"a");
				if (fp != NULL) {
					fprintf(fp, "%s", tmpstr);
					fclose(fp);
				}
				free(tmpstr);
				tmpstr = NULL;
			}
		}
	}

#if defined(WIN32)
	// �`�F�b�N�c�[���֑��o
	SendLogToWnd(cstr, mode);
#endif
}

//----

void	CLog::Write(const yaya::string_t &str, int mode)
{
	Write(str.c_str(), mode);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Filename
 *  �@�\�T�v�F  ����̃t�H�[�}�b�g�Ńt�@�C���������O�ɋL�^���܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Filename(const yaya::string_t &filename)
{
	yaya::string_t	str =  L"- ";
	str += filename;
	str += L"\n";
	Write(str);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Message
 *  �@�\�T�v�F  id�Ŏw�肳�ꂽ����̃��b�Z�[�W�����O�ɏ������݂܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Message(int id)
{
	Write((msglang) ? (yaya::char_t *)msge[id] : (yaya::char_t *)msgj[id], 0);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Error
 *  �@�\�T�v�F  ���O��mode��id�Ŏw�肳�ꂽ�G���[��������������݂܂�
 *
 *  �����@�@�F�@ref         �t�����
 *  �@�@�@�@�@  dicfilename �G���[���N�������ӏ����܂ގ����t�@�C���̖��O
 *  �@�@�@�@�@  linecount   �G���[���N�������s�ԍ�
 *
 *  �@�@�@�@�@  ref��dicfilename��NULL�Alinecount��-1�Ƃ��邱�ƂŁA������
 *  �@�@�@�@�@  ��\���ɂł��܂�
 * -----------------------------------------------------------------------
 */
void	CLog::Error(int mode, int id, const yaya::char_t *ref, const yaya::string_t &dicfilename, int linecount)
{
	if (!enable)
		return;

	// ���O�ɏ������ݕ�������쐬�i�����t�@�C�����ƍs�ԍ��j
	yaya::string_t	logstr;
	if (dicfilename.empty())
		logstr = L"-(-) : ";
	else {
		logstr = dicfilename + L"(";
		if (linecount == -1)
			logstr += L"-) : ";
		else {
			logstr += yaya::ws_itoa(linecount);
			logstr += L") : ";
		}
	}
	// ���O�ɏ������ݕ�������쐬�i�{���j
	if (msglang) {
		// �p��
		if (mode == E_F)
			logstr += msgfe[id];
		else if (mode == E_E)
			logstr += msgee[id];
		else if (mode == E_W)
			logstr += msgwe[id];
		else
			logstr += msgne[id];
	}
	else {
		// ���{��
		if (mode == E_F)
			logstr += msgfj[id];
		else if (mode == E_E)
			logstr += msgej[id];
		else if (mode == E_W)
			logstr += msgwj[id];
		else
			logstr += msgnj[id];
	}
	// ���O�ɏ������ݕ�������쐬�i�t�����j
	if (ref != NULL) {
		logstr += L" : ";
		logstr += ref;
	}
	// ��������
	logstr += L'\n';
	Write(logstr, mode);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& ref, const yaya::string_t& dicfilename, int linecount)
{
	Error(mode, id, (yaya::char_t *)ref.c_str(), dicfilename, linecount);
}

//----

void	CLog::Error(int mode, int id, const yaya::char_t *ref)
{
        Error(mode, id, ref, yaya::string_t(), -1);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& ref)
{
        Error(mode, id, (yaya::char_t *)ref.c_str(), yaya::string_t(), -1);
}

//----

void	CLog::Error(int mode, int id, const yaya::string_t& dicfilename, int linecount)
{
	Error(mode, id, (yaya::char_t *)NULL, dicfilename, linecount);
}

//----

void	CLog::Error(int mode, int id)
{
        Error(mode, id, (yaya::char_t *)NULL, yaya::string_t(), -1);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::Io
 *  �@�\�T�v�F  ���o�͕�����Ǝ��s���Ԃ����O�ɋL�^���܂�
 *  �����@�@�F  io 0/1=�J�n��/�I����
 * -----------------------------------------------------------------------
 */
void	CLog::Io(char io, const yaya::char_t *str)
{
	if (!enable || !iolog)
		return;

	static	yaya::timer		timer;

	if (!io) {
		//ignoreiolog�@�\�B
		if(ignore_iolog_strings.size()!=0){
			yaya::string_t cstr=str;
			std::vector<yaya::string_t>::iterator it;

			for(it = ignore_iolog_strings.begin(); it != ignore_iolog_strings.end(); it++){
				if(cstr.find(*it) != yaya::string_t::npos){
					//���̏o�͂̃��O��}������B
					//�����������ł͂Ȃ�����basis���Ɏ�����������Ȃ��̂ŁB
					//����Abasis���ŕK�����͂Əo�͂̓����Z�b�g�ŏo��͂�
					//-> see basis.cpp ExecuteRequest
					ignore_iolog_noresult=1;
					return;
				}
			}

		}

		Write(L"// request\n");
		Write(str);
		Write(L"\n");

        timer.restart();
	}
	else {
		//���O�}��
		if(ignore_iolog_noresult){
			ignore_iolog_noresult=0;
			return;
		}

		int elapsed_time = timer.elapsed();
		yaya::string_t t_str = L"// response (Execution time : " + yaya::ws_itoa(elapsed_time) + L"[ms])\n";

		Write(t_str);
		Write(str);
		Write(L"\n");
	}
}

void	CLog::Io(char io, const yaya::string_t &str)
{
	Io(io,str.c_str());
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::IoLib
 *  �@�\�T�v�F  �O�����C�u�������o�͕�����Ǝ��s���Ԃ����O�ɋL�^���܂�
 *  �����@�@�F  io 0/1=�J�n��/�I����
 * -----------------------------------------------------------------------
 */
void	CLog::IoLib(char io, const yaya::string_t &str, const yaya::string_t &name)
{
	if (!enable || !iolog)
		return;

	static	yaya::timer		timer;

	if (!io) {
		Write(L"// request to library\n// name : ");
		Write(name + L"\n");
		Write(str + L"\n");

		timer.restart();
	}
	else {
		int elapsed_time = timer.elapsed();
		yaya::string_t t_str = L"// response (Execution time : " + yaya::ws_itoa(elapsed_time) + L"[ms])\n";

		Write(t_str);
		Write(str + L"\n");
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::SendLogToWnd
 *  �@�\�T�v�F  �`�F�b�N�c�[���ɐ��䃁�b�Z�[�W����у��O�������WM_COPYDATA�ő��M���܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CLog::SendLogToWnd(const yaya::char_t *str, int mode)
{
	if (hWnd == NULL)
		return;

	COPYDATASTRUCT cds;
	cds.dwData = mode;
	cds.cbData = (wcslen(str) + 1)*sizeof(yaya::char_t);
	cds.lpData = (LPVOID)str;

	DWORD res_dword = 0;
	::SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds, SMTO_ABORTIFHUNG|SMTO_BLOCK, 5000, &res_dword);
}

//----

void	CLog::SendLogToWnd(const yaya::string_t &str, int mode)
{
	SendLogToWnd((yaya::char_t *)str.c_str(), mode);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::GetCheckerWnd
 *  �@�\�T�v�F  �`�F�b�N�c�[����hWnd���擾���܂���
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
HWND	CLog::GetCheckerWnd(void)
{
	return FindWindow(CLASSNAME_CHECKTOOL, NULL);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::AddIgnoreIologString
 *  �@�\�T�v�F  IO���O�̖������镶���񃊃X�g��ǉ����܂�
 * -----------------------------------------------------------------------
 */
void	CLog::AddIgnoreIologString(const yaya::string_t &ignorestr){
	ignore_iolog_strings.push_back(ignorestr);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLog::AddIgnoreIologString
 *  �@�\�T�v�F  IO���O�̖������镶���񃊃X�g���N���A���܂�
 * -----------------------------------------------------------------------
 */
void	CLog::ClearIgnoreIologString(){
	ignore_iolog_strings.clear();
}
