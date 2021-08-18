/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <arch_smp.h>
#include <arch/generic/debug_uart_8250.h>
#if defined(__riscv)
#	include <arch/riscv64/arch_uart_sifive.h>
#elif defined(__ARM__) || defined(__ARM64__)
#	include <arch/arm/arch_uart_pl011.h>
#endif
#include <boot/addr_range.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/uart.h>
#include <string.h>
#include <kernel/kernel.h>

#include <ByteOrder.h>

extern "C" {
#include <libfdt.h>
}

#include "efi_platform.h"
#include "serial.h"


#define INFO(x...) dprintf("efi/fdt: " x)
#define ERROR(x...) dprintf("efi/fdt: " x)


static void* sDtbTable = NULL;
static uint32 sDtbSize = 0;

static uint32 sBootHart = 0;
static uint64 sTimerFrequency = 10000000;

static addr_range sPlic = {0};
static addr_range sClint = {0};


static void WriteString(const char *str) {dprintf("%s", str);}
static void WriteLn() {dprintf("\n");}
static void WriteHex(uint64_t val, int n) {dprintf("%08" B_PRIx64, val);}
static void WriteInt(int64_t val) {dprintf("%" B_PRId64, val);}


template <typename T> DebugUART*
get_uart(addr_t base, int64 clock) {
	static char buffer[sizeof(T)];
	return new(buffer) T(base, clock);
}


const struct supported_uarts {
	const char*	dtb_compat;
	const char*	kind;
	DebugUART*	(*uart_driver_init)(addr_t base, int64 clock);
} kSupportedUarts[] = {
	{ "ns16550a", UART_KIND_8250, &get_uart<DebugUART8250> },
	{ "ns16550", UART_KIND_8250, &get_uart<DebugUART8250> },
#if defined(__riscv)
	{ "sifive,uart0", UART_KIND_SIFIVE, &get_uart<ArchUARTSifive> },
#elif defined(__ARM__) || defined(__ARM64__)
	{ "arm,pl011", UART_KIND_PL011, &get_uart<ArchUARTPL011> },
	{ "arm,primecell", UART_KIND_PL011, &get_uart<ArchUARTPL011> },
#endif
};


static void WriteStringList(const char* prop, size_t size)
{
	bool first = true;
	const char* propEnd = prop + size;
	while (propEnd - prop > 0) {
		if (first) first = false; else WriteString(", ");
		int curLen = strlen(prop);
		WriteString("'");
		WriteString(prop);
		WriteString("'");
		prop += curLen + 1;
	}
}


