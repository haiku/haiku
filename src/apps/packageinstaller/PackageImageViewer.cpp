/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageImageViewer.h"

#include <BitmapStream.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <Screen.h>
#include <TranslatorRoster.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageImageViewer"


ImageView::ImageView(BPositionIO* imageIO)
	:
	BView(BRect(0, 0, 1, 1), "image_view", B_FOLLOW_NONE, B_WILL_DRAW),
	fImage(NULL)
{
	if (imageIO == NULL)
		return;

	// Initialize and translate the image
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	BBitmapStream stream;
	if (roster->Translate(imageIO, NULL, NULL, &stream, B_TRANSLATOR_BITMAP)
			!= B_OK) {
		return;
	}
	stream.DetachBitmap(&fImage);
}


ImageView::~ImageView()
{
	delete fImage;
}


void
ImageView::AttachedToWindow()
{
	if (fImage == NULL) {
		ResizeTo(75, 75);
		return;
	}

	// We need to resize the view depending on what size has the screen and
	// the image we will be viewing
	BScreen screen(Window());
	BRect frame = screen.Frame();
	BRect image = fImage->Bounds();

	if (image.Width() > (frame.Width() - 100.0f))
		image.right = frame.Width() - 100.0f;
	if (image.Height() > (frame.Height() - 100.0f))
		image.bottom = frame.Height() - 100.f;

	ResizeTo(image.Width(), image.Height());
}


void
ImageView::Draw(BRect updateRect)
{
	if (fImage != NULL)
		DrawBitmapAsync(fImage, Bounds());
	else {
		const char* message = B_TRANSLATE("Image not loaded correctly");
		float width = StringWidth(message);
		DrawString(message, BPoint((Bounds().Width() - width) / 2.0f, 30.0f));
	}
}


void
ImageView::MouseUp(BPoint point)
{
	BWindow* window = Window();
	if (window != NULL)
		window->PostMessage(B_QUIT_REQUESTED);
}


// #pragma mark -


PackageImageViewer::PackageImageViewer(BPositionIO* imageIO)
	:
	BlockingWindow(BRect(100, 100, 100, 100), "")
{
	fBackground = new ImageView(imageIO);
	AddChild(fBackground);

	ResizeTo(fBackground->Bounds().Width(), fBackground->Bounds().Height());

	BScreen screen(this);
	BRect frame = screen.Frame();
	MoveTo((frame.Width() - Bounds().Width()) / 2.0f,
		(frame.Height() - Bounds().Height()) / 2.0f);
}

