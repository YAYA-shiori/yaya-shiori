// 
// AYA version 5
//
// stl::yaya::string_t��char*���Ɏg�����߂̊֐��Ȃ�
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#if defined(POSIX)
# include <iomanip>
# include <sstream>
#endif
#include <string>
#include <stdarg.h>
#include <boost/lexical_cast.hpp>

#include "ccct.h"
#if defined(POSIX)
# include "posix_utils.h"
#endif
#include "globaldef.h"
#include "manifest.h"
#include "misc.h"
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
*  �֐���  �F  ws_atoi
*  �@�\�T�v�F  yaya::string_t��int�֕ϊ�
* -----------------------------------------------------------------------
*/
int	ws_atoi(const yaya::string_t &str, int base)
{
	if (!str.size())
		return 0;
	
	wchar_t	*dmy;
	return (int)wcstol(str.c_str(), &dmy, base);
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_atof
*  �@�\�T�v�F  yaya::string_t��double�֕ϊ�
* -----------------------------------------------------------------------
*/
double	ws_atof(const yaya::string_t &str)
{
	if (!str.size())
		return 0.0;
	
	wchar_t	*dmy;
	return wcstod(str.c_str(), &dmy);
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_itoa
*  �@�\�T�v�F  int��yaya::string_t�֕ϊ�
* -----------------------------------------------------------------------
*/

static void int2bin(yaya::char_t *str, int num, int &off)
{
    int k;

    if ((k = num >> 1) != 0) int2bin(str, k, off);
    *(str + off) = (num & 1) + '0';
    off++;
}

yaya::string_t ws_itoa(int num, int base)
{
	yaya::char_t numtxt[128];
	if ( base == 16 ) {
		yaya::snprintf(numtxt,64,L"%x",num);
	}
	else if ( base == 8 ) {
		yaya::snprintf(numtxt,64,L"%i",num);
	}
	else if ( base == 2 ) {
		int i = 0;
		if (num < 0) {
			num = -num;
			numtxt[i] = '-';
			++i;
		}
		int2bin(numtxt+i, num, i);
		numtxt[i] = '\0';
	}
	else {
		yaya::snprintf(numtxt,64,L"%d",num);
	}
	return numtxt;
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_ftoa
*  �@�\�T�v�F  double��yaya::string_t�֕ϊ�
* -----------------------------------------------------------------------
*/
yaya::string_t	ws_ftoa(double num)
{
	yaya::char_t numtxt[128];
	yaya::snprintf(numtxt,64,L"%f",num);
	return numtxt;
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_eraseend
*  �@�\�T�v�F  yaya::string_t�̏I�[����c�����
* -----------------------------------------------------------------------
*/
void	ws_eraseend(yaya::string_t &str,wchar_t c)
{
	if (!str.size())
		return;
	
	if (str[str.size() - 1] == c)
		str.erase(str.end() - 1);
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_replace
*  �@�\�T�v�F  str����before�����ׂ�after�ɒu�����܂�
* -----------------------------------------------------------------------
*/
void	ws_replace(yaya::string_t &str, const wchar_t *before, const wchar_t *after)
{
	size_t sz_bef = wcslen(before);
	size_t sz_aft = wcslen(after);
	for(size_t rp_pos = 0; ; rp_pos += sz_aft) {
		rp_pos = str.find(before, rp_pos);
		if (rp_pos == yaya::string_t::npos)
			break;
		str.replace(rp_pos, sz_bef, after);
	}
}

/* -----------------------------------------------------------------------
*  �֐���  �F  w_fopen
*  �@�\�T�v�F  UCS-2������̃t�@�C�����ŃI�[�v���ł���fopen
*
*  �⑫�@wchar_t*�𒼐ړn����_wfopen��Win9x�n���T�|�[�g�̂��ߎg���Ȃ��̂ł��B���O�B
* -----------------------------------------------------------------------
*/
#if defined(WIN32) || defined(_WIN32_WCE)
FILE	*w_fopen(const wchar_t *fname, const wchar_t *mode)
{
	// �t�@�C�����ƃI�[�v�����[�h����MBCS�֕ϊ�
	char	*mfname = Ccct::Ucs2ToMbcs(fname, CHARSET_DEFAULT);
	if (mfname == NULL)
		return NULL;
	char	*mmode  = Ccct::Ucs2ToMbcs(mode,  CHARSET_DEFAULT);
	if (mmode == NULL) {
		free(mfname);
		return NULL;
	}
	// �I�[�v��
	FILE	*fp = fopen(mfname, mmode);
	free(mfname);
	free(mmode);
	
	return fp;
}
#else
FILE* w_fopen(const wchar_t* fname, const wchar_t* mode) {
	std::string s_fname = narrow(yaya::string_t(fname));
	std::string s_mode = narrow(yaya::string_t(mode));
	
    fix_filepath(s_fname);
	
    return fopen(s_fname.c_str(), s_mode.c_str());
}
#endif

/* -----------------------------------------------------------------------
*  �֐���  �F  write_utf8bom
*  �@�\�T�v�F  UTF-8 BOM����������
* -----------------------------------------------------------------------
*/
/*
void	write_utf8bom(FILE *fp)
{
fputc(0xef, fp);
fputc(0xbb, fp);
fputc(0xbf, fp);
}
*/

/* -----------------------------------------------------------------------
*  �֐���  �F  decode/encodecipher
*  �@�\�T�v�F  AYA�Í������ꂽ�����𕜍�����
*
*  �����̃r�b�g���]�Ƃ��������ł�
* -----------------------------------------------------------------------
*/
static int decodecipher(const int c)
{
	return (((c & 0x7) << 5) | ((c & 0xf8) >> 3)) ^ 0x5a;
}

static int encodecipher(const int c)
{
	return (((c^ 0x5a) << 3) & 0xF8) | (((c^ 0x5a) >> 5) & 0x7);
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_fgets
*  �@�\�T�v�F  yaya::string_t�Ɏ��o����ȈՔ�fgets�A�Í�������UCS-2 BOM�폜���s�Ȃ�
* -----------------------------------------------------------------------
*/
int ws_fgets(yaya::string_t &str, FILE *stream, int charset, int ayc, int lc, int cutspace)
{
	//ayc = 1 ������
	//lc = 1 BOM�폜
	//cutspace = 1 �擪�̋󔒍폜

	str.erase();
	std::string buf;
	buf.reserve(1000);
	int c = 1;
	
	if (ayc) {
		while (true) {
			c = fgetc(stream);
			if (c == EOF) {
				break;
			}
			c = decodecipher(c);
			buf += static_cast<char>(c);
			if (c == '\x0a') {
				// �s�̏I���
				break;
			}
		}
	}
	else {
		while (true) {
			c = fgetc(stream);
			if (c == EOF) {
				break;
			}
			buf += static_cast<char>(c);
			if (c == '\x0a') {
				// �s�̏I���
				break;
			}
		}
	}
	
	wchar_t *wstr_result = Ccct::MbcsToUcs2(buf, charset);
	if ( ! wstr_result ) { return 0; }

	wchar_t *wstr = wstr_result;
	if (charset == CHARSET_UTF8 && lc == 1) {
		if (wstr[0] == static_cast<yaya::char_t>(0xfeff) || wstr[0] == static_cast<yaya::char_t>(0xffef)) {
			wstr += 1;
		}
	}
	if (cutspace) {
		while (IsSpace(*wstr)) { ++wstr; }
	}
	str = wstr;

	free(wstr_result);
	
	if (c == EOF && str.empty()) {
		return WS_EOF;
	}
	else {
		return str.size();
	}
}

/* -----------------------------------------------------------------------
*  �֐���  �F  ws_fputs
*  �@�\�T�v�F  yaya::string_t���������ފȈՔ�fputs�A�Í������s�Ȃ�
* -----------------------------------------------------------------------
*/
int ws_fputs(const yaya::char_t *str, FILE *stream, int charset, int ayc)
{
	//ayc = 1 ������
	char *str_result = Ccct::Ucs2ToMbcs(str, charset);
	if ( ! str_result ) { return 0; }

	int len = strlen(str_result);

	if (ayc) {
		char *resulttmp = str_result;
		while ( *resulttmp ) {
			*resulttmp = encodecipher(*resulttmp);
			++resulttmp;
		}
	}

	fwrite(str_result,1,len,stream);

	free(str_result);
	
	return len;
}

/* -----------------------------------------------------------------------
*  �֐���  �F  snprintf
*  �@�\�T�v�F  snprintf�݊�����
* -----------------------------------------------------------------------
*/
int yaya::snprintf(yaya::char_t *buf,size_t count,const yaya::char_t *format,...)
{
	va_list list;
	va_start( list, format );

	int result;

#ifdef _MSC_VER
#if _MSC_VER <= 1300
	//�W����݊�
	result = _vsnwprintf(buf,count,format,list);
#else
	result = vswprintf(buf,count*2,format,list);
#endif
#else
	result = vswprintf(buf,count*2,format,list);
#endif

	va_end (list);
	return result;
}