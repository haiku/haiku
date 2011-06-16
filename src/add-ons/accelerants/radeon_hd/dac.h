/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */

#ifndef RADEON_HD_DAC_H
#define RADEON_HD_DAC_H

// DAC Offsets
#define REG_DACA_OFFSET 0
#define REG_DACB_OFFSET 0x200
#define RV620_REG_DACA_OFFSET 0
#define RV620_REG_DACB_OFFSET 0x100

// Signal types
#define FORMAT_PAL 0x0
#define FORMAT_NTSC 0x1
#define FORMAT_VGA 0x2
#define FORMAT_TvCV 0x3

void DACGetElectrical(uint8 type, uint8 dac, uint8 *bandgap, uint8 *whitefine);
void DACSet(uint8 dacIndex, uint32 crtid);
void DACPower(uint8 dacIndex, int mode);


#endif
