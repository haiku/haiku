/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_BUS_H
#define KERNEL_BUS_H


#include <module.h>


struct kernel_args;

int bus_init(struct kernel_args *ka);
int bus_man_init(struct kernel_args *ka);

int bus_register_bus(const char *path);

typedef struct id_list {
	uint32 num_ids;
	uint32 id[0];
} id_list;

#define MAX_DEV_IO_RANGES 8

typedef struct device {
	uint32 vendor_id;
	uint32 device_id;

	uint32 irq;
	addr_t base[MAX_DEV_IO_RANGES];
	addr_t size[MAX_DEV_IO_RANGES];

	char dev_path[256];
} device;

int bus_find_device(int n, id_list *vendor_ids, id_list *device_ids, device *dev);

#endif	/* KERNEL_BUS_H */
