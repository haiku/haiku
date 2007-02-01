/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenMode.h"

#include <stdlib.h>
#include <stdio.h>


/* Note, this headers defines a *private* interface to the Radeon accelerant.
 * It's a solution that works with the current BeOS interface that Haiku
 * adopted.
 * However, it's not a nice and clean solution. Don't use this header in any
 * application if you can avoid it. No other driver is using this, or should
 * be using this.
 * It will be replaced as soon as we introduce an updated accelerant interface
 * which may even happen before R1 hits the streets.
 */

#include "multimon.h"	// the usual: DANGER WILL, ROBINSON!


static combine_mode
get_combine_mode(display_mode& mode)
{
	if (mode.flags & B_SCROLL == 0)
		return kCombineDisable;

	if (mode.virtual_width == mode.timing.h_display * 2)
		return kCombineHorizontally;

	if (mode.virtual_height == mode.timing.v_display * 2)
		return kCombineVertically;

	return kCombineDisable;
}


static float
get_refresh_rate(display_mode& mode)
{
	// we have to be catious as refresh rate cannot be controlled directly,
	// so it suffers under rounding errors and hardware restrictions
	return rint(10 * float(mode.timing.pixel_clock * 1000) / 
		float(mode.timing.h_total * mode.timing.v_total)) / 10.0;
}


/** helper to sort modes by resolution */

static int
compare_mode(const void* _mode1, const void* _mode2)
{
	display_mode *mode1 = (display_mode *)_mode1;
	display_mode *mode2 = (display_mode *)_mode2;
	combine_mode combine1, combine2;
	uint16 width1, width2, height1, height2;

	combine1 = get_combine_mode(*mode1);
	combine2 = get_combine_mode(*mode2);

	width1 = mode1->virtual_width;
	height1 = mode1->virtual_height;
	width2 = mode2->virtual_width;
	height2 = mode2->virtual_height;

	if (combine1 == kCombineHorizontally)
		width1 /= 2;
	if (combine1 == kCombineVertically)
		height1 /= 2;
	if (combine2 == kCombineHorizontally)
		width2 /= 2;
	if (combine2 == kCombineVertically)
		height2 /= 2;

	if (width1 != width2)
		return width1 - width2;

	if (height1 != height2)
		return height1 - height2;

	return (int)(10 * get_refresh_rate(*mode1)
		-  10 * get_refresh_rate(*mode2));
}


//	#pragma mark -


int32
screen_mode::BitsPerPixel() const
{
	switch (space) {
		case B_RGB32:	return 32;
		case B_RGB24:	return 24;
		case B_RGB16:	return 16;
		case B_RGB15:	return 15;
		case B_CMAP8:	return 8;
		default:		return 0;
	}
}


bool
screen_mode::operator==(const screen_mode &other) const
{
	return !(*this != other);
}


bool
screen_mode::operator!=(const screen_mode &other) const
{
	return width != other.width || height != other.height
		|| space != other.space || refresh != other.refresh
		|| combine != other.combine
		|| swap_displays != other.swap_displays
		|| use_laptop_panel != other.use_laptop_panel
		|| tv_standard != other.tv_standard;
}


void
screen_mode::SetTo(display_mode& mode)
{
	width = mode.virtual_width;
	height = mode.virtual_height;
	space = (color_space)mode.space;
	combine = get_combine_mode(mode);
	refresh = get_refresh_rate(mode);

	if (combine == kCombineHorizontally)
		width /= 2;
	else if (combine == kCombineVertically)
		height /= 2;

	swap_displays = false;
	use_laptop_panel = false;
	tv_standard = 0;
}


//	#pragma mark -


ScreenMode::ScreenMode(BWindow* window)
	:
	fWindow(window),
	fUpdatedMode(false)
{
	BScreen screen(window);
	if (screen.GetModeList(&fModeList, &fModeCount) == B_OK) {
		// sort modes by resolution and refresh to make
		// the resolution and refresh menu look nicer
		qsort(fModeList, fModeCount, sizeof(display_mode), compare_mode);
	} else {
		fModeList = NULL;
		fModeCount = 0;
	}
}


ScreenMode::~ScreenMode()
{
	free(fModeList);
}


status_t
ScreenMode::Set(const screen_mode& mode)
{
	if (!fUpdatedMode)
		UpdateOriginalMode();

	BScreen screen(fWindow);
	SetSwapDisplays(&screen, mode.swap_displays);
	SetUseLaptopPanel(&screen, mode.use_laptop_panel);
	SetTVStandard(&screen, mode.tv_standard);

	display_mode displayMode;
	if (!GetDisplayMode(mode, displayMode))
		return B_ENTRY_NOT_FOUND;

	return screen.SetMode(&displayMode, true);
}


