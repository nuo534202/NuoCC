#pragma once

#include <assert.h>

#ifdef NDEBUG

/* Release Mode */
namespace nuocc
{

#define Assert(expr) ((void)0)

}   /* namespace nuocc */

#else

/* Debug Mode */
namespace nuocc
{

#define Assert(expr) assert(expr)

}   /* namespace nuocc */

#endif