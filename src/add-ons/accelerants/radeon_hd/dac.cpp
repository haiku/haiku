/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"
#include "dac.h"


#define TRACE_DAC
#ifdef TRACE_DAC
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


void
DACGetElectrical(uint8 type, uint8 dac,
	uint8 *bandgap, uint8 *whitefine)
{
	radeon_shared_info &info = *gInfo->shared_info;

	// These lookups are based on PCIID, maybe need
	// to extract more from AtomBIOS?
	struct
	{
		uint16 pciIdMin;
		uint16 pciIdMax;
		uint8 bandgap[2][4];
		uint8 whitefine[2][4];
	} list[] = {
		{ 0x791E, 0x791F,
			{ { 0x07, 0x07, 0x07, 0x07 },
			{ 0x07, 0x07, 0x07, 0x07 } },
			{ { 0x09, 0x09, 0x04, 0x09 },
			{ 0x09, 0x09, 0x04, 0x09 } },
		},
		{ 0x793F, 0x7942,
			{ { 0x09, 0x09, 0x09, 0x09 },
			{ 0x09, 0x09, 0x09, 0x09 } },
			{ { 0x0a, 0x0a, 0x08, 0x0a },
			{ 0x0a, 0x0a, 0x08, 0x0a } },
		},
		{ 0x9500, 0x9519,
			{ { 0x00, 0x00, 0x00, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 } },
			{ { 0x00, 0x00, 0x20, 0x00 },
			{ 0x25, 0x25, 0x26, 0x26 } },
		},
		{ 0, 0,
			{ { 0, 0, 0, 0 },
			{ 0, 0, 0, 0 } },
			{ { 0, 0, 0, 0 },
			{ 0, 0, 0, 0 } }
		}
	};

	*bandgap = 0;
	*whitefine = 0;

	// TODO : ATOM BIOS Bandgap / Whitefine lookup

	if (*bandgap == 0 || *whitefine == 0) {
		int i = 0;
		while (list[i].pciIdMin != 0) {
			if (list[i].pciIdMin <= info.device_id
				&& list[i].pciIdMax >= info.device_id) {
				if (*bandgap == 0)
					*bandgap = list[i].bandgap[dac][type];
				if (*whitefine == 0)
					*whitefine = list[i].whitefine[dac][type];
				break;
			}
			i++;
		}
		if (list[i].pciIdMin != 0) {
			TRACE("%s: found new BandGap / WhiteFine in table for card!\n",
				__func__);
		}
	}
}


/* For Cards >= r620 */
void
DACSetModern(uint8 dacIndex, uint32 crtid)
{
	bool istv = false;

	// BIG TODO : NTSC, PAL, ETC.  We assume VGA for now
	uint8 standard = FORMAT_VGA; /* VGA */
	uint32 mode = 2;
	uint32 source = istv ? 0x2 : crtid;

	uint8 bandGap;
	uint8 whiteFine;
	DACGetElectrical(standard, dacIndex, &bandGap, &whiteFine);

	uint32 mask = 0;
	if (bandGap)
		mask |= 0xFF << 16;
	if (whiteFine)
		mask |= 0xFF << 8;

	uint32 dacOffset = (dacIndex == 1) ? RV620_REG_DACA_OFFSET
		: RV620_REG_DACB_OFFSET;

	Write32Mask(OUT, dacOffset + RV620_DACA_MACRO_CNTL, mode, 0xFF);
		// no fine control yet

	Write32Mask(OUT, dacOffset + RV620_DACA_SOURCE_SELECT, source, 0x00000003);

	// enable tv if has TV mux(DACB) and istv
	if (dacIndex)
		Write32Mask(OUT, dacOffset + RV620_DACA_CONTROL2, istv << 8, 0x0100);

	// use fine control from white_fine control register
	Write32Mask(OUT, dacOffset + RV620_DACA_AUTO_CALIB_CONTROL, 0x0, 0x4);
	Write32Mask(OUT, dacOffset + RV620_DACA_BGADJ_SRC, 0x0, 0x30);
	Write32Mask(OUT, dacOffset + RV620_DACA_MACRO_CNTL,
		(bandGap << 16) | (whiteFine << 8), mask);

	// reset the FMT register
	// TODO : ah-la external DxFMTSet
	uint32 fmtOffset = (crtid == 0) ? FMT1_REG_OFFSET : FMT2_REG_OFFSET;
	Write32(OUT, fmtOffset + RV620_FMT1_BIT_DEPTH_CONTROL, 0);

	Write32Mask(OUT, fmtOffset + RV620_FMT1_CONTROL, 0,
		RV62_FMT_PIXEL_ENCODING);
		// 4:4:4 encoding

	Write32(OUT, fmtOffset + RV620_FMT1_CLAMP_CNTL, 0);
		// disable color clamping
}


