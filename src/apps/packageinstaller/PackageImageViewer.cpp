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

enum {
	P_MSG_CLOSE = 'pmic'
};


ImageView::ImageView(BPositionIO *image)
	:
	BView(BRect(0, 0, 1, 1), "image_view", B_FOLLOW_NONE, B_WILL_DRAW),
	fSuccess(true)
{
	if (!image) {
		fSuccess = false;
		return;
	}
	// Initialize and translate the image
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BBitmapStream stream;
	if (roster->Translate(image, NULL, NULL, &stream, B_TRANSLATOR_BITMAP)
			< B_OK) {
		fSuccess = false;
		return;
	}
	stream.DetachBitmap(&fImage);
}


ImageView::~ImageView()
{
}


void
ImageView::AttachedToWindow()
{
	if (!fSuccess) {
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
	if (fSuccess)
		DrawBitmapAsync(fImage, Bounds());
	else {
		float length = StringWidth(B_TRANSLATE("Image not loaded correctly"));
		DrawString(B_TRANSLATE("Image not loaded correctly"),
			BPoint((Bounds().Width() - length) / 2.0f, 30.0f));
	}
}


void
ImageView::MouseUp(BPoint point)
{
	BWindow *parent = Window();
	if (parent)
		parent->PostMessage(P_MSG_CLOSE);
}


// #pragma mark -


PackageImageViewer::PackageImageViewer(BPositionIO *image)
	:
	BWindow(BRect(100, 100, 100, 100), "", B_MODAL_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	fBackground = new ImageView(image);
	AddChild(fBackground);

	ResizeTo(fBackground->Bounds().Width(), fBackground->Bounds().Height());

	BScreen screen(this);
	BRect frame = screen.Frame();
	MoveTo((frame.Width() - Bounds().Width()) / 2.0f,
			(frame.Height() - Bounds().Height()) / 2.0f);
}


PackageImageViewer::~PackageImageViewer()
{
}


void
PackageImageViewer::Go()
{
	// Since this class can be thought of as a modified BAlert window, no use
	// to reinvent a well fledged wheel. This concept has been borrowed from
	// the current BAlert implementation
	fSemaphore = create_sem(0, "ImageViewer");
	if (fSemaphore < B_OK) {
		Quit();
		return;
	}

	BWindow *parent =
		dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
	Show();

	if (parent) {
		status_t ret;
		for (;;) {
			do {
				ret = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, 50000);
			} while (ret == B_INTERRUPTED);

			if (ret == B_BAD_SEM_ID)
				break;
			parent->UpdateIfNeeded();
		}
	}
	else {
		// Since there are no spinlocks, wait until the semaphore is free
		while (acquire_sem(fSemaphore) == B_INTERRUPTED) {
		}
	}

	if (Lock())
		Quit();
}


void
PackageImageViewer::MessageReceived(BMessage *msg)
{
	if (msg->what == P_MSG_CLOSE) {
		if (fSemaphore >= B_OK) {
			delete_sem(fSemaphore);
			fSemaphore = -1;
		}
	} else
		BWindow::MessageReceived(msg);
}

