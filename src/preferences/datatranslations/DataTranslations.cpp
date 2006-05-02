/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */


#include "DataTranslations.h"
#include "DataTranslationsWindow.h"
#include "DataTranslationsSettings.h"

#include <Alert.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <String.h>
#include <TranslatorRoster.h>
#include <TextView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char* kApplicationSignature = "application/x-vnd.haiku-translations";


DataTranslationsApplication::DataTranslationsApplication()
	: BApplication(kApplicationSignature)
{
	fSettings = new DataTranslationsSettings();
	new DataTranslationsWindow();
}


DataTranslationsApplication::~DataTranslationsApplication()
{
	delete fSettings;
}


void
DataTranslationsApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}


void
DataTranslationsApplication::AboutRequested()
{
	BAlert *alert = new BAlert("about", "DataTranslations\n"
		"\twritten by Oliver Siebenmarck and others\n"
		"\tCopyright 2002-2006, Haiku.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 16, &font);

	alert->Go();
}


void
DataTranslationsApplication::_InstallError(const char* name, status_t status)
{
	BString text;
	text << "Could not install " << name << ":\n" << strerror(status);
	(new BAlert("DataTranslations - Error", text.String(), "Stop"))->Go();
}


/*!
	Installs the given entry in the target directory either by moving
	or copying the entry.
*/
status_t
DataTranslationsApplication::_Install(BDirectory& target, BEntry& entry)
{
	// Find out wether we need to copy it
	status_t status = entry.MoveTo(&target, NULL, true);

	if (status == B_OK)
		return B_OK;

	// we need to copy the file

	// TODO!
	return B_ERROR;
}


void
DataTranslationsApplication::_NoTranslatorError(const char* name)
{
	BString text;
	text.SetTo("The item '");
	text << name << "' does not appear to be a Translator and will not be installed.";
	(new BAlert("", text.String(), "Stop"))->Go();
}


void
DataTranslationsApplication::RefsReceived(BMessage* message)
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();

	BPath path;
	status_t status = find_directory(B_USER_ADDONS_DIRECTORY, &path, true);
	if (status != B_OK) {
		_InstallError("translator", status);
		return;
	}

	BDirectory target;
	status = target.SetTo(path.Path());
	if (status == B_OK) {
		if (!target.Contains("Translators"))
			status = target.CreateDirectory("Translators", &target);
		else
			status = target.SetTo(&target, "Translators");
	}
	if (status != B_OK) {
		_InstallError("translator", status);
		return;
	}

	entry_ref ref;
	int32 i = 0;
	while (message->FindRef("refs", i++, &ref) == B_OK) {
		if (!roster->IsTranslator(&ref))
			_NoTranslatorError(ref.name);

    	BEntry entry(&ref, true);
    	status = entry.InitCheck();
    	if (status != B_OK) {
			_InstallError(ref.name, status);
    		continue;
    	}

		if (target.Contains(ref.name)) {
			BString string;
			string << "An item named '" << ref.name
				<< "' already exists in the Translators folder";

			BAlert* alert = new BAlert("DataTranslations - Note", string.String(),
				"Overwrite", "Stop");
			alert->SetShortcut(1, B_ESCAPE);
			if (alert->Go() != 0)
				continue;

			// the original file will be replaced
		}

		// find out wether we need to copy it or not

		status = _Install(target, entry);
		if (status == B_OK) {
			(new BAlert("DataTranslations - Note",
				"The new translator has been installed successfully",
				"OK"))->Go(NULL);
		} else
			_InstallError(ref.name, status);
	}
}


//	#pragma mark -


int
main(int, char**)
{
	DataTranslationsApplication	app;
	app.Run();

	return 0;
}

