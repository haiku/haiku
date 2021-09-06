/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "fdt.h"
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <KernelExport.h>
#include <boot/stage2.h>
#include <arch/generic/debug_uart_8250.h>
#include <arch/riscv64/arch_uart_sifive.h>
#include <arch_cpu_defs.h>

extern "C" {
#include <libfdt.h>
}

#include "mmu.h"
#include "smp.h"
#include "graphics.h"
#include "virtio.h"
#include "Htif.h"
#include "Clint.h"
#include "FwCfg.h"


void* gFdt = NULL;
ClintRegs *volatile gClintRegs = NULL;

static uint64 sTimerFrequrency = 10000000;

static addr_range sPlic = {0};
static addr_range sClint = {0};
static uart_info sUart{};


static bool
HasFdtString(const char* prop, int size, const char* pattern)
{
	int patternLen = strlen(pattern);
	const char* propEnd = prop + size;
	while (propEnd - prop > 0) {
		int curLen = strlen(prop);
		if (curLen == patternLen && memcmp(prop, pattern, curLen + 1) == 0)
			return true;
		prop += curLen + 1;
	}
	return false;
}


static bool
GetReg(const void* fdt, int node, uint32 addressCells, uint32 sizeCells, size_t idx,
	addr_range& range)
{
	int propSize;
	const uint8* prop = (const uint8*)fdt_getprop(fdt, node, "reg", &propSize);
	if (prop == NULL)
		return false;

	size_t entrySize = 4*(addressCells + sizeCells);
	if ((idx + 1)*entrySize > (size_t)propSize)
		return false;

	prop += idx*entrySize;

	switch (addressCells) {
		case 1: range.start = fdt32_to_cpu(*(uint32*)prop); prop += 4; break;
		case 2: range.start = fdt64_to_cpu(*(uint64*)prop); prop += 8; break;
		default: panic("unsupported addressCells");
	}
	switch (sizeCells) {
		case 1: range.size = fdt32_to_cpu(*(uint32*)prop); prop += 4; break;
		case 2: range.size = fdt64_to_cpu(*(uint64*)prop); prop += 8; break;
		default: panic("unsupported sizeCells");
	}
	return true;
}


static uint32
GetInterrupt(const void* fdt, int node)
{
	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", NULL)) {
		return fdt32_to_cpu(*(prop + 1));
	}
	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts", NULL)) {
		return fdt32_to_cpu(*prop);
	}
	dprintf("[!] no interrupt field\n");
	return 0;
}


static void
HandleFdt(const void* fdt, int node, uint32 addressCells, uint32 sizeCells,
	uint32 interruptCells /* from parent node */)
{
	// TODO: handle different field sizes

	const char* name = fdt_get_name(fdt, node, NULL);
	if (strcmp(name, "cpus") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "timebase-frequency", NULL))
			sTimerFrequrency = fdt32_to_cpu(*prop);
	}

	const char* device_type = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);
	if (device_type != NULL && strcmp(device_type, "memory") == 0) {
		gMemBase = (uint8*)fdt64_to_cpu(*((uint64*)fdt_getprop(fdt, node,
			"reg", NULL) + 0));
		gTotalMem = fdt64_to_cpu(*((uint64*)fdt_getprop(fdt, node,
			"reg", NULL) + 1));
		return;
	}
	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);
	if (compatible == NULL) return;
	if (HasFdtString(compatible, compatibleLen, "riscv,clint0")) {
		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		sClint.start = fdt64_to_cpu(*(reg + 0));
		sClint.size  = fdt64_to_cpu(*(reg + 1));
		gClintRegs = (ClintRegs*)sClint.start;
	} else if (HasFdtString(compatible, compatibleLen, "riscv,plic0")) {
		GetReg(fdt, node, addressCells, sizeCells, 0, sPlic);
		int propSize;
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", &propSize)) {
			dprintf("PLIC contexts\n");
			uint32 contextId = 0;
			for (uint32 *it = prop; (uint8_t*)it - (uint8_t*)prop < propSize; it += 2) {
				uint32 phandle = fdt32_to_cpu(*it);
				uint32 interrupt = fdt32_to_cpu(*(it + 1));
				if (interrupt == sExternInt) {
					CpuInfo* cpuInfo = smp_find_cpu(phandle);
					dprintf("  context %" B_PRIu32 ": %" B_PRIu32 "\n", contextId, phandle);
					if (cpuInfo != NULL) {
						cpuInfo->plicContext = contextId;
						dprintf("    hartId: %" B_PRIu32 "\n", cpuInfo->hartId);
					}
				}
				contextId++;
			}
		}
	} else if (HasFdtString(compatible, compatibleLen, "virtio,mmio")) {
		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		virtio_register(
			fdt64_to_cpu(*(reg + 0)), fdt64_to_cpu(*(reg + 1)),
			GetInterrupt(fdt, node));
	} else if (
		strcmp(sUart.kind, "") == 0 && (
			HasFdtString(compatible, compatibleLen, "ns16550a") ||
			HasFdtString(compatible, compatibleLen, "sifive,uart0"))
	) {
		if (HasFdtString(compatible, compatibleLen, "ns16550a"))
			strcpy(sUart.kind, UART_KIND_8250);
		else if (HasFdtString(compatible, compatibleLen, "sifive,uart0"))
			strcpy(sUart.kind, UART_KIND_SIFIVE);

		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		sUart.regs.start = fdt64_to_cpu(*(reg + 0));
		sUart.regs.size  = fdt64_to_cpu(*(reg + 1));
		sUart.irq = GetInterrupt(fdt, node);
		const void* prop = fdt_getprop(fdt, node, "clock-frequency", NULL);
		sUart.clock = (prop == NULL) ? 0 : fdt32_to_cpu(*(uint32*)prop);
	} else if (HasFdtString(compatible, compatibleLen, "qemu,fw-cfg-mmio")) {
		gFwCfgRegs = (FwCfgRegs *volatile)
			fdt64_to_cpu(*(uint64*)fdt_getprop(fdt, node, "reg", NULL));
	} else if (HasFdtString(compatible, compatibleLen, "simple-framebuffer")) {
		gFramebuf.colors = (uint32*)fdt64_to_cpu(
			*(uint64*)fdt_getprop(fdt, node, "reg", NULL));
		gFramebuf.stride = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "stride", NULL)) / 4;
		gFramebuf.width = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "width", NULL));
		gFramebuf.height = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "height", NULL));
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
	int depth = -1;
	while ((node = fdt_next_node(gFdt, node, &depth)) >= 0 && depth >= 0) {
		HandleFdt(gFdt, node, 2, 2, 1);
	}
}


void
fdt_set_kernel_args()
{
	uint32_t fdtSize = fdt_totalsize(gFdt);

	// libfdt requires 8-byte alignment
	gKernelArgs.arch_args.fdt = (void*)(addr_t)kernel_args_malloc(fdtSize, 8);

	if (gKernelArgs.arch_args.fdt != NULL)
		memcpy(gKernelArgs.arch_args.fdt, gFdt, fdt_totalsize(gFdt));
	else
		panic("unable to malloc for FDT!\n");

	gKernelArgs.arch_args.timerFrequency = sTimerFrequrency;

	gKernelArgs.arch_args.htif.start = (addr_t)gHtifRegs;
	gKernelArgs.arch_args.htif.size = sizeof(HtifRegs);

	gKernelArgs.arch_args.plic  = sPlic;
	gKernelArgs.arch_args.clint = sClint;
	gKernelArgs.arch_args.uart  = sUart;
}
