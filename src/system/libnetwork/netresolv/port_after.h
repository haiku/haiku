#ifndef port_after_h
#define port_after_h

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>

#ifndef MIN
#define MIN(x,y) (((x) <= (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) (((x) >= (y)) ? (x) : (y))
#endif

#ifndef ALIGN
#define ALIGN(p) (((uintptr_t)(p) + (sizeof(long) - 1)) & ~(sizeof(long) - 1))
#endif

#endif
