/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_ATOMIC_H__
#define __NVME_ATOMIC_H__

#include <OS.h>


/* 32-bit atomics */
typedef int32 nvme_atomic_t;

#define NVME_ATOMIC_INIT(val)	(val)

static inline void
nvme_atomic_init(nvme_atomic_t* v)
{
	*v = 0;
}

static inline void
nvme_atomic_clear(nvme_atomic_t* v)
{
	*v = 0;
}


static inline int32
nvme_atomic_read(const nvme_atomic_t* v)
{
	return atomic_get(v);
}


static inline void
nvme_atomic_set(nvme_atomic_t* v, int32 new_value)
{
	atomic_set(v, new_value);
}


static inline void
nvme_atomic_add(nvme_atomic_t* v, int32 inc)
{
	atomic_add(v, inc);
}


static inline void
nvme_atomic_sub(nvme_atomic_t* v, int32 dec)
{
	atomic_add(v, -dec);
}


static inline void
nvme_atomic_inc(nvme_atomic_t* v)
{
	nvme_atomic_add(v, 1);
}


static inline void
nvme_atomic_dec(nvme_atomic_t* v)
{
	nvme_atomic_sub(v, 1);
}


static inline int32
nvme_atomic_add_return(nvme_atomic_t* v, int32 inc)
{
	return atomic_add(v, inc);
}


static inline int32
nvme_atomic_sub_return(nvme_atomic_t* v, int32 dec)
{
	return atomic_add(v, -dec);
}


static inline int
nvme_atomic_inc_and_test(nvme_atomic_t *v)
{
	return nvme_atomic_add_return(v, 1) == 0;
}


static inline int
nvme_atomic_dec_and_test(nvme_atomic64_t *v)
{
	return nvme_atomic_sub_return(v, 1) == 0;
}


static inline int
nvme_atomic_test_and_set(nvme_atomic_t* v)
{
	return atomic_test_and_set(v, 1, 0);
}


/* 64-bit atomics */
typedef int64 nvme_atomic64_t;

#define NVME_ATOMIC64_INIT(val) (val)

static inline void
nvme_atomic64_init(nvme_atomic64_t *v)
{
	atomic_set64(v, 0);
}

static inline void
nvme_atomic64_clear(nvme_atomic64_t *v)
{
	atomic_set64(v, 0);
}


static inline int64
nvme_atomic64_read(nvme_atomic64_t *v)
{
	return atomic_get64(v);
}


static inline void
nvme_atomic64_set(nvme_atomic64_t *v, int64_t new_value)
{
	atomic_set64(v, new_value);
}


static inline void
nvme_atomic64_add(nvme_atomic64_t *v, int64_t inc)
{
	atomic_add64(v, inc);
}


static inline void
nvme_atomic64_sub(nvme_atomic64_t *v, int64_t dec)
{
	nvme_atomic64_add(v, -dec);
}


static inline void
nvme_atomic64_inc(nvme_atomic64_t *v)
{
	nvme_atomic64_add(v, 1);
}


static inline void
nvme_atomic64_dec(nvme_atomic64_t *v)
{
	nvme_atomic64_add(v, -1);
}


static inline int64
nvme_atomic64_add_return(nvme_atomic64_t *v, int64_t inc)
{
	return atomic_add64(v, inc);
}


static inline int64_t
nvme_atomic64_sub_return(nvme_atomic64_t *v, int64_t dec)
{
	return nvme_atomic64_add_return(v, -dec);
}


static inline int
nvme_atomic64_inc_and_test(nvme_atomic64_t *v)
{
	return nvme_atomic64_add_return(v, 1) == 0;
}


static inline int
nvme_atomic64_dec_and_test(nvme_atomic64_t *v)
{
	return nvme_atomic64_sub_return(v, 1) == 0;
}


static inline int
nvme_atomic64_test_and_set(nvme_atomic64_t *v)
{
	return atomic_test_and_set64(v, 1, 0);
}


#endif
