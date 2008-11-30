// 
// AYA version 5
//
// 1つのファイルを扱うクラス　CFile1
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
#  include "stdafx.h"
#endif

#include "ccct.h"
#include "file.h"
#include "manifest.h"
#include "globaldef.h"
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
 *  関数名  ：  CFile1::Open
 *  機能概要：  ファイルをオープンします
 *
 *  返値　　：　0/1=失敗/成功(既にロードされている含む)
 * -----------------------------------------------------------------------
 */
int	CFile1::Open(void)
{
	if (fp != NULL)
		return 1;

	char	*filepath = Ccct::Ucs2ToMbcs(name, CHARSET_DEFAULT);
	if (filepath == NULL)
		return 0;

	fp = yaya::w_fopen((wchar_t *)name.c_str(), (wchar_t *)mode.c_str());
	free(filepath);

	long cur = ftell(fp);
	fseek(fp,SEEK_SET,0);
	long start = ftell(fp);
	fseek(fp,SEEK_END,0);
	long end = ftell(fp);
	fseek(fp,SEEK_SET,cur);

	size = end-start;
	
	return (fp != NULL) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::Close
 *  機能概要：  ファイルをクローズします
 *
 *  返値　　：　1/2=成功/ロードされていない、もしくは既にunloadされている
 * -----------------------------------------------------------------------
 */
int	CFile1::Close(void)
{
	if (fp) {
		fclose(fp);
		fp = NULL;
		return 1;
	}
	else {
		return 2;
	}
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::Write
 *  機能概要：  ファイルに文字列を書き込みます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile1::Write(const yaya::string_t &istr)
{
	if (fp == NULL)
		return 0;

	// 文字列をマルチバイト文字コードに変換
	char	*t_istr = Ccct::Ucs2ToMbcs(istr, charset);
	if (t_istr == NULL)
		return 0;

	long	len = (long)strlen(t_istr);

	// 書き込み
	fwrite(t_istr, sizeof(char), len, fp);
	free(t_istr);

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::WriteBin
 *  機能概要：  ファイルにバイナリデータを書き込みます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile1::WriteBin(const yaya::string_t &istr, const yaya::char_t alt)
{
	if (fp == NULL)
		return 0;

	size_t len = istr.size();

	unsigned char *t_istr = reinterpret_cast<unsigned char*>(malloc(len+1));
	t_istr[len] = 0; //念のためゼロ終端（いらない）
	
	//altを0に置き換えつつデータ構築
	for ( size_t i = 0 ; i < len ; ++i ) {
		if ( istr[i] == alt ) {
			t_istr[i] = 0;
		}
		else {
			t_istr[i] = static_cast<unsigned char>(istr[i]);
		}
	}

	// 書き込み
	size_t write = fwrite(t_istr, sizeof(unsigned char), len, fp);
	free(t_istr);

	return write;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::Read
 *  機能概要：  ファイルから文字列を1行読み取ります
 *
 *  返値　　：　-1/0/1=EOF/失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile1::Read(yaya::string_t &ostr)
{
	ostr = L"";

	if (fp == NULL)
		return 0;

	if (yaya::ws_fgets(ostr, fp, charset, 0, bomcheck, false) == yaya::WS_EOF)
		return -1;

	bomcheck++;

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::ReadBin
 *  機能概要：  ファイルからバイナリデータを読み取ります
 *
 *  返値　　：　-1/0/1=EOF/失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile1::ReadBin(yaya::string_t &ostr, size_t len, yaya::char_t alt)
{
	ostr = L"";

	if (fp == NULL)
		return 0;

	unsigned char *f_istr = reinterpret_cast<unsigned char*>(malloc(len+1));
	f_istr[len] = 0; //念のためゼロ終端（いらない）

	size_t read = fread(f_istr,1,len,fp);
	if ( ! read ) {
		free(f_istr);
		return -1;
	}

	for ( size_t i = 0 ; i < read ; ++i ) {
		if ( f_istr[i] == 0 ) {
			ostr.append(1,alt);
		}
		else {
			ostr.append(1,static_cast<yaya::char_t>(f_istr[i]));
		}
	}

	return read;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::FSeek
 *  機能概要：  Cライブラリfseek同等
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int CFile1::FSeek(int offset,int origin){
	if (fp == NULL)
		return 0;

	int result=::fseek(fp,offset,origin);

	if(result!=0){
		return 0;
	}else{
		return 1;
	}
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CFile1::FTell
 *  機能概要：  Cライブラリftell同等
 *  返値　　：　-1/その他=失敗/成功（ftellの結果）
 * -----------------------------------------------------------------------
 */
int CFile1::FTell(){
	if (fp == NULL)
		return -1;

	int result=::ftell(fp);

	if(result<0){
		return -1;
	}else{
		return result;
	}
}

