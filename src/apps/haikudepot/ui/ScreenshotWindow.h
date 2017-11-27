/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <ToolBar.h>
#include <Window.h>

#include "PackageInfo.h"


class BarberPole;
class BitmapView;
class BStringView;


enum {
	MSG_NEXT_SCREENSHOT     = 'nscr',
	MSG_PREVIOUS_SCREENSHOT = 'pscr',
	MSG_DOWNLOAD_START      = 'dlst',
	MSG_DOWNLOAD_STOP       = 'dlsp',
};


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

	static	void				CleanupIcons();

private:
			void				_DownloadScreenshot();

			void				_SetWorkerThread(thread_id thread);

	static	int32				_DownloadThreadEntry(void* data);
			void				_DownloadThread();

			void				_ResizeToFitAndCenter();
			void				_UpdateToolBar();

private:
	enum {
		kProgressIndicatorDelay = 200000 // us
	};

private:
			BMessenger			fOnCloseTarget;
			BMessage			fOnCloseMessage;

			BToolBar*			fToolBar;
			BarberPole*			fBarberPole;
			bool				fBarberPoleShown;
			BStringView*		fIndexView;

			BitmapRef			fScreenshot;
			BitmapView*			fScreenshotView;

			int32				fCurrentScreenshotIndex; // atomic

			PackageInfoRef		fPackage;
			bool				fDownloadPending;

			BLocker				fLock;
			thread_id			fWorkerThread;
};


#endif // SCREENSHOT_WINDOW_H
