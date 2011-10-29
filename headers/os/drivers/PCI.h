/* 
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCI_H
#define _PCI_H


#include <SupportDefs.h>
#include <bus_manager.h>


#ifdef __cplusplus
extern "C" {
#endif


/* pci device info */
typedef struct pci_info pci_info;
typedef struct pci_info {
	uint16	vendor_id;				/* vendor id */
	uint16	device_id;				/* device id */
	uchar	bus;					/* bus number */
	uchar	device;					/* device number on bus */
	uchar	function;				/* function number in device */
	uchar	revision;				/* revision id */
	uchar	class_api;				/* specific register interface type */
	uchar	class_sub;				/* specific device function */
	uchar	class_base;				/* device type (display vs network, etc) */
	uchar	line_size;				/* cache line size in 32 bit words */
	uchar	latency;				/* latency timer */
	uchar	header_type;			/* header type */
	uchar	bist;					/* built-in self-test */
	uchar	reserved;				/* filler, for alignment */
	union {
		struct {
			uint32	cardbus_cis;			/* CardBus CIS pointer */
			uint16	subsystem_id;			/* subsystem (add-in card) id */
			uint16	subsystem_vendor_id;	/* subsystem vendor id */
			uint32	rom_base;				/* rom base addr, view from host */
			uint32	rom_base_pci;			/* rom base addr, viewed from pci */
			uint32	rom_size;				/* rom size */
			uint32	base_registers[6];		/* base registers, view from host */
			uint32	base_registers_pci[6];	/* base registers, view from pci */
			uint32	base_register_sizes[6];	/* size of what base regs point */
			uchar	base_register_flags[6];	/* flags from base address fields */
			uchar	interrupt_line;			/* interrupt line */
			uchar	interrupt_pin;			/* interrupt pin */
			uchar	min_grant;				/* burst period @ 33 Mhz */
			uchar	max_latency;			/* how often PCI access needed */
		} h0;
		struct {
			uint32	base_registers[2];		/* base registers, view from host */
			uint32	base_registers_pci[2];	/* base registers, view from pci */
			uint32	base_register_sizes[2];	/* size of what base regs point */
			uchar	base_register_flags[2];	/* flags from base address fields */
			uchar	primary_bus;
			uchar	secondary_bus;
			uchar	subordinate_bus;
			uchar	secondary_latency;
			uchar	io_base;
			uchar	io_limit;
			uint16	secondary_status;
			uint16	memory_base;
			uint16	memory_limit;
			uint16  prefetchable_memory_base;
			uint16  prefetchable_memory_limit;
			uint32	prefetchable_memory_base_upper32;
			uint32	prefetchable_memory_limit_upper32;
			uint16	io_base_upper16;
			uint16	io_limit_upper16;
			uint32	rom_base;				/* rom base addr, view from host */
			uint32	rom_base_pci;			/* rom base addr, view from pci */
			uchar	interrupt_line;			/* interrupt line */
			uchar	interrupt_pin;			/* interrupt pin */
			uint16	bridge_control;
			uint16	subsystem_id;			/* subsystem (add-in card) id */
			uint16	subsystem_vendor_id;	/* subsystem vendor id */
		} h1;
		struct {
			uint16	subsystem_id;			/* subsystem (add-in card) id */
			uint16	subsystem_vendor_id;	/* subsystem vendor id */

#ifdef __HAIKU_PCI_BUS_MANAGER_TESTING
			// for testing only, not final (do not use!):
			uchar		primary_bus;
			uchar		secondary_bus;
			uchar		subordinate_bus;
			uchar		secondary_latency;
			uint16		reserved;
			uint32		memory_base;
			uint32		memory_limit;
			uint32		memory_base_upper32;
			uint32		memory_limit_upper32;
			uint32		io_base;
			uint32		io_limit;
			uint32		io_base_upper32;
			uint32		io_limit_upper32;
			uint16		secondary_status;
			uint16		bridge_control;
#endif /* __HAIKU_PCI_BUS_MANAGER_TESTING */
		} h2;
	} u;
};


