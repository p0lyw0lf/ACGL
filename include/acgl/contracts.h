#ifndef ACGL_CONTRACTS_H
#define ACGL_CONTRACTS_H

#include <assert.h>

#if defined(DEBUG) || defined(_DEBUG)

#define REQUIRES(x) assert(x)
#define ENSURES(x)  assert(x)
#define ASSERT(x)   assert(x)

#else

#define REQUIRES(x) ((void)0);
#define ENSURES(x)  ((void)0);
#define ASSERT(x)   ((void)0);

#endif

#endif // ACGL_CONTRACTS_H
