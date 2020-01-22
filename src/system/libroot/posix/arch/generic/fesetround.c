/*
 * Copyright (c) 2005-2019 Rich Felker, et al.
 *
 * Part of MUSL, released under the MIT license.
 */


#include <fenv.h>
#include <features.h>

#define hidden __attribute__((__visibility__("hidden")))

/* __fesetround wrapper for arch independent argument check */

hidden int __fesetround(int);

int fesetround(int r)
{
	if (r != FE_TONEAREST
#ifdef FE_DOWNWARD
		&& r != FE_DOWNWARD
#endif
#ifdef FE_UPWARD
		&& r != FE_UPWARD
#endif
#ifdef FE_TOWARDZERO
		&& r != FE_TOWARDZERO
#endif
	)
		return -1;
	return __fesetround(r);
}
