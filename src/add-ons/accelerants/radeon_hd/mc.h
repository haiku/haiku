/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_MC_H
#define RADEON_HD_MC_H


#define R6XX_FB_LOCATION(address, size) \
	(((((address) + (size)) >> 8) & 0xFFFF0000) | (((address) >> 24) & 0xFFFF))
#define R6XX_HDP_LOCATION(address) \
	((((address) >> 8) & 0x00FF0000))


uint64 MCFBLocation(uint16 chipset, uint32* size);
status_t MCFBSetup(uint32 newFbLocation, uint32 newFbSize);


#endif
