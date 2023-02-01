/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */


// TODO: split arch-depending code to per-arch source

#include <arch_cpu_defs.h>
#include <arch_dtb.h>
#include <arch_smp.h>
#include <arch/generic/debug_uart_8250.h>
#if defined(__riscv)
#	include <arch/riscv64/arch_uart_sifive.h>
#elif defined(__ARM__)
#	include <arch/arm/arch_uart_pl011.h>
#elif defined(__aarch64__)
#	include <arch/arm/arch_uart_pl011.h>
#	include <arch/arm64/arch_uart_linflex.h>
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


#define GIC_INTERRUPT_CELL_TYPE		0
#define GIC_INTERRUPT_CELL_ID		1
#define GIC_INTERRUPT_CELL_FLAGS	2
#define GIC_INTERRUPT_TYPE_SPI		0
#define GIC_INTERRUPT_TYPE_PPI		1
#define GIC_INTERRUPT_BASE_SPI		32
#define GIC_INTERRUPT_BASE_PPI		16


//#define TRACE_DUMP_FDT


#define INFO(x...) dprintf("efi/fdt: " x)
#define ERROR(x...) dprintf("efi/fdt: " x)


static void* sDtbTable = NULL;
static uint32 sDtbSize = 0;

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
	{ "snps,dw-apb-uart", UART_KIND_8250, &get_uart<DebugUART8250> },
#if defined(__riscv)
	{ "sifive,uart0", UART_KIND_SIFIVE, &get_uart<ArchUARTSifive> },
#elif defined(__ARM__)
	{ "arm,pl011", UART_KIND_PL011, &get_uart<ArchUARTPL011> },
	{ "brcm,bcm2835-aux-uart", UART_KIND_8250, &get_uart<DebugUART8250> },
#elif defined(__aarch64__)
	{ "arm,pl011", UART_KIND_PL011, &get_uart<ArchUARTPL011> },
	{ "fsl,s32-linflexuart", UART_KIND_LINFLEX, &get_uart<ArchUARTlinflex> },
	{ "brcm,bcm2835-aux-uart", UART_KIND_8250, &get_uart<DebugUART8250> },
#endif
};


#ifdef TRACE_DUMP_FDT
static void
write_string_list(const char* prop, size_t size)
{
	bool first = true;
	const char* propEnd = prop + size;
	while (propEnd - prop > 0) {
		if (first)
			first = false;
		else
			dprintf(", ");
		int curLen = strlen(prop);
		dprintf("'%s'", prop);
		prop += curLen + 1;
	}
}


