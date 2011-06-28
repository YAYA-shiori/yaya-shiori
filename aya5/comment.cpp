// 
// AYA version 5
//
// �R�����g�A�E�g�����N���X�@CComment
// written by umeici. 2004
// 
#ifdef _MSC_VER
#pragma warning( disable : 4786 ) //�u�f�o�b�O�����ł̎��ʎq�؎̂āv
#pragma warning( disable : 4503 ) //�u�������ꂽ���O�̒��������E���z���܂����B���O�͐؂�̂Ă��܂��B�v
#endif

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "comment.h"
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
 *  �֐���  �F  CComment::Process
 *  �@�\�T�v�F  �R�����g�A�E�g�����i/���`��/�j
 * -----------------------------------------------------------------------
 */
void	CComment::Process(yaya::string_t &str)
{
	if (str.empty()) { return; }

	yaya::string_t::size_type found0;
	yaya::string_t::size_type found1;

	while ( true ) {
		//�O�s����R�����g�������Ă�
		if ( flag ) {
			found1 = str.find(L"*/"); //�R�����g�I���ʒu��T�� ����̓N�H�[�g�ɉe������Ȃ�

			if ( found1 != yaya::string_t::npos ) {
				str.erase(0,found1+2); //+2�� */ �̕�
				flag = 0;
			}
			else {
				str.erase(); //�S���Ȃ��������Ƃ�
				break;
			}
		}
		//�����ĂȂ�
		else {
			found0 = Find_IgnoreDQ(str,L"/*");//�X�^�[�g�̂݃N�H�[�g���l��

			if ( found0 != yaya::string_t::npos ) { //�X�^�[�g�ʒu�������s���Ō�������

				found1 = str.find(L"*/",found0+2);

				if ( found1 != yaya::string_t::npos ) { //�X�g�b�v�ʒu�������s���Ō�������
					str.erase(found0,(found1+2-found0)); //+2�� */ �̕�
				}
				else {
					str.erase(found0,str.size()-found0); //�c��S���΂�����
					flag = 1;
					break; //�Ƃ肠�������̍s�͏����I��
				}
			}
			else { //�����R�����g�͂Ȃ�
				break;
			}
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CComment::Process_Top
 *  �@�\�T�v�F  �R�����g�A�E�g�����i�擪��//�j
 * -----------------------------------------------------------------------
 */
void	CComment::Process_Top(yaya::string_t &str)
{
	if (str.empty()) { return; }
	
	const wchar_t *pt = str.c_str();
	
	while ( *pt == L' ' || *pt == L'\t' ) { ++pt; }
	
	//�R�����g
	if ( *pt == L'/' && *(pt+1) == L'/' ) { str.erase(); }
	
	//���łɋ󔒂����̍s���Ȃ��������Ƃ�
	if ( *pt == 0 ) { str.erase(); }
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CComment::Process_Tail
 *  �@�\�T�v�F  �R�����g�A�E�g�����i���r��//�j
 * -----------------------------------------------------------------------
 */
void	CComment::Process_Tail(yaya::string_t &str)
{
	if (str.empty()) { return; }

	yaya::string_t::size_type found = Find_IgnoreDQ(str, L"//");
	if ( found != yaya::string_t::npos ) {
		str.erase(found, (int)str.size() - found);
		CutSpace(str);
	}
}
