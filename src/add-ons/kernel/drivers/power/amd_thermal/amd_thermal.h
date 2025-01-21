/*
 * Copyright 2025, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _AMD_THERMAL_H
#define _AMD_THERMAL_H


enum { /* ioctl op-codes */
	drvOpGetThermalType = B_DEVICE_OP_CODES_END + 10001,
};


struct amd_thermal_type {
	/* Required fields for thermal devices */
	uint32 current_temp;
};

// Registers
#define	AMD_SMN_17H_ADDR	0x60
#define AMD_SMN_17H_DATA	0x64

#define AMD_SMU_17H_THM		0x59800
#define AMD_SMU_17H_CCD_THM(x, y)	(AMD_SMU_17H_THM + (x) + ((y) * 4))

#define GET_CURTMP(reg)		(((reg) >> 21) & 0x7ff)

#define CURTMP_17H_RANGE_SELECTION		(1 << 19)
#define CURTMP_17H_RANGE_ADJUST			490
#define CURTMP_CCD_VALID				(1 << 11)
#define CURTMP_CCD_MASK					0x7ff

#endif // _AMD_THERMAL_H
