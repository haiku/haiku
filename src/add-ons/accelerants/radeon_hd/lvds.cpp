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
#include "lvds.h"


#define TRACE_LVDS
#ifdef TRACE_LVDS
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


// Static microvoltage values taken from Xorg driver
static struct R5xxTMDSBMacro {
	uint16 device;
	uint32 macroSingle;
	uint32 macroDual;
} R5xxTMDSBMacro[] = {
	/*
	 * this list isn't complete yet.
	 *  Some more values for dual need to be dug up
	 */
	{ 0x7104, 0x00F20616, 0x00F20616 }, // R520
	{ 0x7142, 0x00F2061C, 0x00F2061C }, // RV515
	{ 0x7145, 0x00F1061D, 0x00F2061D },
	{ 0x7146, 0x00F1061D, 0x00F1061D }, // RV515
	{ 0x7147, 0x0082041D, 0x0082041D }, // RV505
	{ 0x7149, 0x00F1061D, 0x00D2061D },
	{ 0x7152, 0x00F2061C, 0x00F2061C }, // RV515
	{ 0x7183, 0x00B2050C, 0x00B2050C }, // RV530
	{ 0x71C0, 0x00F1061F, 0x00f2061D },
	{ 0x71C1, 0x0062041D, 0x0062041D }, // RV535
	{ 0x71C2, 0x00F1061D, 0x00F2061D }, // RV530
	{ 0x71C5, 0x00D1061D, 0x00D2061D },
	{ 0x71C6, 0x00F2061D, 0x00F2061D }, // RV530
	{ 0x71D2, 0x00F10610, 0x00F20610 }, // RV530: atombios uses 0x00F1061D
	{ 0x7249, 0x00F1061D, 0x00F1061D }, // R580
	{ 0x724B, 0x00F10610, 0x00F10610 }, // R580: atombios uses 0x00F1061D
	{ 0x7280, 0x0042041F, 0x0042041F }, // RV570
	{ 0x7288, 0x0042041F, 0x0042041F }, // RV570
	{ 0x791E, 0x0001642F, 0x0001642F }, // RS690
	{ 0x791F, 0x0001642F, 0x0001642F }, // RS690
	{ 0x9400, 0x00020213, 0x00020213 }, // R600
	{ 0x9401, 0x00020213, 0x00020213 }, // R600
	{ 0x9402, 0x00020213, 0x00020213 }, // R600
	{ 0x9403, 0x00020213, 0x00020213 }, // R600
	{ 0x9405, 0x00020213, 0x00020213 }, // R600
	{ 0x940A, 0x00020213, 0x00020213 }, // R600
	{ 0x940B, 0x00020213, 0x00020213 }, // R600
	{ 0x940F, 0x00020213, 0x00020213 }, // R600
	{ 0, 0, 0 } /* End marker */
};

static struct RV6xxTMDSBMacro {
	uint16 device;
	uint32 macro;
	uint32 tx;
	uint32 preEmphasis;
} RV6xxTMDSBMacro[] = {
	{ 0x94C1, 0x01030311, 0x10001A00, 0x01801015}, /* RV610 */
	{ 0x94C3, 0x01030311, 0x10001A00, 0x01801015}, /* RV610 */
	{ 0x9501, 0x0533041A, 0x020010A0, 0x41002045}, /* RV670 */
	{ 0x9505, 0x0533041A, 0x020010A0, 0x41002045}, /* RV670 */
	{ 0x950F, 0x0533041A, 0x020010A0, 0x41002045}, /* R680  */
	{ 0x9587, 0x01030311, 0x10001C00, 0x01C01011}, /* RV630 */
	{ 0x9588, 0x01030311, 0x10001C00, 0x01C01011}, /* RV630 */
	{ 0x9589, 0x01030311, 0x10001C00, 0x01C01011}, /* RV630 */
	{ 0, 0, 0, 0} /* End marker */
};


void
LVDSVoltageControl(uint8 lvdsIndex)
{
	bool dualLink = false; // TODO : DualLink
	radeon_shared_info &info = *gInfo->shared_info;

	// TODO : Special RS690 RS600 IGP oneoffs

	if (info.device_chipset < (RADEON_R600 | 0x70))
		Write32Mask(OUT, LVTMA_REG_TEST_OUTPUT, 0x00100000, 0x00100000);

	// Micromanage voltages
	if (info.device_chipset < (RADEON_R600 | 0x10)) {
		for (uint32 i = 0; R5xxTMDSBMacro[i].device; i++) {
			if (R5xxTMDSBMacro[i].device == info.device_id) {
				if (dualLink) {
					Write32(OUT, LVTMA_MACRO_CONTROL,
						R5xxTMDSBMacro[i].macroDual);
				} else {
					Write32(OUT, LVTMA_MACRO_CONTROL,
						R5xxTMDSBMacro[i].macroSingle);
				}
				return;
			}
		}
		TRACE("%s : unhandled chipset 0x%X\n", __func__, info.device_id);
	} else {
		for (uint32 i = 0; RV6xxTMDSBMacro[i].device; i++) {
			if (RV6xxTMDSBMacro[i].device == info.device_id) {
				Write32(OUT, LVTMA_MACRO_CONTROL, RV6xxTMDSBMacro[i].macro);
				Write32(OUT, LVTMA_TRANSMITTER_ADJUST,
					RV6xxTMDSBMacro[i].tx);
				Write32(OUT, LVTMA_PREEMPHASIS_CONTROL,
					RV6xxTMDSBMacro[i].preEmphasis);
				return;
			}
		}
		TRACE("%s : unhandled chipset 0x%X\n", __func__, info.device_id);
	}
}


