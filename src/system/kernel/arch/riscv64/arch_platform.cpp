/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/platform.h>
#include <boot/kernel_args.h>
#include <Htif.h>
#include <Plic.h>
#include <Clint.h>
#include <platform/sbi/sbi_syscalls.h>

#include <debug.h>


uint32 gPlatform;

void* gFDT = NULL;

HtifRegs  *volatile gHtifRegs  = (HtifRegs *volatile)0x40008000;
PlicRegs  *volatile gPlicRegs;
ClintRegs *volatile gClintRegs;


status_t
arch_platform_init(struct kernel_args *args)
{
	gPlatform = args->arch_args.machine_platform;

	debug_early_boot_message("machine_platform: ");
	switch (gPlatform) {
		case kPlatformMNative:
			debug_early_boot_message("Native mmode hooks\n");
			break;
		case kPlatformSbi:
			debug_early_boot_message("SBI\n");
			break;
		default:
			debug_early_boot_message("?\n");
			break;
	}

	gFDT = args->arch_args.fdt;

	gHtifRegs  = (HtifRegs *volatile)args->arch_args.htif.start;
	gPlicRegs  = (PlicRegs *volatile)args->arch_args.plic.start;
	gClintRegs = (ClintRegs *volatile)args->arch_args.clint.start;

	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	if (gPlatform == kPlatformSbi) {
		sbiret res;
		res = sbi_get_spec_version();
		dprintf("SBI spec version: %#lx\n", res.value);
		res = sbi_get_impl_id();
		dprintf("SBI implementation ID: %#lx\n", res.value);
		res = sbi_get_impl_version();
		dprintf("SBI implementation version: %#lx\n", res.value);
		res = sbi_get_mvendorid();
		dprintf("SBI vendor ID: %#lx\n", res.value);
		res = sbi_get_marchid();
		dprintf("SBI arch ID: %#lx\n", res.value);
	}
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
