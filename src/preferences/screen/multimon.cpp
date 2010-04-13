/*
	Copyright (c) 2002, Thomas Kurschel

	Part of Radeon driver
	Multi-Monitor Settings interface
*/


#include "multimon.h"
#include "accelerant_ext.h"

#include <OS.h>
#include <Screen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// prepare parameters so they recognized as tunneled settings
static void
PrepareTunnel(display_mode *mode, display_mode *low, display_mode *high)
{
	memset(mode, 0, sizeof(*mode));

	// mark modes as settings tunnel
	mode->space = low->space = high->space = 0;
	low->virtual_width = 0xffff;
	low->virtual_height = 0xffff;
	high->virtual_width = 0;
	high->virtual_height = 0;
	mode->timing.pixel_clock = 0;
	low->timing.pixel_clock = 'TKTK';
	high->timing.pixel_clock = 'KTKT';
}


// retrieve value of setting "code"
static status_t
GetSetting(BScreen *screen, uint16 code, uint32 *setting)
{
	display_mode mode, low, high;
	status_t result;

	result = TestMultiMonSupport(screen);
	if (result != B_OK)
		return result;

	PrepareTunnel(&mode, &low, &high);

	mode.h_display_start = code;
	mode.v_display_start = 0;

	result = screen->ProposeMode(&mode, &low, &high);
	if (result != B_OK)
		return result;

	*setting = mode.timing.flags;

	return B_OK;
}


// set setting "code" to "value"
static status_t
SetSetting(BScreen *screen, uint16 code, uint32 value)
{
	display_mode mode, low, high;
	status_t result;

	result = TestMultiMonSupport(screen);
	if (result != B_OK)
		return result;

	PrepareTunnel(&mode, &low, &high);

	mode.h_display_start = code;
	mode.v_display_start = 1;
	mode.timing.flags = value;

	return screen->ProposeMode(&mode, &low, &high);
}


// retrieve n-th supported value of setting "code"
static status_t
GetNthSupportedSetting(BScreen *screen, uint16 code, int32 idx,
	uint32 *setting)
{
	display_mode mode, low, high;
	status_t result;

	result = TestMultiMonSupport(screen);
	if (result != B_OK)
		return result;

	PrepareTunnel(&mode, &low, &high);

	mode.h_display_start = code;
	mode.v_display_start = 2;
	mode.timing.flags = idx;

	result = screen->ProposeMode(&mode, &low, &high);
	if (result != B_OK)
		return result;

	*setting = mode.timing.flags;

	return B_OK;
}


// get current Swap Displays settings
status_t
GetSwapDisplays(BScreen *screen, bool *swap)
{
	status_t result;
	uint32 tmp;

	result = GetSetting(screen, ms_swap, &tmp);
	if (result != B_OK)
		return result;

	*swap = tmp != 0;

	return B_OK;
}


// set "Swap Displays"
status_t
SetSwapDisplays(BScreen *screen, bool swap)
{
	return SetSetting(screen, ms_swap, swap);
}


// get current "Use Laptop Panel" settings
status_t
GetUseLaptopPanel(BScreen *screen, bool *use)
{
	status_t result;
	uint32 tmp;

	result = GetSetting(screen, ms_use_laptop_panel, &tmp);
	if (result != B_OK)
		return result;

	*use = tmp != 0;
	return B_OK;
}


// set "Use Laptop Panel"
status_t
SetUseLaptopPanel(BScreen *screen, bool use)
{
	return SetSetting(screen, ms_use_laptop_panel, use);
}


// get n-th supported TV standard
status_t
GetNthSupportedTVStandard(BScreen *screen, int idx, uint32 *standard)
{
	return GetNthSupportedSetting(
		screen, ms_tv_standard, (int32)idx, standard);
}


// get current TV Standard settings
status_t
GetTVStandard(BScreen *screen, uint32 *standard)
{
	return GetSetting(screen, ms_tv_standard, standard);
}


// set TV Standard
status_t
SetTVStandard(BScreen *screen, uint32 standard)
{
	return SetSetting(screen, ms_tv_standard, standard);
}


// Verify existence of Multi-Monitor Settings Tunnel
status_t
TestMultiMonSupport(BScreen *screen)
{
	display_mode *modeList = NULL;
	display_mode low, high;
	uint32 count;
	status_t result;

	// take any valid mode
	result = screen->GetModeList(&modeList, &count);
	if (result != B_OK)
		return result;

	if (count < 1)
		return B_ERROR;

	// set request bits
	modeList[0].timing.flags |= RADEON_MODE_MULTIMON_REQUEST;
	modeList[0].timing.flags &= ~RADEON_MODE_MULTIMON_REPLY;
	low = high = modeList[0];

	result = screen->ProposeMode(&modeList[0], &low, &high);
	if (result != B_OK)
		goto out;

	// check reply bits
	if ((modeList[0].timing.flags & RADEON_MODE_MULTIMON_REQUEST) == 0
		&& (modeList[0].timing.flags & RADEON_MODE_MULTIMON_REPLY) != 0)
		result = B_OK;
	else
		result = B_ERROR;

out:
	free(modeList);
	return result;
}