static void
dump_fdt(const void *fdt)
{
	if (!fdt)
		return;

	int err = fdt_check_header(fdt);
	if (err) {
		dprintf("fdt error: %s\n", fdt_strerror(err));
		return;
	}

	dprintf("fdt tree:\n");

	int node = -1;
	int depth = -1;
	while ((node = fdt_next_node(fdt, node, &depth)) >= 0 && depth >= 0) {
		for (int i = 0; i < depth; i++)
			dprintf("  ");
		// WriteInt(node); WriteString(", "); WriteInt(depth); WriteString(": ");
		dprintf("node('%s')\n", fdt_get_name(fdt, node, NULL));
		depth++;
		for (int prop = fdt_first_property_offset(fdt, node); prop >= 0; prop = fdt_next_property_offset(fdt, prop)) {
			int len;
			const struct fdt_property *property = fdt_get_property_by_offset(fdt, prop, &len);
			if (property == NULL) {
				for (int i = 0; i < depth; i++)
					dprintf("  ");
				dprintf("getting prop at %d: %s\n", prop, fdt_strerror(len));
				break;
			}
			for (int i = 0; i < depth; i++)
				dprintf("  ");
			dprintf("prop('%s'): ", fdt_string(fdt, fdt32_to_cpu(property->nameoff)));
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
				write_string_list((const char*)property->data, fdt32_to_cpu(property->len));
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "reg") == 0) {
				for (uint64_t *it = (uint64_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 2) {
					if (it != (uint64_t*)property->data)
						dprintf(", ");
					dprintf("(0x%08" B_PRIx64 ", 0x%08" B_PRIx64 ")",
						fdt64_to_cpu(*it), fdt64_to_cpu(*(it + 1)));
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
				dprintf("%" B_PRId32, fdt32_to_cpu(*(uint32_t*)property->data));
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupts-extended") == 0
			) {
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 2) {
					if (it != (uint32_t*)property->data)
						dprintf(", ");
					dprintf("(%" B_PRId32 ", %" B_PRId32 ")",
						fdt32_to_cpu(*it), fdt32_to_cpu(*(it + 1)));
				}
			} else if (
				strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "ranges") == 0
			) {
				dprintf("\n");
				depth++;
				// kind
				// child address
				// parent address
				// size
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it += 7) {
					for (int i = 0; i < depth; i++)
						dprintf("  ");
					uint32_t kind = fdt32_to_cpu(*(it + 0));
					switch (kind & 0x03000000) {
					case 0x00000000: dprintf("CONFIG"); break;
					case 0x01000000: dprintf("IOPORT"); break;
					case 0x02000000: dprintf("MMIO"); break;
					case 0x03000000: dprintf("MMIO_64BIT"); break;
					}
					dprintf(" (0x%08" PRIx32 "), child: 0x%08" PRIx64 ", parent: 0x%08" PRIx64 ", len: 0x%08" PRIx64 "\n",
						kind, fdt64_to_cpu(*(uint64_t*)(it + 1)), fdt64_to_cpu(*(uint64_t*)(it + 3)), fdt64_to_cpu(*(uint64_t*)(it + 5)));
				}
				for (int i = 0; i < depth; i++)
					dprintf("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "bus-range") == 0) {
				uint32_t *it = (uint32_t*)property->data;
				dprintf("%" PRId32 ", %" PRId32, fdt32_to_cpu(*it), fdt32_to_cpu(*(it + 1)));
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map-mask") == 0) {
				dprintf("\n");
				depth++;
				for (uint32_t *it = (uint32_t*)property->data; (uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len); it++) {
					for (int i = 0; i < depth; i++)
						dprintf("  ");
					dprintf("0x%08" PRIx32 "\n", fdt32_to_cpu(*(uint32_t*)it));
				}
				for (int i = 0; i < depth; i++)
					dprintf("  ");
				depth--;
			} else if (strcmp(fdt_string(fdt, fdt32_to_cpu(property->nameoff)), "interrupt-map") == 0) {
				dprintf("\n");
				depth++;

				int addressCells = 3;
				int interruptCells = 1;
				int phandleCells = 1;

				uint32 *prop;

				prop = (uint32*)fdt_getprop(fdt, node, "#address-cells", NULL);
				if (prop != NULL)
					addressCells = fdt32_to_cpu(*prop);

				prop = (uint32*)fdt_getprop(fdt, node, "#interrupt-cells", NULL);
				if (prop != NULL)
					interruptCells = fdt32_to_cpu(*prop);

				uint32_t *it = (uint32_t*)property->data;
				while ((uint8_t*)it - (uint8_t*)property->data < fdt32_to_cpu(property->len)) {
					for (int i = 0; i < depth; i++)
						dprintf("  ");

					uint32 childAddr = fdt32_to_cpu(*it);
					dprintf("childAddr: ");
					for (int i = 0; i < addressCells; i++) {
						dprintf("0x%08" PRIx32 " ", fdt32_to_cpu(*it));
						it++;
					}

					uint8 bus = childAddr / (1 << 16) % (1 << 8);
					uint8 dev = childAddr / (1 << 11) % (1 << 5);
					uint8 func = childAddr % (1 << 3);
					dprintf("(bus: %" PRId32 ", dev: %" PRId32 ", fn: %" PRId32 "), ",
						bus, dev, func);

					dprintf("childIrq: ");
					for (int i = 0; i < interruptCells; i++) {
						dprintf("%" PRIu32 " ", fdt32_to_cpu(*it));
						it++;
					}

					uint32 parentPhandle = fdt32_to_cpu(*it);
					it += phandleCells;
					dprintf("parentPhandle: %" PRId32 ", ", parentPhandle);

					int parentAddressCells = 0;
					int parentInterruptCells = 1;

					int parentNode = fdt_node_offset_by_phandle(fdt, parentPhandle);
					if (parentNode >= 0) {
						prop = (uint32*)fdt_getprop(fdt, parentNode, "#address-cells", NULL);
						if (prop != NULL)
							parentAddressCells = fdt32_to_cpu(*prop);

						prop = (uint32*)fdt_getprop(fdt, parentNode, "#interrupt-cells", NULL);
						if (prop != NULL)
							parentInterruptCells = fdt32_to_cpu(*prop);
					}

					dprintf("parentAddress: ");
					for (int i = 0; i < parentAddressCells; i++) {
						dprintf("%" PRIu32 " ", fdt32_to_cpu(*it));
						it++;
					}

					dprintf("parentIrq: ");
					for (int i = 0; i < parentInterruptCells; i++) {
						dprintf("%" PRIu32 " ", fdt32_to_cpu(*it));
						it++;
					}

					dprintf("\n");
				}
				for (int i = 0; i < depth; i++)
					dprintf("  ");
				depth--;
			} else {
				dprintf("?");
			}
			dprintf(" (len %" PRId32 ")\n", fdt32_to_cpu(property->len));
/*
			dump_hex(property->data, fdt32_to_cpu(property->len), depth);
*/
		}
		depth--;
	}
}
#endif