/* For Cards < r620 */
void
DACSetLegacy(uint8 dacIndex, uint32 crtid)
{
	bool istv = false;

	// BIG TODO : NTSC, PAL, ETC.  We assume VGA for now
	uint8 standard = FORMAT_VGA; /* VGA */

	uint8 bandGap;
	uint8 whiteFine;
	DACGetElectrical(standard, dacIndex, &bandGap, &whiteFine);

	uint32 mask = 0;
	if (bandGap)
		mask |= 0xFF << 16;
	if (whiteFine)
		mask |= 0xFF << 8;

	uint32 dacOffset = (dacIndex == 1) ? REG_DACB_OFFSET : REG_DACA_OFFSET;

	Write32Mask(OUT, dacOffset + DACA_CONTROL1, standard, 0x000000FF);
	/* white level fine adjust */
	Write32Mask(OUT, dacOffset + DACA_CONTROL1, (bandGap << 16)
		| (whiteFine << 8), mask);

	if (istv) {
		/* tv enable */
		if (dacIndex) /* TV mux only available on DACB */
			Write32Mask(OUT, dacOffset + DACA_CONTROL2,
				0x00000100, 0x0000FF00);

		/* select tv encoder */
		Write32Mask(OUT, dacOffset + DACA_SOURCE_SELECT,
			0x00000002, 0x00000003);
	} else {
		if (dacIndex) /* TV mux only available on DACB */
			Write32Mask(OUT, dacOffset + DACA_CONTROL2, 0, 0x0000FF00);

		/* select a crtc */
		Write32Mask(OUT, dacOffset + DACA_SOURCE_SELECT,
			crtid & 0x01, 0x00000003);
	}

	Write32Mask(OUT, dacOffset + DACA_FORCE_OUTPUT_CNTL,
		0x00000701, 0x00000701);
	Write32Mask(OUT, dacOffset + DACA_FORCE_DATA,
		0, 0x0000FFFF);
}


void
DACSet(uint8 dacIndex, uint32 crtid)
{
	radeon_shared_info &info = *gInfo->shared_info;

	TRACE("%s: dac %d to crt %d\n", __func__, dacIndex, crtid);

	if (info.device_chipset < (RADEON_R600 | 0x20))
		DACSetLegacy(dacIndex, crtid);
	else
		DACSetModern(dacIndex, crtid);
}


