/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleSettings.h"

#include <FindDirectory.h>
#include <Path.h>
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
	BString langName;

	if (message->FindString("language", &langName) == B_OK) {
		fMessage.RemoveName("language");

		for (int i = 0;; i++) {
			if (message->FindString("language", i, &langName) != B_OK)
				break;
			fMessage.AddString("language", langName);
		}
	}

	if (message->FindString("country", &langName) == B_OK)
		fMessage.ReplaceString("country", langName);

	fSaved = false;
}

