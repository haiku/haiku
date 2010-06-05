/*
 * Copyright 2010 Wim van der Meer <WPJvanderMeer@gmail.com>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Wim van der Meer
 */
#ifndef SCREENSHOT_APP_H
#define SCREENSHOT_APP_H


#include <Application.h>
#include <Catalog.h>


class Utility;


class ScreenshotApp : public BApplication {
public:
						ScreenshotApp();
						~ScreenshotApp();

			void		MessageReceived(BMessage* message);
			void		ReadyToRun();

private:
			BCatalog	fCatalog;
			Utility*	fUtility;
};


#endif // SCREENSHOT_APP_H
