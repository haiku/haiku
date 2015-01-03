/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "PackageInfo.h"


class BitmapView;


class ScreenshotWindow : public BWindow {
public:
								ScreenshotWindow(BWindow* parent, BRect frame);
	virtual						~ScreenshotWindow();

	virtual bool				QuitRequested();

	virtual	void				MessageReceived(BMessage* message);

			void				SetOnCloseMessage(
									const BMessenger& messenger,
									const BMessage& message);

			void				SetPackage(const PackageInfoRef& package);

private:
			void				_DownloadScreenshot();

			void				_SetWorkerThread(thread_id thread);

	static	int32				_DownloadThreadEntry(void* data);
			void				_DownloadThread();

			void				_ResizeToFitAndCenter();

private:
			BMessenger			fOnCloseTarget;
			BMessage			fOnCloseMessage;

			BitmapRef			fScreenshot;
			BitmapView*			fScreenshotView;

			PackageInfoRef		fPackage;
			bool				fDownloadPending;

			BLocker				fLock;
			thread_id			fWorkerThread;
};


#endif // SCREENSHOT_WINDOW_H
