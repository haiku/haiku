/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_VM_H_
#define _KERNEL_ARCH_ARM64_ARCH_VM_H_


#define PAGE_SHIFT		12


bool flush_va_if_accessed(uint64_t pte, addr_t va, int asid);

#endif /* _KERNEL_ARCH_ARM64_ARCH_VM_H_ */
