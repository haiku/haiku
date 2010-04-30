/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <AboutWindow.h>
#include <Application.h>
#include <Alert.h>
#include <Catalog.h>
#include <TextView.h>
#include <FindDirectory.h>
#include <File.h>
#include <Locale.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>


#define TR_CONTEXT "Locale Preflet"


const char* kSignature = "application/x-vnd.Haiku-Locale";

static const uint32 kMsgLocaleSettings = 'LCst';


class Settings {
public:
							Settings();
							~Settings();

	const		BMessage&	Message() const { return fMessage; }
				void		UpdateFrom(BMessage* message);

private:
				status_t	_Open(BFile* file, int32 mode);

				BMessage	fMessage;
				bool		fUpdated;
};


class LocalePreflet : public BApplication {
public:
							LocalePreflet();
	virtual					~LocalePreflet();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			AboutRequested();

private:
				Settings	fSettings;
				LocaleWindow*	fLocaleWindow;
				BCatalog	fCatalog;
};


//-----------------


Settings::Settings()
	:
	fMessage(kMsgLocaleSettings),
	fUpdated(false)
{
	BFile file;
	if (_Open(&file, B_READ_ONLY) != B_OK
		|| fMessage.Unflatten(&file) != B_OK) {
		// set default prefs
		fMessage.AddString("language", "en");
		fMessage.AddString("country", "en_US");
		return;
	}
}


Settings::~Settings()
{
	if (!fUpdated)
		return;

	BFile file;
	if (_Open(&file, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY) != B_OK)
		return;

	fMessage.Flatten(&file);
}


status_t
Settings::_Open(BFile* file, int32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Locale settings");

	return file->SetTo(path.Path(), mode);
}


void
Settings::UpdateFrom(BMessage* message)
{
	BPoint point;
	if (message->FindPoint("window_location", &point) == B_OK)
		fMessage.ReplacePoint("window_location", point);

	BString langName;
	// We make sure there is at least one string before erasing the previous
	// settings, then we add the remaining ones, if any
	if (message->FindString("language", &langName) == B_OK) {
		// Remove any old data as we know we have newer one to replace it
		fMessage.RemoveName("language");
		for (int i = 0;; i++) {
			if (message->FindString("language", i, &langName) != B_OK)
				break;
			fMessage.AddString("language", langName);
		}
	}

	if (message->FindString("country", &langName) == B_OK)
		fMessage.ReplaceString("country", langName);

	fUpdated = true;
}


//	#pragma mark -


LocalePreflet::LocalePreflet()
	:
	BApplication(kSignature)
{
	be_locale->GetAppCatalog(&fCatalog);

	fLocaleWindow = new LocaleWindow();

	if (fSettings.Message().HasPoint("window_location")) {
		BPoint point = fSettings.Message().FindPoint("window_location");
		fLocaleWindow->MoveTo(point);
	}

	fLocaleWindow->Show();
}


LocalePreflet::~LocalePreflet()
{
}


void
LocalePreflet::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSettingsChanged:
			fSettings.UpdateFrom(message);
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
	(new BAboutWindow(TR("Locale"), 2005, authors))->Show();
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	LocalePreflet app;
	app.Run();
	return 0;
}

