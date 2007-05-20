// 
// AYA version 5
//
// AYAの1インスタンスを保持するクラスAYAVM
// written by the Maintenance Shop/C.Ponapalt 2006
// 

#include "ayavm.h"

#include "basis.h"
#include "file.h"
#include "function.h"
#include "lib.h"
#include "misc.h"
#include "parser0.h"
#include "parser1.h"
#include "sysfunc.h"
#include "wordmatch.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


/*-----------------------------------------------
	初期化
	ほぼ乱数初期化用
-----------------------------------------------*/
CAyaVM::CAyaVM(void)
{
#ifdef _DEBUG
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
	tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
	_CrtSetDbgFlag( tmpFlag );
#endif

	init_genrand(static_cast<unsigned long>(time(NULL)));
}

/*-----------------------------------------------
	乱数生成
-----------------------------------------------*/
unsigned int CAyaVM::genrand(void)
{
	return genrand_int32();
}

int CAyaVM::genrand_int(int n)
{
	return genrand_int32() % n;
}

// ちょっとひどいハックですが……

#define FACTORY_DEFINE_THIS(classt,deft) \
	classt ## & CAyaVM:: ## deft ## () { \
		if ( m_ ## deft ## .get() == NULL ) { \
			m_ ## deft ## .reset(new classt ## (*this)); \
		} \
		return *(m_ ## deft ## .get()); \
	} 

#define FACTORY_DEFINE_PLAIN(classt,deft) \
	classt ## & CAyaVM:: ## deft ## () { \
		if ( m_ ## deft ## .get() == NULL ) { \
			m_ ## deft ## .reset(new classt ## ); \
		} \
		return *(m_ ## deft ## .get()); \
	} 


FACTORY_DEFINE_PLAIN(CWordMatch,formulatag_wm)
FACTORY_DEFINE_PLAIN(CWordMatch,function_wm)

FACTORY_DEFINE_THIS(CBasis,basis)

FACTORY_DEFINE_PLAIN(std::vector<CFunction>,function)
FACTORY_DEFINE_PLAIN(CCallDepth,calldepth)
FACTORY_DEFINE_THIS(CSystemFunction,sysfunction)
FACTORY_DEFINE_THIS(CGlobalVariable,variable)

FACTORY_DEFINE_PLAIN(CFile,files)
FACTORY_DEFINE_THIS(CLib,libs)

FACTORY_DEFINE_THIS(CParser0,parser0)
FACTORY_DEFINE_THIS(CParser1,parser1)

