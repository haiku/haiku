/*
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
 * Copyright 2013-2014, Fredrik Holmqvist, fredrik.holmqvist@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <KernelExport.h>

#include <arch/cpu.h>
#include <arch_cpu_defs.h>
#include <kernel.h>

#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "arch_mmu.h"
#include "arch_start.h"
#include "acpi.h"
#include "console.h"
#include "cpu.h"
#ifdef _BOOT_FDT_SUPPORT
#include "dtb.h"
#endif
#include "efi_platform.h"
#include "mmu.h"
#include "quirks.h"
#include "serial.h"
#include "smp.h"
#include "timer.h"


extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);


const efi_system_table		*kSystemTable;
const efi_boot_services		*kBootServices;
const efi_runtime_services	*kRuntimeServices;
efi_handle kImage;


static uint32 sBootOptions;

extern "C" int main(stage2_args *args);
extern "C" void _start(void);
extern "C" void efi_enter_kernel(uint64 pml4, uint64 entry_point, uint64 stack);


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++)
		(**f)();
}


extern "C" uint32
platform_boot_options()
{
	return sBootOptions;
}


static void
convert_preloaded_image(preloaded_elf64_image* image)
{
	fix_address(image->next);
	fix_address(image->name);
	fix_address(image->debug_string_table);
	fix_address(image->syms);
	fix_address(image->rel);
	fix_address(image->rela);
	fix_address(image->pltrel);
	fix_address(image->debug_symbols);
}


/*!	Convert all addresses in kernel_args to 64-bit addresses. */
static void
convert_kernel_args()
{
	if (gKernelArgs.kernel_image->elf_class != ELFCLASS64)
		return;

	fix_address(gKernelArgs.boot_volume);
	fix_address(gKernelArgs.vesa_modes);
	fix_address(gKernelArgs.edid_info);
	fix_address(gKernelArgs.debug_output);
	fix_address(gKernelArgs.boot_splash);
	#if defined(__x86_64__) || defined(__x86__)
	fix_address(gKernelArgs.ucode_data);
	fix_address(gKernelArgs.arch_args.apic);
	fix_address(gKernelArgs.arch_args.hpet);
	#endif

	convert_preloaded_image(static_cast<preloaded_elf64_image*>(
		gKernelArgs.kernel_image.Pointer()));
	fix_address(gKernelArgs.kernel_image);

	// Iterate over the preloaded images. Must save the next address before
	// converting, as the next pointer will be converted.
	preloaded_image* image = gKernelArgs.preloaded_images;
	fix_address(gKernelArgs.preloaded_images);
	while (image != NULL) {
		preloaded_image* next = image->next;
		convert_preloaded_image(static_cast<preloaded_elf64_image*>(image));
		image = next;
	}

	// Fix driver settings files.
	driver_settings_file* file = gKernelArgs.driver_settings;
	fix_address(gKernelArgs.driver_settings);
	while (file != NULL) {
		driver_settings_file* next = file->next;
		fix_address(file->next);
		fix_address(file->buffer);
		file = next;
	}
}


static addr_t
get_kernel_entry(void)
{
	if (gKernelArgs.kernel_image->elf_class == ELFCLASS64) {
		preloaded_elf64_image *image = static_cast<preloaded_elf64_image *>(
			gKernelArgs.kernel_image.Pointer());
		return image->elf_header.e_entry;
	} else if (gKernelArgs.kernel_image->elf_class == ELFCLASS32) {
		preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(
			gKernelArgs.kernel_image.Pointer());
		return image->elf_header.e_entry;
	}
	panic("Unknown kernel format! Not 32-bit or 64-bit!");
	return 0;
}


static void
get_kernel_regions(addr_range& text, addr_range& data)
{
	if (gKernelArgs.kernel_image->elf_class == ELFCLASS64) {
		preloaded_elf64_image *image = static_cast<preloaded_elf64_image *>(
			gKernelArgs.kernel_image.Pointer());
		text.start = image->text_region.start;
		text.size = image->text_region.size;
		data.start = image->data_region.start;
		data.size = image->data_region.size;
		return;
	} else if (gKernelArgs.kernel_image->elf_class == ELFCLASS32) {
		preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(
			gKernelArgs.kernel_image.Pointer());
		text.start = image->text_region.start;
		text.size = image->text_region.size;
		data.start = image->data_region.start;
		data.size = image->data_region.size;
		return;
	}
	panic("Unknown kernel format! Not 32-bit or 64-bit!");
}


extern "C" void
platform_start_kernel(void)
{
	smp_init_other_cpus();
#ifdef _BOOT_FDT_SUPPORT
	dtb_set_kernel_args();
#endif

	addr_t kernelEntry = get_kernel_entry();

	addr_range textRegion = {.start = 0, .size = 0}, dataRegion = {.start = 0, .size = 0};
	get_kernel_regions(textRegion, dataRegion);
	dprintf("kernel:\n");
	dprintf("  text: %#" B_PRIx64 ", %#" B_PRIx64 "\n", textRegion.start, textRegion.size);
	dprintf("  data: %#" B_PRIx64 ", %#" B_PRIx64 "\n", dataRegion.start, dataRegion.size);
	dprintf("  entry: %#lx\n", kernelEntry);

	arch_mmu_init();
	convert_kernel_args();

	// map in a kernel stack
	void *stack_address = NULL;
	if (platform_allocate_region(&stack_address,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE, 0, false)
		!= B_OK) {
		panic("Unabled to allocate a stack");
	}
	gKernelArgs.cpu_kstack[0].start = fix_address((addr_t)stack_address);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	dprintf("Kernel stack at %#" B_PRIx64 "\n", gKernelArgs.cpu_kstack[0].start);

	// Apply any weird EFI quirks
	quirks_init();

	// Begin architecture-centric kernel entry.
	arch_start_kernel(kernelEntry);

	panic("Shouldn't get here!");
}


extern "C" void
platform_exit(void)
{
	kRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
	return;
}


/**
 * efi_main - The entry point for the EFI application
 * @image: firmware-allocated handle that identifies the image
 * @systemTable: EFI system table
 */
extern "C" efi_status
efi_main(efi_handle image, efi_system_table *systemTable)
{
	stage2_args args;

	memset(&args, 0, sizeof(stage2_args));

	kImage = image;
	kSystemTable = systemTable;
	kBootServices = systemTable->BootServices;
	kRuntimeServices = systemTable->RuntimeServices;

	call_ctors();

	console_init();
	serial_init();
	serial_enable();

	sBootOptions = console_check_boot_keys();

	// disable apm in case we ever load a 32-bit kernel...
	gKernelArgs.platform_args.apm.version = 0;

	cpu_init();
	acpi_init();
#ifdef _BOOT_FDT_SUPPORT
	dtb_init();
#endif
	timer_init();
	smp_init();

	main(&args);

	return EFI_SUCCESS;
}
