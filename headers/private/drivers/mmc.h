/*
 * Copyright 2019-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */
#ifndef _MMC_H
#define _MMC_H


#include <device_manager.h>


#define MMC_BUS_MODULE_NAME "bus_managers/mmc_bus/driver_v1"


enum {
	CARD_TYPE_MMC,
	CARD_TYPE_SD,
	CARD_TYPE_SDHC,
	CARD_TYPE_UHS1,
	CARD_TYPE_UHS2,
	CARD_TYPE_SDIO
};


// Commands for SD cards defined in SD Specifications Part 1:
// Physical Layer Simplified Specification Version 8.00
// They are in the common .h file for the mmc stack because the SDHCI driver
// currently needs to map them to the corresponding expected response types.
enum SD_COMMANDS {
	// Basic commands, class 0
	SD_GO_IDLE_STATE 		= 0,
	SD_ALL_SEND_CID			= 2,
	SD_SEND_RELATIVE_ADDR	= 3,
	SD_SELECT_DESELECT_CARD	= 7,
	SD_SEND_IF_COND			= 8,
	SD_SEND_CSD				= 9,
	SD_STOP_TRANSMISSION	= 12,

	// Block oriented read commands, class 2
	SD_READ_SINGLE_BLOCK = 17,
	SD_READ_MULTIPLE_BLOCKS = 18,

	// Application specific commands, class 8
	SD_APP_CMD = 55,

	// I/O mode commands, class 9
	SD_IO_ABORT = 52,
};


enum SDHCI_APPLICATION_COMMANDS {
	SD_SEND_OP_COND = 41,
};


// Interface between mmc_bus and underlying implementation (sdhci_pci or any
// other thing that can execute mmc commands)
typedef struct mmc_bus_interface {
	driver_module_info info;

	status_t (*set_clock)(void* controller, uint32_t kilohertz);
	status_t (*execute_command)(void* controller, uint8_t command,
		uint32_t argument, uint32_t* result);
	status_t (*read_naive)(void* controller, off_t pos,
		void* buffer, size_t* _length);
} mmc_bus_interface;


// Interface between mmc device driver (mmc_disk, sdio drivers, ...) and mmc_bus
typedef struct mmc_device_interface {
	driver_module_info info;
	status_t (*execute_command)(device_node* node, uint8_t command,
		uint32_t argument, uint32_t* result);
	status_t (*read_naive)(device_node* controller, uint16_t rca, off_t pos,
		void* buffer, size_t* _length);
} mmc_device_interface;


// Device attribute paths for the MMC device
static const char* kMmcRcaAttribute = "mmc/rca";
static const char* kMmcTypeAttribute = "mmc/type";


#endif /* _MMC_H */
