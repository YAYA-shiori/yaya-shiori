// 
// AYA version 5
//
// �G�p�֐�
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#if defined(POSIX)
# include <ctime>
#endif
#include <string>
#include <vector>

#include "manifest.h"
#include "misc.h"
#if defined(POSIX) || defined(__MINGW32__)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "wsex.h"
#include "function.h"
#include "sysfunc.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/* -----------------------------------------------------------------------
 *  �֐���  �F  Split
 *  �@�\�T�v�F  ������𕪊����ė]���ȋ󔒂��폜���܂�
 *
 *  �Ԓl�@�@�F  0/1=���s/����
 * -----------------------------------------------------------------------
 */
char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1 = L"";
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += ::wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

//----

char	Split(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1 = L"";
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += sepstr.size();
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  SplitOnly
 *  �@�\�T�v�F  ������𕪊����܂�
 *
 *  �Ԓl�@�@�F  0/1=���s/����
 * -----------------------------------------------------------------------
 */
char	SplitOnly(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = str.find(sepstr);
	if (seppoint == yaya::string_t::npos) {
		dstr0 = str;
		dstr1 = L"";
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += ::wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	return 1;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  Find_IgnoreDQ
 *  �@�\�T�v�F  �_�u��/�V���O���N�H�[�g���𖳎����ĕ����������
 *
 *  �Ԓl�@�@�F  ��=���s   0�E��=����
 * -----------------------------------------------------------------------
 */
yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::char_t *findstr)
{
	yaya::string_t::size_type findpoint = 0;
	yaya::string_t::size_type nextdq = 0;

	while(true){
		findpoint = str.find(findstr, findpoint);
		if (findpoint == yaya::string_t::npos)
			return yaya::string_t::npos;

		nextdq = IsInDQ(str, nextdq, findpoint);
		if (nextdq >= IsInDQ_npos) {
			if (nextdq == IsInDQ_runaway) { //�N�I�[�g���I���Ȃ��܂܏I��
				return yaya::string_t::npos;
			}
			break; //�݂�����
		}
		else { //�N�I�[�g�����B�������Ď���
			findpoint = nextdq;
		}
	}

	return findpoint;
}

yaya::string_t::size_type Find_IgnoreDQ(const yaya::string_t &str, const yaya::string_t &findstr)
{
	return Find_IgnoreDQ(str,findstr.c_str());
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  Split_IgnoreDQ
 *  �@�\�T�v�F  ������𕪊����ė]���ȋ󔒂��폜���܂�
 *  �@�@�@�@�@  �������_�u��/�V���O���N�H�[�g���ł͕������܂���
 *
 *  �Ԓl�@�@�F  0/1=���s/����
 * -----------------------------------------------------------------------
 */

char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::char_t *sepstr)
{
	yaya::string_t::size_type seppoint = Find_IgnoreDQ(str,sepstr);
	if ( seppoint == yaya::string_t::npos ) {
		dstr0 = str;
		dstr1 = L"";
		return 0;
	}

	dstr0.assign(str, 0, seppoint);
	seppoint += wcslen(sepstr);
	dstr1.assign(str, seppoint, str.size() - seppoint);

	CutSpace(dstr0);
	CutSpace(dstr1);

	return 1;
}

//----

char	Split_IgnoreDQ(const yaya::string_t &str, yaya::string_t &dstr0, yaya::string_t &dstr1, const yaya::string_t &sepstr)
{
	return Split_IgnoreDQ(str,dstr0,dstr1,sepstr.c_str());
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  SplitToMultiString
 *  �@�\�T�v�F  ������𕪊�����vector�Ɋi�[���܂�
 *
 *�@�Ԓl�@�@�F�@������(array.size())
 * -----------------------------------------------------------------------
 */
int	SplitToMultiString(const yaya::string_t &str, std::vector<yaya::string_t> *array, const yaya::string_t &delimiter)
{
	if (!str.size())
		return 0;

	const yaya::string_t::size_type dlmlen = delimiter.size();
	yaya::string_t::size_type beforepoint = 0,seppoint;
	size_t count = 1;

	for( ; ; ) {
		// �f���~�^�̔���
		seppoint = str.find(delimiter,beforepoint);
		if (seppoint == yaya::string_t::npos) {
			if ( array ) {
				array->push_back(yaya::string_t(str.begin()+beforepoint,str.end()));
			}
			break;
		}
		// ���o����vector�ւ̒ǉ�
		if ( array ) {
			array->push_back(yaya::string_t(str.begin()+beforepoint,str.begin()+seppoint));
		}
		// ���o���������폜
		beforepoint = seppoint + dlmlen;
		++count;
	}

	return count;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CutSpace
 *  �@�\�T�v�F  �^����ꂽ������̑O��ɔ��p�󔒂��^�u���������ꍇ�A���ׂč폜���܂�
 * -----------------------------------------------------------------------
 */
void	CutSpace(yaya::string_t &str)
{
	CutEndSpace(str);
	CutStartSpace(str);
}

void	CutStartSpace(yaya::string_t &str)
{
	int	len = str.size();
	// �O��
	int	erasenum = 0;
	for(int i = 0; i < len; i++) {
		if (IsSpace(str[i])) {
			erasenum++;
		}
		else {
			break;
		}
	}
	if (erasenum) {
		str.erase(0, erasenum);
	}
}

void	CutEndSpace(yaya::string_t &str)
{
	int	len = str.size();
	// ���
	int erasenum = 0;
	for(int i = len - 1; i >= 0; i--) {
		if (IsSpace(str[i])) {
			erasenum++;
		}
		else {
			break;
		}
	}
	if (erasenum) {
		str.erase(len - erasenum, erasenum);
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CutDoubleQuote
 *  �@�\�T�v�F  �^����ꂽ������̑O��Ƀ_�u���N�H�[�g���������ꍇ�폜���܂�
 * -----------------------------------------------------------------------
 */
void	CutDoubleQuote(yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return;
	// �O��
	if (str[0] == L'\"') {
		str.erase(0, 1);
		len--;
		if (!len)
			return;
	}
	// ���
	if (str[len - 1] == L'\"')
		str.erase(len - 1, 1);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CutSingleQuote
 *  �@�\�T�v�F  �^����ꂽ������̑O��ɃV���O���N�H�[�g���������ꍇ�폜���܂�
 * -----------------------------------------------------------------------
 */
void	CutSingleQuote(yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return;
	// �O��
	if (str[0] == L'\'') {
		str.erase(0, 1);
		len--;
		if (!len)
			return;
	}
	// ���
	if (str[len - 1] == L'\'')
		str.erase(len - 1, 1);
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  AddDoubleQuote
 *  �@�\�T�v�F  �^����ꂽ��������_�u���N�H�[�g�ň݂͂܂�
 * -----------------------------------------------------------------------
 */
void	AddDoubleQuote(yaya::string_t &str)
{
	str = L"\"" + str + L"\"";
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  CutCrLf
 *  �@�\�T�v�F  �^����ꂽ������̌�[�ɉ��s(CRLF)���������ꍇ�폜���܂�
 * -----------------------------------------------------------------------
 */
void	CutCrLf(yaya::string_t &str)
{
	yaya::ws_eraseend(str, L'\n');
	yaya::ws_eraseend(str, L'\r');
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  GetDateString
 *  �@�\�T�v�F  �N����/�����b�̕�������쐬���ĕԂ��܂�
 * -----------------------------------------------------------------------
 */

yaya::string_t GetDateString()
{
    char buf[64];
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(buf, 128, "%Y/%m/%d %H:%M:%S", tm);

#if !defined(POSIX) && !defined(__MINGW32__)
	yaya::char_t wbuf[64];
	for ( size_t i = 0 ; i < 64 ; ++i ) {
		wbuf[i] = buf[i];
		if ( wbuf[i] == 0 ) { break; }
	}
	return wbuf;
#else
    return widen(std::string(buf));
#endif
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsInDQ
 *  �@�\�T�v�F  ��������̎w��ʒu���_�u��/�V���O���N�H�[�g�͈͓������`�F�b�N���܂�
 *
 *  �Ԓl�@�@�F  -1   = �_�u��/�V���O���N�H�[�g�̊O��
 *             !=-1 = �_�u��/�V���O���N�H�[�g�̓���
 *                -2  = �_�u��/�V���O���N�H�[�g�̓����̂܂܃e�L�X�g�I��
 *                >=0 = ���ɊO���ɂȂ�ʒu
 * -----------------------------------------------------------------------
 */
const yaya::string_t::size_type IsInDQ_notindq = static_cast<yaya::string_t::size_type>(-1);
const yaya::string_t::size_type IsInDQ_runaway = static_cast<yaya::string_t::size_type>(-2);
const yaya::string_t::size_type IsInDQ_npos    = static_cast<yaya::string_t::size_type>(-2);

yaya::string_t::size_type IsInDQ(const yaya::string_t &str, yaya::string_t::size_type startpoint, yaya::string_t::size_type checkpoint)
{
	bool dq    = false;
	bool quote = false;

	yaya::string_t::size_type len    = str.size();
	yaya::string_t::size_type found  = startpoint;

	while(true) {
		if (found >= len) {
			found = IsInDQ_runaway;
			break;
		}
		
		found = str.find_first_of(L"'\"",found);
		if (found == yaya::string_t::npos) {
			found = IsInDQ_runaway;
			break;
		}
		else {
			if (found >= checkpoint) {
				if ( (dq && str[found] == L'\"') || (quote && str[found] == L'\'') ) {
					found += 1;
					break;
				}
				if ( ! dq && ! quote ) {
					break;
				}
			}

			if (str[found] == L'\"') {
				if (!quote) {
					dq = !dq;
				}
			}
			else if (str[found] == L'\'') {
				if (!dq ) {
					quote = !quote;
				}
			}

			found += 1;
		}
	}

	if ( dq || quote ) {
		return found;
	}
	else {
		return IsInDQ_notindq;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsDoubleButNotIntString
 *  �@�\�T�v�F  ������Int�������������l�Ƃ��Đ��������������܂�
 *  ���Ӂ@�@�F�@�����l��Double�Ƃ��Đ����Ȓl�Ȃ̂Ŏ�������IsIntString�Ƃ��킹�邱��
 *
 *  �Ԓl�@�@�F  0/1=�~/��
 * -----------------------------------------------------------------------
 */
char	IsDoubleButNotIntString(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	int	dotcount = 0;
	for( ; i < len; i++) {
//		if (!::iswdigit((int)str[i])) {
		if (str[i] < L'0' || str[i] > L'9') {
			if (str[i] == L'.') {
				dotcount++;
			}
			else {
				return 0;
			}
		}
	}

	return dotcount == 1 && (len-advance) > 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsIntString
 *  �@�\�T�v�F  ������10�i�������l�Ƃ��Đ��������������܂�
 *
 *  �Ԓl�@�@�F  0/1=�~/��
 * -----------------------------------------------------------------------
 */
char	IsIntString(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	//32bit
	//2147483647
	if ( (len-i) > 10 ) { return 0; }

	for( ; i < len; i++) {
//		if (!::iswdigit((int)str[i]))
		if (str[i] < L'0' || str[i] > L'9') {
			return 0;
		}
	}

	if ( (len-advance) == 10 ) {
		if ( wcscmp(str.c_str(),L"2147483647") > 0 ) {
			return 0; //Overflow
		}
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsIntBinString
 *  �@�\�T�v�F  ������2�i�������l�Ƃ��Đ��������������܂�
 *  �����@�@�F  header 0/1=�擪"0x"�Ȃ�/����
 *
 *  �Ԓl�@�@�F  0/1=�~/��
 * -----------------------------------------------------------------------
 */
char	IsIntBinString(const yaya::string_t &str, char header)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	if (header) {
		if (::wcsncmp(PREFIX_BIN, str.c_str() + i,PREFIX_BASE_LEN))
			return 0;
		i += PREFIX_BASE_LEN;
	}

	//32bit
	if ( (len-i) > 32 ) { return 0; }
	
	for( ; i < len; i++) {
		yaya::char_t	j = str[i];
		if (j != L'0' && j != L'1')
			return 0;
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsIntHexString
 *  �@�\�T�v�F  ������16�i�������l�Ƃ��Đ��������������܂�
 *
 *  �Ԓl�@�@�F  0/1=�~/��
 * -----------------------------------------------------------------------
 */
char	IsIntHexString(const yaya::string_t &str, char header)
{
	int	len = str.size();
	if (!len)
		return 0;

	int	advance = (str[0] == L'-' || str[0] == L'+') ? 1 : 0;
	int i = advance;

	if (header) {
		if (::wcsncmp(PREFIX_HEX, str.c_str() + i,PREFIX_BASE_LEN))
			return 0;
		i += PREFIX_BASE_LEN;
	}

	//32bit
	//7fffffff
	if ( (len-i) > 8 ) { return 0; }

	for( ; i < len; i++) {
		yaya::char_t	j = str[i];
		if (j >= L'0' && j <= L'9')
			continue;
		else if (j >= L'a' && j <= L'f')
			continue;
		else if (j >= L'A' && j <= L'F')
			continue;

		return 0;
	}

	return (len-advance) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsLegalFunctionName
 *  �@�\�T�v�F  �����񂪊֐����Ƃ��ēK�����𔻒肵�܂�
 *
 *  �Ԓl�@�@�F  0/��0=��/�~
 *
 *  �@�@�@�@�@  1/2/3/4/5/6=�󕶎���/���l�݂̂ō\��/�擪�����l��������"_"/�g���Ȃ��������܂�ł���
 *  �@�@�@�@�@  �@�V�X�e���֐��Ɠ���/���䕶�������͉��Z�q�Ɠ���
 * -----------------------------------------------------------------------
 */
char	IsLegalFunctionName(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 1;

	if (IsIntString(str))
		return 2;

//	if (::iswdigit(str[0]) || str[0] == L'_')
//	if ((str[0] >= L'0' && str[0] <= L'9') || str[0] == L'_') //�`�F�b�N����K�v�͂Ȃ�����
//		return 3;
	if (str[0] == L'_') //�����A���_�[�X�R�A�͏R��Ȃ��ƃ��[�J���ϐ��ƃJ�u��
		return 3;

	for(int i = 0; i < len; i++) {
		yaya::char_t	c = str[i];
		if ((c >= (yaya::char_t)0x0000 && c <= (yaya::char_t)0x0026) ||
			(c >= (yaya::char_t)0x0028 && c <= (yaya::char_t)0x002d) ||
			 c == (yaya::char_t)0x002f ||
			(c >= (yaya::char_t)0x003a && c <= (yaya::char_t)0x0040) ||
			 c == (yaya::char_t)0x005b ||
			(c >= (yaya::char_t)0x005d && c <= (yaya::char_t)0x005e) ||
			 c == (yaya::char_t)0x0060 ||
			(c >= (yaya::char_t)0x007b && c <= (yaya::char_t)0x007f))
			return 4;
	}

	int sysidx = CSystemFunction::FindIndex(str);
	if ( sysidx >= 0 ) { return 5; }

	for(int i= 0; i < FLOWCOM_NUM; i++) {
		if (!str.compare(flowcom[i])) {
			return 6;
		}
	}
	for(int i= 0; i < FORMULATAG_NUM; i++) {
//		if (!str.compare(formulatag[i]))
		if (str.find(formulatag[i]) != yaya::string_t::npos) {
			return 6;
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsLegalVariableName
 *  �@�\�T�v�F  �����񂪕ϐ����Ƃ��ēK�����𔻒肵�܂�
 *
 *  �Ԓl�@�@�F  0/1�`6/16��0=��(�O���[�o���ϐ�)/�~/��(���[�J���ϐ�)
 *
 *  �@�@�@�@�@  1/2/3/4/5/6=�󕶎���/���l�݂̂ō\��/�擪�����l/�g���Ȃ��������܂�ł���
 *  �@�@�@�@�@  �@�V�X�e���֐��Ɠ���/���䕶�������͉��Z�q�Ɠ���
 * -----------------------------------------------------------------------
 */
char	IsLegalVariableName(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 1;

	if (IsIntString(str))
		return 2;

//	if (::iswdigit((int)str[0]))
//	if (str[0] >= L'0' && str[0] <= L'9') //�`�F�b�N����K�v�͂Ȃ�����
//		return 3;

	for(int i = 0; i < len; i++) {
		yaya::char_t	c = str[i];
		if ((c >= (yaya::char_t)0x0000  && c <= (yaya::char_t)0x0026) ||
			(c >= (yaya::char_t)0x0028  && c <= (yaya::char_t)0x002d) ||
			 c == (yaya::char_t)0x002f ||
			(c >= (yaya::char_t)0x003a && c <= (yaya::char_t)0x0040) ||
			 c == (yaya::char_t)0x005b ||
			(c >= (yaya::char_t)0x005d && c <= (yaya::char_t)0x005e) ||
			 c == (yaya::char_t)0x0060 ||
			(c >= (yaya::char_t)0x007b && c <= (yaya::char_t)0x007f))
			return 4;
	}

	int sysidx = CSystemFunction::FindIndex(str);
	if ( sysidx >= 0 ) { return 5; }

	for(int i= 0; i < FLOWCOM_NUM; i++) {
		if (!str.compare(flowcom[i])) {
			return 6;
		}
	}
	for(int i= 0; i < FORMULATAG_NUM; i++) {
//		if (!str.compare(formulatag[i]))
		if (str.find(formulatag[i]) != yaya::string_t::npos) {
			return 6;
		}
	}

	return (str[0] == L'_') ? 16 : 0;
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsLegalStrLiteral
 *  �@�\�T�v�F  �_�u���N�H�[�g�ň͂܂�Ă���ׂ�������̐��������������܂�
 *
 *  �Ԓl�@�@�F  0/1/2/3=����/�_�u���N�H�[�g�����Ă��Ȃ�/
 *  �@�@�@�@�@  �@�_�u���N�H�[�g�ň͂܂�Ă��邪���̒��Ƀ_�u���N�H�[�g����܂���Ă���/
 *  �@�@�@�@�@  �@�_�u���N�H�[�g�ň͂܂�Ă��Ȃ�
 * -----------------------------------------------------------------------
 */
char	IsLegalStrLiteral(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 3;

	// �擪�̃_�u���N�H�[�g�`�F�b�N
	int	flg = (str[0] == L'\"') ? 1 : 0;
	// ��[�̃_�u���N�H�[�g�`�F�b�N
	if (len > 1)
		if (str[len - 1] == L'\"')
			flg += 2;
	// �����Ă���_�u���N�H�[�g�̒T��
	if (len > 2) {
		int	lenm1 = len - 1;
		for(int i = 1; i < lenm1; i++)
			if (str[i] == L'\"') {
				flg = 4;
				break;
			}
	}

	// ���ʂ�Ԃ��܂�
	switch(flg) {
	case 3:
		return 0;
	case 1:
	case 2:
	case 5:
	case 6:
		return 1;
	case 7:
		return 2;
	default:
		return 3;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsLegalPlainStrLiteral
 *  �@�\�T�v�F  �V���O���N�H�[�g�ň͂܂�Ă���ׂ�������̐��������������܂�
 *
 *  �Ԓl�@�@�F  0/1/2/3=����/�_�u���N�H�[�g�����Ă��Ȃ�/
 *  �@�@�@�@�@  �@�_�u���N�H�[�g�ň͂܂�Ă��邪���̒��Ƀ_�u���N�H�[�g����܂���Ă���/
 *  �@�@�@�@�@  �@�_�u���N�H�[�g�ň͂܂�Ă��Ȃ�
 * -----------------------------------------------------------------------
 */
char	IsLegalPlainStrLiteral(const yaya::string_t &str)
{
	int	len = str.size();
	if (!len)
		return 3;

	// �擪�̃V���O���N�H�[�g�`�F�b�N
	int	flg = (str[0] == L'\'') ? 1 : 0;
	// ��[�̃V���O���N�H�[�g�`�F�b�N
	if (len > 1)
		if (str[len - 1] == L'\'')
			flg += 2;
	// �����Ă���V���O���N�H�[�g�̒T��
	if (len > 2) {
		int	lenm1 = len - 1;
		for(int i = 1; i < lenm1; i++)
			if (str[i] == L'\'') {
				flg = 4;
				break;
			}
	}

	// ���ʂ�Ԃ��܂�
	switch(flg) {
	case 3:
		return 0;
	case 1:
	case 2:
	case 5:
	case 6:
		return 1;
	case 7:
		return 2;
	default:
		return 3;
	};
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  IsNt
 *  �@�\�T�v�F  0/1=9x�n/NT�n�@��Ԃ��܂�
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
char	IsNt(void)
{
	OSVERSIONINFO	osi = { sizeof(OSVERSIONINFO) };

	GetVersionEx(&osi);
	return (osi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? 1 : 0;
}
#endif

/* -----------------------------------------------------------------------
 *  �֐���  �F  EscapeString
 *  �@�\�T�v�F  �Z�[�u�t�@�C���ۑ��̍ۂɗL�Q�ȕ�����u�������܂�
 * -----------------------------------------------------------------------
 */

void	EscapeString(yaya::string_t &wstr)
{
	yaya::ws_replace(wstr, L"\"", ESC_DQ);

	for ( size_t i = 0 ; i < wstr.length() ; ++i ) {
		if ( wstr[i] <= END_OF_CTRL_CH ) {
			yaya::string_t replace_text(ESC_CTRL);
			replace_text += (yaya::char_t)(wstr[i] + CTRL_CH_START);

			wstr.replace(i,1,replace_text);
			i += replace_text.length() - 1; //�u����������������Ɉړ�
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  UnescapeString
 *  �@�\�T�v�F  �Z�[�u�t�@�C���ǂݍ��݂̍ۂɗL�Q�ȕ�����߂��܂�
 * -----------------------------------------------------------------------
 */
void	UnescapeString(yaya::string_t &wstr)
{
	yaya::ws_replace(wstr, ESC_DQ, L"\"");

	yaya::string_t::size_type found = 0;
	const size_t len = ::wcslen(ESC_CTRL);
	yaya::char_t ch;
	yaya::char_t str[2] = L"x"; //�u�������p�_�~�[

	while(true) {
		found = wstr.find(ESC_CTRL,found);
		if ( found == yaya::string_t::npos ) {
			break;
		}

		ch = wstr[found + len];
		if ( ch > CTRL_CH_START && ch <= (CTRL_CH_START + END_OF_CTRL_CH) ) {
			str[0] = ch - CTRL_CH_START;
			wstr.replace(found,len + 1,str);
			found += 1;
		}
		else { //�͈͊O�������̂Ŗ���
			found += len;
		}
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  EncodeBase64
 * -----------------------------------------------------------------------
 */

void EncodeBase64(yaya::string_t &out,const char *in,size_t in_len)
{
	int len = in_len;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in);
	static const yaya::char_t table[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	while (len > 0)
	{
		// 1������ 1-6bit  xxxxxx--:--------:--------
		out.append(1,table[static_cast<int>(*p)>>2]);
		
		// 2������ 7-12bit ------xx:xxxx----:--------
		if ( len-1 > 0 )
			out.append(1,table[((static_cast<int>(*p) << 4)&0x30) | ((static_cast<int>(*(p+1)) >> 4)&0x0f)]);
		else
			out.append(1,table[((static_cast<int>(*p) << 4)&0x30) ]);
		
		--len;
		++p;
		
		// 3������ 13-18bit --------:----xxxx:xx------
		if ( len > 0 ) {
			if ( len-1 > 0 ) {
				out.append(1,table[((static_cast<int>(*p) << 2)&0x3C) | ((static_cast<int>(*(p+1)) >> 6)&0x03)]);
			}
			else {
				out.append(1,table[((static_cast<int>(*p) << 2)&0x3C) ]);
			}
			++p;
		}
		else {
			out.append(1,L'=');
		}
		
		// 4������ 19-24bit --------:--------:--xxxxxx
		out.append(1,(--len>0? table[static_cast<int>(*p) & 0x3F]: L'='));
		
		if(--len>0) p++;
	}
}

/* -----------------------------------------------------------------------
 *  �֐���  �F  EncodeURL
 * -----------------------------------------------------------------------
 */

void EncodeURL(yaya::string_t &out,const char *in,size_t in_len)
{
	yaya::char_t chr[4] = L"%00";
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in);

	for ( size_t i = 0 ; i < in_len ; ++i ) {
		int current = static_cast<unsigned char>(p[i]);
		if ( (current >= 'a' && current <= 'z') || (current >= 'A' && current <= 'Z') || (current >= '0' && current <= '9') || current == '.' || current == '_' || current == '-' ) {
			out.append(1,current);
		}
		else if ( current == L' ' ) {
			out.append(1,L'+');
		}
		else {
			yaya::snprintf(chr+1,4,L"%02X",current);
			out.append(chr);
		}
	}
}
