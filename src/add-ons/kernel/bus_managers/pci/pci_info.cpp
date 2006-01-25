/* 
** Copyright 2003-2006, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <KernelExport.h>
#define __HAIKU_PCI_BUS_MANAGER_TESTING 1
#include <PCI.h>
#include <string.h>
#include "pci_info.h"
#include "pci_priv.h"
#include "pci.h"

#define PCI_VERBOSE	1
#define USE_PCI_HEADER 1

#if USE_PCI_HEADER
#	include "pcihdr.h"
#endif


void get_vendor_info(uint16 vendorID, const char **venShort, const char **venFull);
void get_device_info(uint16 vendorID, uint16 deviceID, const char **devShort, const char **devFull);
const char *get_class_info(uint8 class_base, uint8 class_sub, uint8 class_api);
const char *get_capability_name(uint8 cap_id);


static void
print_pci2pci_bridge_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h1.subsystem_id, info->u.h1.subsystem_vendor_id));
	TRACE(("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h1.primary_bus, info->u.h1.secondary_bus, info->u.h1.subordinate_bus, info->u.h1.secondary_latency));
	TRACE(("PCI:   io_base_upper_16  %04x, io_base  %02x\n",
			info->u.h1.io_base_upper16, info->u.h1.io_base));
	TRACE(("PCI:   io_limit_upper_16 %04x, io_limit %02x\n",
			info->u.h1.io_limit_upper16, info->u.h1.io_limit));
	TRACE(("PCI:   memory_base %04x, memory_limit %04x\n",
			info->u.h1.memory_base, info->u.h1.memory_limit));
	TRACE(("PCI:   prefetchable_memory_base_upper32  %08lx, prefetchable_memory_base  %04x\n",
		info->u.h1.prefetchable_memory_base_upper32, info->u.h1.prefetchable_memory_base));
	TRACE(("PCI:   prefetchable_memory_limit_upper32 %08lx, prefetchable_memory_limit %04x\n",
		info->u.h1.prefetchable_memory_limit_upper32, info->u.h1.prefetchable_memory_limit));
	TRACE(("PCI:   bridge_control %02x, secondary_status %04x\n",
			info->u.h1.bridge_control, info->u.h1.secondary_status));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x\n",
			info->u.h1.interrupt_line, info->u.h1.interrupt_pin));
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size ??\n",
			info->u.h1.rom_base, info->u.h1.rom_base_pci));
	for (int i = 0; i < 2; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h1.base_registers[i], info->u.h1.base_registers_pci[i],
			info->u.h1.base_register_sizes[i], info->u.h1.base_register_flags[i]));
}


static void
print_pci2cardbus_bridge_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h2.subsystem_id, info->u.h2.subsystem_vendor_id));
	TRACE(("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h2.primary_bus, info->u.h2.secondary_bus, info->u.h2.subordinate_bus, info->u.h2.secondary_latency));
	TRACE(("PCI:   bridge_control %02x, secondary_status %04x\n",
			info->u.h2.bridge_control, info->u.h2.secondary_status));
	TRACE(("PCI:   memory_base_upper32  %08lx, memory_base  %08lx\n",
		info->u.h2.memory_base_upper32, info->u.h2.memory_base));
	TRACE(("PCI:   memory_limit_upper32 %08lx, memory_limit %08lx\n",
		info->u.h2.memory_limit_upper32, info->u.h2.memory_limit));
	TRACE(("PCI:   io_base_upper32  %08lx, io_base  %08lx\n",
		info->u.h2.io_base_upper32, info->u.h2.io_base));
	TRACE(("PCI:   io_limit_upper32 %08lx, io_limit %08lx\n",
		info->u.h2.io_limit_upper32, info->u.h2.io_limit));
}


static void
print_generic_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size %08lx\n",
			info->u.h0.rom_base, info->u.h0.rom_base_pci, info->u.h0.rom_size));
	TRACE(("PCI:   cardbus_CIS %08lx, subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h0.cardbus_cis, info->u.h0.subsystem_id, info->u.h0.subsystem_vendor_id));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x, min_grant %02x, max_latency %02x\n",
			info->u.h0.interrupt_line, info->u.h0.interrupt_pin, info->u.h0.min_grant, info->u.h0.max_latency));
	for (int i = 0; i < 6; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h0.base_registers[i], info->u.h0.base_registers_pci[i],
			info->u.h0.base_register_sizes[i], info->u.h0.base_register_flags[i]));
}


static void
print_capabilities(const pci_info *info)
{
	uint16	status;
	uint8	cap_ptr;
	uint8	cap_id;
	int		i;

	TRACE(("PCI:   Capabilities: "));
	
	status = pci_read_config(info->bus, info->device, info->function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		TRACE(("(not supported)\n"));
		return;
	}

	switch (info->header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			cap_ptr = pci_read_config(info->bus, info->device, info->function, PCI_capabilities_ptr, 1);
			break;
		case PCI_header_type_cardbus:
			cap_ptr = pci_read_config(info->bus, info->device, info->function, PCI_capabilities_ptr_2, 1);
			break;
		default:
			TRACE(("(unknown header type)\n"));
			return;
	}

	cap_ptr &= ~3;
	if (!cap_ptr) {
		TRACE(("(empty list)\n"));
		return;
	}

	for (i = 0; i < 48; i++) {
		const char *name;
		cap_id  = pci_read_config(info->bus, info->device, info->function, cap_ptr, 1);
		cap_ptr = pci_read_config(info->bus, info->device, info->function, cap_ptr + 1, 1);
		cap_ptr &= ~3;
		if (i) {
			TRACE((", "));
		}
		name = get_capability_name(cap_id);
		if (name) {
			TRACE(("%s", name));
		} else {
			TRACE(("0x%02x", cap_id));
		}
		if (!cap_ptr)
			break;
	}
	TRACE(("\n"));
}


static void
print_info_basic(const pci_info *info, bool verbose)
{
	TRACE(("PCI: [dom %d, bus %2d] bus %3d, device %2d, function %2d: vendor %04x, device %04x, revision %02x\n",
	// XXX this works only as long as PCI manager virtual bus mapping isn't changed:
			(info->bus >> 5) /* domain */, (info->bus & 0x1f) /* bus */,
			info->bus, info->device, info->function, info->vendor_id, info->device_id, info->revision));
	TRACE(("PCI:   class_base %02x, class_function %02x, class_api %02x\n",
			info->class_base, info->class_sub, info->class_api));

	if (verbose) {
#if USE_PCI_HEADER
		const char *venShort;
		const char *venFull;
		get_vendor_info(info->vendor_id, &venShort, &venFull);
		if (!venShort && !venFull) {
			TRACE(("PCI:   vendor %04x: Unknown\n", info->vendor_id));
		} else if (venShort && venFull) {
			TRACE(("PCI:   vendor %04x: %s - %s\n", info->vendor_id, venShort, venFull));
		} else {
			TRACE(("PCI:   vendor %04x: %s\n", info->vendor_id, venShort ? venShort : venFull));
		}
		const char *devShort;
		const char *devFull;
		get_device_info(info->vendor_id, info->device_id, &devShort, &devFull);
		if (!devShort && !devFull) {
			TRACE(("PCI:   device %04x: Unknown\n", info->device_id));
		} else if (devShort && devFull) {
			TRACE(("PCI:   device %04x: %s - %s\n", info->device_id, devShort, devFull));
		} else {
			TRACE(("PCI:   device %04x: %s\n", info->device_id, devShort ? devShort : devFull));
		}
#endif
		TRACE(("PCI:   info: %s\n", get_class_info(info->class_base, info->class_sub, info->class_api)));
	}
	TRACE(("PCI:   line_size %02x, latency %02x, header_type %02x, BIST %02x\n",
			info->line_size, info->latency, info->header_type, info->bist));
			
	switch (info->header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
			print_generic_info(info, verbose);
			break;
		case PCI_header_type_PCI_to_PCI_bridge:
			print_pci2pci_bridge_info(info, verbose);
			break;
		case PCI_header_type_cardbus:
			print_pci2cardbus_bridge_info(info, verbose);
			break;
		default:
			TRACE(("PCI:   unknown header type\n"));
	}

	print_capabilities(info);
}


