/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ScreenshotWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <LayoutBuilder.h>

#include "BitmapView.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenshotWindow"


ScreenshotWindow::ScreenshotWindow(BWindow* parent, BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Screenshot"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fDownloadPending(false),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	fScreenshotView = new BitmapView("screenshot view");
	fScreenshotView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BGroupView* groupView = new BGroupView(B_VERTICAL);
	groupView->SetViewColor(0, 0, 0);
	fScreenshotView->SetLowColor(0, 0, 0);

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(groupView)
			.Add(fScreenshotView)
			.SetInsets(B_USE_WINDOW_INSETS)
		.End()
	;

	CenterOnScreen();
}


ScreenshotWindow::~ScreenshotWindow()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		wait_for_thread(fWorkerThread, NULL);
}


bool
ScreenshotWindow::QuitRequested()
{
	if (fOnCloseTarget.IsValid() && fOnCloseMessage.what != 0)
		fOnCloseTarget.SendMessage(&fOnCloseMessage);

	Hide();
	return false;
}


void
ScreenshotWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ScreenshotWindow::SetOnCloseMessage(
	const BMessenger& messenger, const BMessage& message)
{
	fOnCloseTarget = messenger;
	fOnCloseMessage = message;
}


void
ScreenshotWindow::SetPackage(const PackageInfoRef& package)
{
	if (fPackage == package)
		return;

	fPackage = package;

	BString title = B_TRANSLATE("Screenshot");
	if (package.Get() != NULL) {
		title = package->Title();
		_DownloadScreenshot();
	}
	SetTitle(title);
}


// #pragma mark - private


void
ScreenshotWindow::_DownloadScreenshot()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0) {
		fDownloadPending = true;
		return;
	}

	thread_id thread = spawn_thread(&_DownloadThreadEntry,
		"Screenshot Loader", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		_SetWorkerThread(thread);
}


void
ScreenshotWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock())
		return;

//	bool enabled = thread < 0;
//
//	fPreviewsButton->SetEnabled(enabled);
//	fNextButton->SetEnabled(enabled);
//	fCloseButton->SetEnabled(enabled);

	if (thread >= 0) {
		fWorkerThread = thread;
		resume_thread(fWorkerThread);
	} else {
		fWorkerThread = -1;

		if (fDownloadPending) {
			_DownloadScreenshot();
			fDownloadPending = false;
		}
	}

	Unlock();
}


int32
ScreenshotWindow::_DownloadThreadEntry(void* data)
{
	ScreenshotWindow* window
		= reinterpret_cast<ScreenshotWindow*>(data);
	window->_DownloadThread();
	window->_SetWorkerThread(-1);
	return 0;
}


void
ScreenshotWindow::_DownloadThread()
{
	printf("_DownloadThread()\n");
	if (!Lock()) {
		printf("  failed to lock screenshot window\n");
		return;
	}

	fScreenshotView->UnsetBitmap();

	ScreenshotInfoList screenshotInfos;
	if (fPackage.Get() != NULL)
		screenshotInfos = fPackage->ScreenshotInfos();

	Unlock();

	if (screenshotInfos.CountItems() == 0) {
		printf("  package has no screenshots\n");
		return;
	}

	// Obtain the correct code for the screenshot to display
	// TODO: Once navigation buttons are added, we could use the
	// ScreenshotInfo at the "current" index.
	const ScreenshotInfo& info = screenshotInfos.ItemAtFast(0);

	BMallocIO buffer;
	WebAppInterface interface;

	// Retrieve screenshot from web-app
	status_t status = interface.RetrieveScreenshot(info.Code(),
		info.Width(), info.Height(), &buffer);
	if (status == B_OK && Lock()) {
		printf("got screenshot");
		fScreenshot = BitmapRef(new(std::nothrow)SharedBitmap(buffer), true);
		fScreenshotView->SetBitmap(fScreenshot);
		_ResizeToFitAndCenter();
		Unlock();
	} else {
		printf("  failed to download screenshot\n");
	}
}


void
ScreenshotWindow::_ResizeToFitAndCenter()
{
	float minWidth;
	float minHeight;
	GetSizeLimits(&minWidth, NULL, &minHeight, NULL);
	ResizeTo(minWidth, minHeight);
	CenterOnScreen();
}
