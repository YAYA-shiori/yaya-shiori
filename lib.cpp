// 
// AYA version 5
//
// 外部ライブラリを扱うクラス　CLib
// written by umeici. 2004
// 
// requestの度にlistから対象を検索していますが、静的にloadしているDLLは
// 多くても数個だと思うので、これでも実行速度に問題はないと考えています。
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
 *  関数名  ：  CLib::Add
 *  機能概要：  指定されたファイル名のDLLをロードし、loadを実行します
 *
 *  返値　　：　0/1/2=失敗/成功/既にロードされている
 * -----------------------------------------------------------------------
 */
int	CLib::Add(const yaya::string_t &name)
{
	std::list<CLib1>::iterator it;
	for(it = liblist.begin(); it != liblist.end(); it++) {
		if (name == it->GetName()) {
			return 2;
		}
	}

	liblist.emplace_back(CLib1(vm, name, charset));
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

	//一時文字コード設定保存部にためこんだものを設定
	charset_map::iterator itc = charset_temp_map.find(name);
	if ( itc != charset_temp_map.end() ) {
		it->SetCharset(itc->second);
		charset_temp_map.erase(itc);
	}

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib::Delete
 *  機能概要：  指定されたファイル名のDLLにてunloadを実行し、リリースします
 *
 *  返値　　：　0/1/2=失敗/成功/ロードされていない、もしくは既にunloadされている
 * -----------------------------------------------------------------------
 */
int	CLib::Delete(const yaya::string_t &name)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (name == it->GetName()) {
			int	result = it->Unload();
			it->Release();
			it = liblist.erase(it);
			return result;
		}
	}

	return 2;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib::DeleteAll
 *  機能概要：  すべてのDLLにてunload()を実行し、リリースします
 * -----------------------------------------------------------------------
 */
void	CLib::DeleteAll(void)
{
	liblist.clear();
	charset_temp_map.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib::SetCharsetDynamic
 *  機能概要：  SAORIの文字コードを変更します
 *
 *  返値　　：　1=常に成功
 * -----------------------------------------------------------------------
 */
int	CLib::SetCharsetDynamic(const yaya::string_t &name,int cs)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (name == it->GetName()) {
			it->SetCharset(cs);
			return 1;
		}
	}

	//ファイルの存在ちぇき
	FILE *fp = yaya::w_fopen(name.c_str(),L"rb");
	if ( ! fp ) {
		return 0;
	}
	fclose(fp);

	//まだロードされていない場合は一時mapにためこむ
	charset_temp_map.insert( charset_map::value_type(name,cs) );

	return 1;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  CLib::GetCharsetDynamic
 *  機能概要：  SAORIの文字コードを取得します
 *
 *  返値　　：　文字コード/数値 失敗時-1
 * -----------------------------------------------------------------------
 */
int	CLib::GetCharsetDynamic(const yaya::string_t &name)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (name == it->GetName()) {
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
 *  関数名  ：  CLib::Request
 *  機能概要：  requestを実行し、結果を取得します
 *
 *  返値　　：　0/1=失敗/成功
 * -----------------------------------------------------------------------
 */
int	CLib::Request(const yaya::string_t &name, const yaya::string_t &istr, yaya::string_t &ostr)
{
	for(std::list<CLib1>::iterator it = liblist.begin(); it != liblist.end(); it++) {
		if (name == it->GetName()) {
			vm.logger().IoLib(0, istr, name);
			int	result = it->Request(istr, ostr);
			vm.logger().IoLib(1, ostr, name);
			return result;
		}
	}

	ostr.erase();
	return 0;
}
