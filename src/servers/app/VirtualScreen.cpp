/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "VirtualScreen.h"

#include "HWInterface.h"
#include "Desktop.h"

#include <new>

using std::nothrow;


VirtualScreen::VirtualScreen()
	:
	fScreenList(4, true),
	fDrawingEngine(NULL),
	fHWInterface(NULL)
{
}


VirtualScreen::~VirtualScreen()
{
	_Reset();
}


void
VirtualScreen::_Reset()
{
	ScreenList list;
	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item* item = fScreenList.ItemAt(i);

		list.AddItem(item->screen);
	}

	gScreenManager->ReleaseScreens(list);
	fScreenList.MakeEmpty();
	fSettings.MakeEmpty();

	fFrame.Set(0, 0, 0, 0);
	fDrawingEngine = NULL;
	fHWInterface = NULL;
}


status_t
VirtualScreen::RestoreConfiguration(Desktop& desktop, const BMessage* settings)
{
	_Reset();

	// Copy current Desktop workspace settings
	if (settings)
		fSettings = *settings;

	ScreenList list;
	status_t status = gScreenManager->AcquireScreens(&desktop, NULL, 0, false,
		list);
	if (status < B_OK) {
		// TODO: we would try again here with force == true
		return status;
	}

	for (int32 i = 0; i < list.CountItems(); i++) {
		Screen* screen = list.ItemAt(i);

		AddScreen(screen);
	}

	return B_OK;
}


status_t
VirtualScreen::StoreConfiguration(BMessage& settings)
{
	// store the configuration of all current screens

	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item* item = fScreenList.ItemAt(i);
		Screen* screen = item->screen;
		if (!screen->IsDefaultMode())
			continue;

		BMessage screenSettings;
		screenSettings.AddInt32("id", screen->ID());

		monitor_info info;
		if (screen->GetMonitorInfo(info) == B_OK) {
			screenSettings.AddString("vendor", info.vendor);
			screenSettings.AddString("name", info.name);
			screenSettings.AddInt32("product id", info.product_id);
			screenSettings.AddString("serial", info.serial_number);
			screenSettings.AddInt32("produced week", info.produced.week);
			screenSettings.AddInt32("produced year", info.produced.year);
		}

		screenSettings.AddRect("frame", item->frame);

		display_mode mode;
		screen->GetMode(&mode);

		screenSettings.AddInt32("width", mode.virtual_width);
		screenSettings.AddInt32("height", mode.virtual_height);
		screenSettings.AddInt32("color space", mode.space);
		screenSettings.AddData("timing", B_RAW_TYPE, &mode.timing,
			sizeof(display_timing));

		settings.AddMessage("screen", &screenSettings);
	}

	// store the configuration of all monitors currently not attached

	BMessage screenSettings;
	for (uint32 i = 0; fSettings.FindMessage("screen", i,
			&screenSettings) == B_OK; i++) {
		settings.AddMessage("screen", &screenSettings);
	}

	return B_OK;
}


status_t
VirtualScreen::AddScreen(Screen* screen)
{
	screen_item* item = new(nothrow) screen_item;
	if (item == NULL)
		return B_NO_MEMORY;

	item->screen = screen;

	status_t status = B_ERROR;
	BMessage settings;
	if (_FindConfiguration(screen, settings) == B_OK) {
		// we found settings for this screen, and try to apply them now
		int32 width, height, colorSpace;
		const display_timing* timing;
		ssize_t size;
		if (settings.FindInt32("width", &width) == B_OK
			&& settings.FindInt32("height", &height) == B_OK
			&& settings.FindInt32("color space", &colorSpace) == B_OK
			&& settings.FindData("timing", B_RAW_TYPE, (const void**)&timing,
					&size) == B_OK
			&& size == sizeof(display_timing))
			status = screen->SetMode(width, height, colorSpace, *timing, true);
	}
	if (status < B_OK) {
		// TODO: more intelligent standard mode (monitor preference, desktop default, ...)
		if (screen->SetPreferredMode() != B_OK)
			screen->SetMode(800, 600, B_RGB32, 60.f, false);
	}

	// TODO: this works only for single screen configurations
	fDrawingEngine = screen->GetDrawingEngine();
	fHWInterface = screen->HWInterface();
	fFrame = screen->Frame();

	fScreenList.AddItem(item);

	return B_OK;
}


status_t
VirtualScreen::RemoveScreen(Screen* screen)
{
	// not implemented yet (config changes when running)
	return B_ERROR;
}


void
VirtualScreen::UpdateFrame()
{
	int32 virtualWidth = 0, virtualHeight = 0;

	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		Screen* screen = fScreenList.ItemAt(i)->screen;

		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		screen->GetMode(width, height, colorSpace, frequency);

		// TODO: compute virtual size depending on the actual screen position!
		virtualWidth += width;
		virtualHeight += height;
	}

	fFrame.Set(0, 0, virtualWidth - 1, virtualHeight - 1);
}


/*!
	Returns the smallest frame that spans over all screens
*/
BRect
VirtualScreen::Frame() const
{
	return fFrame;
}


Screen*
VirtualScreen::ScreenAt(int32 index) const
{
	screen_item* item = fScreenList.ItemAt(index);
	if (item != NULL)
		return item->screen;

	return NULL;
}


BRect
VirtualScreen::ScreenFrameAt(int32 index) const
{
	screen_item* item = fScreenList.ItemAt(index);
	if (item != NULL)
		return item->frame;

	return BRect(0, 0, 0, 0);
}


int32
VirtualScreen::CountScreens() const
{
	return fScreenList.CountItems();
}


status_t
VirtualScreen::_FindConfiguration(Screen* screen, BMessage& settings)
{
	monitor_info info;
	bool hasInfo = screen->GetMonitorInfo(info) == B_OK;
	if (!hasInfo) {
		// only look for a matching ID - this is all we have
		for (uint32 i = 0; fSettings.FindMessage("screen", i,
				&settings) == B_OK; i++) {
			int32 id;
			if (settings.FindInt32("id", &id) != B_OK
				|| screen->ID() != id)
				continue;

			// we found our match
			fSettings.RemoveData("screen", i);
			return B_OK;
		}
	}

	// look for a monitor configuration that matches ours

	int32 bestScore = 0;
	int32 bestIndex = -1;
	BMessage stored;
	for (uint32 i = 0; fSettings.FindMessage("screen", i, &stored) == B_OK;
			i++) {
		int32 score = 0;
		int32 id;
		if (stored.FindInt32("id", &id) == B_OK && screen->ID() == id)
			score++;

		const char* vendor;
		const char* name;
		uint32 productID;
		const char* serial;
		int32 week, year;
		if (stored.FindString("vendor", &vendor) == B_OK
			&& stored.FindString("name", &name) == B_OK
			&& stored.FindInt32("product id", (int32*)&productID) == B_OK
			&& stored.FindString("serial", &serial) == B_OK
			&& stored.FindInt32("produced week", &week) == B_OK
			&& stored.FindInt32("produced year", &year) == B_OK) {
			if (!strcasecmp(vendor, info.vendor)
				&& !strcasecmp(name, info.name)
				&& productID == info.product_id) {
				score += 2;
				if (!strcmp(serial, info.serial_number))
					score += 2;
				if (info.produced.year == year && info.produced.week == week)
					score++;
			} else
				score -= 2;
		}

		if (score > bestScore) {
			settings = stored;
			bestScore = score;
			bestIndex = i;
		}
	}

	if (bestIndex >= 0) {
		fSettings.RemoveData("screen", bestIndex);
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}

