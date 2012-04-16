/*
 * Copyright 2002-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */

#include "DataTranslations.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <String.h>
#include <TextView.h>
#include <TranslatorRoster.h>

#include "DataTranslationsWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DataTranslations"


const char* kApplicationSignature = "application/x-vnd.Haiku-DataTranslations";


DataTranslationsApplication::DataTranslationsApplication()
	:
	BApplication(kApplicationSignature)
{
	new DataTranslationsWindow();
}


DataTranslationsApplication::~DataTranslationsApplication()
{
}


void
DataTranslationsApplication::_InstallError(const char* name, status_t status)
{
	BString text;
	snprintf(text.LockBuffer(512), 512,
			B_TRANSLATE("Could not install %s:\n%s"), name, strerror(status));
	text.UnlockBuffer();
	BAlert* alert = new BAlert(B_TRANSLATE("DataTranslations - Error"),
		text.String(), B_TRANSLATE("OK"));
	alert->Go();
}


/*!
	Installs the given entry in the target directory either by moving
	or copying the entry.
*/
status_t
DataTranslationsApplication::_Install(BDirectory& target, BEntry& entry)
{
	// Find out whether we need to copy it
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
	BString text(
		B_TRANSLATE("The item '%name' does not appear to be a Translator and "
		"will not be installed."));
	text.ReplaceAll("%name", name);
	BAlert* alert = new BAlert("", text.String(), B_TRANSLATE("Ok"));
	alert->Go();
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

	int32 i = 0;
	entry_ref ref;
	while (message->FindRef("refs", i++, &ref) == B_OK) {
		if (!roster->IsTranslator(&ref)) {
			_NoTranslatorError(ref.name);
			continue;
		}

		BEntry entry(&ref, true);
		status = entry.InitCheck();
		if (status != B_OK) {
			_InstallError(ref.name, status);
			continue;
		}

		if (target.Contains(ref.name)) {
			BString string(
				B_TRANSLATE("An item named '%name' already exists in the "
				"Translators folder! Shall the existing translator be "
				"overwritten?"));
			string.ReplaceAll("%name", ref.name);

			BAlert* alert = new BAlert(B_TRANSLATE("DataTranslations - Note"),
				string.String(), B_TRANSLATE("Cancel"),
				B_TRANSLATE("Overwrite"));
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() != 1)
				continue;

			// the original file will be replaced
		}

		// find out whether we need to copy it or not

		status = _Install(target, entry);
		if (status == B_OK) {
			BAlert* alert = new BAlert(B_TRANSLATE("DataTranslations - Note"),
				B_TRANSLATE("The new translator has been installed "
					"successfully."), B_TRANSLATE("OK"));
			alert->Go(NULL);
		} else
			_InstallError(ref.name, status);
	}
}


//	#pragma mark -


int
main(int, char**)
{
	DataTranslationsApplication app;
	app.Run();

	return 0;
}
