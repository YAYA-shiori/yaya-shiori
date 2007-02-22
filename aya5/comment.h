// 
// AYA version 5
//
// コメントアウト処理クラス　CComment
// written by umeici. 2004
// 

#ifndef	CCOMMENTH
#define	CCOMMENTH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include "globaldef.h"

class	CComment
{
protected:
	char	flag;		// 領域フラグ 0/1=コメントエリア外/内
public:
	CComment(void) { flag = 0; }
	~CComment(void) {}

	void	Process(yaya::string_t &str);

	void	Process_Top(yaya::string_t &str);
	void	Process_Tail(yaya::string_t &str);
};

//----

#endif
