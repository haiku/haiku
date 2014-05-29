/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include "paging/x86_physical_page_mapper.h"


// #pragma mark -


static X86PhysicalPageMapper sPhysicalPageMapper;
static TranslationMapPhysicalPageMapper sKernelPageMapper;


// #pragma mark - Initialization


status_t
mapped_physical_page_ops_init(kernel_args* args,
	X86PhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper)
{
	new(&sPhysicalPageMapper) X86PhysicalPageMapper;
	new(&sKernelPageMapper) TranslationMapPhysicalPageMapper;

	_pageMapper = &sPhysicalPageMapper;
	_kernelPageMapper = &sKernelPageMapper;
	return B_OK;
}
