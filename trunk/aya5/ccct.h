// 
// AYA version 5
//
// �����R�[�h�ϊ��N���X�@Ccct
//
// �ϊ������̃R�[�h�͈ȉ��̃T�C�g�Ō��J����Ă�����̂𗘗p���Ă���܂��B
// class CUnicodeF
// kamoland
// http://kamoland.com/comp/unicode.html
// 
// ���p�������͂Ȃ������������̂ł��̂܂܎g�킹�Ă��������܂����B
//
// �ׂ����_�͂킽���̂ق��ŘM��܂������A�̐S�̓ǂݑւ������̓I���W�i���̂܂܂ł��B
//
// �ύX�_
// �E�I���W�i���̓��P�[��Japanese�̂ݑΉ��������̂ŁAOS�f�t�H���g����𒲂ׂ�setlocale����@�\��ǉ��B
// �EMBC to UCS2�A�y�т��̋t���s���֐��������ւ��܂����B
// �E�������̂��߂̏C�����኱�B
// �@1.�I���W�i���ł͕ϊ��O�ɕK�v�ȗ̈�̃T�C�Y���������蒲�ׂĂ��܂������A���̏����͏Ȃ��A�ǂ̂悤��
// �@  �ϊ�����Ă����܂邾���̗̈��p�ӂ���悤�ɂ��܂����B
// �@2.�I���W�i����calloc�ł������A�ɗ�malloc�֒u�������܂����B�኱�����i�͂��j�B
//

#ifndef CCCTH
#define CCCTH

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <string>

#include "globaldef.h"

//----

class Ccct {
public:
	static char		*Ucs2ToMbcs(const wchar_t *wstr, int charset);
	static char		*Ucs2ToMbcs(const yaya::string_t &wstr, int charset);
	static wchar_t	*MbcsToUcs2(const char *mstr, int charset);
	static wchar_t	*MbcsToUcs2(const std::string &mstr, int charset);

	static bool     CheckInvalidCharset(int charset);
	static int      CharsetTextToID(const wchar_t *ctxt);
	static const wchar_t *CharsetIDToTextW(const int charset);
	static const char    *CharsetIDToTextA(const int charset);

#if defined(WIN32) || defined(_WIN32_WCE)
    static wchar_t*	mbcs_to_utf16be(const char *pAnsiStr, int charset);
    static char*	utf16be_to_utf8(const wchar_t *pUcsStr);
    static char*	utf16be_to_mbcs(const wchar_t *pUcsStr, int charset);
    static wchar_t*	utf8_to_utf16be(const char *pUtf8Str);

	static char		*sys_setlocale(int category);

protected:
    static size_t	utf16be_to_utf8_sub( char *pUtf8, const wchar_t *pUcs2, size_t nUcsNum);
    static size_t	utf8_to_utf16be_sub( wchar_t *pUcs2, const char *pUtf8, size_t nUtf8Num);
#endif
};

//----

#endif
