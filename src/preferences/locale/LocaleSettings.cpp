/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleSettings.h"

#include <FindDirectory.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <SupportDefs.h>


static const uint32 kMsgLocaleSettings = 'LCst';


LocaleSettings::LocaleSettings()
	:
	fMessage(kMsgLocaleSettings),
	fSaved(false)
{
	// Set default preferences
	fMessage.AddString("language", "en");
	fMessage.AddString("country", "en_US");
}


status_t
LocaleSettings::Load()
{
	BFile file;
	status_t err;
	err = _Open(&file, B_READ_ONLY);
	if (err != B_OK)
		return err;
	err = fMessage.Unflatten(&file);
	if (err == B_OK)
		fSaved = true;
	return err;
}


status_t
LocaleSettings::Save()
{
	// Save on disk for next time we reboot
	BFile file;
	status_t err;
	err = _Open(&file, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (err != B_OK)
		return err;

	err = fMessage.Flatten(&file);
	if (err == B_OK)
		fSaved = true;
	return err;
}


status_t
LocaleSettings::_Open(BFile* file, int32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Locale settings");

	return file->SetTo(path.Path(), mode);
}


void
LocaleSettings::UpdateFrom(BMessage* message)
{
	BString messageContent;

	if (message->FindString("language", &messageContent) == B_OK) {
		fMessage.RemoveName("language");

		for (int i = 0;; i++) {
			if (message->FindString("language", i, &messageContent) != B_OK)
				break;
			fMessage.AddString("language", messageContent);
		}
		fSaved = false;
	}

	if (message->FindString("country", &messageContent) == B_OK) {
		fMessage.ReplaceString("country", messageContent);
		fMessage.RemoveName("shortTimeFormat");
		fMessage.RemoveName("longTimeFormat");
		fSaved = false;
	}

	if (message->FindString("shortTimeFormat", &messageContent) == B_OK) {
		fMessage.RemoveName("shortTimeFormat");
		fMessage.AddString("shortTimeFormat", messageContent);
		fSaved = false;
	}

	if (message->FindString("longTimeFormat", &messageContent) == B_OK) {
		fMessage.RemoveName("longTimeFormat");
		fMessage.AddString("longTimeFormat", messageContent);
		fSaved = false;
	}

	if (fSaved == false) {
		// Send to all running apps to notify them they should update their
		// settings
		fMessage.what = B_LOCALE_CHANGED;
		be_roster->Broadcast(&fMessage);
	}
}

