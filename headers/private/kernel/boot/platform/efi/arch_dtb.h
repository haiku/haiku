/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_ARCH_DTB_H
#define KERNEL_BOOT_PLATFORM_EFI_ARCH_DTB_H


#include <SupportDefs.h>


void arch_handle_fdt(const void* fdt, int node);
void arch_dtb_set_kernel_args(void);


#endif /* KERNEL_BOOT_PLATFORM_EFI_ARCH_DTB_H */