bool
dtb_has_fdt_string(const char* prop, int size, const char* pattern)
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


uint32
dtb_get_address_cells(const void* fdt, int node)
{
	uint32 res = 2;

	int parent = fdt_parent_offset(fdt, node);
	if (parent < 0)
		return res;

	uint32 *prop = (uint32*)fdt_getprop(fdt, parent, "#address-cells", NULL);
	if (prop == NULL)
		return res;

	res = fdt32_to_cpu(*prop);
	return res;
}


uint32
dtb_get_size_cells(const void* fdt, int node)
{
	uint32 res = 1;

	int parent = fdt_parent_offset(fdt, node);
	if (parent < 0)
		return res;

	uint32 *prop = (uint32*)fdt_getprop(fdt, parent, "#size-cells", NULL);
	if (prop == NULL)
		return res;

	res = fdt32_to_cpu(*prop);
	return res;
}


bool
dtb_get_reg(const void* fdt, int node, size_t idx, addr_range& range)
{
	uint32 addressCells = dtb_get_address_cells(fdt, node);
	uint32 sizeCells = dtb_get_size_cells(fdt, node);

	int propSize;
	const uint8* prop = (const uint8*)fdt_getprop(fdt, node, "reg", &propSize);
	if (prop == NULL)
		return false;

	size_t entrySize = 4 * (addressCells + sizeCells);
	if ((idx + 1) * entrySize > (size_t)propSize)
		return false;

	prop += idx * entrySize;

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

	int parent = fdt_parent_offset(fdt, node);
	if (parent >= 0) {
		uint32 parentAddressCells = dtb_get_address_cells(fdt, parent);

		uint32 rangesSize = 0;
		uint32 *ranges = (uint32 *)fdt_getprop(fdt, parent, "ranges", (int *)&rangesSize);
		if (ranges == NULL)
			return true;

		uint32 rangesPos = 0;
		while (rangesSize >= (rangesPos + parentAddressCells + addressCells + sizeCells)) {
			addr_t childAddress;
			addr_t parentAddress;
			size_t rangeSize;

			if (addressCells == 1) {
				childAddress = fdt32_to_cpu(*(uint32*)(ranges+rangesPos));
			} else {
				childAddress = fdt64_to_cpu(*(uint64*)(ranges+rangesPos));
			}
			rangesPos += addressCells;

			if (parentAddressCells == 1) {
				parentAddress = fdt32_to_cpu(*(uint32*)(ranges+rangesPos));
			} else {
				parentAddress = fdt64_to_cpu(*(uint64*)(ranges+rangesPos));
			}
			rangesPos += parentAddressCells;

			if (sizeCells == 1) {
				rangeSize = fdt32_to_cpu(*(uint32*)(ranges+rangesPos));
			} else {
				rangeSize = fdt64_to_cpu(*(uint64*)(ranges+rangesPos));
			}
			rangesPos += sizeCells;

			if ((range.start >= childAddress) && (range.start <= childAddress + rangeSize)) {
				range.start -= childAddress;
				range.start += parentAddress;
				break;
			}
		}
	}

	return true;
}


static int
dtb_get_interrupt_parent(const void* fdt, int node)
{
	while (node >= 0) {
		uint32* prop;
		prop = (uint32*)fdt_getprop(fdt, node, "interrupt-parent", NULL);
		if (prop != NULL) {
			uint32_t phandle = fdt32_to_cpu(*prop);
			return fdt_node_offset_by_phandle(fdt, phandle);
		}

		node = fdt_parent_offset(fdt, node);
	}

	return -1;
}


static uint32
dtb_get_interrupt_cells(const void* fdt, int node)
{
	int intc_node = dtb_get_interrupt_parent(fdt, node);
	if (intc_node >= 0) {
		uint32* prop = (uint32*)fdt_getprop(fdt, intc_node, "#interrupt-cells", NULL);
		if (prop != NULL) {
			return fdt32_to_cpu(*prop);
		}
	}

	return 1;
}


uint32
dtb_get_interrupt(const void* fdt, int node)
{
	uint32 interruptCells = dtb_get_interrupt_cells(fdt, node);

	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", NULL)) {
		return fdt32_to_cpu(*(prop + 1));
	}
	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts", NULL)) {
		if ((interruptCells == 1) || (interruptCells == 2)) {
			return fdt32_to_cpu(*prop);
		} else if (interruptCells == 3) {
			uint32 interruptType = fdt32_to_cpu(prop[GIC_INTERRUPT_CELL_TYPE]);
			uint32 interruptNumber = fdt32_to_cpu(prop[GIC_INTERRUPT_CELL_ID]);
			if (interruptType == GIC_INTERRUPT_TYPE_SPI)
				interruptNumber += GIC_INTERRUPT_BASE_SPI;
			else if (interruptType == GIC_INTERRUPT_TYPE_PPI)
				interruptNumber += GIC_INTERRUPT_BASE_PPI;

			return interruptNumber;
		} else {
			panic("unsupported interruptCells");
		}
	}
	dprintf("[!] no interrupt field\n");
	return 0;
}


