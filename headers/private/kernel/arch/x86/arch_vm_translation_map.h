/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_ARCH_x86_VM_TRANSLATION_MAP_H
#define _KERNEL_ARCH_x86_VM_TRANSLATION_MAP_H


#include <arch/vm_translation_map.h>


// quick function to return the physical pgdir of a mapping, needed for a context switch
#ifdef __cplusplus
extern "C"
#endif
void *i386_translation_map_get_pgdir(vm_translation_map *map);

#endif /* _KERNEL_ARCH_x86_VM_TRANSLATION_MAP_H */
