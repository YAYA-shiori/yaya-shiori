#pragma once

#ifdef FIX_OLD_COMPILER

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
#define std_shared_ptr shared_ptr

#else

using std::shared_ptr;
#define std_shared_ptr shared_ptr

#endif

