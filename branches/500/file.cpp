// 
// AYA version 5
//
// �t�@�C���������N���X�@CFile
// written by umeici. 2004
// 
// write/read�̓x��list����Ώۂ��������Ă��܂����A��x�Ɏ�舵���t�@�C����
// �����Ă������Ǝv���̂ŁA����ł����s���x�ɖ��͂Ȃ��ƍl���Ă��܂��B
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <list>
#include <algorithm>

#include "file.h"
#include "misc.h"
#include "globaldef.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::ProcessOpenMode
 *  �@�\�T�v�F  AYA�`����FOPEN�p�����[�^�w�肩��Afopen�����߂ł���`����
 *�@�@�@�@�@�@�@�ϊ����A�����ɏ������`�F�b�N���܂��B
 *
 *  �Ԓl�@�@�F�@true/false=����/�s��
 * -----------------------------------------------------------------------
 */
bool CFile::ProcessOpenMode(yaya::string_t &t_mode)
{
	if (!t_mode.compare(L"read"))
		t_mode = L"r";
	else if (!t_mode.compare(L"write"))
		t_mode = L"w";
	else if (!t_mode.compare(L"append"))
		t_mode = L"a";
	else if (!t_mode.compare(L"read_binary"))
		t_mode = L"rb";
	else if (!t_mode.compare(L"write_binary"))
		t_mode = L"wb";
	else if (!t_mode.compare(L"append_binary"))
		t_mode = L"ab";
	if (!t_mode.compare(L"read_random"))
		t_mode = L"r+";
	else if (!t_mode.compare(L"write_random"))
		t_mode = L"w+";
	else if (!t_mode.compare(L"append_random"))
		t_mode = L"a+";
	else if (!t_mode.compare(L"read_binary_random"))
		t_mode = L"rb+";
	else if (!t_mode.compare(L"write_binary_random"))
		t_mode = L"wb+";
	else if (!t_mode.compare(L"append_binary_random"))
		t_mode = L"ab+";

	if (
		t_mode.compare(L"r") &&
		t_mode.compare(L"w") &&
		t_mode.compare(L"a") &&
		t_mode.compare(L"rb") &&
		t_mode.compare(L"wb") &&
		t_mode.compare(L"ab") &&
		t_mode.compare(L"r+") &&
		t_mode.compare(L"w+") &&
		t_mode.compare(L"a+") &&
		t_mode.compare(L"rb+") &&
		t_mode.compare(L"wb+") &&
		t_mode.compare(L"ab+")
		) {
		return false;
	}
	else {
		return true;
	}
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::Add
 *  �@�\�T�v�F  �w�肳�ꂽ�t�@�C�����I�[�v�����܂�
 *
 *  �Ԓl�@�@�F�@0/1/2=���s/����/���ɃI�[�v�����Ă���
 * -----------------------------------------------------------------------
 */
int	CFile::Add(const yaya::string_t &name, const yaya::string_t &mode)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return 2;
	}

	yaya::string_t	t_mode = mode;
	if ( ! ProcessOpenMode(t_mode) ) {
		return 0;
	}

	filelist.push_back(CFile1(name, charset, t_mode));
	it = filelist.end();
	it--;
	if (!it->Open()) {
		filelist.erase(it);
		return 0;
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::Delete
 *  �@�\�T�v�F  �w�肳�ꂽ�t�@�C�����N���[�Y���܂�
 *
 *  �Ԓl�@�@�F�@1/2=����/�I�[�v������Ă��Ȃ��A�������͊���fclose����Ă���
 * -----------------------------------------------------------------------
 */
int	CFile::Delete(const yaya::string_t &name)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		int	result = it->Close();
		it = filelist.erase(it);
		return result;
	}

	return 2;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::DeleteAll
 *  �@�\�T�v�F  ���ׂẴt�@�C�����N���[�Y���܂�
 * -----------------------------------------------------------------------
 */
void	CFile::DeleteAll(void)
{
	filelist.clear();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::Write
 *  �@�\�T�v�F  �t�@�C���ɕ�������������݂܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
int	CFile::Write(const yaya::string_t &name, const yaya::string_t &istr)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->Write(istr);
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::WriteBin
 *  �@�\�T�v�F  �t�@�C���Ƀo�C�i���f�[�^���������݂܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
int	CFile::WriteBin(const yaya::string_t &name, const yaya::string_t &istr, const yaya::char_t alt)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->WriteBin(istr,alt);
	}

	CFile1 tempfile(name, charset, L"wb");
	if ( ! tempfile.Open() ) {
		return 0;
	}
	int result = tempfile.WriteBin(istr,alt);
	tempfile.Close();
	return result;

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::Read
 *  �@�\�T�v�F  �t�@�C�����當�����1�s�ǂݎ��܂�
 *
 *  �Ԓl�@�@�F�@-1/0/1=EOF/���s/����
 * -----------------------------------------------------------------------
 */
int	CFile::Read(const yaya::string_t &name, yaya::string_t &ostr)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->Read(ostr);
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::ReadBin
 *  �@�\�T�v�F  �t�@�C������o�C�i���f�[�^��ǂݎ��܂�
 *
 *  �Ԓl�@�@�F�@-1/0/1=EOF/���s/����
 * -----------------------------------------------------------------------
 */
int	CFile::ReadBin(const yaya::string_t &name, yaya::string_t &ostr, size_t len, yaya::char_t alt)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->ReadBin(ostr,len,alt);
	}

	CFile1 tempfile(name, charset, L"rb");
	if ( ! tempfile.Open() ) {
		return 0;
	}
	int result = tempfile.ReadBin(ostr,len,alt);
	tempfile.Close();
	return result;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::ReadEncode
 *  �@�\�T�v�F  �t�@�C������o�C�i���f�[�^���G���R�[�h���ēǂݎ��܂�
 *
 *  �Ԓl�@�@�F�@-1/0/1=EOF/���s/����
 * -----------------------------------------------------------------------
 */
int	CFile::ReadEncode(const yaya::string_t &name, yaya::string_t &ostr, size_t len, const yaya::string_t &type)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->ReadEncode(ostr,len,type);
	}

	CFile1 tempfile(name, charset, L"rb");
	if ( ! tempfile.Open() ) {
		return 0;
	}
	int result = tempfile.ReadEncode(ostr,len,type);
	tempfile.Close();
	return result;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::Size
 *  �@�\�T�v�F  �t�@�C���T�C�Y�����
 *  �Ԓl�@�@�F�@<0���s >=0����
 * -----------------------------------------------------------------------
 */
long CFile::Size(const yaya::string_t &name)
{
	std::list<CFile1>::const_iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->Size();
	}

	return -1;
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::FSeek
 *  �@�\�T�v�F  C���C�u����fseek����
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
int CFile::FSeek(const yaya::string_t &name,int offset,const yaya::string_t &s_mode)
{
	int mode;

	if (s_mode.compare(L"SEEK_CUR")==0 || s_mode.compare(L"current")==0){
		mode=SEEK_CUR;
	}
	else if (s_mode.compare(L"SEEK_END")==0 || s_mode.compare(L"end")==0){
		mode=SEEK_END;
	}
	else if (s_mode.compare(L"SEEK_SET")==0 || s_mode.compare(L"start")==0){
		mode=SEEK_SET;
	}
	else{
		return 0;
	}

	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->FSeek(offset,mode);
	}

	return 0;
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CFile::FTell
 *  �@�\�T�v�F  C���C�u����ftell����
 *  �Ԓl�@�@�F�@-1/���̑�=���s/�����iftell�̌��ʁj
 * -----------------------------------------------------------------------
 */
int CFile::FTell(const yaya::string_t &name)
{
	std::list<CFile1>::iterator it = std::find(filelist.begin(),filelist.end(),name);
	if ( it != filelist.end() ) {
		return it->FTell();
	}

	return 0;
}


