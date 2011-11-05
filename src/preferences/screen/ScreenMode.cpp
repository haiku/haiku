/*
 * Copyright 2005-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenMode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <InterfaceDefs.h>
#include <String.h>

#include <compute_display_timing.h>


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
	if ((mode.flags & B_SCROLL) == 0)
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
	return rint(10 * float(mode.timing.pixel_clock * 1000)
		/ float(mode.timing.h_total * mode.timing.v_total)) / 10.0;
}


/*!	Helper to sort modes by resolution */
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
	fUpdatedModes(false)
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
ScreenMode::Set(const screen_mode& mode, int32 workspace)
{
	if (!fUpdatedModes)
		UpdateOriginalModes();

	BScreen screen(fWindow);

	if (workspace == ~0)
		workspace = current_workspace();

	// TODO: our app_server doesn't fully support workspaces, yet
	SetSwapDisplays(&screen, mode.swap_displays);
	SetUseLaptopPanel(&screen, mode.use_laptop_panel);
	SetTVStandard(&screen, mode.tv_standard);

	display_mode displayMode;
	if (!_GetDisplayMode(mode, displayMode))
		return B_ENTRY_NOT_FOUND;

	return screen.SetMode(workspace, &displayMode, true);
}


status_t
ScreenMode::Get(screen_mode& mode, int32 workspace) const
{
	display_mode displayMode;
	BScreen screen(fWindow);

	if (workspace == ~0)
		workspace = current_workspace();

	if (screen.GetMode(workspace, &displayMode) != B_OK)
		return B_ERROR;

	mode.SetTo(displayMode);

	// TODO: our app_server doesn't fully support workspaces, yet
	if (GetSwapDisplays(&screen, &mode.swap_displays) != B_OK)
		mode.swap_displays = false;
	if (GetUseLaptopPanel(&screen, &mode.use_laptop_panel) != B_OK)
		mode.use_laptop_panel = false;
	if (GetTVStandard(&screen, &mode.tv_standard) != B_OK)
		mode.tv_standard = 0;

	return B_OK;
}


status_t
ScreenMode::GetOriginalMode(screen_mode& mode, int32 workspace) const
{
	if (workspace == ~0)
		workspace = current_workspace();
		// TODO this should use kMaxWorkspaces
	else if (workspace > 31)
		return B_BAD_INDEX;

	mode = fOriginal[workspace];

	return B_OK;
}


status_t
ScreenMode::Set(const display_mode& mode, int32 workspace)
{
	if (!fUpdatedModes)
		UpdateOriginalModes();

	BScreen screen(fWindow);

	if (workspace == ~0)
		workspace = current_workspace();

	// BScreen::SetMode() needs a non-const display_mode
	display_mode nonConstMode;
	memcpy(&nonConstMode, &mode, sizeof(display_mode));
	return screen.SetMode(workspace, &nonConstMode, true);
}


status_t
ScreenMode::Get(display_mode& mode, int32 workspace) const
{
	BScreen screen(fWindow);

	if (workspace == ~0)
		workspace = current_workspace();

	return screen.GetMode(workspace, &mode);
}


/*!	This method assumes that you already reverted to the correct number
	of workspaces.
*/
status_t
ScreenMode::Revert()
{
	if (!fUpdatedModes)
		return B_ERROR;

	status_t result = B_OK;
	screen_mode current;
	for (int32 workspace = 0; workspace < count_workspaces(); workspace++) {
		if (Get(current, workspace) == B_OK && fOriginal[workspace] == current)
			continue;

		BScreen screen(fWindow);

		// TODO: our app_server doesn't fully support workspaces, yet
		if (workspace == current_workspace()) {
			SetSwapDisplays(&screen, fOriginal[workspace].swap_displays);
			SetUseLaptopPanel(&screen, fOriginal[workspace].use_laptop_panel);
			SetTVStandard(&screen, fOriginal[workspace].tv_standard);
		}

		result = screen.SetMode(workspace, &fOriginalDisplayMode[workspace],
			true);
		if (result != B_OK)
			break;
	}

	return result;
}


