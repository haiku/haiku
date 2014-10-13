/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vincent Duvert, vincent.duvert@free.fr
 *		John Scipione, jscipione@gmail.com
 */


#include "IconsSaver.h"

#include <stdlib.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <MimeType.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#include "IconDisplay.h"
#include "VectorIcon.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Icons"


#define RAND_BETWEEN(a, b) ((rand() % ((b) - (a) + 1) + (a)))


static const int32 kMaxConcurrentIcons = 15;
static const int32 kMinIconWidthPercentage = 5;
	// percentage of the screen width
static const int32 kMaxIconWidthPercentage = 20;
	// same here
static const int32 kMinIconCount = 20;
static const int32 kMaxIconCount = 384;


const rgb_color kBackgroundColor = ui_color(B_DESKTOP_COLOR);


BScreenSaver* instantiate_screen_saver(BMessage* msg, image_id image)
{
	return new IconsSaver(msg, image);
}


//	#pragma mark - IconsSaver


IconsSaver::IconsSaver(BMessage* archive, image_id image)
	:
	BScreenSaver(archive, image),
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
IconsSaver::StartSaver(BView* view, bool /*preview*/)
{
	if (fVectorIcons.CountItems() < kMinIconCount)
		_GetVectorIcons();

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

	fIcons = new IconDisplay[kMaxConcurrentIcons];

	fMaxSize = (screenRect.IntegerWidth() * kMaxIconWidthPercentage) / 100;
	fMinSize = (screenRect.IntegerWidth() * kMinIconWidthPercentage) / 100;
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
	fIcons = NULL;
}


void
IconsSaver::Draw(BView* view, int32 frame)
{
	static int32 previousFrame = 0;

	// update drawing
	if (fBackBitmap->Lock()) {
		for (uint8 i = 0 ; i < kMaxConcurrentIcons ; i++)
			fIcons[i].ClearOn(fBackView);

		int32 delta = frame - previousFrame;
		for (uint8 i = 0 ; i < kMaxConcurrentIcons ; i++)
			fIcons[i].DrawOn(fBackView, delta);

		fBackView->Sync();
		fBackBitmap->Unlock();
	}

	// sync the view with the back buffer
	view->DrawBitmap(fBackBitmap);
	previousFrame = frame;

	if (fVectorIcons.CountItems() < kMinIconCount)
		return;

	// restart one icon
	for (uint8 i = 0 ; i < kMaxConcurrentIcons ; i++) {
		if (!fIcons[i].IsRunning()) {
			uint16 size = RAND_BETWEEN(fMinSize, fMaxSize);
			uint16 maxX = view->Frame().IntegerWidth() - size;
			uint16 maxY = view->Frame().IntegerHeight() - size;

			BRect iconFrame(0, 0, size, size);
			iconFrame.OffsetTo(RAND_BETWEEN(0, maxX), RAND_BETWEEN(0, maxY));

			// Check that the icon doesn't overlap with others
			for (uint8 j = 0 ; j < kMaxConcurrentIcons ; j++) {
				if (fIcons[j].IsRunning() &&
					iconFrame.Intersects(fIcons[j].GetFrame()))
					return;
			}

			int32 index = RAND_BETWEEN(0, fVectorIcons.CountItems() - 1);
			fIcons[i].Run(fVectorIcons.ItemAt(index), iconFrame);
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


//	#pragma mark - IconsSaver private methods


void
IconsSaver::_GetVectorIcons()
{
	// Load vector icons from the MIME type database
	BMessage types;
	if (BMimeType::GetInstalledTypes(&types) != B_OK)
		return;

	const char* type;
	for (int32 i = 0; types.FindString("types", i, &type) == B_OK; i++) {
		BMimeType mimeType(type);
		if (mimeType.InitCheck() != B_OK)
			continue;

		uint8* data;
		size_t size;

		if (mimeType.GetIcon(&data, &size) != B_OK) {
			// didn't find an icon
			continue;
		}

		vector_icon* icon = (vector_icon*)malloc(sizeof(vector_icon));
		if (icon == NULL) {
			// ran out of memory, delete the icon data
			delete[] data;
			continue;
		}

		icon->data = data;
		icon->size = size;

		// found a vector icon, add it to the list
		fVectorIcons.AddItem(icon);
		if (fVectorIcons.CountItems() >= kMaxIconCount) {
			// this is enough to choose from, stop eating memory...
			return;
		}
	}
}
