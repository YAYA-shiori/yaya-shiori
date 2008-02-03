// 
// AYA version 5
//
// ファイルを扱うクラス　CFile/CFile1
// written by umeici. 2004
// 
// CFileが全体を管理します。個々のファイルはCFile1で管理します。
//

#ifndef	FILEH
#define	FILEH

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# include "stdafx.h"
#endif

#include <list>

#include "globaldef.h"

class	CFile1
{
protected:
	yaya::string_t	name;
	FILE	*fp;
	int		charset;
	yaya::string_t	mode;

	int	bomcheck;

public:
	CFile1(const yaya::string_t &n, int cs, yaya::string_t md)
	{
		name     = n;
		charset  = cs;
		mode     = md;
		fp       = NULL;

		bomcheck = 1;
	}

	CFile1(void) { fp = NULL; }
	~CFile1(void) { Close(); }

	yaya::string_t	GetName(void) { return name; }

	int	Open(void);
	int	Close(void);

	int	Write(const yaya::string_t &istr);
	int	Read(yaya::string_t &ostr);

	int	WriteBin(const yaya::string_t &istr, const yaya::char_t alt);
	int	ReadBin(yaya::string_t &ostr, size_t len, yaya::char_t alt);
};

//----

class	CFile
{
protected:
	std::list<CFile1>	filelist;
	int			charset;

public:
	~CFile(void) { DeleteAll(); }

	void	SetCharset(int cs) { charset = cs; }

	int		Add(const yaya::string_t &name, const yaya::string_t &mode);
	int		Delete(const yaya::string_t &name);
	void	DeleteAll(void);

	int		Write(const yaya::string_t &name, const yaya::string_t &istr);
	int		Read(const yaya::string_t &name, yaya::string_t &ostr);

	int		WriteBin(const yaya::string_t &name, const yaya::string_t &istr, const yaya::char_t alt);
	int		ReadBin(const yaya::string_t &name, yaya::string_t &ostr, size_t len, yaya::char_t alt);
};

//----

#endif
