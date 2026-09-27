/*
** AYA version 5
**
** SQLite amalgamation (sqlite/sqlite3.c) built with the compile-time options YAYA uses.
** Compile this file as C (not C++) and do not use precompiled headers.
*/

/* Extensions are never loaded from dictionaries. This also removes the dependency on libdl. */
#define SQLITE_OMIT_LOAD_EXTENSION 1

#ifdef __EMSCRIPTEN__
/* No threads in the emscripten build. */
#define SQLITE_THREADSAFE 0
#endif

#include "sqlite/sqlite3.c"
