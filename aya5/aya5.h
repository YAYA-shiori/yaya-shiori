// 
// AYA version 5
//
// written by umeici. 2004
// 

#ifndef	AYA5H
#define	AYA5H

//----

#if defined(WIN32) || defined(_WIN32_WCE)
# define DLLEXPORT __declspec(dllexport)
# define RETERNTYPE BOOL
# define FUNCATTRIB __cdecl
#elif defined(POSIX)
# define DLLEXPORT
# define RETERNTYPE int
# define FUNCATTRIB
#endif

namespace yaya {
#if defined(WIN32) || defined(_WIN32_WCE)
	typedef HGLOBAL global_t;
#elif defined(POSIX)
	typedef char* global_t;
#endif
}; // namespace yaya {

extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB load(yaya::global_t h, long len);
extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB unload();
extern "C" DLLEXPORT yaya::global_t FUNCATTRIB request(yaya::global_t h, long *len);

#if defined(WIN32)
extern "C" DLLEXPORT RETERNTYPE FUNCATTRIB logsend(long hwnd);
#endif

//----

#endif
