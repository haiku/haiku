/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>


extern "C" void
platform_release_heap(struct stage2_args *args, void *base)
{
	return;
}


extern "C" status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	return B_ERROR;
}
