/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Locale.h"
#include "LocaleWindow.h"

#include <Application.h>
#include <Alert.h>
#include <TextView.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>


const char *kSignature = "application/x-vnd.Haiku-Locale";

static const uint32 kMsgLocaleSettings = 'LCst';


class Settings {
	public:
		Settings();
		~Settings();

		const BMessage &Message() const { return fMessage; }
		void UpdateFrom(BMessage *message);

	private:
		status_t Open(BFile *file, int32 mode);

		BMessage	fMessage;
		bool		fUpdated;
};


class Locale : public BApplication {
	public:
		Locale();
		virtual ~Locale();

		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage *message);

		virtual void AboutRequested();
		virtual bool QuitRequested();

	private:
		Settings	fSettings;
		BWindow		*fOpenWindow;
		BRect		fWindowFrame;
};


//-----------------


Settings::Settings()
	:
	fMessage(kMsgLocaleSettings),
	fUpdated(false)
{
	BFile file;
	if (Open(&file, B_READ_ONLY) != B_OK
		|| fMessage.Unflatten(&file) != B_OK) {
		// set default prefs
		fMessage.AddRect("window_frame", BRect(50, 50, 550, 500));
		return;
	}
}


Settings::~Settings()
{
	// only save the settings if something has changed
	if (!fUpdated)
		return;

	BFile file;
	if (Open(&file, B_CREATE_FILE | B_WRITE_ONLY) != B_OK)
		return;

	fMessage.Flatten(&file);
}


status_t 
Settings::Open(BFile *file, int32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Locale settings");

	return file->SetTo(path.Path(), mode);
}


void 
Settings::UpdateFrom(BMessage *message)
{
	BRect frame;
	if (message->FindRect("window_frame", &frame) == B_OK)
		fMessage.ReplaceRect("window_frame", frame);

	fUpdated = true;
}


//	#pragma mark -


Locale::Locale()
	: BApplication(kSignature)
{
	fWindowFrame = fSettings.Message().FindRect("window_frame");

	BWindow* window = new LocaleWindow(fWindowFrame);
	window->Show();
}


Locale::~Locale()
{
}


void 
Locale::ReadyToRun()
{
	// are there already windows open?
	if (CountWindows() != 1)
		return;

	// if not, ask the user to open a file
	PostMessage(kMsgOpenOpenWindow);
}


void 
Locale::MessageReceived(BMessage *message)
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
Locale::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Locale\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2005, Haiku.\n\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 7, &font);

	alert->Go();
}


bool 
Locale::QuitRequested()
{
	return true;
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	Locale app;

	app.Run();
	return 0;
}
