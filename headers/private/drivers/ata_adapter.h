/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-04, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _ATA_PCI_H
#define _ATA_PCI_H

/*
	ATA adapter library

	Module to simplify writing an ATA adapter driver.

	The interface is not very abstract, i.e. the actual driver is
	free to access any controller or channel data of this library.
*/


#include <bus/ATA.h>
#include <bus/PCI.h>
#include <ata_types.h>


// one Physical Region Descriptor (PRD)
// (one region must not cross 64K boundary;
//  the PRD table must not cross a 64K boundary)
typedef struct prd_entry {
	uint32 address;				// physical address of block (must be even)
	uint16 count;				// size of block, 0 stands for 65536 (must be even)
	uint8 res6;
	LBITFIELD8_2(
		res7_0 : 7,
		EOT : 1					// 1 for last entry
	);
} prd_entry;

// IDE bus master command register
#define ATA_BM_COMMAND_START_STOP		0x01
#define ATA_BM_COMMAND_READ_FROM_DEVICE	0x08

// IDE bus master status register
#define ATA_BM_STATUS_ACTIVE		0x01
#define ATA_BM_STATUS_ERROR			0x02
#define ATA_BM_STATUS_INTERRUPT		0x04
#define ATA_BM_STATUS_MASTER_DMA	0x20
#define ATA_BM_STATUS_SLAVE_DMA		0x40
#define ATA_BM_STATUS_SIMPLEX_DMA	0x80

// offset of bus master registers
enum {
	ATA_BM_COMMAND_REG	= 0,
	ATA_BM_STATUS_REG	= 2,
	ATA_BM_PRDT_ADDRESS	= 4
		// offset of PRDT register; content must be dword-aligned
};

// bit mask in class_api of PCI configuration
// (for adapters that can run in compatability mode)
enum {
	ATA_API_PRIMARY_NATIVE		= 1,	// primary channel is in native mode
	ATA_API_PRIMARY_FIXED		= 2,	// primary channel can be switched to native mode
	ATA_API_SECONDARY_NATIVE	= 4,	// secondary channel is in native mode
	ATA_API_SECONDARY_FIXED		= 8		// secondary channel can be switched to native mode
};


// (maximum) size of S/G table
// there are so many restrictions that we want to keep it inside one page
// to be sure that we fulfill them all
#define ATA_ADAPTER_MAX_SG_COUNT (B_PAGE_SIZE / sizeof( prd_entry ))


// channel node items
// io address of command block (uint16)
#define ATA_ADAPTER_COMMAND_BLOCK_BASE "ata_adapter/command_block_base"
// io address of control block (uint16)
#define ATA_ADAPTER_CONTROL_BLOCK_BASE "ata_adapter/control_block_base"
// interrupt number (uint8)
// can also be defined in controller node if both channels use same IRQ!
#define ATA_ADAPTER_INTNUM "ata_adapter/irq"
// 0 if primary channel, 1 if secondary channel, 2 if tertiary, ... (uint8)
#define ATA_ADAPTER_CHANNEL_INDEX "ata_adapter/channel_index"

// controller node items
// io address of bus master registers (uint16)
#define ATA_ADAPTER_BUS_MASTER_BASE "ata_adapter/bus_master_base"


// info about one channel
typedef struct ata_adapter_channel_info {
	pci_device_module_info *pci;
	pci_device *device;

	uint16 command_block_base;	// io address command block
	uint16 control_block_base; // io address control block
	uint16 bus_master_base;
	int intnum;				// interrupt number

	uint32 lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore

	ata_channel ataChannel;
	device_node *node;

	int32 (*inthand)( void *arg );

	area_id prd_area;
	prd_entry *prdt;
	uint32 prdt_phys;
	uint32 dmaing;
} ata_adapter_channel_info;


// info about controller
typedef struct ata_adapter_controller_info {
	pci_device_module_info *pci;
	pci_device *device;

	uint16 bus_master_base;

	uint32 lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore

	device_node *node;
} ata_adapter_controller_info;


