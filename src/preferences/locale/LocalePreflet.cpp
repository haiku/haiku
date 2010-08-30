/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>. All rightts reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <AboutWindow.h>
#include <Application.h>
#include <Alert.h>
#include <Catalog.h>
#include <TextView.h>
#include <Locale.h>

#include "LocalePreflet.h"
#include "LocaleSettings.h"
#include "LocaleWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Locale Preflet"


const char* kSignature = "application/x-vnd.Haiku-Locale";


class LocalePreflet : public BApplication {
	public:
							LocalePreflet();
		virtual				~LocalePreflet();

		virtual	void		MessageReceived(BMessage* message);
		virtual	void		AboutRequested();

private:
		LocaleSettings		fSettings;
		LocaleSettings		fOriginalSettings;
		LocaleWindow*		fLocaleWindow;
};


//	#pragma mark -


LocalePreflet::LocalePreflet()
	:
	BApplication(kSignature)
{
	fLocaleWindow = new LocaleWindow();

	fOriginalSettings.Load();
	fSettings = fOriginalSettings;

	/*
	if (fSettings.Message().HasPoint("window_location")) {
		BPoint point = fSettings.Message().FindPoint("window_location");
		fLocaleWindow->MoveTo(point);
	}
	*/

	fLocaleWindow->Show();
}


LocalePreflet::~LocalePreflet()
{
	fSettings.Save();
}


void
LocalePreflet::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSettingsChanged:
			fSettings.UpdateFrom(message);
			break;

		case kMsgRevert:
			fSettings = fOriginalSettings;
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
LocalePreflet::AboutRequested()
{
	const char* authors[3];
	authors[0] = "Axel Dörfler";
	authors[1] = "Adrien Destugues";
	authors[2] = NULL;
	BAboutWindow about(B_TRANSLATE("Locale"), 2005, authors);
	about.Show();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	LocalePreflet app;
	app.Run();
	return 0;
}

