// 
// AYA version 5
//
// �O�����C�u�����������N���X�@CLib
// written by umeici. 2004
// 
// request�̓x��list����Ώۂ��������Ă��܂����A�ÓI��load���Ă���DLL��
// �����Ă������Ǝv���̂ŁA����ł����s���x�ɖ��͂Ȃ��ƍl���Ă��܂��B
//

#if defined(WIN32)
# include "stdafx.h"
#endif

#include <list>

#include "lib.h"

#include "ayavm.h"

#include "log.h"
#include "wsex.h"
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
 *  �֐���  �F  CLib::Add
 *  �@�\�T�v�F  �w�肳�ꂽ�t�@�C������DLL�����[�h���Aload�����s���܂�
 *
 *  �Ԓl�@�@�F�@0/1/2=���s/����/���Ƀ��[�h����Ă���
 * -----------------------------------------------------------------------
 */
int	CLib::Add(const yaya::string_t &name)
{
	std::list<CLib1>::iterator it;
	for(it = liblist.begin(); it != liblist.end(); it++) {
		if (!name.compare(it->GetName())) {
			return 2;
		}
	}

	liblist.push_back(CLib1(vm, name, charset));
	it = liblist.end();
	it--;
	if (!it->LoadLib()) {
		liblist.erase(it);
		return 0;
	}

	if (!it->Load()) {
		liblist.erase(it);
		return 0;
	}

	//�ꎞ�����R�[�h�ݒ�ۑ����ɂ��߂��񂾂��̂�ݒ�
	charset_map::iterator itc = charset_temp_map.find(name);
	if ( itc != charset_temp_map.end() ) {
		it->SetCharset(itc->second);
		charset_temp_map.erase(itc);
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib::Delete
 *  �@�\�T�v�F  �w�肳�ꂽ�t�@�C������DLL�ɂ�unload�����s���A�����[�X���܂�
 *
 *  �Ԓl�@�@�F�@0/1/2=���s/����/���[�h����Ă��Ȃ��A�������͊���unload����Ă���
 * -----------------------------------------------------------------------
 */
int	CLib::Delete(const yaya::string_t &name)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (!name.compare(it->GetName())) {
			int	result = it->Unload();
			it->Release();
			it = liblist.erase(it);
			return result;
		}
	}

	return 2;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib::DeleteAll
 *  �@�\�T�v�F  ���ׂĂ�DLL�ɂ�unload()�����s���A�����[�X���܂�
 * -----------------------------------------------------------------------
 */
void	CLib::DeleteAll(void)
{
	liblist.clear();
	charset_temp_map.clear();
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib::SetCharsetDynamic
 *  �@�\�T�v�F  SAORI�̕����R�[�h��ύX���܂�
 *
 *  �Ԓl�@�@�F�@1=��ɐ���
 * -----------------------------------------------------------------------
 */
int	CLib::SetCharsetDynamic(const yaya::string_t &name,int cs)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (!name.compare(it->GetName())) {
			it->SetCharset(cs);
			return 1;
		}
	}

	//�t�@�C���̑��݂�����
	FILE *fp = yaya::w_fopen(name.c_str(),L"rb");
	if ( ! fp ) {
		return 0;
	}
	fclose(fp);

	//�܂����[�h����Ă��Ȃ��ꍇ�͈ꎞmap�ɂ��߂���
	charset_temp_map.insert( charset_map::value_type(name,cs) );

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib::GetCharsetDynamic
 *  �@�\�T�v�F  SAORI�̕����R�[�h���擾���܂�
 *
 *  �Ԓl�@�@�F�@�����R�[�h/���l ���s��-1
 * -----------------------------------------------------------------------
 */
int	CLib::GetCharsetDynamic(const yaya::string_t &name)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (!name.compare(it->GetName())) {
			return it->GetCharset();
		}
	}

	charset_map::const_iterator itr = charset_temp_map.find(name);
	if ( itr != charset_temp_map.end() ) {
		return itr->second;
	}
	return -1;
}


/* -----------------------------------------------------------------------
 *  �֐���  �F  CLib::Request
 *  �@�\�T�v�F  request�����s���A���ʂ��擾���܂�
 *
 *  �Ԓl�@�@�F�@0/1=���s/����
 * -----------------------------------------------------------------------
 */
int	CLib::Request(const yaya::string_t &name, const yaya::string_t &istr, yaya::string_t &ostr)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (!name.compare(it->GetName())) {
			vm.logger().IoLib(0, istr, name);
			int	result = it->Request(istr, ostr);
			vm.logger().IoLib(1, ostr, name);
			return result;
		}
	}

	ostr = L"";
	return 0;
}