static int64
dtb_get_clock_frequency(const void* fdt, int node)
{
	uint32* prop;
	int len = 0;

	prop = (uint32*)fdt_getprop(fdt, node, "clock-frequency", NULL);
	if (prop != NULL) {
		return fdt32_to_cpu(*prop);
	}

	prop = (uint32*)fdt_getprop(fdt, node, "clocks", &len);
	if (prop != NULL) {
		uint32_t phandle = fdt32_to_cpu(*prop);
		int offset = fdt_node_offset_by_phandle(fdt, phandle);
		if (offset > 0) {
			uint32* prop2 = (uint32*)fdt_getprop(fdt, offset, "clock-frequency", NULL);
			if (prop2 != NULL) {
				return fdt32_to_cpu(*prop2);
			}
		}
	}

	return 0;
}


static void
dtb_handle_fdt(const void* fdt, int node)
{
	arch_handle_fdt(fdt, node);

	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);

	if (compatible == NULL)
		return;

	// check for a uart if we don't have one
	uart_info &uart = gKernelArgs.arch_args.uart;
	if (uart.kind[0] == 0) {
		for (uint32 i = 0; i < B_COUNT_OF(kSupportedUarts); i++) {
			if (dtb_has_fdt_string(compatible, compatibleLen,
					kSupportedUarts[i].dtb_compat)) {

				memcpy(uart.kind, kSupportedUarts[i].kind,
					sizeof(uart.kind));

				dtb_get_reg(fdt, node, 0, uart.regs);
				uart.irq = dtb_get_interrupt(fdt, node);
				uart.clock = dtb_get_clock_frequency(fdt, node);

				gUART = kSupportedUarts[i].uart_driver_init(uart.regs.start,
					uart.clock);
			}
		}

		if (gUART != NULL)
			gUART->InitEarly();
	}
}


static void
dtb_handle_chosen_node(const void *fdt)
{
	int chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen < 0)
		return;

	int len;
	const char *stdoutPath = (const char *)fdt_getprop(fdt, chosen, "stdout-path", &len);
	if (stdoutPath == NULL)
		return;

	// stdout-path can optionally contain a ":" separator character
	// The part after the ":" character specifies the UART configuration
	// We can ignore it here as the UART should be already initialized
	// by the UEFI firmware (e.g. U-Boot or TianoCore)

	char *separator = strchr(stdoutPath, ':');
	int namelen = (separator == NULL) ? len - 1 : separator - stdoutPath;

	int stdoutNode = fdt_path_offset_namelen(fdt, stdoutPath, namelen);
	if (stdoutNode < 0)
		return;

	dtb_handle_fdt(fdt, stdoutNode);
}


void
dtb_init()
{
	efi_configuration_table *table = kSystemTable->ConfigurationTable;
	size_t entries = kSystemTable->NumberOfTableEntries;

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

#ifdef TRACE_DUMP_FDT
		dump_fdt(sDtbTable);
#endif

		dtb_handle_chosen_node(sDtbTable);

		int node = -1;
		int depth = -1;
		while ((node = fdt_next_node(sDtbTable, node, &depth)) >= 0 && depth >= 0) {
			dtb_handle_fdt(sDtbTable, node);
		}
		break;
	}
}


void
dtb_set_kernel_args()
{
	// pack into proper location if the architecture cares
	if (sDtbTable != NULL) {
		// libfdt requires 8-byte alignment
		gKernelArgs.arch_args.fdt = (void*)(addr_t)kernel_args_malloc(sDtbSize, 8);

		if (gKernelArgs.arch_args.fdt != NULL)
			memcpy(gKernelArgs.arch_args.fdt, sDtbTable, sDtbSize);
		else
			ERROR("unable to malloc for fdt!\n");
	}

	uart_info &uart = gKernelArgs.arch_args.uart;
	dprintf("Chosen UART:\n");
	if (uart.kind[0] == 0) {
		dprintf("kind: None!\n");
	} else {
		dprintf("  kind: %s\n", uart.kind);
		dprintf("  regs: %#" B_PRIx64 ", %#" B_PRIx64 "\n", uart.regs.start, uart.regs.size);
		dprintf("  irq: %" B_PRIu32 "\n", uart.irq);
		dprintf("  clock: %" B_PRIu64 "\n", uart.clock);
	}

	arch_dtb_set_kernel_args();
}
