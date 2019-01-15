/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "arch/x86/arch_altcodepatch.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <elf.h>
#include <kernel.h>
#include <vm_defs.h>



typedef struct altcodepatch {
	uint32 kernel_offset;
	uint16 length;
	uint16 tag;
} altcodepatch;


extern altcodepatch altcodepatch_begin;
extern altcodepatch altcodepatch_end;


void
arch_altcodepatch_replace(uint16 tag, void* newcodepatch, size_t length)
{
	uint32 count = 0;

	// we need to write to the text area
	struct elf_image_info* info = elf_get_kernel_image();
	const uint32 kernelProtection = B_KERNEL_READ_AREA | B_KERNEL_EXECUTE_AREA;
	set_area_protection(info->text_region.id, kernelProtection | B_KERNEL_WRITE_AREA);

	for (altcodepatch* patch = &altcodepatch_begin; patch < &altcodepatch_end;
			patch++) {
		if (patch->tag != tag)
			continue;
		void* address = (void*)(KERNEL_LOAD_BASE + patch->kernel_offset);
		if (patch->length < length)
			panic("can't copy patch: new code is too long\n");
		memcpy(address, newcodepatch, length);
		count++;
	}

	// disable write after patch
	set_area_protection(info->text_region.id, kernelProtection);

	dprintf("arch_altcodepatch_replace found %" B_PRIu32 " altcodepatches\n", count);
}