typedef struct pci_module_info pci_module_info;
typedef struct pci_module_info {
	bus_manager_info	binfo;

	uint8			(*read_io_8) (int32 mapped_io_addr);
	void			(*write_io_8) (int32 mapped_io_addr, uint8 value);
	uint16			(*read_io_16) (int32 mapped_io_addr);
	void			(*write_io_16) (int32 mapped_io_addr, uint16 value);
	uint32			(*read_io_32) (int32 mapped_io_addr);
	void			(*write_io_32) (int32 mapped_io_addr, uint32 value);

	int32			(*get_nth_pci_info) (
						int32		index,	/* index into pci device table */
						pci_info 	*info	/* caller-supplied buf for info */
					);
	uint32			(*read_pci_config) (
						uchar	bus,		/* bus number */
						uchar	device,		/* device # on bus */
						uchar	function,	/* function # in device */
						uchar	offset,		/* offset in configuration space */
						uchar	size		/* # bytes to read (1, 2 or 4) */
					);
	void			(*write_pci_config) (
						uchar	bus,		/* bus number */
						uchar	device,		/* device # on bus */
						uchar	function,	/* function # in device */
						uchar	offset,		/* offset in configuration space */
						uchar	size,		/* # bytes to write (1, 2 or 4) */
						uint32	value		/* value to write */
					);

	void*			(*ram_address) 
						(const void* physical_address_in_system_memory);

	status_t		(*find_pci_capability) (
						uchar	bus,
						uchar	device,
						uchar	function,
						uchar	cap_id,
						uchar	*offset
					);

	status_t		(*reserve_device) (
						uchar 		bus,
						uchar		device,
						uchar		function,
						const char*	driver_name,
						void*		cookie);
	status_t		(*unreserve_device) (
						uchar 		bus,
						uchar 		device,
						uchar 		function,
						const char*	driver_name,
						void*		cookie);

	status_t		(*update_interrupt_line) (
						uchar bus,
						uchar device,
						uchar function,
						uchar newInterruptLineValue);
};

#define	B_PCI_MODULE_NAME		"bus_managers/pci/v1"


/* offsets in PCI config space to the elements of the predefined header */
/* offsets common to all header types */
#define PCI_vendor_id			0x00	/* vendor id */
#define PCI_device_id			0x02	/* device id */
#define PCI_command				0x04	/* command */
#define PCI_status				0x06	/* status */
#define PCI_revision			0x08	/* revision id */
#define PCI_class_api			0x09	/* specific register interface type */
#define PCI_class_sub			0x0A	/* specific device function */
#define PCI_class_base			0x0B	/* device type */
#define PCI_line_size			0x0C	/* cache line size in 32 bit words */
#define PCI_latency				0x0D	/* latency timer */
#define PCI_header_type			0x0E	/* header type */
#define PCI_bist				0x0F	/* built-in self-test */

/* offsets common to header types 0x00 and 0x01 */
#define PCI_base_registers		0x10	/* base registers */
#define PCI_interrupt_line		0x3C	/* interrupt line */
#define PCI_interrupt_pin		0x3D	/* interrupt pin */

/* offsets common to header type 0x00 */
#define PCI_cardbus_cis			0x28	/* CardBus CIS pointer */
#define PCI_subsystem_vendor_id		0x2C	/* subsystem vendor id */
#define PCI_subsystem_id		0x2E	/* subsystem id */
#define PCI_rom_base			0x30	/* expansion rom base address */
#define PCI_capabilities_ptr		0x34	/* point to start of cap list */
#define PCI_min_grant			0x3E	/* burst period @ 33 Mhz */
#define PCI_max_latency			0x3F	/* how often need PCI access */

/* offsets common to the elements of header type 0x01 (PCI-to-PCI bridge) */
#define PCI_primary_bus								0x18
#define PCI_secondary_bus							0x19
#define PCI_subordinate_bus							0x1A
#define PCI_secondary_latency						0x1B
#define PCI_io_base									0x1C
#define PCI_io_limit								0x1D
#define PCI_secondary_status						0x1E
#define PCI_memory_base								0x20
#define PCI_memory_limit							0x22
#define PCI_prefetchable_memory_base				0x24
#define PCI_prefetchable_memory_limit				0x26
#define PCI_prefetchable_memory_base_upper32		0x28
#define PCI_prefetchable_memory_limit_upper32		0x2C
#define PCI_io_base_upper16							0x30
#define PCI_io_limit_upper16						0x32
#define PCI_sub_vendor_id_1							0x34
#define PCI_sub_device_id_1							0x36
#define PCI_bridge_rom_base							0x38
#define PCI_bridge_control							0x3E

