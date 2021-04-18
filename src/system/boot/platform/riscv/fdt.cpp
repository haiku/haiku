/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "fdt.h"
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <KernelExport.h>
#include <boot/stage2.h>

extern "C" {
#include <libfdt.h>
}

#include "mmu.h"
#include "graphics.h"
#include "virtio.h"


void* gFdt = NULL;


static void
HandleFdt(const void* fdt, int node, uint32_t addressCells, uint32_t sizeCells,
	uint32_t interruptCells /* from parent node */)
{
	// TODO: handle different field sizes

	const char* device_type = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);
	if (device_type != NULL && strcmp(device_type, "memory") == 0) {
		gMemBase = (uint8*)fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node,
			"reg", NULL) + 0));
		gTotalMem = fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node,
			"reg", NULL) + 1));
		return;
	}
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", NULL);
	if (compatible == NULL) return;
	if (strcmp(compatible, "virtio,mmio") == 0) {
		virtio_register(
			fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node, "reg", NULL) + 0)),
			fdt64_to_cpu(*((uint64_t*)fdt_getprop(fdt, node, "reg", NULL) + 1)),
			fdt32_to_cpu(*((uint32_t*)fdt_getprop(fdt, node,
				"interrupts-extended", NULL) + 1))
		);
	} else if (strcmp(compatible, "simple-framebuffer") == 0) {
		gFramebuf.colors = (uint32_t*)fdt64_to_cpu(
			*(uint64_t*)fdt_getprop(fdt, node, "reg", NULL));
		gFramebuf.stride = fdt32_to_cpu(
			*(uint32_t*)fdt_getprop(fdt, node, "stride", NULL)) / 4;
		gFramebuf.width = fdt32_to_cpu(
			*(uint32_t*)fdt_getprop(fdt, node, "width", NULL));
		gFramebuf.height = fdt32_to_cpu(
			*(uint32_t*)fdt_getprop(fdt, node, "height", NULL));
	}
}


void
fdt_init(void* fdt)
{
	dprintf("FDT: %p\n", fdt);
	gFdt = fdt;

	int res = fdt_check_header(gFdt);
	if (res != 0) {
		panic("Invalid FDT: %s\n", fdt_strerror(res));
	}

	dprintf("FDT valid, size: %" B_PRIu32 "\n", fdt_totalsize(gFdt));

	int node = -1;
	int depth = 0;
	while ((node = fdt_next_node(gFdt, node, &depth)) >= 0) {
		HandleFdt(gFdt, node, 2, 2, 2);
	}
}


void
fdt_set_kernel_args()
{
	gKernelArgs.arch_args.fdt = kernel_args_malloc(fdt_totalsize(gFdt));
	if (gKernelArgs.arch_args.fdt != NULL) {
		memcpy(gKernelArgs.arch_args.fdt, gFdt, fdt_totalsize(gFdt));
	} else
		panic("unable to malloc for FDT!\n");
}
