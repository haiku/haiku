/*
 * Copyright 2017, Alexander Coers. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HIGHPOINT_ATA_H
#define _HIGHPOINT_ATA_H

#include <SupportDefs.h>


enum {
	ATA_MWORD_DMA0 = 0x00,
	ATA_MWORD_DMA1 = 0x01,
	ATA_MWORD_DMA2 = 0x02,
	ATA_ULTRA_DMA0 = 0x10,
	ATA_ULTRA_DMA1 = 0x11,
	ATA_ULTRA_DMA2 = 0x12,
	ATA_ULTRA_DMA3 = 0x13,
	ATA_ULTRA_DMA4 = 0x14,
	ATA_ULTRA_DMA5 = 0x15,
	ATA_ULTRA_DMA6 = 0x16
};

enum {
	CFG_HPT366_OLD,
	CFG_HPT366,
	CFG_HPT370,
	CFG_HPT372,
	CFG_HPT374,
	CFG_HPTUnknown	// no supported option found
};

#define ATA_HIGHPOINT_ID        0x1103

#define ATA_HPT366              0x0004
#define ATA_HPT372              0x0005
#define ATA_HPT302              0x0006
#define ATA_HPT371              0x0007
#define ATA_HPT374              0x0008

struct HPT_controller_info {
	uint16 deviceID;
	uint8 revisionID;
	uint8 function;
	bool configuredDMA;		// is DMA already configured
	int configOption;		// some HPT devices need different settings
	uint8 maxDMA;			// see enum
};


#endif // _HIGHPOINT_ATA_H
