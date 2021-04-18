/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _FDT_H_
#define _FDT_H_


extern void* gFdt;


void fdt_init(void* fdt);
void fdt_set_kernel_args();


#endif	// _FDT_H_