void
ScreenMode::UpdateOriginalModes()
{
	BScreen screen(fWindow);
	for (int32 workspace = 0; workspace < count_workspaces(); workspace++) {
		if (screen.GetMode(workspace, &fOriginalDisplayMode[workspace])
				== B_OK) {
			Get(fOriginal[workspace], workspace);
			fUpdatedModes = true;
		}
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
	if (!_GetDisplayMode(mode, displayMode))
		return B_ERROR;

	BScreen screen(fWindow);
	if (screen.GetPixelClockLimits(&displayMode, &minClock, &maxClock) < B_OK)
		return B_ERROR;

	uint32 total = displayMode.timing.h_total * displayMode.timing.v_total;
	min = minClock * 1000.0 / total;
	max = maxClock * 1000.0 / total;

	return B_OK;
}


status_t
ScreenMode::GetMonitorInfo(monitor_info& info, float* _diagonalInches)
{
	BScreen screen(fWindow);
	status_t status = screen.GetMonitorInfo(&info);
	if (status != B_OK)
		return status;

	if (_diagonalInches != NULL) {
		*_diagonalInches = round(sqrt(info.width * info.width
			+ info.height * info.height) / 0.254) / 10.0;
	}

	// Some older CRT monitors do not contain the monitor range information
	// (EDID1_MONITOR_RANGES) in their EDID info resulting in the min/max
	// horizontal/vertical frequencies being zero.  In this case, set the
	// vertical frequency range to 60..85 Hz.
	if (info.min_vertical_frequency == 0) {
		info.min_vertical_frequency = 60;
		info.max_vertical_frequency = 85;
	}

	// TODO: If the names aren't sound, we could see if we find/create a
	// database for the entries with user presentable names; they are fine
	// for the models I could test with so far.

	uint32 id = (info.vendor[0] << 24) | (info.vendor[1] << 16)
		| (info.vendor[2] << 8) | (info.vendor[3]);

	switch (id) {
		case 'ADI\0':
			strcpy(info.vendor, "ADI MicroScan");
			break;
		case 'AAC\0':
		case 'ACR\0':
		case 'API\0':
			strcpy(info.vendor, "Acer");
			break;
		case 'ACT\0':
			strcpy(info.vendor, "Targa");
			break;
		case 'APP\0':
			strcpy(info.vendor, "Apple");
			break;
		case 'AUO\0':
			strcpy(info.vendor, "AU Optronics");
			break;
		case 'BNQ\0':
			strcpy(info.vendor, "BenQ");
			break;
		case 'CPL\0':
			strcpy(info.vendor, "ALFA");
			break;
		case 'CPQ\0':
			strcpy(info.vendor, "Compaq");
			break;
		case 'DEL\0':
			strcpy(info.vendor, "Dell");
			break;
		case 'DPC\0':
			strcpy(info.vendor, "Delta Electronics");
			break;
		case 'DWE\0':
			strcpy(info.vendor, "Daewoo");
			break;
		case 'ECS\0':
			strcpy(info.vendor, "Elitegroup");
			break;
		case 'ELS\0':
			strcpy(info.vendor, "ELSA");
			break;
		case 'EMA\0':
			strcpy(info.vendor, "eMachines");
			break;
		case 'EIZ\0':
		case 'ENC\0':
			strcpy(info.vendor, "Eizo");
			break;
		case 'EPI\0':
			strcpy(info.vendor, "Envision");
			break;
		case 'FCM\0':
			strcpy(info.vendor, "Funai Electronics");
			break;
		case 'FUS\0':
			strcpy(info.vendor, "Fujitsu-Siemens");
			break;
		case 'GSM\0':
			strcpy(info.vendor, "LG");
			break;
		case 'GWY\0':
			strcpy(info.vendor, "Gateway");
			break;
		case 'HIQ\0':
		case 'HEI\0':
			strcpy(info.vendor, "Hyundai");
			break;
		case 'HIT\0':
		case 'HTC\0':
			strcpy(info.vendor, "Hitachi");
			break;
		case 'HSL\0':
			strcpy(info.vendor, "Hansol");
			break;
		case 'HWP\0':
			strcpy(info.vendor, "Hewlett Packard");
			break;
		case 'ICL\0':
			strcpy(info.vendor, "Fujitsu");
			break;
		case 'IVM\0':
			strcpy(info.vendor, "Iiyama");
			break;
		case 'LEN\0':
			strcpy(info.vendor, "Lenovo");
			break;
		case 'LPL\0':
			strcpy(info.vendor, "LG Phillips");
			break;
		case 'LTN\0':
			strcpy(info.vendor, "Lite-On");
			break;
		case 'MAX\0':
			strcpy(info.vendor, "Maxdata");
			break;
		case 'MED\0':
			strcpy(info.vendor, "Medion");
			break;
		case 'MEI\0':
			strcpy(info.vendor, "Panasonic");
			break;
		case 'MEL\0':
			strcpy(info.vendor, "Mitsubishi");
			break;
		case 'MIR\0':
			strcpy(info.vendor, "miro");
			break;
		case 'MTC\0':
			strcpy(info.vendor, "Mitac");
			break;
		case 'NAN\0':
			strcpy(info.vendor, "Nanao");
			break;
		case 'NOK\0':
			strcpy(info.vendor, "Nokia");
			break;
		case 'OQI\0':
			strcpy(info.vendor, "Optiquest");
			break;
		case 'PHL\0':
			strcpy(info.vendor, "Philips");
			break;
		case 'PTS\0':
			strcpy(info.vendor, "Proview");
			break;
		case 'QDS\0':
			strcpy(info.vendor, "Quanta Display");
			break;
		case 'REL\0':
			strcpy(info.vendor, "Relisys");
			break;
		case 'SAM\0':
			strcpy(info.vendor, "Samsung");
			break;
		case 'SDI\0':
			strcpy(info.vendor, "Samtron");
			break;
		case 'SHP\0':
			strcpy(info.vendor, "Sharp");
			break;
		case 'SNI\0':
			strcpy(info.vendor, "Siemens");
			break;
		case 'SNY\0':
			strcpy(info.vendor, "Sony");
			break;
		case 'SPT\0':
			strcpy(info.vendor, "Sceptre");
			break;
		case 'SRC\0':
			strcpy(info.vendor, "Shamrock");
			break;
		case 'SUN\0':
			strcpy(info.vendor, "Sun Microsystems");
			break;
		case 'TAT\0':
			strcpy(info.vendor, "Tatung");
			break;
		case 'TOS\0':
		case 'TSB\0':
			strcpy(info.vendor, "Toshiba");
			break;
		case 'UNM\0':
			strcpy(info.vendor, "Unisys");
			break;
		case 'VIZ\0':
			strcpy(info.vendor, "Vizio");
			break;
		case 'VSC\0':
			strcpy(info.vendor, "ViewSonic");
			break;
		case 'ZCM\0':
			strcpy(info.vendor, "Zenith");
			break;
	}

	// Remove extraneous vendor strings and whitespace

	BString name(info.name);
	name.IReplaceAll(info.vendor, "");
	name.Trim();

	strcpy(info.name, name.String());

	return B_OK;
}


status_t
ScreenMode::GetDeviceInfo(accelerant_device_info& info)
{
	BScreen screen(fWindow);
	return screen.GetDeviceInfo(&info);
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


const display_mode&
ScreenMode::DisplayModeAt(int32 index)
{
	if (index < 0)
		index = 0;
	else if (index >= (int32)fModeCount)
		index = fModeCount - 1;

	return fModeList[index];
}


int32
ScreenMode::CountModes()
{
	return fModeCount;
}


/*!	Searches for a similar mode in the reported mode list, and if that does not
	find a matching mode, it will compute the mode manually using the GTF.
*/
bool
ScreenMode::_GetDisplayMode(const screen_mode& mode, display_mode& displayMode)
{
	uint16 virtualWidth, virtualHeight;
	int32 bestIndex = -1;
	float bestDiff = 999;

	virtualWidth = mode.combine == kCombineHorizontally
		? mode.width * 2 : mode.width;
	virtualHeight = mode.combine == kCombineVertically
		? mode.height * 2 : mode.height;

	// try to find mode in list provided by driver
	for (uint32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].virtual_width != virtualWidth
			|| fModeList[i].virtual_height != virtualHeight
			|| (color_space)fModeList[i].space != mode.space)
			continue;

		// Accept the mode if the computed refresh rate of the mode is within
		// 0.6 percent of the refresh rate specified by the caller.  Note that
		// refresh rates computed from mode parameters is not exact; especially
		// some of the older modes such as 640x480, 800x600, and 1024x768.
		// The tolerance of 0.6% was obtained by examining the various possible
		// modes.

		float refreshDiff = fabs(get_refresh_rate(fModeList[i]) - mode.refresh);
		if (refreshDiff < 0.006 * mode.refresh) {
			// Accept this mode.
			displayMode = fModeList[i];
			displayMode.h_display_start = 0;
			displayMode.v_display_start = 0;

			// Since the computed refresh rate of the selected mode might differ
			// from selected refresh rate by a few tenths (e.g. 60.2 instead of
			// 60.0), tweak the pixel clock so the the refresh rate of the mode
			// matches the selected refresh rate.

			displayMode.timing.pixel_clock = uint32(((displayMode.timing.h_total
				* displayMode.timing.v_total * mode.refresh) / 1000.0) + 0.5);
			return true;
		}

		// Mode not acceptable.

		if (refreshDiff < bestDiff) {
			bestDiff = refreshDiff;
			bestIndex = i;
		}
	}

	// we didn't find the exact mode, but something very similar?
	if (bestIndex == -1)
		return false;

	displayMode = fModeList[bestIndex];
	displayMode.h_display_start = 0;
	displayMode.v_display_start = 0;

	// For the mode selected by the width, height, and refresh rate, compute
	// the video timing parameters for the mode by using the VESA Generalized
	// Timing Formula (GTF).
	compute_display_timing(mode.width, mode.height, mode.refresh, false,
		&displayMode.timing);

	return true;
}

