/*
 * Copyright 2024 Daniel Martin, dalmemail@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef NVMM_BITOPS
#define NVMM_BITOPS

// bitops macros taken from sys/cdefs.h (NetBSD)
#define NBBY 8 // (Number of Bits in a BYte)
#define __BIT(__n)      \
	(((uintmax_t)(__n) >= NBBY * sizeof(uintmax_t)) ? 0 : \
	((uintmax_t)1 << (uintmax_t)((__n) & (NBBY * sizeof(uintmax_t) - 1))))

#define __BITS(__m, __n)        \
	((__BIT(max_c((__m), (__n)) + 1) - 1) ^ (__BIT(min_c((__m), (__n))) - 1))

#define __LOWEST_SET_BIT(__mask) ((((__mask) - 1) & (__mask)) ^ (__mask))
#define __SHIFTOUT(__x, __mask) (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))
#define	__SHIFTIN(__x, __mask) ((__x) * __LOWEST_SET_BIT(__mask))

#endif /* NVMM_BITOPS */
