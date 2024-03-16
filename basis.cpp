//
// AYA version 5
//
// ��Ȑ�����s�Ȃ��N���X�@CBasis
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
//#include <filesystem>

#include "fix_unistd.h"

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
#include "dir_enum.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

//for compatibility only
#define MSGLANG_JAPANESE 0
#define MSGLANG_ENGLISH  1

/* -----------------------------------------------------------------------
 * CBasis�R���X�g���N�^
 * -----------------------------------------------------------------------
 */

CBasis::CBasis(CAyaVM &vmr) : vm(vmr)
{
	ResetSuppress();

	checkparser = 0;
	iolog       = 1;

	msglang_for_compat = MSGLANG_JAPANESE;

	dic_charset       = CHARSET_SJIS;
	setting_charset   = CHARSET_SJIS;
	output_charset    = CHARSET_UTF8;
	file_charset      = CHARSET_SJIS;
	save_charset      = CHARSET_UTF8;
	save_old_charset  = CHARSET_SJIS;
	extension_charset = CHARSET_SJIS;
	log_charset       = CHARSET_UTF8;

	encode_savefile = false;
	auto_save = true;

#if defined(WIN32)
	hlogrcvWnd  = NULL;
#endif
	run         = 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetModuleHandle
 *  �@�\�T�v�F  ���W���[���n���h�����擾���܂�
 *
 *  ���łɃ��W���[���̎�t�@�C�����擾���s���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::SetModuleName(const yaya::string_t &s,const yaya::char_t *trailer,const yaya::char_t *mode)
{
	modulename = s;
	modename = mode;

	if ( trailer ) {
		config_file_name_trailer = trailer;
	}
	else {
		config_file_name_trailer.erase(config_file_name_trailer.begin(), config_file_name_trailer.end());
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetPath
 *  �@�\�T�v�F  HGLOBAL�Ɋi�[���ꂽ�t�@�C���p�X���擾���܂� HGLOBAL�͊J�����܂���
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
void	CBasis::SetPath(yaya::global_t h, int len)
{
	// �擾�Ɨ̈�J��
	std::string	mbpath;
	mbpath.assign((char *)h, 0, len);
	//GlobalFree(h); //load���ŊJ��
	h = NULL;

	// �����R�[�h��UCS-2�֕ϊ��i�����ł̃}���`�o�C�g�����R�[�h��OS�f�t�H���g�j
	Ccct::MbcsToUcs2Buf(base_path, mbpath, CHARSET_DEFAULT);

	//�Ōオ\�ł�/�ł��Ȃ���Α���
	if (base_path.length() == 0 || ( (base_path[base_path.length()-1] != L'/') && (base_path[base_path.length()-1] != L'\\') ) ) {
		base_path += L"\\";
	}

	load_path = base_path;
}
#elif defined(POSIX)
void	CBasis::SetPath(yaya::global_t h, int len)
{
	// �擾�Ɨ̈�J��
	base_path = widen(std::string(h, static_cast<std::string::size_type>(len)));
	//free(h); //load���ŊJ��
	h = NULL;
	// �X���b�V���ŏI����ĂȂ���Εt����B
	if (base_path.length() == 0 || base_path[base_path.length() - 1] != L'/') {
		base_path += L'/';
	}
	// ���W���[���n���h���̎擾�͏o���Ȃ��̂ŁA�͋Z�ňʒu��m��B
	// ���̃f�B���N�g���ɂ���S�Ă�*.dll(case insensitive)��T���A
	// ���g��yaya.dll�Ƃ�����������܂�ł�����A�����I�ԁB
	// �������Ή�����*.txt���������dll�̒��g�͌����Ɏ��֍s���B
	modulename = L"yaya";
	DIR* dh = opendir(narrow(base_path).c_str());
	if (dh == NULL) {
		std::cerr << narrow(base_path) << "is not a directory!" << std::endl;
	exit(1);
	}
	while (true) {
	struct dirent* ent = readdir(dh);
	if (ent == NULL) {
	    break; // ��������
	}
	std::string fname(ent->d_name, strlen(ent->d_name)/*ent->d_namlen*/);	// by umeici. 2005/1/16 5.6.0.232
	if (lc(get_extension(fname)) == "dll") {
		std::string txt_file = narrow(base_path) + change_extension(fname, "txt");
	    struct stat sb;
	    if (::stat(txt_file.c_str(), &sb) == 0) {
		// txt�t�@�C��������̂ŁA���g�����Ă݂�B
		if (file_content_search(narrow(base_path) + fname, "yaya.dll") != std::string::npos) {
		    // �����YAYA��DLL�ł���B
		    modulename = widen(drop_extension(fname));
		    break;
		}
	    }
	}
	}
	closedir(dh);
	load_path = base_path;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetLogRcvWnd
 *  �@�\�T�v�F  �`�F�b�N�c�[������n���ꂽhWnd��ێ����܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
void	CBasis::SetLogRcvWnd(long hwnd)
{
	hlogrcvWnd = (HWND)hwnd;
	vm.logger().Start(logpath, log_charset, hlogrcvWnd, iolog);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetLogger
 *  �@�\�T�v�F  ���O�@�\�������� / �Đݒ肵�܂�
 * -----------------------------------------------------------------------
 */
void CBasis::SetLogger(void)
{
	vm.logger().Start(logpath, log_charset, hlogrcvWnd, iolog);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::Configure
 *  �@�\�T�v�F  load���ɍs�������ݒ菈��
 * -----------------------------------------------------------------------
 */
void	CBasis::Configure(void)
{
	// ��b�ݒ�t�@�C���i�Ⴆ��aya.txt�j��ǂݎ��
	std::vector<CDic1>	dics;
	LoadBaseConfigureFile(dics);
	// ��b�ݒ�t�@�C���ǂݎ��ŏd�ĂȃG���[�����������ꍇ�͂����ŏI��
	if (suppress)
		return;

	// ���M���O���J�n
	SetLogger();

	// �����ǂݍ��݂ƍ\�����
	if (vm.parser0().Parse(dic_charset, dics))
		SetSuppress();

	{
		CLogExCode logex(vm);

		if (checkparser)
			logex.OutExecutionCodeForCheck();

		// �O��I�����ɕۑ������ϐ��𕜌�
		RestoreVariable();

		if (checkparser)
			logex.OutVariableInfoForCheck();
	}

	// �����܂ł̏����ŏd�ĂȃG���[�����������ꍇ�͂����ŏI��
	if (suppress)
		return;

	// �O�����C�u�����ƃt�@�C���̕����R�[�h��������
	vm.libs().SetCharset(extension_charset);
	vm.files().SetCharset(file_charset);

	run = 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::Termination
 *  �@�\�T�v�F  unload���ɍs���I������
 * -----------------------------------------------------------------------
 */
void	CBasis::Termination(void)
{
	// ����}�~����Ă��Ȃ���ΏI�����̏��������s
	if (!suppress) {
		// unload
		ExecuteUnload();
		// ���[�h���Ă��邷�ׂẴ��C�u������unload
		vm.libs().DeleteAll();
		// �J���Ă��邷�ׂẴt�@�C�������
		vm.files().DeleteAll();
		// �ϐ��̕ۑ�
		if ( auto_save ) {
			SaveVariable();
		}
	}

	// ���M���O���I��
	vm.logger().Termination();

	//
	loadindex.Init();
	unloadindex.Init();
	requestindex.Init();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::IsSuppress
 *  �@�\�T�v�F  ���݂̎����}�~��Ԃ�Ԃ��܂�
 *
 *  �Ԓl�@�@�F  0/1=��}�~/�}�~
 *
 *  ��b�ݒ�t�@�C���̓ǂݎ��⎫���t�@�C���̉�͒��ɁA����p������ȃG���[�����������
 *  SetSuppress()�ɂ���ė}�~�ݒ肳��܂��B�}�~�ݒ肳���ƁAload/request/unload�ł̓��삪
 *  ���ׂă}�X�N����܂��B���̎��Arequest�̕Ԓl�͏�ɋ󕶎���ɂȂ�܂��B�iHGLOBAL=NULL�A
 *  len=0�ŉ������܂��j
 * -----------------------------------------------------------------------
 */
char	CBasis::IsSuppress(void)
{
	return suppress;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CSystemFunction::ToFullPath
 *  �@�\�T�v�F  �n���ꂽ�����񂪑��΃p�X�\�L�Ȃ��΃p�X�ɏ��������܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32)
yaya::string_t	CBasis::ToFullPath(const yaya::string_t& str)
{
	yaya::char_t	drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_wsplitpath(str.c_str(), drive, dir, fname, ext);
	yaya::string_t aret = str;

	if (!::wcslen(drive))
		aret = vm.basis().base_path + str;

	yaya::ws_replace(aret,L"/",L"\\");
	size_t index;
	while((index = aret.find(L"\\\\")) != yaya::string_t::npos)
		aret.replace(index,2,L"\\");

	return aret;
}
#elif defined(POSIX)
yaya::string_t CBasis::ToFullPath(const yaya::string_t& str)
{
	yaya::string_t aret = str;
	if (!(str.length() > 0 && str[0] == L'/')) {
		aret = vm.basis().base_path + str;
	}

	yaya::ws_replace(aret, L"/", L"\\");
	size_t index;
	while ((index = aret.find(L"\\\\")) != yaya::string_t::npos)
		aret.replace(index, 2, L"\\");

	return aret;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetSuppress
 *  �@�\�T�v�F  ��������}�~��ݒ肵�܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::SetSuppress(void)
{
	suppress = 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ResetSuppress
 *  �@�\�T�v�F  ��������}�~�@�\�����Z�b�g���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ResetSuppress(void)
{
	suppress = 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::LoadBaseConfigureFile
 *  �@�\�T�v�F  ��b�ݒ�t�@�C����ǂݎ��A�e��p�����[�^���Z�b�g���܂�
 *
 *  ��b�ݒ�t�@�C����DLL�Ɠ��K�w�ɑ��݂��閼�O��"DLL��t�@�C����.txt"�̃t�@�C���ł��B
 *
 *  �����t�@�C���̕����R�[�h��Shift_JIS�ȊO�ɂ�UTF-8��OS�f�t�H���g�̃R�[�h�ɑΉ��ł��܂����A
 *  ���̊�b�ݒ�t�@�C����OS�f�t�H���g�̃R�[�h�œǂݎ���邱�Ƃɒ��ӂ��Ă��������B
 *  ���ۉ��Ɋւ��čl������ꍇ�́A���̃t�@�C�����̋L�q�Ƀ}���`�o�C�g�������g�p����ׂ��ł�
 *  ����܂���i�����R�[�h0x7F�ȉ���ASCII�����݂̂ŋL�q���ׂ��ł��j�B
 * -----------------------------------------------------------------------
 */
void	CBasis::LoadBaseConfigureFile(std::vector<CDic1> &dics)
{
	// �ݒ�t�@�C��("name".txt)�ǂݎ��

	// �t�@�C�����J��
	yaya::string_t	filename = load_path + modulename + config_file_name_trailer + L".txt";

	// �ǂݍ��ݓ����͕����R�[�h����`����Ă��Ȃ��̂ŁACHARSET_UNDEF�ɂ���
	LoadBaseConfigureFile_Base(filename,dics,CHARSET_UNDEF);

	if ( yayamsg::IsEmpty() ) { //�G���[���b�Z�[�W�e�[�u�����ǂ߂Ă��Ȃ�
		SetParameter(L"messagetxt",msglang_for_compat == MSGLANG_JAPANESE ? L"messagetxt/japanese.txt" : L"messagetxt/english.txt");
	}
}

void	CBasis::LoadBaseConfigureFile_Base(yaya::string_t filename,std::vector<CDic1> &dics,char cset)
{
	// �t�@�C�����J��
	FILE	*fp = yaya::w_fopen(filename.c_str(), L"r");
	if (fp == NULL) {
		vm.logger().Error(E_E, 5, filename);
		SetSuppress();
		return;
	}

	// �ǂݎ�菈��
	CComment	comment;
	yaya::string_t	cmd, param;
	size_t line=0;
	
	yaya::string_t	readline;
	readline.reserve(1000);

	char cset_real;
	std::string buf;
	buf.reserve(1000);

	while ( true ) {
		line += 1;

		// 1�s�ǂݍ���
		cset_real = cset;

		if ( cset == CHARSET_UNDEF ) {
			//��̐ݒ�ŕύX����Ă���\��������̂ŁA����㏑�����邱��
			//always overwrite cset_real because setting_charset may be modified in SetParameter function
			cset_real = setting_charset;
		}

		if (yaya::ws_fgets(buf, readline, fp, cset_real, 0, line) == yaya::WS_EOF) {
			// �t�@�C�������
			fclose(fp);

			break;
		}

		// ���s�͏���
		CutCrLf(readline);

		// �R�����g�A�E�g����
		comment.Process_Top(readline);
		comment.Process(readline);
		comment.Process_Tail(readline);

		// ��s�A�������͑S�̂��R�����g�s�������ꍇ�͎��̍s��
		if (readline.size() == 0) {
			continue;
		}

		// �p�����[�^��ݒ�
		if (Split(readline, cmd, param, L",")) {
			SetParameter(cmd, param, &dics);
		}
		else {
			vm.logger().Error(E_W, 0, filename, line);
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SetParameter
 *  �@�\�T�v�F  LoadBaseConfigureFile����Ă΂�܂��B�e��p�����[�^��ݒ肵�܂�
 * -----------------------------------------------------------------------
 */
bool CBasis::SetParameter(const yaya::string_t &cmd, const yaya::string_t &param, std::vector<CDic1> *dics)
{
	//include
	if ( cmd == L"include" ) {
		yaya::string_t param1, param2;
		Split(param, param1, param2, L",");

		yaya::string_t filename = load_path + param1;

		char cset = setting_charset;
		if ( param2.size() ) {
			char cx = Ccct::CharsetTextToID(param2.c_str());
			if ( cx != CHARSET_DEFAULT ) {
				cset = cx;
			}
		}
		LoadBaseConfigureFile_Base(filename,*dics,cset);
		return true;
	}
	//includeEX
	else if ( cmd == L"includeEX" ) {
		yaya::string_t param1, param2;
		Split(param, param1, param2, L",");

		yaya::string_t filename = load_path + param1;

		char cset = setting_charset;
		if ( param2.size() ) {
			char cx = Ccct::CharsetTextToID(param2.c_str());
			if ( cx != CHARSET_DEFAULT ) {
				cset = cx;
			}
		}

		//posix�ł�npos��unsigned��-1�Ńo�J�ł������ɂȂ�̂ł��̑΍�
		yaya::string_t load_path_bak = load_path;
		auto s_pos = filename.rfind('/');
		s_pos = (s_pos == yaya::string_t::npos) ? (0) : (s_pos);
		auto bs_pos = filename.rfind('\\');
		bs_pos = (bs_pos == yaya::string_t::npos) ? (0) : (bs_pos);
		load_path = filename.substr(0,std::max(s_pos,bs_pos))+L'/';

		yaya::string_t base_path_bak = base_path;
		base_path = load_path;

		LoadBaseConfigureFile_Base(filename,*dics,cset);

		load_path = load_path_bak;
		base_path = base_path_bak;

		return true;
	}
	// dic
	else if ( (cmd == L"dic" || cmd == L"dicif") && dics) {
		yaya::string_t param1,param2;
		Split(param, param1, param2, L",");

		yaya::string_t	filename = base_path + param1;
#ifdef POSIX
		fix_filepath(filename);
#endif

		char cset = dic_charset;
		if ( param2.size() ) {
			char cx = Ccct::CharsetTextToID(param2.c_str());
			if ( cx != CHARSET_DEFAULT ) {
				cset = cx;
			}
		}

		if ( cmd == L"dicif" ) {
			FILE *fp = yaya::w_fopen(filename.c_str(), L"rb");
			if ( !fp ) {
				return true; //skip loading if file not exist
			}
			fclose(fp);
		}

		dics->emplace_back(CDic1(filename,cset));
		return true;
	}
	// dicdir
	else if ( cmd == L"dicdir" && dics) {
		yaya::string_t param1,param2;
		Split(param, param1, param2, L",");

		//if the target folder has _loading_order.txt & not has param2 in this line, then includeEX this file
		if(param2.empty()) {
			yaya::string_t file = param1 + L"/_loading_order_override.txt";
			yaya::string_t filename = load_path + file;
			FILE *fp = yaya::w_fopen(filename.c_str(), L"rb");

			if ( ! fp ) {
				file = param1 + L"/_loading_order.txt";
				filename = load_path + file;

				fp = yaya::w_fopen(filename.c_str(), L"rb");
			}

			//_waccess is not use as it does not support mixed forward and back slashes
			if ( fp ) {
				fclose(fp);
				return SetParameter(L"includeEX", file, dics);
			}
		}

		//else (loading_order not exist | param2 exist) include this folder
		{
			yaya::string_t dirname = base_path + param1;
			CDirEnum	  ef(dirname);
			CDirEnumEntry entry;
			bool		  aret = true;
			
			while(ef.next(entry)) {
				//If the file suffix is bak or tmp, skip it.
				size_t extbegpos=entry.name.rfind('.');
				if(extbegpos!=entry.name.npos) {
					yaya::string_t ext=entry.name.substr(extbegpos+1);
					if(ext==L"bak" || ext==L"tmp") {
						continue;
					}
				}

				yaya::string_t relpath_and_cs = param1 + L"\\" + entry.name + L',' + param2;

				if(entry.isdir) {
					aret &= SetParameter(L"dicdir", relpath_and_cs, dics);
				}
				else {
					aret &= SetParameter(L"dic", relpath_and_cs, dics);
				}
			}
			return aret;
		}
	}
	// messagetxt
	else if ( cmd == L"messagetxt" ) { //�����ꉻ
		yaya::string_t param1,param2;
		Split(param, param1, param2, L",");

		char cset = CHARSET_UTF8; //UTF8�Œ�

		if ( param2.size() ) {
			char cx = Ccct::CharsetTextToID(param2.c_str());
			if ( cx != CHARSET_DEFAULT ) {
				cset = cx;
			}
		}
		if ( yayamsg::LoadMessageFromTxt(load_path,param1,cset) ) {
			messagetxt_path = load_path + param1;
		}
		return true;
	}
	// msglang(for compatibility)
	else if ( cmd == L"msglang" ) {
		if (param == L"english") {
			msglang_for_compat = MSGLANG_ENGLISH;
		}
		else {
			msglang_for_compat = MSGLANG_JAPANESE;
		}
		return true;
	}
	// log
	else if ( cmd == L"log" ) {
		if ( param.empty() ) {
			logpath.erase();
		}
		else {
			logpath = base_path + param;
		}
		return true;
	}
	// basepath
	else if ( cmd == L"basepath" ) {
		CDirEnum dirCheck(param);
		CDirEnumEntry dirCheckTmp;

		if ( dirCheck.next(dirCheckTmp) ) { //something exist in directory
			#if defined(WIN32) || defined(_WIN32_WCE)
			if(param[1]==L':')
			#elif defined(POSIX)
			if(param[0]==L'/')
			#endif
				base_path = param;
			else
				base_path += param;

			//�Ōオ\�ł�/�ł��Ȃ���Α���
			if (base_path.length() == 0 || ( (base_path[base_path.length()-1] != L'/') && (base_path[base_path.length()-1] != L'\\') ) ) {
				#if defined(WIN32) || defined(_WIN32_WCE)
				base_path += L"\\";
				#elif defined(POSIX)
				base_path += L'/';
				#endif
			}
			return true;
		}
		else{
			return false;
		}
	}
	// iolog
	else if ( cmd == L"iolog" ) {
		iolog = param != L"off";
		return true;
	}
	// �Z�[�u�f�[�^�Í���
	else if ( cmd == L"save.encode" ) {
		encode_savefile = param == L"on";
		return true;
	}
	// �����Z�[�u
	else if ( cmd == L"save.auto" ) {
		auto_save = param != L"off";
		return true;
	}
	// charset
	else if ( cmd == L"charset" ) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
		setting_charset   = dic_charset;
		output_charset    = dic_charset;
		file_charset      = dic_charset;
		save_charset      = dic_charset;
		save_old_charset  = dic_charset;
		extension_charset = dic_charset;
		log_charset       = dic_charset;
		return true;
	}
	// charset
	else if ( cmd == L"charset.dic" ) {
		dic_charset       = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.setting" ) {
		setting_charset   = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.output" ) {
		output_charset    = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.file" ) {
		file_charset      = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.save" ) {
		save_charset      = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.save.old" ) {
		save_old_charset  = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.extension" ) {
		extension_charset = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	else if ( cmd == L"charset.log" ) {
		log_charset = Ccct::CharsetTextToID(param.c_str());
		return true;
	}
	// fncdepth
	else if ( cmd == L"fncdepth" ) {
		size_t f_depth = (size_t)yaya::ws_atoll(param, 10);
		vm.call_limit().SetMaxDepth((f_depth < 2 && f_depth != 0) ? 2 : f_depth);
		return true;
	}
	else if ( cmd == L"looplimit" ) {
		size_t loop_max = (size_t)yaya::ws_atoll(param, 10);
		vm.call_limit().SetMaxLoop(loop_max);
		return true;
	}
	else if ( cmd == L"maxlognum" ) {
		size_t maxlognum = (size_t)yaya::ws_atoll(param, 10);
		vm.logger().SetMaxLogNum(maxlognum);
		return true;
	}
	// checkparser closed function
	else if ( cmd == L"checkparser" ) {
		checkparser = param == L"on";
		return true;
	}
	// iolog.filter.keyword
	else if ( cmd == L"iolog.filter.keyword" ){
		vm.logger().AddIologFilterKeyword(param);
		return true;
	}
	// old syntax : ignoreiolog
	else if ( cmd == L"ignoreiolog" ){
		//Remove "ID:" and possible spaces from the param variable
		yaya::string_t::size_type pos = param.find(L"ID");
		if(pos == std::wstring::npos || pos != 0)
			return false;
		pos = param.find_first_not_of(L" \t", pos+2);
		if(pos == std::wstring::npos || param[pos] != L':')
			return false;
		pos = param.find_first_not_of(L" \t", pos+1);
		if(pos == std::wstring::npos)
			return false;
		vm.logger().AddIologFilterKeyword(param.substr(pos));
		return true;
	}
	// iolog.filter.keyword.regex
	else if ( cmd == L"iolog.filter.keyword.regex" ){
		vm.logger().AddIologFilterKeywordRegex(param);
		return true;
	}
	// iolog.filter.keyword.delete (for SETSETTING)
	else if ( cmd == L"iolog.filter.keyword.delete" ){
		vm.logger().DeleteIologFilterKeyword(param);
		return true;
	}
	// iolog.filter.keyword.regex.delete (for SETSETTING)
	else if ( cmd == L"iolog.filter.keyword.regex.delete" ){
		vm.logger().DeleteIologFilterKeywordRegex(param);
		return true;
	}
	// iolog.filter.mode
	else if ( cmd == L"iolog.filter.mode" ){
		vm.logger().SetIologFilterMode(
			(param.find(L"white") != yaya::string_t::npos) || (param.find(L"allow") != yaya::string_t::npos)
			);
		return true;
	}

	return false;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::GetParameter
 *  �@�\�T�v�F  �e��p�����[�^�𕶎���ŕԂ��܂�
 * -----------------------------------------------------------------------
 */
static void CBasis_ConvertStringArray(const std::vector<yaya::string_t> &array,CValue &var)
{
	var.array().clear();

	std::vector<yaya::string_t>::const_iterator itr = array.begin();

	while ( itr != array.end() ) {
		var.array().emplace_back(CValueSub(*itr));
		++itr;
	}
}

CValue CBasis::GetParameter(const yaya::string_t &cmd)
{
	// log
	if ( cmd == L"log" ) {
		return logpath;
	}
	// iolog
	else if ( cmd == L"iolog" ) {
		return yaya::string_t(iolog ? L"on" : L"off");
	}
	// save.encode
	else if ( cmd == L"save.encode" ) {
		return yaya::string_t(encode_savefile ? L"on" : L"off");
	}
	// save.auto
	else if ( cmd == L"save.auto" ) {
		return yaya::string_t(auto_save ? L"on" : L"off");
	}
	// msglang
	else if ( cmd == L"msglang" ) { //obsolete, for compatibility
		return yaya::string_t(msglang_for_compat == MSGLANG_ENGLISH ? L"english" : L"japanese");
	}
	// messagetxt
	else if ( cmd == L"messagetxt" ) {
		return messagetxt_path;
	}
	// charset
	else if ( cmd == L"charset" ) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	// charset
	else if ( cmd == L"charset.dic" ) {
		return Ccct::CharsetIDToTextW(dic_charset);
	}
	else if ( cmd == L"charset.setting" ) {
		return Ccct::CharsetIDToTextW(setting_charset);
	}
	else if ( cmd == L"charset.output" ) {
		return Ccct::CharsetIDToTextW(output_charset);
	}
	else if ( cmd == L"charset.file" ) {
		return Ccct::CharsetIDToTextW(file_charset);
	}
	else if ( cmd == L"charset.save" ) {
		return Ccct::CharsetIDToTextW(save_charset);
	}
	else if ( cmd == L"charset.save.old" ) {
		return Ccct::CharsetIDToTextW(save_old_charset);
	}
	else if ( cmd == L"charset.extension" ) {
		return Ccct::CharsetIDToTextW(extension_charset);
	}
	else if ( cmd == L"charset.log" ) {
		return Ccct::CharsetIDToTextW(log_charset);
	}
	// fncdepth
	else if ( cmd == L"fncdepth" ) {
		return CValue((yaya::int_t)vm.call_limit().GetMaxDepth());
	}
	// looplimit
	else if ( cmd == L"looplimit" ) {
		return CValue((yaya::int_t)vm.call_limit().GetMaxLoop());
	}
	// maxlognum
	else if ( cmd == L"maxlognum" ) {
		return CValue((yaya::int_t)vm.logger().GetMaxLogNum());
	}
	// checkparser closed function
	else if ( cmd == L"checkparser" ) {
		return checkparser ? L"on" : L"off";
	}
	// iolog.filter.keyword (old syntax : ignoreiolog)
	else if ( cmd == L"iolog.filter.keyword" || cmd == L"ignoreiolog" ){
		CValue value(F_TAG_ARRAY, 0/*dmy*/);
		CBasis_ConvertStringArray(vm.logger().GetIologFilterKeyword(),value);
		return value;
	}
	// iolog.filter.keyword.regex
	else if ( cmd == L"iolog.filter.keyword.regex" ){
		CValue value(F_TAG_ARRAY, 0/*dmy*/);
		CBasis_ConvertStringArray(vm.logger().GetIologFilterKeywordRegex(),value);
		return value;
	}
	// iolog.filter.keyword.delete (for SETSETTING only)
	else if(cmd == L"iolog.filter.keyword.delete"){
		return yaya::string_t(); //NOOP
	}
	// iolog.filter.keyword.regex.delete (for SETSETTING only)
	else if(cmd == L"iolog.filter.keyword.regex.delete"){
		return yaya::string_t(); //NOOP
	}
	// iolog.filter.mode
	else if ( cmd == L"iolog.filter.mode" ){
		return vm.logger().GetIologFilterMode() ? L"allowlist" : L"denylist";
	}
	return yaya::string_t();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::SaveVariable
 *  �@�\�T�v�F  �ϐ��l���t�@�C���ɕۑ����܂�
 *
 *  �t�@�C������"DLL��t�@�C����_variable.cfg"�ł��B
 *  �t�@�C���t�H�[�}�b�g��1�s1�ϐ��A�f���~�^���p�J���}�ŁA
 *
 *  �ϐ���,���e,�f���~�^
 *
 *  �̌`���ŕۑ�����܂��B���e�͐���/�����̏ꍇ�͂��̂܂܁A������ł̓_�u���N�H�[�g����܂��B
 *  �z��̏ꍇ�͊e�v�f�Ԃ��R�����ŕ�������܂��B�ȉ��ɗv�f��3�A�f���~�^"@"�ł̗�������܂��B
 *
 *  var,1:"TEST":0.3,@
 *
 *  �f���~�^�̓_�u���N�H�[�g����܂���B
 *
 *  ��b�ݒ�t�@�C���Őݒ肵�������R�[�h�ŕۑ�����܂��B
 * -----------------------------------------------------------------------
 */
void	CBasis::SaveVariable(const yaya::char_t* pName)
{
	// �ϐ��̕ۑ�
	std::string old_locale = yaya::get_safe_str(setlocale(LC_NUMERIC,NULL));
	setlocale(LC_NUMERIC,"English"); //�����_�����

	bool ayc = encode_savefile;

	// �t�@�C�����J��
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = GetSavefilePath();
	}
	else {
		filename = base_path + pName;
	}

	if ( ayc ) {
		char *s_filestr = Ccct::Ucs2ToMbcs(filename,CHARSET_DEFAULT);
#if defined(WIN32)
		DeleteFileA(s_filestr);
#else
	std::remove(s_filestr);
#endif
		free(s_filestr);
		s_filestr=0;

		filename += L".ays"; //ayc���Ƃ��Ԃ�̂Łc
	}
	else {
		filename += L".ays"; //ayc���Ƃ��Ԃ�̂Łc

		char *s_filestr = Ccct::Ucs2ToMbcs(filename,CHARSET_DEFAULT);
#if defined(WIN32)
		DeleteFileA(s_filestr);
#else
	std::remove(s_filestr);
#endif
		free(s_filestr);
		s_filestr=0;

		filename.erase(filename.size()-4,4);
	}

	vm.logger().Message(7);
	vm.logger().Filename(filename);
	FILE	*fp = yaya::w_fopen(filename.c_str(), L"w");
	if (fp == NULL) {
		vm.logger().Error(E_E, 57, filename);
		return;
	}
/*
#if defined(WIN32)
	// UTF-8�̏ꍇ�͐擪��BOM��ۑ�
	if (charset == CHARSET_UTF8)
		write_utf8bom(fp);
#endif
	// UTF-8�Ȃ̂�BOM��t����̂͂�߂������X�������Ɓc
	// �g���u���̌����ɂȂ�̂ŁB

	// �����ł��B�O���Ă��܂��܂��B
	// ����
	// UTF-8�ɂ̓o�C�g�I�[�_�[�ɂ��o���G�[�V���������݂��Ȃ��̂�BOM�͕K�v�Ȃ��B
	// �t�^���邱�Ƃ͏o����B�������Ή����Ă��Ȃ��\�t�g�œǂ߂Ȃ��Ȃ�̂ŕt���Ȃ��ق���
	// �ǂ��B
*/

	// �����R�[�h
	yaya::string_t str;
	yaya::string_t wstr;
	str.reserve(1000);

	str = L"//savefile_charset,";
	str += Ccct::CharsetIDToTextW(save_charset);
	str += L"\n";

	yaya::ws_fputs(str,fp,save_charset,ayc);

	// �����ۑ�
	size_t	var_num = vm.variable().GetNumber();

	for(size_t i = 0; i < var_num; i++) {
		CVariable	*var = vm.variable().GetPtr(i);
		// Void�͕��u
		if (var->value_const().IsVoid())
			continue;
		// ���e���󕶎���̕ϐ��͕ۑ����Ȃ�
		//if (var->value_const().IsStringReal() && !var->value_const().s_value.size())
		//	continue;
		// ���e����ėp�z��̕ϐ��͕ۑ����Ȃ�
		//if (var->value_const().GetType() == F_TAG_ARRAY && !var->value_const().array_size())
		//	continue;
		// �����t���O�������Ă���ϐ��͕ۑ����Ȃ�
		if (var->IsErased())
			continue;

		// ���O�̕ۑ�
		str = var->name;
		str += L',';

		// �l�̕ۑ�
		switch(var->value_const().GetType()) {
		case F_TAG_INT:
			str += yaya::ws_lltoa(var->value_const().i_value);
			str += L',';
			break;
		case F_TAG_DOUBLE:
			str += yaya::ws_ftoa(var->value_const().d_value);
			str += L',';
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
					if(itv != itvbegin)
						str += L':';
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
			str += L',';
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
		// �f���~�^�̕ۑ�
		wstr = var->delimiter;
		EscapeString(wstr);
		str += L"\"";
		str += wstr;
		str += L"\",";
		wstr = var->watcher+L'|'+var->setter+L'|'+var->destorier;
		if(wstr!=L"||")
			str += wstr;

		str += L"\n";

		yaya::ws_fputs(str,fp,save_charset,ayc);
	}

	// �t�@�C�������
	fclose(fp);

	// �����_���C����߂�
	setlocale(LC_NUMERIC,old_locale.c_str());

	vm.logger().Message(8);
}
/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::RestoreVariable
 *  �@�\�T�v�F  �O��ۑ������ϐ����e�𕜌����܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::RestoreVariable(const yaya::char_t* pName)
{
	std::string old_locale = yaya::get_safe_str(setlocale(LC_NUMERIC,NULL));
	setlocale(LC_NUMERIC,"English"); //�����_�����

	bool ayc = encode_savefile;

	// �t�@�C�����J��
	yaya::string_t	filename;
	if ( ! pName || ! *pName ) {
		filename = GetSavefilePath();
	}
	else {
		filename = base_path + pName;
	}

	vm.logger().Message(6);
	vm.logger().Filename(filename);

	FILE *fp = NULL;

	//�Í����Z�[�u�t�@�C���Ή�
	if ( ayc ) {
		filename += L".ays";
		fp = yaya::w_fopen(filename.c_str(), L"r");
		if (!fp) {
			filename.erase(filename.size()-4,4);
			fp = yaya::w_fopen(filename.c_str(), L"r");
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
		fp = yaya::w_fopen(filename.c_str(), L"r");
		if (!fp) {
			filename += L".ays";
			fp = yaya::w_fopen(filename.c_str(), L"r");
			if (!fp) {
				vm.logger().Error(E_N, 0);
				return;
			}
			else {
				ayc = true;
				encode_savefile = true; //�ȒP�ɖ߂���Ă��܂��̂�h�~
			}
		}
	}

	// ���e��ǂݎ��A�����������Ă���
	yaya::string_t	linebuffer;
	linebuffer.reserve(2000);

	yaya::string_t	readline;
	readline.reserve(1000);
	
	yaya::string_t	parseline;
	yaya::string_t	varname, value, delimiter, watcher, setter, destorier;

	char savefile_charset = save_old_charset;
	std::string buf;
	buf.reserve(1000);

	for (int i = 1; ; i++) {
		watcher.erase(), setter.erase(), destorier.erase();
		// 1�s�ǂݍ���
		if (yaya::ws_fgets(buf, readline, fp, savefile_charset, ayc, i, false) == yaya::WS_EOF)
			break;
		// ���s�͏���
		CutCrLf(readline);
		// ��s�Ȃ玟�̍s��
		if (readline.size() == 0) {
			linebuffer.erase();
			continue;
		}
		// ���ɓǂݎ��ς̕�����ƌ���
		linebuffer += readline;
		// �_�u���N�H�[�e�[�V���������Ă��邩�m�F����B���Ă��Ȃ��ꍇ�́A
		// ���̍s�֒l�������Ă���Ǝv����̂Ŏ��̍s�̓ǂݎ���
		if (IsInDQ(linebuffer, 0, linebuffer.size() - 1) != IsInDQ_notindq)
			continue;

		parseline = linebuffer;
		linebuffer.erase();

		// �ϐ������擾
		if (!Split_IgnoreDQ(parseline, varname, value, L",")) {
			vm.logger().Error(E_W, 1, filename, i);
			continue;
		}
		// Charset
		if (varname == L"//savefile_charset") {
			savefile_charset = Ccct::CharsetTextToID(value.c_str());
			continue;
		}
		// �ϐ����̐�����������
		if (IsLegalVariableName(varname)) {
			vm.logger().Error(E_W, 2, filename, i);
			continue;
		}
		// �l�ƃf���~�^�����o��
		parseline = value;
		if (!Split_IgnoreDQ(parseline, value, delimiter, L",")) {
			vm.logger().Error(E_W, 3, filename, i);
			continue;
		}
		//
		parseline = delimiter;
		yaya::string_t unused;
		Split_IgnoreDQ(parseline, delimiter, unused, L",");
		if (!IsLegalStrLiteral(delimiter)){
			delimiter = parseline;
			if (Split_IgnoreDQ(parseline, delimiter, watcher, L",")) {
				parseline = watcher;
				if (Split_IgnoreDQ(parseline, watcher, unused, L","))//�����̃o�[�W�����œ�����\���̂���ǉ����̔j��
					vm.logger().Error(E_W,23);
				parseline = watcher;
				Split_IgnoreDQ(parseline, watcher, setter, L"|");
				parseline = setter;
				Split_IgnoreDQ(parseline, setter, destorier, L"|");
			}
			CutDoubleQuote(delimiter);
			UnescapeString(delimiter);
		}
		else
			delimiter = parseline;
		// �l���`�F�b�N���Č^�𔻒�
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
		// �f���~�^�̐�����������
		if (!delimiter.size()) {
			vm.logger().Error(E_W, 5, filename, i);
			continue;
		}
		// �ϐ����쐬
		int	index = vm.variable().Make(varname, 0);
		vm.variable().SetType(index, type);

		if (type == F_TAG_INT) {
			// �����^
			vm.variable().SetValue(index, yaya::ws_atoll(value));
		}
		else if (type == F_TAG_DOUBLE) {
			// �����^
			vm.variable().SetValue(index, yaya::ws_atof(value));
		}
		else if (type == F_TAG_STRING) {
			// ������^
			CutDoubleQuote(value);
			UnescapeString(value);
			vm.variable().SetValue(index, value);
		}
		else if (type == F_TAG_ARRAY) {
			// �z��^
			RestoreArrayVariable(*(vm.variable().GetValuePtr(index)), value);
		}
		else if (type == F_TAG_HASH) {
			// �A�z�z��^
			RestoreHashVariable(*(vm.variable().GetValuePtr(index)), value);
		}
		else {
			vm.logger().Error(E_W, 6, filename, i);
			continue;
		}
		vm.variable().SetDelimiter(index, delimiter);
		CVariable& v = *vm.variable().GetPtr(index);
		v.set_watcher(watcher);
		v.set_destorier(destorier);
		v.set_setter(setter);
	}

	// �t�@�C�������
	fclose(fp);

	setlocale(LC_NUMERIC,old_locale.c_str());

	vm.logger().Message(8);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::RestoreArrayVariable
 *  �@�\�T�v�F  RestoreVariable����Ă΂�܂��B�z��ϐ��̓��e�𕜌����܂�
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

		if (par != ESC_IARRAY) {
			if (par == ESC_IVOID) {
				var.array().emplace_back(CValueSub());
			}
			else if(IsIntString(par)) {
				var.array().emplace_back(CValueSub(yaya::ws_atoll(par, 10)));
			}
			else if (IsDoubleButNotIntString(par)) {
				var.array().emplace_back(CValueSub( yaya::ws_atof(par) ));
			}
			else {
				CutDoubleQuote(par);
				UnescapeString(par);
				var.array().emplace_back(CValueSub(par));
			}
		}

		if (!splitResult) {
			break;
		}
		value = remain;
	}
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::RestoreHashVariable
 *  �@�\�T�v�F  RestoreVariable����Ă΂�܂��B�z��ϐ��̓��e�𕜌����܂�
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
 *  �֐���  �F  CBasis::ExecuteLoad
 *  �@�\�T�v�F  load�֐������s���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteLoad(void)
{
	if (IsSuppress() || loadindex.IsNotFound()) {
		return;
	}

	ptrdiff_t funcpos = loadindex.Find(vm,L"load");
	if(funcpos < 0) {
		return;
	}

	// �������idll�̃p�X�j���쐬
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	arg.array().emplace_back(CValueSub(base_path));

	// ���s�@���ʂ͎g�p���Ȃ��̂ł��̂܂܎̂Ă�
	vm.call_limit().InitCall();
	CLocalVariable	lvar(vm);
	vm.logger().Io(0, base_path);

	vm.function_exec().func[funcpos].Execute(arg, lvar);
	yaya::string_t empty;
	vm.logger().Io(1, empty);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ExecuteRequest
 *  �@�\�T�v�F  request�֐������s���܂�
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

	ptrdiff_t funcpos = requestindex.Find(vm,L"request");

	if ( funcpos < 0 ) {
		GlobalFree(h);
		h = NULL;
		*len = 0;
		return NULL;
	}

	// ���͕�������擾
	std::string	istr;
	istr.assign((char *)h, 0, (size_t)*len);

	// �������i���͕�����j���쐬�@�����ŕ����R�[�h��UCS-2�֕ϊ�
	CValue	arg(F_TAG_ARRAY, 0/*dmy*/);
	wchar_t	*wistr = Ccct::MbcsToUcs2(istr, output_charset);
	if (wistr != NULL) {
		vm.logger().Io(0, wistr);
		arg.array().emplace_back(CValueSub(wistr));
		free(wistr);
		wistr = NULL;
	}
	else {
		vm.logger().Io(0, L"");
	}

	// ���s
	vm.call_limit().InitCall();
	CValue	result;
	try{
		CLocalVariable	lvar(vm);
		result = vm.function_exec().func[funcpos].Execute(arg, lvar);
	}
	catch (const yaya::memory_error&) {
		if(vm.call_limit().StackCall().size()>512)
			CallOnMemoryLimit();
		else
			CallOnMemoryError();
	}
	catch (const std::bad_alloc&) {
		CallOnMemoryLimit();
	}

	// ���ʂ𕶎���Ƃ��Ď擾���A�����R�[�h��MBCS�ɕϊ�
	yaya::string_t	res = result.GetValueString();
	vm.logger().Io(1, res);
	char	*mostr = Ccct::Ucs2ToMbcs(res, output_charset);
	if (mostr == NULL) {
		// �����R�[�h�ϊ����s�ANULL��Ԃ�
		*len = 0;
		GlobalFree(h);
		h = NULL;
		return NULL;
	}

	// �����R�[�h�ϊ������������̂ŁA���ʂ�GMEM�փR�s�[���ĕԂ�
	unsigned int oldlen = *len;
	*len = (long)strlen(mostr);
	unsigned int copylen = *len + 1;

	//�[���I�[���܂ށc�c�݊������̂���
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
void CBasis::CallOnMemoryLimit()
{
	CBasisFuncPos shiori_OnMemoryLimit;
	ptrdiff_t funcpos = shiori_OnMemoryLimit.Find(vm, L"shiori.OnMemoryLimit");
	size_t lock = vm.call_limit().temp_unlock();

	if (funcpos >= 0) {
		vm.function_exec().func[funcpos].Execute();//get info from GETCALLSTACK
	}
	else {
		vm.logger().Error(E_E,99);
	}

	vm.call_limit().reset_lock(lock);
}
void CBasis::CallOnMemoryError()
{
	CBasisFuncPos shiori_OnMemoryError;
	ptrdiff_t funcpos = shiori_OnMemoryError.Find(vm, L"shiori.OnMemoryError");
	size_t lock = vm.call_limit().temp_unlock();

	if (funcpos >= 0) {
		vm.function_exec().func[funcpos].Execute();//get info from GETCALLSTACK
	}
	else {
		vm.logger().Error(E_E,100);
	}

	vm.call_limit().reset_lock(lock);
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

	// ���͕�������擾
	std::string istr(h, *len);
	// �������i���͕�����j���쐬�@�����ŕ����R�[�h��UCS-2�֕ϊ�
	CValue arg(F_TAG_ARRAY, 0/*dmy*/);
	wchar_t *wistr = Ccct::MbcsToUcs2(istr, output_charset);

	if (wistr != NULL) {
		vm.logger().Io(0, wistr);
		arg.array().emplace_back(CValueSub(wistr));
		free(wistr);
		wistr = NULL;
	}
	else {
		yaya::string_t empty;
		vm.logger().Io(0, empty);
	}

	// ���s
	vm.call_limit().InitCall();
	CValue	result;
	try{
		CLocalVariable	lvar(vm);
		result = vm.function_exec().func[funcpos].Execute(arg, lvar);
	}
	catch (const yaya::memory_error&) {
		if(vm.call_limit().StackCall().size()>512)
			CallOnMemoryLimit();
		else
			CallOnMemoryError();
	}
	catch (const std::bad_alloc&) {
		CallOnMemoryLimit();
	}

	// ���ʂ𕶎���Ƃ��Ď擾���A�����R�[�h��MBCS�ɕϊ�
	yaya::string_t	res = result.GetValueString();
	vm.logger().Io(1, res);
	char *mostr = Ccct::Ucs2ToMbcs(res, output_charset);

	if (mostr == NULL) {
		// �����R�[�h�ϊ����s�ANULL��Ԃ�
		*len = 0;
		return NULL;
	}

	// �����R�[�h�ϊ������������̂ŁA���ʂ�GMEM�փR�s�[���ĕԂ�
	*len = (long)strlen(mostr);
	char* r_h = static_cast<char*>(malloc(*len));
	memcpy(r_h, mostr, *len);
	free(mostr);
	mostr = NULL;
	return r_h;
}
void CBasis::CallOnMemoryLimit()
{
	CBasisFuncPos shiori_OnMemoryLimit;
	ptrdiff_t funcpos = shiori_OnMemoryLimit.Find(vm, L"shiori.OnMemoryLimit");
	size_t lock = vm.call_limit().temp_unlock();

	if (funcpos >= 0) {
		vm.function_exec().func[funcpos].Execute();//get info from GETCALLSTACK
	}
	else {
		vm.logger().Error(E_E,99);
	}

	vm.call_limit().reset_lock(lock);
}
void CBasis::CallOnMemoryError()
{
	CBasisFuncPos shiori_OnMemoryError;
	ptrdiff_t funcpos = shiori_OnMemoryError.Find(vm, L"shiori.OnMemoryError");
	size_t lock = vm.call_limit().temp_unlock();

	if (funcpos >= 0) {
		vm.function_exec().func[funcpos].Execute();//get info from GETCALLSTACK
	}
	else {
		vm.logger().Error(E_E,100);
	}

	vm.call_limit().reset_lock(lock);
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasis::ExecuteUnload
 *  �@�\�T�v�F  unload�֐������s���܂�
 * -----------------------------------------------------------------------
 */
void	CBasis::ExecuteUnload(void)
{
	if ( IsSuppress() || unloadindex.IsNotFound() ) {
		return;
	}

	ptrdiff_t funcpos = unloadindex.Find(vm,L"unload");
	if(funcpos < 0) {
		return;
	}

	// ���s�@���������@���ʂ͎g�p���Ȃ��̂ł��̂܂܎̂Ă�
	vm.call_limit().InitCall();
	yaya::string_t empty;
	vm.logger().Io(0, empty);
	vm.function_exec().func[funcpos].Execute();
	vm.logger().Io(1, empty);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CBasisFuncPos::CBasisFuncPos
 *  �@�\�T�v�F  �֐��ʒu��T���A�ʒu�Ɓu�T�������ǂ����v���L���b�V�����܂�
 * -----------------------------------------------------------------------
 */
ptrdiff_t CBasisFuncPos::Find(CAyaVM &vm,const yaya::char_t *name)
{
	if ( is_try_find ) {
		return pos_saved;
	}

	pos_saved = vm.function_exec().GetFunctionIndexFromName(name);
	is_try_find = true;

	if(pos_saved < 0) {
		vm.logger().Error(E_E, 32, name);
	}

	return pos_saved;
}