// interface of IDE adapter library
typedef struct {
	module_info info;

	void (*set_channel)(ata_adapter_channel_info *channel,
					ata_channel ataChannel);

	// function calls that can be forwarded from actual driver
	status_t (*write_command_block_regs)(ata_adapter_channel_info *channel,
					ata_task_file *tf, ata_reg_mask mask);
	status_t (*read_command_block_regs)(ata_adapter_channel_info *channel,
					ata_task_file *tf, ata_reg_mask mask);

	uint8 (*get_altstatus) (ata_adapter_channel_info *channel);
	status_t (*write_device_control) (ata_adapter_channel_info *channel, uint8 val);

	status_t (*write_pio)(ata_adapter_channel_info *channel, uint16 *data, int count, bool force_16bit);
	status_t (*read_pio)(ata_adapter_channel_info *channel, uint16 *data, int count, bool force_16bit);

	status_t (*prepare_dma)(ata_adapter_channel_info *channel, const physical_entry *sg_list,
					size_t sg_list_count, bool to_device);
	status_t (*start_dma)(ata_adapter_channel_info *channel);
	status_t (*finish_dma)(ata_adapter_channel_info *channel);

	// default functions that should be replaced by a more specific version
	// (copy them from source code of this library and modify them at will)
	int32 (*inthand)(void *arg);

	// functions that must be called by init/uninit etc. of channel driver
	status_t (*init_channel)(device_node *node,
					ata_adapter_channel_info **cookie, size_t total_data_size,
					int32 (*inthand)(void *arg));
	void (*uninit_channel)(ata_adapter_channel_info *channel);
	void (*channel_removed)(ata_adapter_channel_info *channel);

	// publish channel node
	status_t (*publish_channel)(device_node *controller_node,
					const char *channel_module_name, uint16 command_block_base,
					uint16 control_block_base, uint8 intnum, bool can_dma,
					uint8 channel_index, const char *name,
					const io_resource *resources, device_node **node);
	// verify channel configuration and publish node on success
	status_t (*detect_channel)(pci_device_module_info *pci, pci_device *pciDevice,
					device_node *controller_node, const char *channel_module_name,
					bool controller_can_dma, uint16 command_block_base,
					uint16 control_block_base, uint16 bus_master_base,
					uint8 intnum, uint8 channel_index, const char *name,
					device_node **node, bool supports_compatibility_mode);

	// functions that must be called by init/uninit etc. of controller driver
	status_t (*init_controller)(device_node *node,
					ata_adapter_controller_info **cookie, size_t total_data_size);
	void (*uninit_controller)(ata_adapter_controller_info *controller);
	void (*controller_removed)(ata_adapter_controller_info *controller);

	// publish controller node
	status_t (*publish_controller)(device_node *parent, uint16 bus_master_base,
					io_resource *resources, const char *controller_driver,
					const char *controller_driver_type, const char *controller_name,
					bool can_dma, bool can_cq, uint32 dma_alignment, uint32 dma_boundary,
					uint32 max_sg_block_size, device_node **node);
	// verify controller configuration and publish node on success
	status_t (*detect_controller)(pci_device_module_info *pci, pci_device *pciDevice,
					device_node *parent, uint16 bus_master_base,
					const char *controller_driver, const char *controller_driver_type,
					const char *controller_name, bool can_dma, bool can_cq,
					uint32 dma_alignment, uint32 dma_boundary, uint32 max_sg_block_size,
					device_node **node);
	// standard master probe for controller that registers controller and channel nodes
	status_t (*probe_controller)(device_node *parent, const char *controller_driver,
					const char *controller_driver_type, const char *controller_name,
					const char *channel_module_name, bool can_dma, bool can_cq,
					uint32 dma_alignment, uint32 dma_boundary, uint32 max_sg_block_size,
					bool supports_compatibility_mode);
} ata_adapter_interface;


#define ATA_ADAPTER_MODULE_NAME "generic/ata_adapter/v1"

#endif	/* _ATA_PCI_H */
