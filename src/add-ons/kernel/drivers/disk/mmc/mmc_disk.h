/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#ifndef _MMC_DISK_H
#define _MMC_DISK_H


#include <device_manager.h>
#include <KernelExport.h>


#define SDHCI_DEVICE_TYPE_ITEM "sdhci/type"

typedef struct {
	device_node* 	node;
	status_t 		media_status;
} mmc_disk_driver_info;


#endif /*_MMC_DISK_H*/