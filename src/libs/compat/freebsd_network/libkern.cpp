/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All rights reserved.
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

extern "C" {
#include <compat/sys/libkern.h>
}

#include <util/Random.h>


u_int
read_random(void* buf, u_int len)
{
	uint8* bufs = (uint8*)buf;
	for (int i = 0; i < len; i++)
		bufs[i] = secure_get_random<uint8>();
	return len;
}


void
arc4rand(void *ptr, u_int len, int reseed)
{
	read_random(ptr, len);
}


uint32_t
arc4random(void)
{
	uint32_t ret;

	arc4rand(&ret, sizeof ret, 0);
	return ret;
}
