/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_NESTED_PAGING_H
#define KERNEL_ARCH_X86_PAGING_NESTED_PAGING_H


#include "../64bit/paging.h"


// EPT PML4E bits
#define EPT_PML4E_PRESENT			(1LL << 0)
#define EPT_PML4E_WRITABLE			(1LL << 1)
#define EPT_PML4E_EXECUTABLE		(1LL << 2)
#define EPT_PML4E_ADDRESS_MASK		0x0000fffffffff000L

// EPT PDPTE bits
#define EPT_PDPTE_PRESENT			(1LL << 0)
#define EPT_PDPTE_WRITABLE			(1LL << 1)
#define EPT_PDPTE_EXECUTABLE		(1LL << 2)
#define EPT_PDPTE_ADDRESS_MASK		0x0000fffffffff000L

// EPT PDE bits
#define EPT_PDE_PRESENT				(1LL << 0)
#define EPT_PDE_WRITABLE			(1LL << 1)
#define EPT_PDE_EXECUTABLE			(1LL << 2)
#define EPT_PDE_ADDRESS_MASK		0x0000fffffffff000L

// EPT PTE bits.
#define EPT_PTE_PRESENT				(1LL << 0) // FIXME: actually READABLE
#define EPT_PTE_WRITABLE			(1LL << 1)
#define EPT_PTE_EXECUTABLE			(1LL << 2)
#define EPT_PTE_IGNORE_PAT			(1LL << 6)
#define EPT_PTE_ACCESSED			(1LL << 8)
#define EPT_PTE_DIRTY				(1LL << 9)
#define EPT_PTE_ADDRESS_MASK		0x0000fffffffff000L

// Memory types
#define EPT_PTE_CACHING_DISABLED		(0LL << 3)
#define EPT_PTE_WRITE_COMBINING			(1LL << 3)
#define EPT_PTE_WRITE_THROUGH			(4LL << 3)
#define EPT_PTE_WRITE_PROTECT			(5LL << 3)
#define EPT_PTE_WRITE_BACK				(6LL << 3)


#endif	// KERNEL_ARCH_X86_PAGING_NESTED_PAGING_H
