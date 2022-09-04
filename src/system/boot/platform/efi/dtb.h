/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DTB_H
#define DTB_H

#ifndef _ASSEMBLER

#include "efi_platform.h"

#include <util/FixedWidthPointer.h>


extern void dtb_init();
extern void dtb_set_kernel_args();

bool dtb_get_reg(const void* fdt, int node, size_t idx, addr_range& range);
uint32 dtb_get_interrupt(const void* fdt, int node);
bool dtb_has_fdt_string(const char* prop, int size, const char* pattern);


#endif /* !_ASSEMBLER */

#endif /* DTB_H */
