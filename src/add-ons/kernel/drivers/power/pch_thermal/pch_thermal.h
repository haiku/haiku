/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCH_THERMAL_H
#define _PCH_THERMAL_H


enum { /* ioctl op-codes */
	drvOpGetThermalType = B_DEVICE_OP_CODES_END + 10001,
};


struct pch_thermal_type {
	/* Required fields for thermal devices */
	uint32 critical_temp;
	uint32 current_temp;

	/* Optional HOT temp, S4 sleep threshold */
	uint32 hot_temp;
};

// Registers
#define PCH_THERMAL_TEMP			0x00
#define PCH_THERMAL_TEMP_TSR_SHIFT		0
#define PCH_THERMAL_TEMP_TSR_MASK		0xff
#define PCH_THERMAL_TSC				0x04
#define PCH_THERMAL_TSC_CPDE			(1 << 0)
#define PCH_THERMAL_TSC_PLDB			(1 << 7)
#define PCH_THERMAL_TSS				0x06
#define PCH_THERMAL_TSS_SMIS			(1 << 2)
#define PCH_THERMAL_TSS_GPES			(1 << 3)
#define PCH_THERMAL_TSS_TSDSS			(1 << 4)
#define PCH_THERMAL_TSEL			0x08
#define PCH_THERMAL_TSEL_ETS			(1 << 0)
#define PCH_THERMAL_TSEL_PLDB			(1 << 7)
#define PCH_THERMAL_TSREL			0x0a
#define PCH_THERMAL_TSREL_ESTR			(1 << 0)
#define PCH_THERMAL_TSREL_PLDB			(1 << 7)
#define PCH_THERMAL_TSMIC			0x0c
#define PCH_THERMAL_TSMIC_ATST			(1 << 0)
#define PCH_THERMAL_TSMIC_PLDB			(1 << 7)
#define PCH_THERMAL_CTT				0x10
#define PCH_THERMAL_CTT_CTRIP_SHIFT		0
#define PCH_THERMAL_CTT_CTRIP_MASK		0x1ff
#define PCH_THERMAL_TAHV			0x14
#define PCH_THERMAL_TAHV_AH_SHIFT		0
#define PCH_THERMAL_TAHV_AH_MASK		0x1ff
#define PCH_THERMAL_TALV			0x18
#define PCH_THERMAL_TALV_AL_SHIFT		0
#define PCH_THERMAL_TALV_AL_MASK		0x1ff
#define PCH_THERMAL_TSPM			0x1c
#define PCH_THERMAL_TSPM_LTT_SHIFT		0
#define PCH_THERMAL_TSPM_LTT_MASK		0x1ff
#define PCH_THERMAL_TSPM_MAXTSST_SHIFT	9
#define PCH_THERMAL_TSPM_MAXTSST_MASK	0xf
#define PCH_THERMAL_TSPM_DTSSIC0		(1 << 13)
#define PCH_THERMAL_TSPM_DTSSS0EN		(1 << 14)
#define PCH_THERMAL_TSPM_TSPMLOCK		(1 << 15)
#define PCH_THERMAL_TL				0x40
#define PCH_THERMAL_TL_T0L_SHIFT		0
#define PCH_THERMAL_TL_T0L_MASK			0x1ff
#define PCH_THERMAL_TL_T1L_SHIFT		10
#define PCH_THERMAL_TL_T1L_MASK			0x1ff
#define PCH_THERMAL_TL_T2L_SHIFT		20
#define PCH_THERMAL_TL_T2L_MASK			0x1ff
#define PCH_THERMAL_TL_TTEN				(1 << 29)
#define PCH_THERMAL_TL_TT13EN			(1 << 30)
#define PCH_THERMAL_TL_TTL				(1 << 31)
#define PCH_THERMAL_TL2				0x50
#define PCH_THERMAL_TL2_TL2LOCK_SHIFT	15
#define PCH_THERMAL_TL2_PCMTEN_SHIFT	16
#define PCH_THERMAL_PHL				0x60
#define PCH_THERMAL_PHL_PHLL_SHIFT		0
#define PCH_THERMAL_PHL_PHLL_MASK		0x1ff
#define PCH_THERMAL_PHL_PHLE			(1 << 15)
#define PCH_THERMAL_PHLC			0x62
#define PCH_THERMAL_PHLC_PHLL			(1 << 0)
#define PCH_THERMAL_TAS				0x80
#define PCH_THERMAL_TAS_ALHE			(1 << 0)
#define PCH_THERMAL_TAS_AHLE			(1 << 1)
#define PCH_THERMAL_TSPIEN			0x82
#define PCH_THERMAL_TSPIEN_ALHEN		(1 << 0)
#define PCH_THERMAL_TSPIEN_AHLEN		(1 << 1)
#define PCH_THERMAL_TSGPEN			0x84
#define PCH_THERMAL_TSGPEN_ALHEN		(1 << 0)
#define PCH_THERMAL_TSGPEN_AHLEN		(1 << 1)
#define PCH_THERMAL_TCFD			0xf0
#define PCH_THERMAL_TCFD_TCD			(1 << 0)


#endif // _PCH_THERMAL_H

