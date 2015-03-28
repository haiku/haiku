/*
 * Copyright 2012-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef __FDT_SUPPORT_H
#define __FDT_SUPPORT_H


#include <KernelExport.h>


void dump_fdt(const void *fdt);
status_t fdt_get_cell_count(const void* fdt, int node,
	int32 &addressCells, int32 &sizeCells);

phys_addr_t fdt_get_device_reg(const void* fdt, int node);
phys_addr_t fdt_get_device_reg_byname(const void* fdt, const char* name);
phys_addr_t fdt_get_device_reg_byalias(const void* fdt, const char* alias);


#endif /*__FDT_SUPPORT_H*/
