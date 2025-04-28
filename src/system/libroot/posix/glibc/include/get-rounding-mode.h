#ifndef _GET_ROUNDING_MODE_H
#define _GET_ROUNDING_MODE_H	1

#include <fenv.h>

static inline int
get_rounding_mode()
{
	return fegetround();
}

#endif /* _GET_ROUNDING_MODE_H */
