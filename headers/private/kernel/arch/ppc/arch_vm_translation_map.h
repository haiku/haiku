/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_ARCH_PPC_VM_TRANSLATION_MAP_H
#define _KERNEL_ARCH_PPC_VM_TRANSLATION_MAP_H

#include <arch/vm_translation_map.h>

#ifdef __cplusplus
extern "C"
#endif
void ppc_translation_map_change_asid(vm_translation_map *map);

#endif /* _KERNEL_ARCH_PPC_VM_TRANSLATION_MAP_H */
