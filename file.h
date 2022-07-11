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

#include <stdio.h>
#include <list>

#include "globaldef.h"
#include "manifest.h"

class	CFile1
{
protected:
	yaya::string_t	name;
	FILE	*fp;
	int		charset;
	yaya::int_t size;
	yaya::string_t	mode;

	int	bomcheck;

public:
	CFile1(const yaya::string_t &n, int cs, yaya::string_t md)
	{
		name     = n;
		charset  = cs;
		mode     = md;
		fp       = NULL;

		size = 0;
		bomcheck = 1;
	}

	CFile1(void) {
		charset = CHARSET_UTF8;
		fp = NULL;

		size = 0;
		bomcheck = 1;
	}
	~CFile1(void) { Close(); }

	inline const yaya::string_t& GetName(void) const { return name; }
	inline bool operator==(const yaya::string_t &n) const { return n == name; }

	bool Open(void);
	int	Close(void);

	inline yaya::int_t Size(void) const { return size; }

	bool Write(const yaya::string_t &istr);
	bool WriteBin(const yaya::string_t &istr, const yaya::char_t alt);
	bool WriteDecode(const yaya::string_t &istr, const yaya::string_t &type);

	int	Read(yaya::string_t &ostr);
	int	ReadBin(yaya::string_t &ostr, size_t len, yaya::char_t alt);
	int	ReadEncode(yaya::string_t &ostr, size_t len, const yaya::string_t &type);

	bool	   FSeek(yaya::int_t offset, int origin);
	yaya::int_t FTell();
};

//----

class	CFile
{
protected:
	std::list<CFile1>	filelist;
	int			charset;

	bool    ProcessOpenMode(yaya::string_t &mode);

public:
	CFile() { charset = CHARSET_UTF8; }
	~CFile(void) { DeleteAll(); }

	void	SetCharset(int cs) { charset = cs; }

	int		Add(const yaya::string_t &name, const yaya::string_t &mode);
	int		Delete(const yaya::string_t &name);
	void	DeleteAll(void);

	bool	Write(const yaya::string_t &name, const yaya::string_t &istr);
	bool	WriteBin(const yaya::string_t &name, const yaya::string_t &istr, const yaya::char_t alt);
	bool	WriteDecode(const yaya::string_t &name, const yaya::string_t &istr, const yaya::string_t &type);

	int		Read(const yaya::string_t &name, yaya::string_t &ostr);
	int		ReadBin(const yaya::string_t &name, yaya::string_t &ostr, size_t len, yaya::char_t alt);
	int		ReadEncode(const yaya::string_t &name, yaya::string_t &ostr, size_t len, const yaya::string_t &type);

	yaya::int_t Size(const yaya::string_t &name);

	bool	   FSeek(const yaya::string_t &name, yaya::int_t offset, const yaya::string_t &mode);
	yaya::int_t FTell(const yaya::string_t &name);
};

//----

#endif