/* PCI type 2 header offsets */
#define PCI_capabilities_ptr_2						0x14
#define PCI_secondary_status_2						0x16
#define PCI_primary_bus_2							0x18
#define PCI_secondary_bus_2							0x19
#define PCI_subordinate_bus_2						0x1A
#define PCI_secondary_latency_2						0x1B
#define PCI_memory_base0_2							0x1C
#define PCI_memory_limit0_2							0x20
#define PCI_memory_base1_2							0x24
#define PCI_memory_limit1_2							0x28
#define PCI_io_base0_2								0x2C
#define PCI_io_limit0_2								0x30
#define PCI_io_base1_2								0x34
#define PCI_io_limit1_2								0x38
#define PCI_bridge_control_2						0x3E
#define PCI_sub_vendor_id_2							0x40
#define PCI_sub_device_id_2							0x42
#define PCI_card_interface_2						0x44


/* values for the class_base field in the common header */
#define PCI_early						0x00
#define PCI_mass_storage				0x01
#define PCI_network						0x02
#define PCI_display						0x03
#define PCI_multimedia					0x04
#define PCI_memory						0x05
#define PCI_bridge						0x06
#define PCI_simple_communications		0x07
#define PCI_base_peripheral				0x08
#define PCI_input						0x09
#define PCI_docking_station				0x0A
#define PCI_processor					0x0B
#define PCI_serial_bus					0x0C
#define PCI_wireless					0x0D
#define PCI_intelligent_io				0x0E
#define PCI_satellite_communications	0x0F
#define PCI_encryption_decryption		0x10
#define PCI_data_acquisition			0x11
#define PCI_undefined					0xFF

/* values for the class_sub field for class_base = 0x00 (early) */
#define PCI_early_not_vga		0x00
#define PCI_early_vga			0x01

/* values for the class_sub field for class_base = 0x01 (mass storage) */
#define PCI_scsi				0x00
#define PCI_ide					0x01
#define PCI_floppy				0x02
#define PCI_ipi					0x03
#define PCI_raid				0x04
#define PCI_ata					0x05
#define PCI_sata				0x06
#define PCI_sas					0x07
#define PCI_mass_storage_other	0x80

/* values of the class_api field for class_base = 0x01, class_sub = 0x06 */
#define PCI_sata_other			0x00
#define PCI_sata_ahci			0x01

/* values for the class_sub field for class_base = 0x02 (network) */
#define PCI_ethernet			0x00
#define PCI_token_ring			0x01
#define PCI_fddi				0x02
#define PCI_atm					0x03
#define PCI_isdn				0x04
#define PCI_network_other		0x80

/* values for the class_sub field for class_base = 0x03 (display) */
#define PCI_vga					0x00
#define PCI_xga					0x01
#define PCI_3d					0x02
#define PCI_display_other		0x80

/* values for the class_sub field for class_base = 0x04 (multimedia device) */
#define PCI_video				0x00
#define PCI_audio				0x01
#define PCI_telephony			0x02
#define PCI_hd_audio			0x03
#define PCI_multimedia_other	0x80

/* values for the class_sub field for class_base = 0x05 (memory) */
#define PCI_ram					0x00
#define PCI_flash				0x01
#define PCI_memory_other		0x80

/* values for the class_sub field for class_base = 0x06 (bridge) */
#define PCI_host				0x00
#define PCI_isa					0x01
#define PCI_eisa				0x02
#define PCI_microchannel		0x03
#define PCI_pci					0x04
#define PCI_pcmcia				0x05
#define PCI_nubus				0x06
#define PCI_cardbus				0x07
#define PCI_raceway				0x08
#define PCI_bridge_transparent	0x09
#define PCI_bridge_infiniband	0x0A
#define PCI_bridge_other		0x80

/* values for the class_sub field for class_base = 0x07 (simple	comm ctrlers) */
#define PCI_serial						0x00
#define PCI_parallel					0x01
#define PCI_multiport_serial			0x02
#define PCI_modem						0x03
#define PCI_simple_communications_other	0x80

/* values of the class_api field for class_base = 0x07 and class_sub = 0x00 */
#define PCI_serial_xt			0x00
#define PCI_serial_16450		0x01
#define PCI_serial_16550		0x02

/* values of the class_api field for class_base = 0x07 and class_sub = 0x01 */
#define PCI_parallel_simple			0x00
#define PCI_parallel_bidirectional	0x01
#define PCI_parallel_ecp			0x02