/* For Cards >= r620 */
void
DACPowerModern(uint8 dacIndex, int mode)
{
	TRACE("%s: dacIndex: %d; mode: %d\n", __func__, dacIndex, mode);

	uint32 dacOffset = (dacIndex == 1) ? RV620_REG_DACB_OFFSET
		: RV620_REG_DACA_OFFSET;
	uint32 powerdown;

	switch (mode) {
		case RHD_POWER_ON:
			TRACE("%s: dacIndex: %d; POWER_ON\n", __func__, dacIndex);
			// TODO : SensedType Detection?
			powerdown = 0;
			if (!(Read32(OUT, dacOffset + RV620_DACA_ENABLE) & 0x01))
				Write32Mask(OUT, dacOffset + RV620_DACA_ENABLE, 0x1, 0xff);
			Write32Mask(OUT, dacOffset + RV620_DACA_FORCE_OUTPUT_CNTL,
				0x01, 0x01);
			Write32Mask(OUT, dacOffset + RV620_DACA_POWERDOWN, 0x0, 0xff);
			snooze(20);
			Write32Mask(OUT, dacOffset + RV620_DACA_POWERDOWN,
				powerdown, 0xFFFFFF00);
			Write32(OUT, dacOffset + RV620_DACA_FORCE_OUTPUT_CNTL, 0x0);
			Write32(OUT, dacOffset + RV620_DACA_SYNC_TRISTATE_CONTROL, 0x0);
			return;
		case RHD_POWER_RESET:
			TRACE("%s: dacIndex: %d; POWER_RESET\n", __func__, dacIndex);
			// No action
			return;
		case RHD_POWER_SHUTDOWN:
			TRACE("%s: dacIndex: %d; POWER_SHUTDOWN\n", __func__, dacIndex);
		default:
			Write32(OUT, dacOffset + RV620_DACA_POWERDOWN, 0x01010100);
			Write32(OUT, dacOffset + RV620_DACA_POWERDOWN, 0x01010101);
			Write32(OUT, dacOffset + RV620_DACA_ENABLE, 0);
			Write32Mask(OUT, dacOffset + RV620_DACA_FORCE_DATA, 0, 0xffff);
			Write32Mask(OUT, dacOffset + RV620_DACA_FORCE_OUTPUT_CNTL,
				0x701, 0x701);
			return;
	}
}


/* For Cards < r620 */
void
DACPowerLegacy(uint8 dacIndex, int mode)
{
	uint32 dacOffset = (dacIndex == 1) ? REG_DACB_OFFSET : REG_DACA_OFFSET;
	uint32 powerdown;

	switch (mode) {
		case RHD_POWER_ON:
			TRACE("%s: dacIndex: %d; POWER_ON\n", __func__, dacIndex);
			// TODO : SensedType Detection?
			powerdown = 0;
			Write32(OUT, dacOffset + DACA_ENABLE, 1);
			Write32(OUT, dacOffset + DACA_POWERDOWN, 0);
			snooze(14);
			Write32Mask(OUT, dacOffset + DACA_POWERDOWN, powerdown, 0xFFFFFF00);
			snooze(2);
			Write32(OUT, dacOffset + DACA_FORCE_OUTPUT_CNTL, 0);
			Write32Mask(OUT, dacOffset + DACA_SYNC_SELECT, 0, 0x00000101);
			Write32(OUT, dacOffset + DACA_SYNC_TRISTATE_CONTROL, 0);
			return;
		case RHD_POWER_RESET:
			TRACE("%s: dacIndex: %d; POWER_RESET\n", __func__, dacIndex);
			// No action
			return;
		case RHD_POWER_SHUTDOWN:
			TRACE("%s: dacIndex: %d; POWER_SHUTDOWN\n", __func__, dacIndex);
		default:
			Write32Mask(OUT, dacOffset + DACA_FORCE_DATA, 0, 0x0000FFFF);
			Write32Mask(OUT, dacOffset + DACA_FORCE_OUTPUT_CNTL,
				0x0000701, 0x0000701);
			Write32(OUT, dacOffset + DACA_POWERDOWN, 0x01010100);
			Write32(OUT, dacOffset + DACA_POWERDOWN, 0x01010101);
			Write32(OUT, dacOffset + DACA_ENABLE, 0);
			Write32(OUT, dacOffset + DACA_ENABLE, 0);
			return;
	}
}


void
DACPower(uint8 dacIndex, int mode)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset < (RADEON_R600 | 0x20))
		DACPowerLegacy(dacIndex, mode);
	else
		DACPowerModern(dacIndex, mode);
}