void
pci_print_info()
{
	pci_info info;
	for (long index = 0; B_OK == pci_get_nth_pci_info(index, &info); index++) {
		print_info_basic(&info, PCI_VERBOSE);
	}
}


const char *
get_class_info(uint8 class_base, uint8 class_sub, uint8 class_api)
{
	switch (class_base) {
		case PCI_early:
			switch (class_sub) {
				case PCI_early_not_vga:
					return "Not VGA-compatible pre-2.0 PCI specification device";
				case PCI_early_vga:
					return "VGA-compatible pre-2.0 PCI specification device";
				default:
					return "Unknown pre-2.0 PCI specification device";
			}
			
		case PCI_mass_storage:
			switch (class_sub) {
				case PCI_scsi:
					return "SCSI mass storage controller";
				case PCI_ide:
					return "IDE mass storage controller";
				case PCI_floppy:
					return "Floppy disk controller";
				case PCI_ipi:
					return "IPI mass storage controller";
				case PCI_raid:
					return "RAID mass storage controller";
				case PCI_mass_storage_other:
					return "Other mass storage controller";
				default:
					return "Unknown mass storage controller";
			}

		case PCI_network:
			switch (class_sub) {
				case PCI_ethernet:
					return "Ethernet network controller";
				case PCI_token_ring:
					return "Token ring network controller";
				case PCI_fddi:
					return "FDDI network controller";
				case PCI_atm:
					return "ATM network controller";
				case PCI_isdn:
					return "ISDN network controller";
				case PCI_network_other:
					return "Other network controller";
				default:
					return "Unknown network controller";
			}

		case PCI_display:
			switch (class_sub) {
				case PCI_vga:
					switch (class_api) {
						case 0x00:
							return "VGA-compatible display controller";
						case 0x01:
							return "8514-compatible display controller";
						default:
							return "Unknown VGA display controller";
					}
				case PCI_xga:
					return "XGA display controller";
				case PCI_3d:
					return "3D display controller";
				case PCI_display_other:
					return "Other display controller";
				default:
					return "Unknown display controller";
			}

		case PCI_multimedia:
			switch (class_sub) {
				case PCI_video:
					return "Video multimedia device";
				case PCI_audio:
					return "Audio multimedia device";
				case PCI_telephony:
					return "Computer telephony multimedia device";
				case PCI_multimedia_other:
					return "Other multimedia device";
				default:
					return "Unknown multimedia device";
			}

		case PCI_memory:
			switch (class_sub) {
				case PCI_ram:
					return "RAM memory controller";
				case PCI_flash:
					return "Flash memory controller";
				case PCI_memory_other:
					return "Other memory controller";
				default:
					return "Unknown memory controller";
			}

		case PCI_bridge:
			switch (class_sub) {
				case PCI_host:
					return "Host/PCI bridge device";
				case PCI_isa:
					return "PCI/ISA bridge device";
				case PCI_eisa:
					return "PCI/EISA bridge device";
				case PCI_microchannel:
					return "PCI/Micro Channel bridge device";
				case PCI_pci:
					switch (class_api) {
						case 0x00:
							return "PCI/PCI bridge device";
						case 0x01:
							return "Transparent PCI/PCI bridge device";
						default:
							return "Unknown PCI/PCI bridge device";
					}
				case PCI_pcmcia:
					return "PCI/PCMCIA bridge device";
				case PCI_nubus:
					return "PCI/NuBus bridge device";
				case PCI_cardbus:
					return "PCI/CardBus bridge device";
				case PCI_raceway:
					if (class_api & 1)
						return "PCI/RACEway bridge device, end-point mode";
					else
						return "PCI/RACEway bridge device, transparent mode";
				case PCI_bridge_other:
					return "Other bridge device";
				default:
					return "Unknown bridge device";
			}

		case PCI_simple_communications:
			switch (class_sub) {
				case PCI_serial:
					switch (class_api) {
						case PCI_serial_xt:
							return "Generic XT-compatible serial communications controller";
						case PCI_serial_16450:
							return "16450-compatible serial communications controller";
						case PCI_serial_16550:
							return "16550-compatible serial communications controller";
						case 0x03:
							return "16650-compatible serial communications controller";
						case 0x04:
							return "16750-compatible serial communications controller";
						case 0x05:
							return "16850-compatible serial communications controller";
						case 0x06:
							return "16950-compatible serial communications controller";
						default:
							return "Unknown serial communications controller";
					}
				case PCI_parallel:
					switch (class_api) {
						case PCI_parallel_simple:
							return "Simple parallel port communications controller";
						case PCI_parallel_bidirectional:
							return "Bi-directional parallel port communications controller";
						case PCI_parallel_ecp:
							return "ECP 1.x compliant parallel port communications controller";
						case 0x03:
							return "IEEE 1284 parallel communications controller";
						case 0xfe:
							return "IEEE 1284 parallel communications target device";
						default:
							return "Unknown parallel communications controller";
					}										
				case PCI_multiport_serial:
					return "Multiport serial communications controller";
				case PCI_modem:
					switch (class_api) {
						case 0x00:
							return "Generic modem";
						case 0x01:
							return "Hayes-compatible modem, 16450-compatible interface";
						case 0x02:
							return "Hayes-compatible modem, 16550-compatible interface";
						case 0x03:
							return "Hayes-compatible modem, 16650-compatible interface";
						case 0x04:
							return "Hayes-compatible modem, 16750-compatible interface";
						default:
							return "Unknown modem communications controller";
					}
				case PCI_simple_communications_other:
					return "Other simple communications controller";
				default:
					return "Unknown simple communications controller";
			}

		case PCI_base_peripheral:
			switch (class_sub) {
				case PCI_pic:
					switch (class_api) {
						case PCI_pic_8259:
							return "Generic 8259 programmable interrupt controller (PIC)";
						case PCI_pic_isa:
							return "ISA programmable interrupt controller (PIC)";
						case PCI_pic_eisa:
							return "EISA programmable interrupt controller (PIC)";
						case 0x10:
							return "IO advanced programmable interrupt controller (APIC)";
						case 0x20:
							return "IO(x) advanced programmable interrupt controller (APIC)";
						default:
							return "Unknown programmable interrupt controller (PIC)";
					}
				case PCI_dma:
					switch (class_api) {
						case PCI_dma_8237:
							return "Generic 8237 DMA controller";
						case PCI_dma_isa:
							return "ISA DMA controller";
						case PCI_dma_eisa:
							return "EISA DMA controller";
						default:
							return "Unknown DMA controller";
					}
				case PCI_timer:
					switch (class_api) {
						case PCI_timer_8254:
							return "Generic 8254 timer";
						case PCI_timer_isa:
							return "ISA system timers";
						case PCI_timer_eisa:
							return "EISA system timers";
						default:
							return "Unknown timer";
					}
				case PCI_rtc:
					switch (class_api) {
						case PCI_rtc_generic:
							return "Generic real time clock (RTC) controller";
						case PCI_rtc_isa:
							return "ISA real time clock (RTC) controller";
						default:
							return "Unknown real time clock (RTC) controller";
					}
				case PCI_generic_hot_plug:
					return "Generic PCI Hot-Plug controller";
				case PCI_system_peripheral_other:
					return "Other base system peripheral";
				default:
					return "Unknown base system peripheral";
			}

		case PCI_input:
			switch (class_sub) {
				case PCI_keyboard:
					return "Keyboard controller";
				case PCI_pen:
					return "Digitizer (pen) input device";
				case PCI_mouse:
					return "Mouse controller";
				case PCI_scanner:
					return "Scanner controller";
				case PCI_gameport:
					switch (class_api) {
						case 0x00:
							return "Generic gameport controller";
						case 0x10:
							return "Gameport controller";
						default:
							return "Unknown gameport controller";
					}
				case PCI_input_other:
					return "Other input controller";
				default:
					return "Unknown input controller";
			}

		case PCI_docking_station:
			switch (class_sub) {
				case PCI_docking_generic:
					return "Generic docking station";
				case 0x80:
					return "Other type of docking station";
				default:
					return "Unknown docking station";
			}

		case PCI_processor:
			switch (class_sub) {
				case PCI_386:
					return "386 processor";
				case PCI_486:
					return "486 processor";
				case PCI_pentium:
					return "Pentium processor";
				case PCI_alpha:
					return "Alpha processor";
				case PCI_PowerPC:
					return "PowerPC processor";
				case PCI_mips:
					return "MIPS processor";
				case PCI_coprocessor:
					return "Co-processor";
				default:
					return "Unknown processor";
			}

		case PCI_serial_bus:
			switch (class_sub) {
				case PCI_firewire:
					switch (class_api) {
						case 0x00:
							return "Firewire (IEEE 1394) serial bus controller";
						case 0x10:
							return "Firewire (IEEE 1394) OpenHCI serial bus controller";
						default:
							return "Unknown Firewire (IEEE 1394) serial bus controller";
					}
				case PCI_access:
					return "ACCESS serial bus controller";
				case PCI_ssa:
					return "Serial Storage Architecture (SSA) controller";
				case PCI_usb:
					switch (class_api) {
						case PCI_usb_uhci:
							return "USB UHCI controller";
						case PCI_usb_ohci:
							return "USB OHCI controller";
						case 0x80:
							return "Other USB controller";
						case 0xfe:
							return "USB device";
						default:
							return "Unknown USB serial bus controller";
					}
				case PCI_fibre_channel:
					return "Fibre Channel serial bus controller";
				case 0x05:
					return "System Management Bus (SMBus) controller";
				default:
					return "Unknown serial bus controller";
			}

		case PCI_wireless:
			switch (class_sub) {
				case 0x00:
					return "iRDA compatible wireless controller";
				case 0x01:
					return "Consumer IR wireless controller";
				case 0x10:
					return "RF wireless controller";
				case 0x80:
					return "Other wireless controller";
				default:
					return "Unknown wireless controller";
			}

		case PCI_intelligent_io:
			switch (class_sub) {
				case 0x00:
					return "Intelligent IO controller";
				default:
					return "Unknown intelligent IO controller";
			}

		case PCI_satellite_communications:
			switch (class_sub) {
				case 0x01:
					return "TV satellite communications controller";
				case 0x02:
					return "Audio satellite communications controller";
				case 0x03:
					return "Voice satellite communications controller";
				case 0x04:
					return "Data satellite communications controller";
				default:
					return "Unknown satellite communications controller";
			}

		case PCI_encryption_decryption:
			switch (class_sub) {
				case 0x00:
					return "Network and computing encryption/decryption controller";
				case 0x10:
					return "Entertainment encryption/decryption controller";
				case 0x80:
					return "Other encryption/decryption controller";
				default:
					return "Unknown encryption/decryption controller";
			}

		case PCI_data_acquisition:
			switch (class_sub) {
				case 0x00:
					return "DPIO modules (data acquisition and signal processing controller)";
				case 0x80:
					return " Other data acquisition and signal processing controller";
				default:
					return "Unknown data acquisition and signal processing controller";
			}

		case PCI_undefined:
			return "Does not fit any defined class";

		default:
			return "Unknown device class base";
	}
}


