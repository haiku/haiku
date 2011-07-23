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
#include "tmds.h"


#define TRACE_TMDS
#ifdef TRACE_TMDS
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


/*
 * From Xorg Driver
 * This information is not provided in an atombios data table.
 */
static struct R5xxTMDSAMacro {
	uint16 device;
	uint32 macro;
} R5xxTMDSAMacro[] = {
	{ 0x7104, 0x00C00414 }, /* R520  */
	{ 0x7142, 0x00A00415 }, /* RV515 */
	{ 0x7145, 0x00A00416 }, /* M54   */
	{ 0x7146, 0x00C0041F }, /* RV515 */
	{ 0x7147, 0x00C00418 }, /* RV505 */
	{ 0x7149, 0x00800416 }, /* M56   */
	{ 0x7152, 0x00A00415 }, /* RV515 */
	{ 0x7183, 0x00600412 }, /* RV530 */
	{ 0x71C1, 0x00C0041F }, /* RV535 */
	{ 0x71C2, 0x00A00416 }, /* RV530 */
	{ 0x71C4, 0x00A00416 }, /* M56   */
	{ 0x71C5, 0x00A00416 }, /* M56   */
	{ 0x71C6, 0x00A00513 }, /* RV530 */
	{ 0x71D2, 0x00A00513 }, /* RV530 */
	{ 0x71D5, 0x00A00513 }, /* M66   */
	{ 0x7249, 0x00A00513 }, /* R580  */
	{ 0x724B, 0x00A00513 }, /* R580  */
	{ 0x7280, 0x00C0041F }, /* RV570 */
	{ 0x7288, 0x00C0041F }, /* RV570 */
	{ 0x9400, 0x00910419 }, /* R600: */
	{ 0, 0} /* End marker */
};

static struct Rv6xxTMDSAMacro {
	uint16 device;
	uint32 pll;
	uint32 tx;
} Rv6xxTMDSAMacro[] = {
	{ 0x94C1, 0x00010416, 0x00010308 }, /* RV610 */
	{ 0x94C3, 0x00010416, 0x00010308 }, /* RV610 */
	{ 0x9501, 0x00010416, 0x00010308 }, /* RV670: != atombios */
	{ 0x9505, 0x00010416, 0x00010308 }, /* RV670: != atombios */
	{ 0x950F, 0x00010416, 0x00010308 }, /* R680 : != atombios */
	{ 0x9581, 0x00030410, 0x00301044 }, /* M76 */
	{ 0x9587, 0x00010416, 0x00010308 }, /* RV630 */
	{ 0x9588, 0x00010416, 0x00010388 }, /* RV630 */
	{ 0x9589, 0x00010416, 0x00010388 }, /* RV630 */
	{ 0, 0, 0} /* End marker */
};


void
TMDSVoltageControl(uint8 tmdsIndex)
{
	int i;

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset < (RADEON_R600 | 0x10)) {
		for (i = 0; R5xxTMDSAMacro[i].device; i++) {
			if (R5xxTMDSAMacro[i].device == info.device_id) {
				Write32(OUT, TMDSA_MACRO_CONTROL, R5xxTMDSAMacro[i].macro);
				return;
			}
		}
		TRACE("%s : unhandled chipset 0x%X\n", __func__, info.device_id);
	} else {
		for (i = 0; Rv6xxTMDSAMacro[i].device; i++) {
			if (Rv6xxTMDSAMacro[i].device == info.device_id) {
				Write32(OUT, TMDSA_PLL_ADJUST, Rv6xxTMDSAMacro[i].pll);
				Write32(OUT, TMDSA_TRANSMITTER_ADJUST, Rv6xxTMDSAMacro[i].tx);
				return;
			}
		}
		TRACE("%s : unhandled chipset 0x%X\n", __func__, info.device_id);
	}
}


bool
TMDSSense(uint8 tmdsIndex)
{
	// For now radeon cards only have TMDSA and no TMDSB

	// Backup current TMDS values
	uint32 loadDetect = Read32(OUT, TMDSA_LOAD_DETECT);

	// Call TMDS load detect on TMDSA
	Write32Mask(OUT, TMDSA_LOAD_DETECT, 0x00000001, 0x00000001);
	snooze(1);

	// Check result of TMDS load detect
	bool result = Read32(OUT, TMDSA_LOAD_DETECT) & 0x00000010;

	// Restore saved value
	Write32Mask(OUT, TMDSA_LOAD_DETECT, loadDetect, 0x00000001);

	return result;
}


