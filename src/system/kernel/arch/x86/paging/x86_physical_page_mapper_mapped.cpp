/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


/*!	Physical page mapper implementation for use where the whole of physical
	memory is permanently mapped into the kernel address space.

	This is used on x86_64 where the virtual address space is likely a great
	deal larger than the amount of physical memory in the machine, so it can
	all be mapped in permanently, which is faster and makes life much easier.
*/


#include "paging/x86_physical_page_mapper_mapped.h"

#include <new>

#include <cpu.h>
#include <smp.h>
#include <vm/vm.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "paging/x86_physical_page_mapper.h"
#include "paging/X86PagingStructures.h"
#include "paging/X86VMTranslationMap.h"

