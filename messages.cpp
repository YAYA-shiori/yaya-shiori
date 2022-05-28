// 
// AYA version 5
//
// ログメッセージ
// written by umeici. 2004
// 

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "messages.h"

#include "globaldef.h"
#include "wsex.h"
#include "ccct.h"
#include "misc.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

static void ClearMessageArrays()
{
	yayamsg::msgf.clear();
	yayamsg::msge.clear();
	yayamsg::msgw.clear();
	yayamsg::msgn.clear();
	yayamsg::msgj.clear();
}

/* -----------------------------------------------------------------------
 *  関数名  ：  yayamsg::IsEmpty
 *  機能概要：  エラーメッセージテーブルが空かどうかを判定します
 * -----------------------------------------------------------------------
 */
bool yayamsg::IsEmpty(void)
{
	return yayamsg::msgf.empty() && yayamsg::msge.empty() && yayamsg::msgw.empty();
}


/* -----------------------------------------------------------------------
 *  関数名  ：  yayamsg::LoadMessageFromTxt
 *  機能概要：  テキストファイルからエラーメッセージテーブルに読み込みます
 * -----------------------------------------------------------------------
 */
#if defined(WIN32) || defined(_WIN32_WCE)
extern void*  g_hModule;
#endif

bool yayamsg::LoadMessageFromTxt(const yaya::string_t &basepath,yaya::string_t &filename /* in and out : filename should not const */,char cset)
{
	std::vector<yaya::string_t> lines;

#if defined(WIN32) || defined(_WIN32_WCE)
	{

		yaya::string_t name_w = filename;

		yaya::ws_replace(name_w,L"messagetxt",L"");
		yaya::ws_replace(name_w,L".txt",L"");
		yaya::ws_replace(name_w,L"\\",L"");
		yaya::ws_replace(name_w,L"/",L"");

		std::string name_a = Ccct::Ucs2ToPlainASCII(name_w);

		HRSRC hFound = ::FindResourceA((HINSTANCE)g_hModule,name_a.c_str(),"MESSAGETXT");
		if ( ! hFound ) {
			return false;
		}

		HGLOBAL hRes = ::LoadResource((HINSTANCE)g_hModule,hFound);
		if ( ! hRes ) {
			return false;
		}

		size_t sz = ::SizeofResource((HINSTANCE)g_hModule,hFound);

		void *p = ::LockResource(hRes);
		if ( ! p ) {
			return false;
		}

		std::string restxt((const char*)p,sz);

		yaya::char_t *pc = Ccct::MbcsToUcs2(restxt,cset);
		if ( ! pc ) {
			return false;
		}

		yaya::char_t *pcs = pc;
		while ( true ) {
			yaya::char_t *pce = wcschr(pcs,L'\n');
			if ( ! pce ) {
				lines.emplace_back(yaya::string_t(pcs));
				break;
			}

			*pce = 0;
			lines.emplace_back(yaya::string_t(pcs));

			pcs = pce + 1;
		}

		free(pc);
	}

#else
	{
		yaya::string_t file = basepath + filename;

		FILE *fp = yaya::w_fopen(file.c_str(), L"rb");
		if (fp == NULL) {
			file = basepath + L"messagetxt/" + filename + L".txt";
			fp = yaya::w_fopen(file.c_str(), L"rb");

			if ( fp ) {
				filename = L"messagetxt/" + filename + L".txt";
			}
			else {
				return false;
			}
		}

		yaya::string_t linebuf;

		while ( true ) {
			if (yaya::ws_fgets(linebuf, fp, cset, 0 /*no_enc*/, 1 /*skip_bom*/, 1 /*cut_heading_space*/) == yaya::WS_EOF) {
				break;
			}

			lines.emplace_back(linebuf);
		}

		fclose(fp);
	}

#endif

	MessageArray *ptr = NULL;

	yaya::string_t line;

	ClearMessageArrays();

	for ( std::vector<yaya::string_t>::const_iterator it = lines.begin() ; it != lines.end() ; ++it )
	{
		line = *it;

		CutCrLf(line);

		if( line.substr(0,3)==L"!!!" ) {
			yaya::string_t type = line.substr(3);

			if ( type == L"msgf" ) {
				ptr = &msgf;
			}
			else if ( type == L"msge" ) {
				ptr = &msge;
			}
			else if ( type == L"msgw" ) {
				ptr = &msgw;
			}
			else if ( type == L"msgn" ) {
				ptr = &msgn;
			}
			else if ( type == L"msgj" ) {
				ptr = &msgj;
			}
			else {
				ptr = NULL;
			}
			continue;
		}
		
		if ( line.substr(0,2)==L"//" ) {
			continue;
		}
		
		if ( line.substr(0,1)==L"*" ) {
			if ( ptr ) {
				line=line.substr(1);
				yaya::ws_replace(line,L"\\n", L"\r\n");
				ptr->emplace_back(line);
			}
			continue;
		}
	}

	return true;
}

/* -----------------------------------------------------------------------
 *  関数名  ：  yayamsg::GetTextFromTable
 *  機能概要：  エラーメッセージテーブルから文字列を抜き出します。
 * -----------------------------------------------------------------------
 */
const yaya::string_t yayamsg::GetTextFromTable(int mode,int id)
{
	yayamsg::MessageArray *ptr;
	const yaya::char_t *emsg;

	if (mode == E_F) {
		ptr = &yayamsg::msgf;
		emsg = L"fatal F";
	}
	else if (mode == E_E) {
		ptr = &yayamsg::msge;
		emsg = L"error E";
	}
	else if (mode == E_W) {
		ptr = &yayamsg::msgw;
		emsg = L"warning W";
	}
	else if (mode == E_N) {
		ptr = &yayamsg::msgn;
		emsg = L"note N";
	}
	else {
		ptr = &yayamsg::msgj;
		emsg = L"//msg M";
	}

	if ( id < 0 || ptr->size() <= static_cast<size_t>(id) ) { //catch overflow
		if ( id == 0 && ptr == &yayamsg::msgj ) {
			//need special care, because msgj id=0 will call before loading configure file.
			return L"// AYA request log (before loading base configure file)\r\n// load time: ";
		}
		else {
			yaya::char_t buf[64] = L"";
			yaya::snprintf(buf,63,L"%04d",id);

			if(IsEmpty())
				return yaya::string_t(emsg) + buf + L" : (please specify messagetxt)\r\n";
			else
				return yaya::string_t(emsg) + buf + L" : (not found, please update messagetxt or contact the developer)\r\n";
		}
	}
	else {
		return (*ptr)[id];
	}
}

namespace yayamsg {

	// fatal
	MessageArray msgf;

	// error
	MessageArray msge;

	// warning
	MessageArray msgw;

	// note
	MessageArray msgn;

	// other
	MessageArray msgj;

}
