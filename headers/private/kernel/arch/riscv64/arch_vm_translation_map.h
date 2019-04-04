/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H
#define _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H

#include <arch/vm_translation_map.h>

#ifdef __cplusplus
extern "C" {
#endif

void riscv64_translation_map_change_asid(VMTranslationMap *map);

status_t riscv64_map_address_range(addr_t virtualAddress,
	phys_addr_t physicalAddress, size_t size);
void riscv64_unmap_address_range(addr_t virtualAddress, size_t size);
status_t riscv64_remap_address_range(addr_t *virtualAddress, size_t size,
	bool unmap);

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H */