void
LVDSPower(uint8 lvdsIndex, int command)
{
	bool dualLink = false;	// TODO : dualLink

	if (lvdsIndex == 0) {
		TRACE("LVTMA not yet supported :(\n");
		return;
	} else {
		// Select TMDSB (which is on LVDS)
		Write32Mask(OUT, LVTMA_MODE, 0x00000001, 0x00000001);
	}

	switch (command) {
		case RHD_POWER_ON:
			TRACE("%s: LVDS %d Power On\n", __func__, lvdsIndex);
			Write32Mask(OUT, LVTMA_CNTL, 0x1, 0x00000001);

			if (dualLink) {
				Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE,
					0x00003E3E, 0x00003E3E);
			} else {
				Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE,
					0x0000003E, 0x00003E3E);
			}

			Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0x00000001, 0x00000001);
			snooze(2);
			Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x00000002);
			// TODO : Enable HDMI
			return;

		case RHD_POWER_RESET:
			TRACE("%s: LVDS %d Power Reset\n", __func__, lvdsIndex);
			Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE, 0, 0x00003E3E);
			return;

		case RHD_POWER_SHUTDOWN:
		default:
			TRACE("%s: LVDS %d Power Shutdown\n", __func__, lvdsIndex);
			Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
			snooze(2);
			Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x00000001);

			Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE, 0, 0x00003E3E);
			Write32Mask(OUT, LVTMA_CNTL, 0, 0x00000001);
			// TODO : Disable HDMI
			return;
	}
}


status_t
LVDSSet(uint8 lvdsIndex, display_mode *mode)
{
	TRACE("%s: LVDS %d Set\n", __func__, lvdsIndex);

	uint16 crtid = 0; // TODO : assume CRT0

	if (lvdsIndex == 0) {
		TRACE("LVTMA not yet supported :(\n");
		return B_ERROR;
	} else {
		// Select TMDSB (which is on LVDS)
		Write32Mask(OUT, LVTMA_MODE, 0x00000001, 0x00000001);
	}

	// Clear HPD events
	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x0000000C);
	Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE, 0, 0x00070000);

	Write32Mask(OUT, LVTMA_CNTL, 0, 0x00000010);

	// Disable LVDS (TMDSB) transmitter
	Write32Mask(OUT, LVTMA_TRANSMITTER_ENABLE, 0, 0x00003E3E);

	// Reset dither bits
	Write32Mask(OUT, LVTMA_BIT_DEPTH_CONTROL, 0, 0x00010101);
	Write32Mask(OUT, LVTMA_BIT_DEPTH_CONTROL, LVTMA_DITHER_RESET_BIT,
		LVTMA_DITHER_RESET_BIT);
	snooze(2);
	Write32Mask(OUT, LVTMA_BIT_DEPTH_CONTROL, 0, LVTMA_DITHER_RESET_BIT);
	Write32Mask(OUT, LVTMA_BIT_DEPTH_CONTROL, 0, 0xF0000000);
		// Undocumented depth control bit from Xorg

	Write32Mask(OUT, LVTMA_CNTL, 0x00001000, 0x00011000);
		// Reset phase for vsync and use RGB color

	Write32Mask(OUT, LVTMA_SOURCE_SELECT, crtid, 0x00010101);
		// Assign to CRTC

	Write32(OUT, LVTMA_COLOR_FORMAT, 0);

	// TODO : Detect DualLink via SynthClock?
	Write32Mask(OUT, LVTMA_CNTL, 0, 0x01000000);

	// TODO : only > R600 - disable split mode
	Write32Mask(OUT, LVTMA_CNTL, 0, 0x20000000);

	Write32Mask(OUT, LVTMA_FORCE_OUTPUT_CNTL, 0, 0x00000001);
		// Disable force data

	Write32Mask(OUT, LVTMA_DCBALANCER_CONTROL, 0x00000001, 0x00000001);
		// Enable DC balancer

	LVDSVoltageControl(lvdsIndex);

	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0x00000010, 0x00000010);
		// use IDCLK

	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0x20000000, 0x20000000);
		// use clock selected by next write

	// TODO : coherent mode?
	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x10000000);

	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x03FF0000);
		// Clear current LVDS clock

	// Reset PLL's
	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
	snooze(2);
	Write32Mask(OUT, LVTMA_TRANSMITTER_CONTROL, 0, 0x00000002);
	snooze(20);

	// Restart LVDS data sync
	Write32Mask(OUT, LVTMA_DATA_SYNCHRONIZATION, 0x00000001, 0x00000001);
	Write32Mask(OUT, LVTMA_DATA_SYNCHRONIZATION, 0x00000100, 0x00000100);
	snooze(20);
	Write32Mask(OUT, LVTMA_DATA_SYNCHRONIZATION, 0, 0x00000001);

	// TODO : Set HDMI mode

	return B_OK;
}


void
LVDSAllIdle()
{
	LVDSPower(1, RHD_POWER_RESET);
}
