// 
// AYA version 5
//
// 外部ライブラリを扱うクラス　CLib/CLib1
// written by umeici. 2004
// 
// CLibが全体を管理します。個々のDLLはCLib1で管理します。
//

#ifndef	LIBH
#define	LIBH

//----

#if defined(WIN32)
# include "stdafx.h"
#endif

#include <list>

#include "aya5.h"
#include "globaldef.h"

class CAyaVM;

class	CLib1
{
protected:
	aya::string_t	name;
#if defined(WIN32)
	typedef HMODULE	module_t;

	bool (*loadlib)(aya::global_t h, long len);
	bool (*unloadlib)(void);
#elif defined(POSIX)
	typedef void*	module_t;

	int (*loadlib)(char* h, long len);
	int (*unloadlib)(void);
#endif
	aya::global_t (*requestlib)(aya::global_t h, long *len);

	module_t hDLL;
	bool    isAlreadyLoaded;
	int		charset;

private:
	CLib1(void);

	CAyaVM &vm;

public:
	CLib1(CAyaVM &vmr, const aya::string_t &n, int cs) : vm(vmr)
	{
		name    = n;
		charset = cs;
		hDLL    = NULL;
		loadlib = NULL;
		unloadlib = NULL;
		requestlib = NULL;
		isAlreadyLoaded = false;
	}

	~CLib1(void) { Unload(); Release(); }

	aya::string_t	GetName(void) { return name; }

	int		LoadLib(void);
	int		Load(void);
	int		Unload(void);
	void	Release(void);

	void	SetCharset(int cs) { charset = cs; }
	int		GetCharset(void) { return charset; }

	int		Request(const aya::string_t &istr, aya::string_t &ostr);
};

//----

typedef std::map<aya::string_t,int> charset_map;

class	CLib
{
protected:
	std::list<CLib1>	liblist;
	charset_map charset_temp_map;
	int			charset;

private:
	CAyaVM &vm;

	CLib(void);

public:
	CLib(CAyaVM &vmr) : vm(vmr) {
	}
	~CLib(void) { DeleteAll(); }

	void	SetCharset(int cs) { charset = cs; }
	int		SetCharsetDynamic(const aya::string_t &name,int cs);

	int		GetCharset(void) { return charset; }
	int		GetCharsetDynamic(const aya::string_t &name);

	int		Add(const aya::string_t &name);
	int		Delete(const aya::string_t &name);
	void	DeleteAll(void);

	int		Request(const aya::string_t &name, const aya::string_t &istr, aya::string_t &ostr);
};

//----

#endif