status_t
TMDSPower(uint8 tmdsIndex, int command)
{
	// For now radeon cards only have TMDSA and no TMDSB
	switch (command) {
		case RHD_POWER_ON:
		{
			TRACE("%s: TMDS %d Power On\n", __func__, tmdsIndex);
			Write32Mask(OUT, TMDSA_CNTL, 0x1, 0x00000001);
			Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0x00000001, 0x00000001);
			snooze(20);

			// Reset transmitter
			Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
			snooze(2);
			Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000002);

			snooze(30);

			// Restart data sync
			// TODO : <R600
			Write32Mask(OUT, TMDSA_DATA_SYNCHRONIZATION_R600,
				0x00000001, 0x00000001);
			snooze(2);
			Write32Mask(OUT, TMDSA_DATA_SYNCHRONIZATION_R600,
				0x00000100, 0x00000100);
			Write32Mask(OUT, TMDSA_DATA_SYNCHRONIZATION_R600, 0, 0x00000001);

			// TODO : DualLink, for now we assume not.
			Write32Mask(OUT, TMDSA_TRANSMITTER_ENABLE, 0x0000001F, 0x00001F1F);

			// TODO : HdmiEnable true
			return B_OK;
		}
		case RHD_POWER_RESET:
		{
			TRACE("%s: TMDS %d Power Reset\n", __func__, tmdsIndex);
			Write32Mask(OUT, TMDSA_TRANSMITTER_ENABLE, 0, 0x00001F1F);
			return B_OK;
		}
		case RHD_POWER_SHUTDOWN:
		default:
		{
			Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
			snooze(2);
			Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000001);
			Write32Mask(OUT, TMDSA_TRANSMITTER_ENABLE, 0, 0x00001F1F);
			Write32Mask(OUT, TMDSA_CNTL, 0, 0x00000001);
			// TODO : HdmiEnable false
		}
	}
	return B_OK;
}


status_t
TMDSSet(uint8 tmdsIndex, display_mode *mode)
{
	TRACE("%s: TMDS %d Set\n", __func__, tmdsIndex);

	// Clear HPD events
	Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0, 0x0000000C);
	Write32Mask(OUT, TMDSA_TRANSMITTER_ENABLE, 0, 0x00070000);
	Write32Mask(OUT, TMDSA_CNTL, 0, 0x00000010);

	// Disable transmitter
	Write32Mask(OUT, TMDSA_TRANSMITTER_ENABLE, 0, 0x00001D1F);

	// Disable bit reduction and reset dither
	Write32Mask(OUT, TMDSA_BIT_DEPTH_CONTROL, 0, 0x00010101);
	// TODO : <R600
	Write32Mask(OUT, TMDSA_BIT_DEPTH_CONTROL, 0x02000000, 0x02000000);
	snooze(2);
	Write32Mask(OUT, TMDSA_BIT_DEPTH_CONTROL, 0, 0x02000000);

	// Reset phase on vsync
	Write32Mask(OUT, TMDSA_CNTL, 0x00001000, 0x00011000);
	// TODO : for now we assume crt 0, needs refactoring
	Write32Mask(OUT, TMDSA_SOURCE_SELECT, 0, 0x00010101);

	Write32(OUT, TMDSA_COLOR_FORMAT, 0);

	// Set single link for now
	// TODO : SynthClock > 165000 this is DualLink
	Write32Mask(OUT, TMDSA_CNTL, 0, 0x01000000);

	// Disable force data
	Write32Mask(OUT, TMDSA_FORCE_OUTPUT_CNTL, 0, 0x00000001);

	// Enable DC balancer
	Write32Mask(OUT, TMDSA_DCBALANCER_CONTROL, 0x00000001, 0x00000001);

	TMDSVoltageControl(tmdsIndex);

	// USE IDCLK
	Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0x00000010, 0x00000010);

	// TODO : if coherent? For now lets asume false
	Write32Mask(OUT, TMDSA_TRANSMITTER_CONTROL, 0x10000000, 0x10000000);

	// TODO : HdmiSetMode(mode)
	return B_OK;
}
