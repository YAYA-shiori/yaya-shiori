// 
// AYA version 5
//
// ファイルを扱うクラス　CFile
// written by umeici. 2004
// 
// write/readの度にlistから対象を検索していますが、一度に取り扱うファイルは
// 多くても数個だと思うので、これでも実行速度に問題はないと考えています。
//

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <list>

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
 *  関数名  ：  CFile::Add
 *  機能概要：  指定されたファイルをオープンします
 *
 *  返値　　：　0/1/2=失敗/成功/既にオープンしている
 * -----------------------------------------------------------------------
 */
int	CFile::Add(const yaya::string_t &name, const yaya::string_t &mode)
{
	std::list<CFile1>::iterator it;
	for(it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName()))
			return 2;

	yaya::string_t	t_mode = mode;
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
		)
		return 0;

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
 *  関数名  ：  CFile::Delete
 *  機能概要：  指定されたファイルをクローズします
 *
 *  返値　　：　1/2=成功/オープンされていない、もしくは既にfcloseされている
 * -----------------------------------------------------------------------
 */
int	CFile::Delete(const yaya::string_t &name)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName())) {
			int	result = it->Close();
			it = filelist.erase(it);
			return result;
		}

	return 2;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::DeleteAll
 *  機能概要：  すべてのファイルをクローズします
 * -----------------------------------------------------------------------
 */
void	CFile::DeleteAll(void)
{
	filelist.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::Write
 *  機能概要：  ファイルに文字列を書き込みます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile::Write(const yaya::string_t &name, const yaya::string_t &istr)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName()))
			return it->Write(istr);

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::WriteBin
 *  機能概要：  ファイルにバイナリデータを書き込みます
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile::WriteBin(const yaya::string_t &name, const yaya::string_t &istr, const yaya::char_t alt)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName()))
			return it->WriteBin(istr,alt);

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::Read
 *  機能概要：  ファイルから文字列を1行読み取ります
 *
 *  返値　　：　-1/0/1=EOF/失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile::Read(const yaya::string_t &name, yaya::string_t &ostr)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName())) {
			int	result = it->Read(ostr);
			return result;
		}

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::ReadBin
 *  機能概要：  ファイルからバイナリデータを読み取ります
 *
 *  返値　　：　-1/0/1=EOF/失敗/成功
 * -----------------------------------------------------------------------
 */
int	CFile::ReadBin(const yaya::string_t &name, yaya::string_t &ostr, size_t len, yaya::char_t alt)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName())) {
			int	result = it->ReadBin(ostr,len,alt);
			return result;
		}

	return 0;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::FSeek
 *  機能概要：  Cライブラリfseek同等
 *  返値　　：　0/1=失敗/成功
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

	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName())) {
			int	result = it->FSeek(offset,mode);
			return result;
		}

	return 0;
}


/* -----------------------------------------------------------------------
 *  関数名  ：  CFile::FTell
 *  機能概要：  Cライブラリftell同等
 *  返値　　：　-1/その他=失敗/成功（ftellの結果）
 * -----------------------------------------------------------------------
 */
int CFile::FTell(const yaya::string_t &name)
{
	for(std::list<CFile1>::iterator it = filelist.begin(); it != filelist.end(); it++)
		if (!name.compare(it->GetName())) {
			int	result = it->FTell();
			return result;
		}

	return 0;
}



