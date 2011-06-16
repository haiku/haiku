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
				if (*bandgap == 0) *bandgap = list[i].bandgap[dac][type];
				if (*whitefine == 0) *whitefine = list[i].whitefine[dac][type];
				break;
			}
			i++;
		}
		if (list[i].pciIdMin != 0)
			TRACE("%s: found new BandGap / WhiteFine in table for card!\n",
				__func__);
	}
}


void
DACSet(uint8 dacIndex, uint32 crtid)
{
	bool istv = false;
	uint8 bandGap;
	uint8 whiteFine;

	// BIG TODO : NTSC, PAL, ETC.  We assume VGA for now
	uint8 standard = FORMAT_VGA; /* VGA */

	DACGetElectrical(standard, dacIndex, &bandGap, &whiteFine);

	uint32 mask = 0;
	if (bandGap) mask |= 0xFF << 16;
	if (whiteFine) mask |= 0xFF << 8;

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
DACPower(uint8 dacIndex, int mode)
{
	uint32 dacOffset = (dacIndex == 1) ? REG_DACB_OFFSET : REG_DACA_OFFSET;
	uint32 powerdown;

	switch (mode) {
		// TODO : RHD_POWER_OFF, etc
		case RHD_POWER_ON:
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
	}
}
