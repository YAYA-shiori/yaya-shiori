// 
// AYA version 5
//
// �R�����g�A�E�g�����N���X�@CComment
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
	char	flag;		// �̈�t���O 0/1=�R�����g�G���A�O/��
public:
	CComment(void) { flag = 0; }
	~CComment(void) {}

	void	Process(yaya::string_t &str);

	void	Process_Top(yaya::string_t &str);
	void	Process_Tail(yaya::string_t &str);
};

//----

#endif