static void DumpFdt(const void *fdt)
{
	if (!fdt)
		return;

	int err = fdt_check_header(fdt);
	if (err) {
		WriteString("fdt error: ");
		WriteString(fdt_strerror(err));
		WriteLn();
		return;
	}

	WriteString("fdt tree:"); WriteLn();

	int node = -1;
	int depth = -1;
	while ((node = fdt_next_node(fdt, node, &depth)) >= 0 && depth >= 0) {
		for (int i = 0; i < depth; i++) WriteString("  ");
		// WriteInt(node); WriteString(", "); WriteInt(depth); WriteString(": ");
		WriteString("node('");
		WriteString(fdt_get_name(fdt, node, NULL));
		WriteString("')"); WriteLn();
		depth++;
		for (int prop = fdt_first_property_offset(fdt, node); prop >= 0; prop = fdt_next_property_offset(fdt, prop)) {
			int len;
			const struct fdt_property *property = fdt_get_property_by_offset(fdt, prop, &len);
			if (property == NULL) {
				for (int i = 0; i < depth; i++) WriteString("  ");
				WriteString("getting prop at ");
				WriteInt(prop);
				WriteString(": ");
				WriteString(fdt_strerror(len));
				WriteLn();
				break;
			}
			for (int i = 0; i < depth; i++) WriteString("  ");
			WriteString("prop('");
			WriteString(fdt_string(fdt, fdt32_to_cpu(property->nameoff)));
			WriteString("'): ");
			if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "compatible") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "model") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "serial-number") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "status") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "device_type") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "riscv,isa") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "mmu-type") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "format") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bootargs") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "stdout-path") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "reg-names") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "reset-names") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "clock-names") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "clock-output-names") == 0
			) {
				WriteStringList((const char*)property->data, fdt32_to_cpu(property->len));
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "reg") == 0) {
				for (uint64_t *it = (uint64_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 2) {
					if (it != (uint64_t*)property->data) WriteString(", ");
					WriteString("(0x");
					WriteHex(fdt64_to_cpu(*it), 8);
					WriteString(", 0x");
					WriteHex(fdt64_to_cpu(*(it + 1)), 8);
					WriteString(")");
				}
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "phandle") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "clock-frequency") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "timebase-frequency") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#address-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#size-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "#interrupt-cells") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupts") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-parent") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "boot-hartid") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "riscv,ndev") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "value") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "offset") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "regmap") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bank-width") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "width") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "height") == 0 ||
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "stride") == 0
			) {
				WriteInt(fdt32_to_cpu(*(uint32_t*)property->data));
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupts-extended") == 0
			) {
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 2) {
					if (it != (uint32_t*)property->data) WriteString(", ");
					WriteString("(");
					WriteInt(fdt32_to_cpu(*it));
					WriteString(", ");
					WriteInt(fdt32_to_cpu(*(it + 1)));
					WriteString(")");
				}
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "ranges") == 0
			) {
				WriteLn();
				depth++;
				// kind
				// child address
				// parent address
				// size
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 7) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					uint32_t kind = fdt32_to_cpu(*(it + 0));
					switch (kind & 0x03000000) {
					case 0x00000000: WriteString("CONFIG"); break;
					case 0x01000000: WriteString("IOPORT"); break;
					case 0x02000000: WriteString("MMIO"); break;
					case 0x03000000: WriteString("MMIO_64BIT"); break;
					}
					WriteString(" (0x"); WriteHex(kind, 8);
					WriteString("), ");
					WriteString("child: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 1)), 8);
					WriteString(", ");
					WriteString("parent: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 3)), 8);
					WriteString(", ");
					WriteString("len: 0x"); WriteHex(fdt64_to_cpu(*(uint64_t*)(it + 5)), 8);
					WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bus-range") == 0) {
				uint32_t *it = (uint32_t*)property->data;
				WriteInt(fdt32_to_cpu(*it));
				WriteString(", ");
				WriteInt(fdt32_to_cpu(*(it + 1)));
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map-mask") == 0) {
				WriteLn();
				depth++;
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it++) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(uint32_t*)it), 8);
					WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map") == 0) {
				WriteLn();
				depth++;
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 6) {
					for (int i = 0; i < depth; i++) WriteString("  ");
					// child unit address
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 0)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 1)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 2)), 8);
					WriteString(", ");
					WriteString("0x"); WriteHex(fdt32_to_cpu(*(it + 3)), 8);

					WriteString(", bus: "); WriteInt(fdt32_to_cpu(*(it + 0)) / (1 << 16) % (1 << 8));
					WriteString(", dev: "); WriteInt(fdt32_to_cpu(*(it + 0)) / (1 << 11) % (1 << 5));
					WriteString(", fn: "); WriteInt(fdt32_to_cpu(*(it + 0)) % (1 << 3));

					WriteString(", childIrq: ");
					// child interrupt specifier
					WriteInt(fdt32_to_cpu(*(it + 3)));
					WriteString(", parentIrq: (");
					// interrupt-parent
					WriteInt(fdt32_to_cpu(*(it + 4)));
					WriteString(", ");
					WriteInt(fdt32_to_cpu(*(it + 5)));
					WriteString(")");
					WriteLn();
					if (((it - (uint32_t*)property->data) / 6) % 4 == 3 && ((uint8_t*)(it + 6) - (uint8_t*)property->data < fdt32_to_cpu(property->len)))
						WriteLn();
				}
				for (int i = 0; i < depth; i++) WriteString("  ");
				depth--;
			} else {
				WriteString("?");
			}
			WriteString(" (len ");
			WriteInt(fdt32_to_cpu(property->len));
			WriteString(")"); WriteLn();
