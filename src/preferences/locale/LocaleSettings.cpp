/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleSettings.h"

#include <FindDirectory.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Path.h>
#include <String.h>
#include <SupportDefs.h>


using BPrivate::gMutableLocaleRoster;


static const uint32 kMsgLocaleSettings = 'LCst';


LocaleSettings::LocaleSettings()
	:
	fMessage(kMsgLocaleSettings)
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
	return fMessage.Unflatten(&file);
}


status_t
LocaleSettings::Save()
{
	return B_OK;
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

	BLocale locale(*be_locale);

	if (message->FindString("language", &messageContent) == B_OK) {
		fMessage.RemoveName("language");

		for (int i = 0;; i++) {
			if (message->FindString("language", i, &messageContent) != B_OK)
				break;
			fMessage.AddString("language", messageContent);
		}
		gMutableLocaleRoster->SetPreferredLanguages(message);
	}

	bool localeChanged = false;
	if (message->FindString("country", &messageContent) == B_OK) {
		fMessage.ReplaceString("country", messageContent);
		fMessage.RemoveName("shortTimeFormat");
		fMessage.RemoveName("longTimeFormat");
		locale.SetCountry(messageContent.String());
		localeChanged = true;
	}

	if (message->FindString("shortTimeFormat", &messageContent) == B_OK) {
		fMessage.RemoveName("shortTimeFormat");
		fMessage.AddString("shortTimeFormat", messageContent);
		locale.SetTimeFormat(messageContent, false);
		localeChanged = true;
	}

	if (message->FindString("longTimeFormat", &messageContent) == B_OK) {
		fMessage.RemoveName("longTimeFormat");
		fMessage.AddString("longTimeFormat", messageContent);
		locale.SetTimeFormat(messageContent, true);
		localeChanged = true;
	}

	if (localeChanged)
		gMutableLocaleRoster->SetDefaultLocale(locale);
}
