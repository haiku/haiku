/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H
#define KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H


#include <OS.h>

#include <util/DoublyLinkedList.h>


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;
struct kernel_args;


status_t mapped_physical_page_ops_init(kernel_args* args,
	X86PhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper);


#endif	// KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_MAPPED_H
