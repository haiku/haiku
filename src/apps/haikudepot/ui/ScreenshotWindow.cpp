/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ScreenshotWindow.h"

#include <algorithm>

#include <Autolock.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <StringView.h>

#include "BarberPole.h"
#include "BitmapView.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenshotWindow"


static const rgb_color kBackgroundColor = { 51, 102, 152, 255 };
	// Drawn as a border around the screenshots and also what's behind their
	// transparent regions

static BitmapRef sNextButtonIcon(
	new(std::nothrow) SharedBitmap(RSRC_ARROW_LEFT), true);
static BitmapRef sPreviousButtonIcon(
	new(std::nothrow) SharedBitmap(RSRC_ARROW_RIGHT), true);


ScreenshotWindow::ScreenshotWindow(BWindow* parent, BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Screenshot"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fBarberPoleShown(false),
	fDownloadPending(false),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	atomic_set(&fCurrentScreenshotIndex, 0);

	fBarberPole = new BarberPole("barber pole");
	fBarberPole->SetExplicitMaxSize(BSize(100, B_SIZE_UNLIMITED));
	fBarberPole->Hide();

	fIndexView = new BStringView("screenshot index", NULL);

	fToolBar = new BToolBar();
	fToolBar->AddAction(MSG_PREVIOUS_SCREENSHOT, this,
		sNextButtonIcon->Bitmap(BITMAP_SIZE_22),
		NULL, NULL);
	fToolBar->AddAction(MSG_NEXT_SCREENSHOT, this,
		sPreviousButtonIcon->Bitmap(BITMAP_SIZE_22),
		NULL, NULL);
	fToolBar->AddView(fIndexView);
	fToolBar->AddGlue();
	fToolBar->AddView(fBarberPole);

	fScreenshotView = new BitmapView("screenshot view");
	fScreenshotView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fScreenshotView->SetScaleBitmap(false);

	BGroupView* groupView = new BGroupView(B_VERTICAL);
	groupView->SetViewColor(kBackgroundColor);

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 3, 0, 0)
		.Add(fToolBar)
		.AddStrut(3)
		.AddGroup(groupView)
			.Add(fScreenshotView)
			.SetInsets(B_USE_WINDOW_INSETS)
		.End()
	;

	fScreenshotView->SetLowColor(kBackgroundColor);
		// Set after attaching all views to prevent it from being overriden
		// again by BitmapView::AllAttached()

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
		case MSG_NEXT_SCREENSHOT:
		{
			atomic_add(&fCurrentScreenshotIndex, 1);
			_UpdateToolBar();
			_DownloadScreenshot();
			break;
		}

		case MSG_PREVIOUS_SCREENSHOT:
			atomic_add(&fCurrentScreenshotIndex, -1);
			_UpdateToolBar();
			_DownloadScreenshot();
			break;

		case MSG_DOWNLOAD_START:
			if (!fBarberPoleShown) {
				fBarberPole->Start();
				fBarberPole->Show();
				fBarberPoleShown = true;
			}
			break;

		case MSG_DOWNLOAD_STOP:
			if (fBarberPoleShown) {
				fBarberPole->Hide();
				fBarberPole->Stop();
				fBarberPoleShown = true;
			}
			break;

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

	atomic_set(&fCurrentScreenshotIndex, 0);

	_UpdateToolBar();
}


/* static */ void
ScreenshotWindow::CleanupIcons()
{
	sNextButtonIcon.Unset();
	sPreviousButtonIcon.Unset();
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
	if (!Lock()) {
		HDERROR("failed to lock screenshot window");
		return;
	}

	fScreenshotView->UnsetBitmap();

	ScreenshotInfoList screenshotInfos;
	if (fPackage.Get() != NULL)
		screenshotInfos = fPackage->ScreenshotInfos();

	Unlock();

	if (screenshotInfos.CountItems() == 0) {
		HDINFO("package has no screenshots");
		return;
	}

	// Obtain the correct code for the screenshot to display
	// TODO: Once navigation buttons are added, we could use the
	// ScreenshotInfo at the "current" index.
	const ScreenshotInfo& info = screenshotInfos.ItemAtFast(
		atomic_get(&fCurrentScreenshotIndex));

	BMallocIO buffer;
	WebAppInterface interface;

	// Only indicate being busy with the download if it takes a little while
	BMessenger messenger(this);
	BMessageRunner delayedMessenger(messenger,
		new BMessage(MSG_DOWNLOAD_START),
		kProgressIndicatorDelay, 1);

	// Retrieve screenshot from web-app
	status_t status = interface.RetrieveScreenshot(info.Code(),
		info.Width(), info.Height(), &buffer);

	delayedMessenger.SetCount(0);
	messenger.SendMessage(MSG_DOWNLOAD_STOP);

	if (status == B_OK && Lock()) {
		HDINFO("got screenshot");
		fScreenshot = BitmapRef(new(std::nothrow)SharedBitmap(buffer), true);
		fScreenshotView->SetBitmap(fScreenshot);
		_ResizeToFitAndCenter();
		Unlock();
	} else
		HDERROR("failed to download screenshot");
}


void
ScreenshotWindow::_ResizeToFitAndCenter()
{
	// Find out dimensions of the largest screenshot of this package
	ScreenshotInfoList screenshotInfos;
	if (fPackage.Get() != NULL)
		screenshotInfos = fPackage->ScreenshotInfos();

	int32 largestScreenshotWidth = 0;
	int32 largestScreenshotHeight = 0;

	const uint32 numScreenshots = fPackage->ScreenshotInfos().CountItems();
	for (uint32 i = 0; i < numScreenshots; i++) {
		const ScreenshotInfo& info = screenshotInfos.ItemAtFast(i);
		if (info.Width() > largestScreenshotWidth)
			largestScreenshotWidth = info.Width();
		if (info.Height() > largestScreenshotHeight)
			largestScreenshotHeight = info.Height();
	}

	fScreenshotView->SetExplicitMinSize(
		BSize(largestScreenshotWidth, largestScreenshotHeight));
	Layout(false);

	// TODO: Limit window size to screen size (with a little margin),
	//       the image should then become scrollable.

	float minWidth;
	float minHeight;
	GetSizeLimits(&minWidth, NULL, &minHeight, NULL);
	ResizeTo(minWidth, minHeight);
	CenterOnScreen();
}


void
ScreenshotWindow::_UpdateToolBar()
{
	const int32 numScreenshots = fPackage->ScreenshotInfos().CountItems();
	const int32 currentIndex = atomic_get(&fCurrentScreenshotIndex);

	fToolBar->SetActionEnabled(MSG_PREVIOUS_SCREENSHOT,
		currentIndex > 0);
	fToolBar->SetActionEnabled(MSG_NEXT_SCREENSHOT,
		currentIndex < numScreenshots - 1);

	BString text;
	text << currentIndex + 1;
	text << " / ";
	text << numScreenshots;
	fIndexView->SetText(text);
}