/* values for the class_sub field for class_base = 0x08 (system peripherals) */
#define PCI_pic						0x00
#define PCI_dma						0x01
#define PCI_timer					0x02
#define PCI_rtc						0x03
#define PCI_generic_hot_plug		0x04
#define PCI_system_peripheral_other	0x80

/* values of the class_api field for class_base = 0x08 and class_sub = 0x00 */
#define PCI_pic_8259			0x00
#define PCI_pic_isa				0x01
#define PCI_pic_eisa			0x02

/* values of the class_api field for class_base = 0x08 and class_sub = 0x01 */
#define PCI_dma_8237			0x00
#define PCI_dma_isa				0x01
#define PCI_dma_eisa			0x02

/* values of the class_api field for class_base = 0x08 and class_sub = 0x02 */
#define PCI_timer_8254			0x00
#define PCI_timer_isa			0x01
#define PCI_timer_eisa			0x02

/* values of the class_api field for class_base = 0x08 and class_sub = 0x03 */
#define PCI_rtc_generic			0x00
#define PCI_rtc_isa				0x01

/* values for the class_sub field for class_base = 0x09 (input devices) */
#define PCI_keyboard			0x00
#define PCI_pen					0x01
#define PCI_mouse				0x02
#define PCI_scanner				0x03
#define PCI_gameport			0x04
#define PCI_input_other			0x80

/* values for the class_sub field for class_base = 0x0A (docking stations) */
#define PCI_docking_generic		0x00
#define PCI_docking_other		0x80

/* values for the class_sub field for class_base = 0x0B (processor) */
#define PCI_386				0x00
#define PCI_486				0x01
#define PCI_pentium			0x02
#define PCI_alpha			0x10
#define PCI_PowerPC			0x20
#define PCI_mips			0x30
#define PCI_coprocessor		0x40

/* values for the class_sub field for class_base = 0x0C (serial bus ctrlr) */
#define PCI_firewire		0x00
#define PCI_access			0x01
#define PCI_ssa				0x02
#define PCI_usb				0x03
#define PCI_fibre_channel	0x04
#define PCI_smbus			0x05
#define PCI_infiniband		0x06
#define PCI_ipmi			0x07
#define PCI_sercos			0x08
#define PCI_canbus			0x09

/* values of the class_api field for class_base = 0x0C and class_sub = 0x03 */
#define PCI_usb_uhci		0x00
#define PCI_usb_ohci		0x10
#define PCI_usb_ehci		0x20
#define PCI_usb_xhci		0x30	/* Extensible Host Controller Interface */

/* values for the class_sub field for class_base = 0x0d (wireless controller) */
#define PCI_wireless_irda			0x00
#define PCI_wireless_consumer_ir	0x01
#define PCI_wireless_rf				0x02
#define PCI_wireless_bluetooth		0x03
#define PCI_wireless_broadband		0x04
#define PCI_wireless_80211A			0x10
#define PCI_wireless_80211B			0x20
#define PCI_wireless_other			0x80


/* masks for command register bits */
#define PCI_command_io				0x001
#define PCI_command_memory			0x002
#define PCI_command_master			0x004
#define PCI_command_special			0x008
#define PCI_command_mwi				0x010
#define PCI_command_vga_snoop		0x020
#define PCI_command_parity			0x040
#define PCI_command_address_step	0x080
#define PCI_command_serr			0x100
#define PCI_command_fastback		0x200
#define PCI_command_int_disable		0x400


/* masks for status register bits */
#define PCI_status_capabilities				0x0010
#define PCI_status_66_MHz_capable			0x0020
#define PCI_status_udf_supported			0x0040
#define PCI_status_fastback					0x0080
#define PCI_status_parity_signalled			0x0100
#define PCI_status_devsel					0x0600
#define PCI_status_target_abort_signalled	0x0800
#define PCI_status_target_abort_received	0x1000
#define PCI_status_master_abort_received	0x2000
#define PCI_status_serr_signalled			0x4000
#define PCI_status_parity_error_detected	0x8000


/* masks for devsel field in status register */
#define PCI_status_devsel_fast			0x0000
#define PCI_status_devsel_medium		0x0200
#define PCI_status_devsel_slow			0x0400


/* masks for header type register */
#define PCI_header_type_mask			0x7F
#define PCI_multifunction				0x80


/* types of PCI header */
#define PCI_header_type_generic				0x00
#define PCI_header_type_PCI_to_PCI_bridge	0x01
#define PCI_header_type_cardbus				0x02


