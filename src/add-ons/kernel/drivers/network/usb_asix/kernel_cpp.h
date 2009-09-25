#ifndef _KERNEL_CPP_H_
#define _KERNEL_CPP_H_

#include <malloc.h>

inline void *
operator new(size_t size)
{
	return malloc(size);
}


inline void *
operator new[](size_t size)
{
	return malloc(size);
}


inline void
operator delete(void *pointer)
{
	free(pointer);
}


inline void
operator delete[](void *pointer)
{
	free(pointer);
}


inline void
terminate(void)
{
}


static inline void
__throw()
{
}

#endif // _KERNEL_CPP_H_
