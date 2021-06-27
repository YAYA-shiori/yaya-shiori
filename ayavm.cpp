// 
// AYA version 5
//
// AYAの1インスタンスを保持するクラスAYAVM
// written by the Maintenance Shop/C.Ponapalt 2006
// 

#ifdef _MSC_VER
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
#endif

#include "ayavm.h"

#include "basis.h"
#include "file.h"
#include "function.h"
#include "lib.h"
#include "misc.h"
#include "parser0.h"
#include "parser1.h"
#include "sysfunc.h"

#include "babel/babel.h"

#ifdef POSIX
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/*-----------------------------------------------
	コピーコンストラクタ
	shared_ptrをディープコピーする点に注意
-----------------------------------------------*/

template<class T> void shared_ptr_deep_copy(std_shared_ptr<T> &in,std_shared_ptr<T> &out) {
	out.reset(new T(*in));
}

CAyaVM::CAyaVM(CAyaVM &ovm)
{
	#define copy_new(name) shared_ptr_deep_copy(ovm.name,name);
	copy_new(m_basis);
	copy_new(m_function);
	copy_new(m_functionmap);
	copy_new(m_gdefines);
	copy_new(m_calldepth);
	copy_new(m_sysfunction);
	copy_new(m_variable);
	copy_new(m_files);
	copy_new(m_libs);
	copy_new(m_parser0);
	copy_new(m_parser1);
	#undef copy_new

	m_logger = ovm.m_logger;
	rs_sysfunc = ovm.rs_sysfunc;
	rs_internal = ovm.rs_internal;
}

CAyaVM* CAyaVM::get_a_deep_copy()
{
	CAyaVM *nvm = new CAyaVM(*this);
	return nvm;
}

/*-----------------------------------------------
	初期化
	ほぼ乱数初期化用
-----------------------------------------------*/
void CAyaVM::Load(void)
{
#ifdef _DEBUG
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
	tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
	_CrtSetDbgFlag( tmpFlag );
#endif
	unsigned int dwSeed;

	babel::init_babel();

#ifdef POSIX
	struct timeval tv;
	gettimeofday(&tv,NULL);
	dwSeed = tv.tv_usec;
#else
	dwSeed = ::GetTickCount();
#endif

	init_genrand(rs_internal,dwSeed);
	init_genrand(rs_sysfunc,dwSeed);
}

/*-----------------------------------------------
	終了
-----------------------------------------------*/
void CAyaVM::Unload(void)
{
}

/*-----------------------------------------------
	乱数生成
-----------------------------------------------*/
unsigned int CAyaVM::genrand(void)
{
	return genrand_int32(rs_internal);
}

int CAyaVM::genrand_int(int n)
{
	return genrand_int32(rs_internal) % n;
}
unsigned int CAyaVM::genrand_sysfunc(void)
{
	return genrand_int32(rs_sysfunc);
}

int CAyaVM::genrand_sysfunc_int(int n)
{
	return genrand_int32(rs_sysfunc) % n;
}

void CAyaVM::genrand_sysfunc_srand(int n)
{
	init_genrand(rs_sysfunc,n);
}

void CAyaVM::genrand_sysfunc_srand_array(const unsigned long a[],const int n)
{
	init_by_array(rs_sysfunc,a,n);
}

// ちょっとひどいハックですが……

#define FACTORY_DEFINE_THIS(classt,deft) \
	classt & CAyaVM:: deft () { \
		if ( m_ ## deft .get() == NULL ) { \
			m_ ## deft .reset(new classt (*this)); \
		} \
		return *(m_ ## deft .get()); \
	} 

#define FACTORY_DEFINE_PLAIN(classt,deft) \
	classt & CAyaVM:: deft () { \
		if ( m_ ## deft .get() == NULL ) { \
			m_ ## deft .reset(new classt); \
		} \
		return *(m_ ## deft .get()); \
	} 


FACTORY_DEFINE_PLAIN(yaya::indexmap,functionmap)

FACTORY_DEFINE_PLAIN(std::vector<CDefine>,gdefines)

FACTORY_DEFINE_THIS(CBasis,basis)

FACTORY_DEFINE_PLAIN(std::vector<CFunction>,function)
FACTORY_DEFINE_PLAIN(CCallDepth,calldepth)
FACTORY_DEFINE_THIS(CSystemFunction,sysfunction)
FACTORY_DEFINE_THIS(CGlobalVariable,variable)

FACTORY_DEFINE_PLAIN(CFile,files)
FACTORY_DEFINE_THIS(CLib,libs)

FACTORY_DEFINE_THIS(CParser0,parser0)
FACTORY_DEFINE_THIS(CParser1,parser1)



