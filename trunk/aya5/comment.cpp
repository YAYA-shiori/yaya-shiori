// 
// AYA version 5
//
// �R�����g�A�E�g�����N���X�@CComment
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "comment.h"
#include "misc.h"
#include "globaldef.h"

/* -----------------------------------------------------------------------
 *  �֐���  �F  CComment::Process
 *  �@�\�T�v�F  �R�����g�A�E�g�����i/���`��/�j
 * -----------------------------------------------------------------------
 */
void	CComment::Process(yaya::string_t &str)
{
	if (str.empty()) { return; }

	yaya::string_t	outstr;
	while(true) {
		yaya::string_t::size_type found0 = Find_IgnoreDQ(str,L"/*");
		yaya::string_t::size_type found1 = Find_IgnoreDQ(str,L"*/");

		if (found0 == yaya::string_t::npos && found1 == yaya::string_t::npos) {
			if (!flag)
				outstr.append(str);
			break;
		}
		if (found0 == yaya::string_t::npos || (found0 != yaya::string_t::npos && found1 != yaya::string_t::npos && found0 >= found1)) {
			if (!flag)
				outstr.append(str,0,found1);
			flag = 0;
			str.erase(0,found1+2); //+2�� */�̂Ԃ�
		}
		else {
			if (!flag)
				outstr.append(str,0,found0);
			flag = 1;
			str.erase(0,found0+2); //+2�� /*�̂Ԃ�
		}
	}
	str = outstr;
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