status_t
ScreenMode::Get(screen_mode& mode)
{
	display_mode displayMode;
	BScreen screen(fWindow);
	if (screen.GetMode(&displayMode) != B_OK)
		return B_ERROR;

	mode.SetTo(displayMode);

	if (GetSwapDisplays(&screen, &mode.swap_displays) != B_OK)
		mode.swap_displays = false;
	if (GetUseLaptopPanel(&screen, &mode.use_laptop_panel) != B_OK)
		mode.use_laptop_panel = false;
	if (GetTVStandard(&screen, &mode.tv_standard) != B_OK)
		mode.tv_standard = 0;

	return B_OK;
}


status_t
ScreenMode::Revert()
{
	if (!fUpdatedMode)
		return B_OK;

	screen_mode current;
	if (Get(current) == B_OK && fOriginal == current)
		return B_OK;

	BScreen screen(fWindow);
	SetSwapDisplays(&screen, fOriginal.swap_displays);
	SetUseLaptopPanel(&screen, fOriginal.use_laptop_panel);
	SetTVStandard(&screen, fOriginal.tv_standard);

	return screen.SetMode(&fOriginalDisplayMode, true);
}


void
ScreenMode::UpdateOriginalMode()
{
	BScreen screen(fWindow);
	if (screen.GetMode(&fOriginalDisplayMode) == B_OK) {
		fUpdatedMode = true;
		Get(fOriginal);
	}
}


bool
ScreenMode::SupportsColorSpace(const screen_mode& mode, color_space space)
{
	return true;
}


status_t
ScreenMode::GetRefreshLimits(const screen_mode& mode, float& min, float& max)
{
	uint32 minClock, maxClock;
	display_mode displayMode;
	if (!GetDisplayMode(mode, displayMode))
		return B_ERROR;

	BScreen screen(fWindow);
	if (screen.GetPixelClockLimits(&displayMode, &minClock, &maxClock) < B_OK)
		return B_ERROR;

	uint32 total = displayMode.timing.h_total * displayMode.timing.v_total;
	min = minClock * 1000.0 / total;
	max = maxClock * 1000.0 / total;

	return B_OK;
}


screen_mode
ScreenMode::ModeAt(int32 index)
{
	if (index < 0)
		index = 0;
	else if (index >= (int32)fModeCount)
		index = fModeCount - 1;

	screen_mode mode;
	mode.SetTo(fModeList[index]);

	return mode;
}


int32
ScreenMode::CountModes()
{
	return fModeCount;
}


bool
ScreenMode::GetDisplayMode(const screen_mode& mode, display_mode& displayMode)
{
	uint16 virtualWidth, virtualHeight;
	int32 bestIndex = -1;
	float bestDiff = 999;

	virtualWidth = mode.combine == kCombineHorizontally ? 
		mode.width * 2 : mode.width;
	virtualHeight = mode.combine == kCombineVertically ? 
		mode.height * 2 : mode.height;

	// try to find mode in list provided by driver
	for (uint32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].virtual_width != virtualWidth
			|| fModeList[i].virtual_height != virtualHeight
			|| (color_space)fModeList[i].space != mode.space)
			continue;

		float refresh = get_refresh_rate(fModeList[i]);
		if (refresh == mode.refresh) {
			// we have luck - we can use this mode directly
			displayMode = fModeList[i];
			displayMode.h_display_start = 0;
			displayMode.v_display_start = 0;
			return true;
		}

		float diff = fabs(refresh - mode.refresh);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestIndex = i;
		}
	}

	// we didn't find the exact mode, but something very similar?
	if (bestIndex == -1)
		return false;

	// now, we are better of using GMT formula, but
	// as we don't have it, we just tune the pixel
	// clock of the best mode.

	displayMode = fModeList[bestIndex];
	displayMode.h_display_start = 0;
	displayMode.v_display_start = 0;

	// after some fiddling, it looks like this is the formula
	// used by the original panel (notice that / 10 happens before
	// multiplying with refresh rate - this leads to different
	// rounding)
	displayMode.timing.pixel_clock = ((uint32)displayMode.timing.h_total
		* displayMode.timing.v_total / 10 * int32(mode.refresh * 10)) / 1000;

	return true;
}

