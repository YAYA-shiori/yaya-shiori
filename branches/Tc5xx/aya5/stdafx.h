// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���A
//            �܂��͎Q�Ɖ񐔂������A�����܂�ύX����Ȃ�
//            �v���W�F�N�g��p�̃C���N���[�h �t�@�C�����L�q���܂��B
//

#if !defined(AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_)
#define AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_

#if _MSC_VER > 1000
# pragma once
#endif // _MSC_VER > 1000

// STL�g�p���Ƀf�o�b�O�r���h�Ŕ������閳�ʂȃ��[�j���O��}�~
#if defined(_MSC_VER)
# pragma warning (disable: 4786)
#endif

// Boost��CE�n�Œʂ�Ȃ����Ƃ̑΍�
#if defined(_WIN32_WCE)
# define BOOST_USE_WINDOWS_H
# define BOOST_NO_STD_LOCALE
#endif

// VC8 Secure C �֐��΍�
#if (_MSC_VER >= 1400)
# pragma warning(disable : 4996)
#endif

#define WIN32_LEAN_AND_MEAN		// Windows �w�b�_�[����w�ǎg�p����Ȃ��X�^�b�t�����O���܂�

#include <windows.h>
#include <winreg.h>
#include <Winperf.h>

#include "winbase.h"
#include <winnls.h>
#include <assert.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_STDAFX_H__308D3A4E_B685_4328_B501_D274F42D9159__INCLUDED_)
