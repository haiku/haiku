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


// Interface between mmc_bus and underlying implementation (sdhci_pci or any
// other thing that can execute mmc commands)
typedef struct mmc_bus_interface {
	driver_module_info info;

	status_t (*set_clock)(void* controller, uint32_t kilohertz);
	status_t (*execute_command)(void* controller, uint8_t command,
		uint32_t argument, uint32_t* result);
} mmc_bus_interface;


// Interface between mmc device driver (mmc_disk, sdio drivers, ...) and mmc_bus
typedef struct mmc_device_interface {
	driver_module_info info;

	status_t (*execute_command)(device_node* node, uint8_t command,
		uint32_t argument, uint32_t* result);
} mmc_device_interface;


// Device attribute paths for the MMC device
static const char* kMmcRcaAttribute = "mmc/rca";
static const char* kMmcTypeAttribute = "mmc/type";


#endif /* _MMC_H */
