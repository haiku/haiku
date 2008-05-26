/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __IDE_H__
#define __IDE_H__


#include <device_manager.h>
#include <KernelExport.h>



// Controller Driver Node

// attributes:

// node type
#define IDE_BUS_TYPE_NAME "bus/ide/v1"
// maximum number of devices connected to controller (uint8, optional, default:2)
#define IDE_CONTROLLER_MAX_DEVICES_ITEM "ide/max_devices"
// set to not-0 if DMA is supported (uint8, optional, default:0)
// (if so, publish necessary blkdev restriction too)
#define IDE_CONTROLLER_CAN_DMA_ITEM "ide/can_DMA"
// set to not-0 if CQ is supported (uint8, optional, default:1)
#define IDE_CONTROLLER_CAN_CQ_ITEM "ide/can_CQ"
// name of controller (string, required)
#define IDE_CONTROLLER_CONTROLLER_NAME_ITEM "ide/controller_name"

union ide_task_file;
typedef unsigned int ide_reg_mask;

// channel cookie, issued by ide bus manager
typedef struct ide_bus_info *ide_channel;

// interface of controller driver
typedef struct {
	driver_module_info info;

	void (*set_channel)(void *cookie, ide_channel channel);

	status_t (*write_command_block_regs)
		(void *channel_cookie, union ide_task_file *tf, ide_reg_mask mask);
	status_t (*read_command_block_regs)
		(void *channel_cookie, union ide_task_file *tf, ide_reg_mask mask);

	uint8 (*get_altstatus) (void *channel_cookie);
	status_t (*write_device_control) (void *channel_cookie, uint8 val);

	status_t (*write_pio) (void *channel_cookie, uint16 *data, int count, bool force_16bit );
	status_t (*read_pio) (void *channel_cookie, uint16 *data, int count, bool force_16bit );

	status_t (*prepare_dma)(void *channel_cookie,
							const physical_entry *sg_list, size_t sg_list_count,
	                        bool write);
	status_t (*start_dma)(void *channel_cookie);
	status_t (*finish_dma)(void *channel_cookie);
} ide_controller_interface;


// Interface for Controller Driver

// interface of bus manager as seen from controller driver
// use this interface as the fixed consumer of your controller driver
typedef struct {
	driver_module_info info;

	// status - status read from controller (_not_ alt_status, as reading
	//          normal status acknowledges IRQ request of device)
	status_t	(*irq_handler)( ide_channel channel, uint8 status );
} ide_for_controller_interface;


#define IDE_FOR_CONTROLLER_MODULE_NAME "bus_managers/ide/controller/driver_v1"


#endif	/* __IDE_H__ */
