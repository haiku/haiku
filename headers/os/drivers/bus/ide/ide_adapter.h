/*
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	IDE adapter library

	Module to simplify writing an IDE adapter driver.
	
	The interface is not very abstract, i.e. the actual driver is
	free to access any controller or channel data of this library.
*/

#ifndef _IDE_PCI_H
#define _IDE_PCI_H

#include <bus/PCI.h>
#include <bus/IDE.h>
#include <device_manager.h>


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


// command register
typedef struct ide_bm_command {
	LBITFIELD8_4(
		start_stop	: 1,		// start BM by changing from 0 to 1; 
								// stop BM by changing from 1 to 0
		res0_1		: 2,
		from_device	: 1,		// true - read from device, false - write to device
		res0_4		: 4
	);
} ide_bm_command;


// status register
typedef struct ide_bm_status {
	LBITFIELD8_7(
		active		: 1,		// 1, if BM is active
		error		: 1,		// 1, if error occured; write 1 to reset
		interrupt	: 1,		// 1, if INTRQ was raised, write 1 to reset
		res0_3		: 2,
		device0_dma	: 1,		// 1, if BIOS/driver has setup DMA for device 0
		device1_dma	: 1,		// 1, if BIOS/driver has setup DMA for device 1
		simplex		: 1			// 1, if only one channel can use DMA at a time
	);
} ide_bm_status;


// offset of bus master registers
enum {
	ide_bm_command_reg	= 0,	// see ide_bm_command
	ide_bm_status_reg	= 2,	// see ide_bm_status
	ide_bm_prdt_address	= 4		// offset of PRDT register; content must be dword-aligned
};

// bit mask in class_api of PCI configuration
// (for adapters that can run in compatability mode)
enum {
	ide_api_primary_native		= 1,	// primary channel is in native mode
	ide_api_primary_fixed		= 2,	// primary channel can be switched to native mode
	ide_api_secondary_native	= 4,	// secondary channel is in native mode
	ide_api_secondary_fixed		= 8		// secondary channel can be switched to native mode
};


// (maximum) size of S/G table
// there are so many restrictions that we want to keep it inside one page
// to be sure that we fulfill them all
#define IDE_ADAPTER_MAX_SG_COUNT (B_PAGE_SIZE / sizeof( prd_entry ))


// channel node items
// io address of command block (uint16)
#define IDE_ADAPTER_COMMAND_BLOCK_BASE "ide_adapter/command_block_base"
// io address of control block (uint16)
#define IDE_ADAPTER_CONTROL_BLOCK_BASE "ide_adapter/control_block_base"
// interrupt number (uint8)
// can also be defined in controller node if both channels use same IRQ!
#define IDE_ADAPTER_INTNUM "ide_adapter/irq"
// non-zero if primary channel, else secondary channel (uint8)
#define IDE_ADAPTER_IS_PRIMARY "ide_adapter/is_primary"


// controller node items
// io address of bus master registers (uint16)
#define IDE_ADAPTER_BUS_MASTER_BASE "ide_adapter/bus_master_base"


// info about one channel
typedef struct ide_adapter_channel_info {
	pci_device_module_info *pci;
	pci_device device;
	
	uint16 command_block_base;	// io address command block
	uint16 control_block_base; // io address control block
	uint16 bus_master_base;
	int intnum;				// interrupt number	
	
	uint32 lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore
	
	ide_channel ide_channel;
	pnp_node_handle node;
	
	int32 (*inthand)( void *arg );
	
	area_id prd_area;
	prd_entry *prdt;
	uint32 prdt_phys;
	uint32 dmaing;
} ide_adapter_channel_info;


// info about controller
typedef struct ide_adapter_controller_info {
	pci_device_module_info *pci;
	pci_device device;
	
	uint16 bus_master_base;
	
	uint32 lost;			// != 0 if device got removed, i.e. if it must not
							// be accessed anymore
	
	pnp_node_handle node;
} ide_adapter_controller_info;