/*
			dump_hex(property->data, fdt32_to_cpu(property->len), depth);
*/
		}
		depth--;
	}
}



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
GetReg(const void* fdt, int node, uint32 addressCells, uint32 sizeCells, size_t idx, addr_range& range)
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
GetInterrupt(const void* fdt, int node, uint32 interruptCells)
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
	if (strcmp(name, "chosen") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "boot-hartid", NULL))
			sBootHart = fdt32_to_cpu(*prop);
	} else if (strcmp(name, "cpus") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "timebase-frequency", NULL))
			sTimerFrequency = fdt32_to_cpu(*prop);
	}

	const char* deviceType = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);

	if (deviceType != NULL) {
		if (strcmp(deviceType, "cpu") == 0) {
			CpuInfo* info;
			arch_smp_register_cpu(&info);
			if (info == NULL)
				return;
			info->id = fdt32_to_cpu(*(uint32*)fdt_getprop(fdt, node, "reg", NULL));
			dprintf("cpu\n");
			dprintf("  id: %" B_PRIu32 "\n", info->id);
		}
	}

	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);

	if (compatible == NULL)
		return;

	if (HasFdtString(compatible, compatibleLen, "riscv,clint0")) {
		GetReg(fdt, node, addressCells, sizeCells, 0, sClint);
		return;
	}

	if (HasFdtString(compatible, compatibleLen, "riscv,plic0")
		|| HasFdtString(compatible, compatibleLen, "sifive,plic-1.0.0")) {
		GetReg(fdt, node, addressCells, sizeCells, 0, sPlic);
		return;
	}

	// TODO: We should check for the "chosen" uart and prioritize that one

	uart_info &uart = gKernelArgs.arch_args.uart;
	if (uart.kind[0] != 0)
		return;

	// check for a uart if we don't have one
	for (uint32 i = 0; i < B_COUNT_OF(kSupportedUarts); i++) {
		if (HasFdtString(compatible, compatibleLen,
				kSupportedUarts[i].dtb_compat)) {

			memcpy(uart.kind, kSupportedUarts[i].kind,
				sizeof(uart.kind));

			GetReg(fdt, node, addressCells, sizeCells, 0, uart.regs);
			uart.irq = GetInterrupt(fdt, node, interruptCells);
			const void* prop = fdt_getprop(fdt, node, "clock-frequency", NULL);

			uart.clock = (prop == NULL) ? 0 : fdt32_to_cpu(*(uint32*)prop);

			gUART = kSupportedUarts[i].uart_driver_init(uart.regs.start,
				uart.clock);
		}
	}

	if (gUART != NULL)
		gUART->InitEarly();
}


void
dtb_init()
{
	efi_configuration_table *table = kSystemTable->ConfigurationTable;
	size_t entries = kSystemTable->NumberOfTableEntries;

	// Ensure uart is empty before we scan for one
	memset(&gKernelArgs.arch_args.uart, 0, sizeof(uart_info));

	INFO("Probing for device trees from UEFI...\n");

	// Try to find an FDT
	for (uint32 i = 0; i < entries; i++) {
		if (!table[i].VendorGuid.equals(DEVICE_TREE_GUID))
			continue;

		void* dtbPtr = (void*)(table[i].VendorTable);

		int res = fdt_check_header(dtbPtr);
		if (res != 0) {
			ERROR("Invalid FDT from UEFI table %d: %s\n", i, fdt_strerror(res));
			continue;
		}

		sDtbTable = dtbPtr;
		sDtbSize = fdt_totalsize(dtbPtr);
	
		INFO("Valid FDT from UEFI table %d, size: %" B_PRIu32 "\n", i, sDtbSize);

		if (false)
			DumpFdt(sDtbTable);

		int node = -1;
		int depth = -1;
		while ((node = fdt_next_node(sDtbTable, node, &depth)) >= 0 && depth >= 0) {
			HandleFdt(sDtbTable, node, 2, 2, 1);
		}
		break;
	}
}


void
dtb_set_kernel_args()
{
	// pack into proper location if the architecture cares
	if (sDtbTable != NULL) {
		#if defined(__ARM__) || defined(__riscv)
		gKernelArgs.arch_args.fdt = kernel_args_malloc(sDtbSize);
		if (gKernelArgs.arch_args.fdt != NULL)
			memcpy(gKernelArgs.arch_args.fdt, sDtbTable, sDtbSize);
		else
			ERROR("unable to malloc for fdt!\n");
		#endif
	}

#ifdef __riscv
	dprintf("bootHart: %" B_PRIu32 "\n", sBootHart);
	gKernelArgs.arch_args.bootHart = sBootHart;
	dprintf("timerFrequency: %" B_PRIu64 "\n", sTimerFrequency);
	gKernelArgs.arch_args.timerFrequency = sTimerFrequency;

//	gKernelArgs.arch_args.htif  = {.start = 0x40008000, .size = 0x10};
	gKernelArgs.arch_args.htif  = {.start = 0, .size = 0};
	gKernelArgs.arch_args.plic  = sPlic;
	gKernelArgs.arch_args.clint = sClint;
#endif
#if defined(__ARM__) || defined(__riscv)
	uart_info &uart = gKernelArgs.arch_args.uart;
	dprintf("Chosen UART:\n");
	if (uart.kind[0] == 0) {
		dprintf("kind: None!\n");
	} else {
		dprintf("  kind: %s", uart.kind);
		dprintf("\n");
		dprintf("  regs: %#" B_PRIx64 ", %#" B_PRIx64 "\n", uart.regs.start, uart.regs.size);
		dprintf("  irq: %" B_PRIu32 "\n", uart.irq);
		dprintf("  clock: %" B_PRIu64 "\n", uart.clock);
	}
#endif
}
