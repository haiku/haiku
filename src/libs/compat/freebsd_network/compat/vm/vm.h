/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_VM_VM_H_
#define _FBSD_COMPAT_VM_VM_H_


#include <stdint.h>
#include <KernelExport.h>


#ifdef B_HAIKU_64_BIT

typedef uint64_t vm_offset_t;
typedef uint64_t vm_paddr_t;

#else

typedef uint32_t vm_offset_t;
typedef uint32_t vm_paddr_t;

#endif

typedef void *pmap_t;


#define vmspace_pmap(...)	NULL
#define pmap_extract(...)	NULL


vm_paddr_t pmap_kextract(vm_offset_t virtualAddress);

#define vtophys(virtualAddress) pmap_kextract((vm_offset_t)(virtualAddress))

#endif	/* _FBSD_COMPAT_VM_VM_H_ */
