/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#include <string.h>


status_t 
platform_add_boot_device(struct stage2_args* args, NodeList* devicesList)
{
#warning IMPLEMENT platform_add_boot_device
	return B_ERROR;
}


status_t
platform_get_boot_partition(struct stage2_args* args, Node* bootDevice,
	NodeList* list, boot::Partition** _partition)
{
#warning IMPLEMENT platform_get_boot_partition
	return B_ERROR;
}


status_t
platform_add_block_devices(stage2_args* args, NodeList* devicesList)
{
#warning IMPLEMENT platform_add_block_devices
	return B_ERROR;
}


status_t 
platform_register_boot_device(Node* device)
{
#warning IMPLEMENT platform_register_boot_device
	return B_ERROR;
}

