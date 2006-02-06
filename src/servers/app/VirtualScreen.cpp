/*
 * Copyright 2005-2006, Haiku.
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
	status_t status = gScreenManager->AcquireScreens(&desktop, NULL, 0, false, list);
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
	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		screen_item* item = fScreenList.ItemAt(i);
		Screen* screen = item->screen;
		if (!screen->IsDefaultMode())
			continue;

		BMessage screenSettings;
		screenSettings.AddInt32("id", screen->ID());
		//screenSettings.AddString("name", "-");
		screenSettings.AddRect("frame", item->frame);

		// TODO: or just store a display_mode completely?
		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		screen->GetMode(width, height, colorSpace, frequency);

		screenSettings.AddInt32("width", width);
		screenSettings.AddInt32("height", height);
		screenSettings.AddInt32("color space", colorSpace);
		screenSettings.AddFloat("frequency", frequency);

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
		float frequency;
		if (settings.FindInt32("width", &width) == B_OK
			&& settings.FindInt32("height", &height) == B_OK
			&& settings.FindInt32("color space", &colorSpace) == B_OK
			&& settings.FindFloat("frequency", &frequency) == B_OK)
			status = screen->SetMode(width, height, colorSpace, frequency, true);
	}
	if (status < B_OK) {
		// TODO: more intelligent standard mode (monitor preference, desktop default, ...)
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
	// TODO: we probably want to identify the resolution by connected monitor,
	//		and not the display driver used...
	//		For now, we just use the screen ID, which is almost nothing, anyway...

	uint32 i = 0;
	while (fSettings.FindMessage("screen", i++, &settings) == B_OK) {
		int32 id;
		if (settings.FindInt32("id", &id) != B_OK
			|| screen->ID() != id)
			continue;

		// we found our match
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}