// interface of IDE adapter library
typedef struct {
	module_info minfo;

	// function calls that can be forwarded from actual driver
	status_t (*write_command_block_regs)
		(ide_adapter_channel_info *channel, ide_task_file *tf, ide_reg_mask mask);
	status_t (*read_command_block_regs)
		(ide_adapter_channel_info *channel, ide_task_file *tf, ide_reg_mask mask);

	uint8 (*get_altstatus) (ide_adapter_channel_info *channel);
	status_t (*write_device_control) (ide_adapter_channel_info *channel, uint8 val);	

	status_t (*write_pio) (ide_adapter_channel_info *channel, uint16 *data, int count, bool force_16bit );
	status_t (*read_pio) (ide_adapter_channel_info *channel, uint16 *data, int count, bool force_16bit );

	status_t (*prepare_dma)(ide_adapter_channel_info *channel, 
							const physical_entry *sg_list, size_t sg_list_count,
	                        bool to_device );
	status_t (*start_dma)(ide_adapter_channel_info *channel);
	status_t (*finish_dma)(ide_adapter_channel_info *channel);
	
	
	// default functions that should be replaced by a more specific version
	// (copy them from source code of this library and modify them at will)
	int32 (*inthand)( void *arg );
	
	// functions that must be called by init/uninit etc. of channel driver
	status_t (*init_channel)( pnp_node_handle node, ide_channel ide_channel, 
		ide_adapter_channel_info **cookie, size_t total_data_size,
		int32 (*inthand)( void *arg ) );
	status_t (*uninit_channel)( ide_adapter_channel_info *channel );
	void (*channel_removed)( pnp_node_handle node, ide_adapter_channel_info *channel );

	// publish channel node
	status_t (*publish_channel)( pnp_node_handle controller_node, 
		const char *channel_module_name,
		uint16 command_block_base, uint16 control_block_base,
		uint8 intnum, bool can_dma, bool is_primary, const char *name, 
		io_resource_handle *resources, pnp_node_handle *node );
	// verify channel configuration and publish node on success
	status_t (*detect_channel)( 	
		pci_device_module_info *pci, pci_device pci_device,
		pnp_node_handle controller_node, 
		const char *channel_module_name, bool controller_can_dma,
		uint16 command_block_base, uint16 control_block_base, uint16 bus_master_base,
		uint8 intnum, bool is_primary, const char *name, pnp_node_handle *node,
		bool supports_compatibility_mode );

	// functions that must be called by init/uninit etc. of controller driver
	status_t (*init_controller)( pnp_node_handle node, void *user_cookie, 
		ide_adapter_controller_info **cookie, size_t total_data_size );
	status_t (*uninit_controller)( ide_adapter_controller_info *controller );
	void (*controller_removed)( pnp_node_handle node, ide_adapter_controller_info *controller );

	// publish controller node
	status_t (*publish_controller)( 	
		pnp_node_handle parent, 
		uint16 bus_master_base,	io_resource_handle *resources, 
		const char *controller_driver, const char *controller_driver_type,
		const char *controller_name, 
		bool can_dma, bool can_cq, uint32 dma_alignment, uint32 dma_boundary,
		uint32 max_sg_block_size, pnp_node_handle *node );
	// verify controller configuration and publish node on success
	status_t (*detect_controller)( 	
		pci_device_module_info *pci, pci_device pci_device,
		pnp_node_handle parent,	uint16 bus_master_base,	
		const char *controller_driver, const char *controller_driver_type,
		const char *controller_name, 
		bool can_dma, bool can_cq, uint32 dma_alignment, uint32 dma_boundary,
		uint32 max_sg_block_size, pnp_node_handle *node );
	// standard master probe for controller that registers controller and channel nodes
	status_t (*probe_controller)( pnp_node_handle parent,
		const char *controller_driver, const char *controller_driver_type,
		const char *controller_name, const char *channel_module_name,
		bool can_dma, bool can_cq, uint32 dma_alignment, uint32 dma_boundary,
		uint32 max_sg_block_size, bool supports_compatibility_mode );
} ide_adapter_interface;


#define IDE_ADAPTER_MODULE_NAME "generic/ide_adapter/v1"

#endif