/* masks for built in self test (bist) register bits */
#define PCI_bist_code				0x0F
#define PCI_bist_start				0x40
#define PCI_bist_capable			0x80


/* masks for flags in the various base address registers */
#define PCI_address_space			0x01
#define PCI_register_start			0x10
#define PCI_register_end			0x24
#define PCI_register_ppb_end		0x18
#define PCI_register_pcb_end		0x14


/* masks for flags in memory space base address registers */
#define PCI_address_type_32			0x00
#define PCI_address_type_32_low		0x02
#define PCI_address_type_64			0x04
#define PCI_address_type			0x06
#define PCI_address_prefetchable	0x08
#define PCI_address_memory_32_mask	0xFFFFFFF0


/* masks for flags in i/o space base address registers */
#define PCI_address_io_mask			0xFFFFFFFC


/* masks for flags in expansion rom base address registers */
#define PCI_rom_enable			0x00000001
#define PCI_rom_shadow			0x00000010	/* 2 rom copied at shadow (C0000) */
#define PCI_rom_copy			0x00000100	/* 4 rom is allocated copy */
#define PCI_rom_bios			0x00001000	/* 8 rom is bios copy */
#define PCI_rom_address_mask	0xFFFFF800


/* PCI interrupt pin values */
#define PCI_pin_mask			0x07
#define PCI_pin_none			0x00
#define PCI_pin_a				0x01
#define PCI_pin_b				0x02
#define PCI_pin_c				0x03
#define PCI_pin_d				0x04
#define PCI_pin_max				0x04


/* PCI Capability Codes */
#define PCI_cap_id_reserved			0x00
#define PCI_cap_id_pm				0x01
#define PCI_cap_id_agp				0x02
#define PCI_cap_id_vpd				0x03
#define PCI_cap_id_slotid			0x04
#define PCI_cap_id_msi				0x05
#define PCI_cap_id_chswp			0x06
#define PCI_cap_id_pcix				0x07
#define PCI_cap_id_ldt				0x08
#define PCI_cap_id_vendspec			0x09
#define PCI_cap_id_debugport		0x0a
#define PCI_cap_id_cpci_rsrcctl		0x0b
#define PCI_cap_id_hotplug			0x0c
#define PCI_cap_id_subvendor		0x0d
#define PCI_cap_id_agp8x			0x0e
#define PCI_cap_id_secure_dev		0x0f
#define PCI_cap_id_pcie				0x10
#define PCI_cap_id_msix				0x11
#define PCI_cap_id_sata				0x12
#define PCI_cap_id_pciaf			0x13


/* Power Management Control Status Register settings */
#define PCI_pm_mask					0x03
#define PCI_pm_ctrl					0x02
#define PCI_pm_d1supp				0x0200
#define PCI_pm_d2supp				0x0400
#define PCI_pm_status				0x04
#define PCI_pm_state_d0				0x00
#define PCI_pm_state_d1				0x01
#define PCI_pm_state_d2				0x02
#define PCI_pm_state_d3				0x03


/* MSI registers */
#define PCI_msi_control			0x02
#define PCI_msi_address			0x04
#define PCI_msi_address_high	0x08
#define PCI_msi_data			0x08
#define PCI_msi_data_64bit		0x0c
#define PCI_msi_mask			0x10
#define PCI_msi_pending			0x14

/* MSI control register values */
#define PCI_msi_control_enable		0x0001
#define PCI_msi_control_vector		0x0100
#define PCI_msi_control_64bit		0x0080
#define PCI_msi_control_mme_mask	0x0070
#define PCI_msi_control_mme_1		0x0000
#define PCI_msi_control_mme_2		0x0010
#define PCI_msi_control_mme_4		0x0020
#define PCI_msi_control_mme_8		0x0030
#define PCI_msi_control_mme_16		0x0040
#define PCI_msi_control_mme_32		0x0050
#define PCI_msi_control_mmc_mask	0x000e
#define PCI_msi_control_mmc_1		0x0000
#define PCI_msi_control_mmc_2		0x0002
#define PCI_msi_control_mmc_4		0x0004
#define PCI_msi_control_mmc_8		0x0006
#define PCI_msi_control_mmc_16		0x0008
#define PCI_msi_control_mmc_32		0x000a


#ifdef __cplusplus
}
#endif


#endif	/* _PCI_H */
