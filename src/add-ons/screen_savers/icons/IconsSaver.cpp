/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/


#include "IconsSaver.h"

#include <stdio.h>
#include <stdlib.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <MimeType.h>
#include <StringView.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#include "IconDisplay.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Icons"


#define MAX_ICONS 15
#define MAX_SIZE 20 // In percentage of the screen width
#define MIN_SIZE 5 // Same here
#define RAND_BETWEEN(a, b) ((rand() % ((b) - (a) + 1) + (a)))


const rgb_color kBackgroundColor = ui_color(B_DESKTOP_COLOR);


BScreenSaver* instantiate_screen_saver(BMessage* msg, image_id image)
{
	return new IconsSaver(msg, image);
}


IconsSaver::IconsSaver(BMessage* msg, image_id image)
	:
	BScreenSaver(msg, image),
	fVectorIconsCount(0),
	fIcons(NULL),
	fBackBitmap(NULL),
	fBackView(NULL),
	fMinSize(0),
	fMaxSize(0)
{
}


IconsSaver::~IconsSaver()
{
}


status_t
IconsSaver::StartSaver(BView *view, bool /*preview*/)
{
	if (fVectorIconsCount <= 0) {
		// Load the vector icons from the MIME types
		BMessage types;
		BMimeType::GetInstalledTypes(&types);

		for (int32 index = 0 ; ; index++) {
			const char* type;
			if (types.FindString("types", index, &type) != B_OK)
				break;

			BMimeType mimeType(type);
			uint8* vectorData = NULL;
			size_t size = 0;

			if (mimeType.GetIcon(&vectorData, &size) != B_OK || size == 0)
				continue;

			VectorIcon* icon = new VectorIcon;
			icon->data = vectorData;
			icon->size = size;

			fVectorIcons.AddItem(icon);
		}

		fVectorIconsCount = fVectorIcons.CountItems();
	}

	srand(system_time() % INT_MAX);

	BRect screenRect(0, 0, view->Frame().Width(), view->Frame().Height());
	fBackBitmap = new BBitmap(screenRect, B_RGBA32, true);
	if (!fBackBitmap->IsValid())
		return B_NO_MEMORY;

	fBackView = new BView(screenRect, "back view", 0, 0);
	if (fBackView == NULL)
		return B_NO_MEMORY;

	fBackView->SetViewColor(kBackgroundColor);
	fBackView->SetHighColor(kBackgroundColor);

	fBackBitmap->AddChild(fBackView);
	if (fBackBitmap->Lock()) {
		fBackView->FillRect(fBackView->Frame());
		fBackView->SetDrawingMode(B_OP_ALPHA);
		fBackView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		fBackView->Sync();
		fBackBitmap->Unlock();
	}

	fIcons = new IconDisplay[MAX_ICONS];

	fMaxSize = (screenRect.IntegerWidth() * MAX_SIZE) / 100;
	fMinSize = (screenRect.IntegerWidth() * MIN_SIZE) / 100;
	if (fMaxSize > 255)
		fMaxSize = 255;

	if (fMaxSize > screenRect.IntegerHeight())
		fMaxSize = screenRect.IntegerHeight();

	if (fMinSize > fMaxSize)
		fMinSize = fMaxSize;

	return B_OK;
}


void
IconsSaver::StopSaver()
{
	delete[] fIcons;
}


void
IconsSaver::Draw(BView *view, int32 frame)
{
	static int32 previousFrame = 0;

	// Update drawing
	if (fBackBitmap->Lock()) {
		for (uint8 i = 0 ; i < MAX_ICONS ; i++) {
			fIcons[i].ClearOn(fBackView);
		}

		int32 delta = frame - previousFrame;

		for (uint8 i = 0 ; i < MAX_ICONS ; i++) {
			fIcons[i].DrawOn(fBackView, delta);
		}
		fBackView->Sync();
		fBackBitmap->Unlock();
	}

	// Sync the view with the back buffer
	view->DrawBitmap(fBackBitmap);
	previousFrame = frame;

	if (fVectorIconsCount <= 0)
		return;

	// Restart one icon
	for (uint8 i = 0 ; i < MAX_ICONS ; i++) {
		if (!fIcons[i].IsRunning()) {
			uint16 size = RAND_BETWEEN(fMinSize, fMaxSize);
			uint16 maxX = view->Frame().IntegerWidth() - size;
			uint16 maxY = view->Frame().IntegerHeight() - size;

			BRect iconFrame(0, 0, size, size);
			iconFrame.OffsetTo(RAND_BETWEEN(0, maxX), RAND_BETWEEN(0, maxY));

			// Check that the icon doesn't overlap with others
			for (uint8 j = 0 ; j < MAX_ICONS ; j++) {
				if (fIcons[j].IsRunning() &&
					iconFrame.Intersects(fIcons[j].GetFrame()))
					return;
			}

			int32 index = RAND_BETWEEN(0, fVectorIconsCount - 1);

			fIcons[i].Run((VectorIcon*)fVectorIcons.ItemAt(index), iconFrame);

			return;
		}
	}
}


void
IconsSaver::StartConfig(BView* view)
{
	BPrivate::BuildScreenSaverDefaultSettingsView(view, "Icons",
		B_TRANSLATE("by Vincent Duvert"));
}