const char *
get_capability_name(uint8 cap_id)
{
	switch (cap_id) {
		case PCI_cap_id_reserved:
			return "reserved";
		case PCI_cap_id_pm:
			return "PM";
		case PCI_cap_id_agp:
			return "AGP";
		case PCI_cap_id_vpd:
			return "VPD";
		case PCI_cap_id_slotid:
			return "SlotID";
		case PCI_cap_id_msi:
			return "MSI";
		case PCI_cap_id_chswp:
			return "chswp";
		case PCI_cap_id_pcix:
			return "PCI-X";
		case PCI_cap_id_ldt:
			return "ldt";
		case PCI_cap_id_vendspec:
			return "vendspec";
		case PCI_cap_id_debugport:
			return "DebugPort";
		case PCI_cap_id_cpci_rsrcctl:
			return "cpci_rsrcctl";
		case PCI_cap_id_hotplug:
			return "HotPlug";
		default:
			return NULL;
	}
}


#if USE_PCI_HEADER
void
get_vendor_info(uint16 vendorID, const char **venShort, const char **venFull)
{
	for (int i = 0; i < (int)PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == vendorID) {
			if (0 == strcmp(PciVenTable[i].VenShort, PciVenTable[i].VenFull)) {
				*venShort = PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = NULL;
			} else {
				*venShort = PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = PciVenTable[i].VenFull[0] ? PciVenTable[i].VenFull : NULL;
			}
			return;
		}
	}
	*venShort = NULL;
	*venFull = NULL;
}


void
get_device_info(uint16 vendorID, uint16 deviceID, const char **devShort, const char **devFull)
{
	for (int i = 0; i < (int)PCI_DEVTABLE_LEN; i++) {
		if (PciDevTable[i].VenId == vendorID && PciDevTable[i].DevId == deviceID ) {
			*devShort = PciDevTable[i].Chip[0] ? PciDevTable[i].Chip : NULL;
			*devFull = PciDevTable[i].ChipDesc[0] ? PciDevTable[i].ChipDesc : NULL;
			return;
		}
	}
	*devShort = NULL;
	*devFull = NULL;
}
#endif	/* USE_PCI_HEADER */
