/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef MMU_H
#define MMU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void mmu_init(void);
extern void mmu_init_for_kernel(void);
extern void *mmu_allocate(void *virtualAddress, size_t size);
extern void mmu_free(void *virtualAddress, size_t size);

#ifdef __cplusplus
}
#endif

#endif	/* MMU_H */
