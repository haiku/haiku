/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>


status_t 
platform_get_boot_device(struct stage2_args *args, Node **_device)
{
	return B_ERROR;
}


status_t
platform_get_boot_partition(struct stage2_args *args, NodeList *list,
	boot::Partition **_partition)
{
	return B_ERROR;
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
	return B_ERROR;
}

