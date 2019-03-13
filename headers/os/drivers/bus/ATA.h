/*
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __ATA_H__
#define __ATA_H__

#include <device_manager.h>
#include <KernelExport.h>

// Controller Driver Node

// attributes:

// node type
#define ATA_BUS_TYPE_NAME "bus/ata/v1"
// maximum number of devices connected to controller (uint8, optional, default:2)
#define ATA_CONTROLLER_MAX_DEVICES_ITEM "ata/max_devices"
// set to not-0 if DMA is supported (uint8, optional, default:0)
#define ATA_CONTROLLER_CAN_DMA_ITEM "ata/can_DMA"
// name of controller (string, required)
#define ATA_CONTROLLER_CONTROLLER_NAME_ITEM "ata/controller_name"

union ata_task_file;
typedef unsigned int ata_reg_mask;

// channel cookie, issued by ata bus manager
typedef void* ata_channel;

// interface of controller driver
typedef struct {
	driver_module_info info;

	void (*set_channel)(void *cookie, ata_channel channel);

	status_t (*write_command_block_regs)(void *channelCookie,
		union ata_task_file *file, ata_reg_mask mask);
	status_t (*read_command_block_regs)(void *channelCookie,
		union ata_task_file *file, ata_reg_mask mask);

	uint8 (*get_altstatus)(void *channelCookie);
	status_t (*write_device_control)(void *channelCookie, uint8 val);

	status_t (*write_pio)(void *channelCookie, uint16 *data, int count,
		bool force16Bit);
	status_t (*read_pio)(void *channelCookie, uint16 *data, int count,
		bool force16Bit);

	status_t (*prepare_dma)(void *channelCookie, const physical_entry *sg_list,
		size_t sg_list_count, bool write);
	status_t (*start_dma)(void *channelCookie);
	status_t (*finish_dma)(void *channelCookie);
} ata_controller_interface;


// Interface for Controller Driver

// interface of bus manager as seen from controller driver
// use this interface as the fixed consumer of your controller driver
typedef struct {
	driver_module_info info;

	// status - status read from controller (_not_ alt_status, as reading
	//          normal status acknowledges IRQ request of device)
	status_t	(*interrupt_handler)(ata_channel channel, uint8 status);
} ata_for_controller_interface;

#define ATA_FOR_CONTROLLER_MODULE_NAME "bus_managers/ata/controller/driver_v1"

#endif	/* __ATA_H__ */
