// 
// AYA version 5
//
// written by umeici. 2004
// 

#ifndef	AYA5H
#define	AYA5H

//----

#if defined(AYA_MAKE_EXE)
# define DLLEXPORT
# define BOOL_TYPE   BOOL
# define FUNCATTRIB
#elif defined(WIN32) || defined(_WIN32_WCE)
# define DLLEXPORT __declspec(dllexport)
# define BOOL_TYPE   BOOL
# define FUNCATTRIB __cdecl
#elif defined(POSIX)
# define DLLEXPORT
# define BOOL_TYPE   int
# define FUNCATTRIB
#endif

namespace yaya {
#if defined(WIN32) || defined(_WIN32_WCE)
	typedef HGLOBAL global_t;
#elif defined(POSIX)
	typedef char* global_t;
#endif
}; // namespace yaya {

extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB load(yaya::global_t h, long len);
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB unload();
extern "C" DLLEXPORT yaya::global_t FUNCATTRIB request(yaya::global_t h, long *len);

extern "C" DLLEXPORT long FUNCATTRIB multi_load(yaya::global_t h, long len);
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB multi_unload(long id);
extern "C" DLLEXPORT yaya::global_t FUNCATTRIB multi_request(long id,yaya::global_t h, long *len);

#if defined(WIN32)
extern "C" DLLEXPORT BOOL_TYPE FUNCATTRIB logsend(long hwnd);
#endif

//----

#endif
