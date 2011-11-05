/*
 * Copyright 2003-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Screen.h>

#include <Window.h>

#include <PrivateScreen.h>


using namespace BPrivate;


BScreen::BScreen(screen_id id)
{
	fScreen = BPrivateScreen::Get(id.id);
}


BScreen::BScreen(BWindow* window)
{
	fScreen = BPrivateScreen::Get(window);
}


BScreen::~BScreen()
{
	BPrivateScreen::Put(fScreen);
}


bool
BScreen::IsValid()
{
	return fScreen != NULL && fScreen->IsValid();
}


status_t
BScreen::SetToNext()
{
	if (fScreen != NULL) {
		BPrivateScreen* screen = BPrivateScreen::GetNext(fScreen);
		if (screen != NULL) {
			fScreen = screen;
			return B_OK;
		}
	}
	return B_ERROR;
}


color_space
BScreen::ColorSpace()
{
	if (fScreen != NULL)
		return fScreen->ColorSpace();

	return B_NO_COLOR_SPACE;
}


BRect
BScreen::Frame()
{
	if (fScreen != NULL)
		return fScreen->Frame();

	return BRect(0, 0, 0, 0);
}


screen_id
BScreen::ID()
{
	if (fScreen != NULL) {
		screen_id id = { fScreen->ID() };
		return id;
	}

	return B_MAIN_SCREEN_ID;
}


status_t
BScreen::WaitForRetrace()
{
	return WaitForRetrace(B_INFINITE_TIMEOUT);
}


status_t
BScreen::WaitForRetrace(bigtime_t timeout)
{
	if (fScreen != NULL)
		return fScreen->WaitForRetrace(timeout);

	return B_ERROR;
}


uint8
BScreen::IndexForColor(uint8 red, uint8 green, uint8 blue, uint8 alpha)
{
	if (fScreen != NULL)
		return fScreen->IndexForColor(red, green, blue, alpha);

	return 0;
}


rgb_color
BScreen::ColorForIndex(const uint8 index)
{
	if (fScreen != NULL)
		return fScreen->ColorForIndex(index);

	return rgb_color();
}


uint8
BScreen::InvertIndex(uint8 index)
{
	if (fScreen != NULL)
		return fScreen->InvertIndex(index);

	return 0;
}


const color_map*
BScreen::ColorMap()
{
	if (fScreen != NULL)
		return fScreen->ColorMap();

	return NULL;
}


status_t
BScreen::GetBitmap(BBitmap** _bitmap, bool drawCursor, BRect* bounds)
{
	if (fScreen != NULL)
		return fScreen->GetBitmap(_bitmap, drawCursor, bounds);

	return B_ERROR;
}


status_t
BScreen::ReadBitmap(BBitmap* bitmap, bool drawCursor, BRect* bounds)
{
	if (fScreen != NULL)
		return fScreen->ReadBitmap(bitmap, drawCursor, bounds);

	return B_ERROR;
}


rgb_color
BScreen::DesktopColor()
{
	if (fScreen != NULL)
		return fScreen->DesktopColor(B_CURRENT_WORKSPACE_INDEX);

	return rgb_color();
}


rgb_color
BScreen::DesktopColor(uint32 workspace)
{
	if (fScreen != NULL)
		return fScreen->DesktopColor(workspace);

	return rgb_color();
}


void
BScreen::SetDesktopColor(rgb_color color, bool stick)
{
	if (fScreen != NULL)
		fScreen->SetDesktopColor(color, B_CURRENT_WORKSPACE_INDEX, stick);
}


void
BScreen::SetDesktopColor(rgb_color color, uint32 workspace, bool stick)
{
	if (fScreen != NULL)
		fScreen->SetDesktopColor(color, workspace, stick);
}


status_t
BScreen::ProposeMode(display_mode* target, const display_mode* low,
	const display_mode* high)
{
	if (fScreen != NULL)
		return fScreen->ProposeMode(target, low, high);

	return B_ERROR;
}


status_t
BScreen::GetModeList(display_mode** _modeList, uint32* _count)
{
	if (fScreen != NULL)
		return fScreen->GetModeList(_modeList, _count);

	return B_ERROR;
}


status_t
BScreen::GetMode(display_mode* mode)
{
	if (fScreen != NULL)
		return fScreen->GetMode(B_CURRENT_WORKSPACE_INDEX, mode);

	return B_ERROR;
}


status_t
BScreen::GetMode(uint32 workspace, display_mode* mode)
{
	if (fScreen != NULL)
		return fScreen->GetMode(workspace, mode);

	return B_ERROR;
}


status_t
BScreen::SetMode(display_mode* mode, bool makeDefault)
{
	if (fScreen != NULL)
		return fScreen->SetMode(B_CURRENT_WORKSPACE_INDEX, mode, makeDefault);

	return B_ERROR;
}


status_t
BScreen::SetMode(uint32 workspace, display_mode* mode, bool makeDefault)
{
	if (fScreen != NULL)
		return fScreen->SetMode(workspace, mode, makeDefault);

	return B_ERROR;
}


status_t
BScreen::GetDeviceInfo(accelerant_device_info* info)
{
	if (fScreen != NULL)
		return fScreen->GetDeviceInfo(info);

	return B_ERROR;
}


status_t
BScreen::GetMonitorInfo(monitor_info* info)
{
	if (fScreen != NULL)
		return fScreen->GetMonitorInfo(info);

	return B_ERROR;
}


status_t
BScreen::GetPixelClockLimits(display_mode* mode, uint32* _low, uint32* _high)
{
	if (fScreen != NULL)
		return fScreen->GetPixelClockLimits(mode, _low, _high);

	return B_ERROR;
}


status_t
BScreen::GetTimingConstraints(display_timing_constraints* constraints)
{
	if (fScreen != NULL)
		return fScreen->GetTimingConstraints(constraints);

	return B_ERROR;
}


status_t
BScreen::SetDPMS(uint32 dpmsState)
{
	if (fScreen != NULL)
		return fScreen->SetDPMS(dpmsState);

	return B_ERROR;
}


uint32
BScreen::DPMSState()
{
	if (fScreen != NULL)
		return fScreen->DPMSState();

	return 0;
}


uint32
BScreen::DPMSCapabilites()
{
	if (fScreen != NULL)
		return fScreen->DPMSCapabilites();

	return 0;
}


//	#pragma mark - Deprecated methods


BPrivate::BPrivateScreen*
BScreen::private_screen()
{
	return fScreen;
}


status_t
BScreen::ProposeDisplayMode(display_mode* target, const display_mode* low,
	const display_mode* high)
{
	return ProposeMode(target, low, high);
}


void*
BScreen::BaseAddress()
{
	if (fScreen != NULL)
		return fScreen->BaseAddress();

	return NULL;
}


uint32
BScreen::BytesPerRow()
{
	if (fScreen != NULL)
		return fScreen->BytesPerRow();

	return 0;
}
