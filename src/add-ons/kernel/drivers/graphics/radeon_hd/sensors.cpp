/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "sensors.h"


#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x...) dprintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


int32
radeon_thermal_query(radeon_info &info)
{
	// return GPU temp in millidegrees C

	uint32 rawTemp = 0;
	int32 finalTemp = 0;

	if (info.chipsetID >= RADEON_LOMBOK) {
		rawTemp = (read32(info.registers + SI_CG_MULT_THERMAL_STATUS)
			& SI_CTF_TEMP_MASK) >> SI_CTF_TEMP_SHIFT;

		if (rawTemp & 0x200)
			finalTemp = 255;
		else
			finalTemp = rawTemp & 0x1ff;

		return finalTemp * 1000;
	} else if (info.chipsetID == RADEON_JUNIPER) {
		uint32 offset = (read32(info.registers + EVERGREEN_CG_THERMAL_CTRL)
			& EVERGREEN_TOFFSET_MASK) >> EVERGREEN_TOFFSET_SHIFT;
		rawTemp = (read32(info.registers + EVERGREEN_CG_TS0_STATUS)
			& EVERGREEN_TS0_ADC_DOUT_MASK) >> EVERGREEN_TS0_ADC_DOUT_SHIFT;

		if (offset & 0x100)
			finalTemp = rawTemp / 2 - (0x200 - offset);
		else
			finalTemp = rawTemp / 2 + offset;

		return finalTemp * 1000;
	} else if (info.chipsetID == RADEON_SUMO
		|| info.chipsetID == RADEON_SUMO2) {
		uint32 rawTemp = read32(info.registers + EVERGREEN_CG_THERMAL_STATUS)
			& 0xff;
		finalTemp = rawTemp - 49;

		return finalTemp * 1000;
	} else if (info.chipsetID >= RADEON_CEDAR) {
		rawTemp = (read32(info.registers + EVERGREEN_CG_MULT_THERMAL_STATUS)
			& EVERGREEN_ASIC_T_MASK) >> EVERGREEN_ASIC_T_SHIFT;

		if (rawTemp & 0x400)
			finalTemp = -256;
		else if (rawTemp & 0x200)
			finalTemp = 255;
		else if (rawTemp & 0x100) {
			finalTemp = rawTemp & 0x1ff;
			finalTemp |= ~0x1ff;
		} else
			finalTemp = rawTemp & 0xff;

		return (finalTemp * 1000) / 2;
	} else if (info.chipsetID >= RADEON_RV770) {
		rawTemp = (read32(info.registers + R700_CG_MULT_THERMAL_STATUS)
			& R700_ASIC_T_MASK) >> R700_ASIC_T_SHIFT;
		if (rawTemp & 0x400)
			finalTemp = -256;
		else if (rawTemp & 0x200)
			finalTemp = 255;
		else if (rawTemp & 0x100) {
			finalTemp = rawTemp & 0x1ff;
			finalTemp |= ~0x1ff;
		} else
			finalTemp = rawTemp & 0xff;

		return (finalTemp * 1000) / 2;
	} else if (info.chipsetID >= RADEON_R600) {
		rawTemp = (read32(info.registers + R600_CG_THERMAL_STATUS)
			& R600_ASIC_T_MASK) >> R600_ASIC_T_SHIFT;
		finalTemp = rawTemp & 0xff;

		if (rawTemp & 0x100)
			finalTemp -= 256;

		return finalTemp * 1000;
	}

	return -1;
}
