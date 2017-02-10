/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <boot/heap.h>


extern "C" status_t
heap_init(struct stage2_args *args)
{
	return B_OK;
}


extern "C" void
heap_release(struct stage2_args *args)
{
}
