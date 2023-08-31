#ifndef _DIAGASSERT
#define	_DIAGASSERT(x)	assert(x)
#endif

#ifndef __arraycount
#define	__arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
#endif

#ifndef CLLADDR
#define CLLADDR(sdl) (const void *)((sdl)->sdl_data + (sdl)->sdl_nlen)
#endif
