// 
// AYA version 5
//
// AYA��1�C���X�^���X��ێ�����N���XAYAVM
// written by the Maintenance Shop/C.Ponapalt 2006
// 

#ifdef _MSC_VER
#pragma warning( disable : 4786 ) //�u�f�o�b�O�����ł̎��ʎq�؎̂āv
#pragma warning( disable : 4503 ) //�u�������ꂽ���O�̒��������E���z���܂����B���O�͐؂�̂Ă��܂��B�v
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
#include "wordmatch.h"

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
	������
	�قڗ����������p
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

	init_genrand(dwSeed);
}

/*-----------------------------------------------
	�I��
-----------------------------------------------*/
void CAyaVM::Unload(void)
{
}

/*-----------------------------------------------
	��������
-----------------------------------------------*/
unsigned int CAyaVM::genrand(void)
{
	return genrand_int32();
}

int CAyaVM::genrand_int(int n)
{
	return genrand_int32() % n;
}

// ������ƂЂǂ��n�b�N�ł����c�c

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

FACTORY_DEFINE_THIS(CBasis,basis)

FACTORY_DEFINE_PLAIN(std::vector<CFunction>,function)
FACTORY_DEFINE_PLAIN(CCallDepth,calldepth)
FACTORY_DEFINE_THIS(CSystemFunction,sysfunction)
FACTORY_DEFINE_THIS(CGlobalVariable,variable)

FACTORY_DEFINE_PLAIN(CFile,files)
FACTORY_DEFINE_THIS(CLib,libs)

FACTORY_DEFINE_THIS(CParser0,parser0)
FACTORY_DEFINE_THIS(CParser1,parser1)



