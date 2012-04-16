/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or
registered trademarks of Be Incorporated in the United States and other
countries. Other brand product names are registered trademarks or trademarks
of their respective holders. All rights reserved.
*/


#include "Settings.h"

#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Locale.h>
#include <MailSettings.h>
#include <Message.h>
#include <String.h>
#include <UTF8.h>

#include <mail_encoding.h>


#define B_TRANSLATION_CONTEXT "Settings"


using namespace BPrivate ;


Settings::Settings()
	:
	fMailWindowFrame(BRect(0, 0, 200, 400)),
	fPrintSettings(NULL),
	fAutoMarkRead(true),
	fSignature(),
	fReplyPreamble(),
	fWrapMode(true),
	fAttachAttributes(true),
	fColoredQuotes(true),
	fShowButtonBar(true),
	fWarnAboutUnencodableCharacters(true),
	fStartWithSpellCheckOn(false),
	fShowSpamGUI(true),
	fDefaultChain(0),
	fUseAccountFrom(0),
	fMailCharacterSet(B_MS_WINDOWS_CONVERSION),
	fContentFont(be_fixed_font)
{
	fContentFont.SetSize(12.0);
	fSignature = B_TRANSLATE("None");

	LoadSettings();

	fContentFont.SetSpacing(B_BITMAP_SPACING);

	_CheckForSpamFilterExistence();
}


Settings::~Settings()
{
}


void
Settings::SetPrintSettings(const BMessage* printSettings)
{
	BAutolock lock(be_app);

	if (printSettings == fPrintSettings)
		return;

	delete fPrintSettings;
	if (printSettings)
		fPrintSettings = new BMessage(*printSettings);
	else
		fPrintSettings = NULL;
}


bool
Settings::HasPrintSettings()
{
	BAutolock lock(be_app);
	return fPrintSettings != NULL;
}


BMessage
Settings::PrintSettings()
{
	BAutolock lock(be_app);
	return BMessage(*fPrintSettings);
}


void
Settings::ClearPrintSettings()
{
	delete fPrintSettings;
	fPrintSettings = NULL;
}


void
Settings::SetWindowFrame(BRect frame)
{
	BAutolock lock(be_app);
	fMailWindowFrame = frame;
}


BRect
Settings::WindowFrame()
{
	BAutolock lock(be_app);
	return fMailWindowFrame;
}


status_t
Settings::_GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("Mail");
	return create_directory(path.Path(), 0755);
}


status_t
Settings::SaveSettings()
{
	BMailSettings chainSettings;

	if (fDefaultChain != ~0UL) {
		chainSettings.SetDefaultOutboundChainID(fDefaultChain);
		chainSettings.Save();
	}

	BPath path;
	status_t status = _GetSettingsPath(path);
	if (status != B_OK)
		return status;

	path.Append("Mail Settings~");

	BFile file;
	status = file.SetTo(path.Path(),
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);

	if (status != B_OK)
		return status;

	BMessage settings('BeMl');
	settings.AddRect("MailWindowSize", fMailWindowFrame);

	font_family fontFamily;
	font_style fontStyle;
	fContentFont.GetFamilyAndStyle(&fontFamily, &fontStyle);

	settings.AddString("FontFamily", fontFamily);
	settings.AddString("FontStyle", fontStyle);
	settings.AddFloat("FontSize", fContentFont.Size());

	settings.AddBool("WordWrapMode", fWrapMode);
	settings.AddBool("AutoMarkRead", fAutoMarkRead);
	settings.AddString("SignatureText", fSignature);
	settings.AddInt32("CharacterSet", fMailCharacterSet);
	settings.AddInt8("ShowButtonBar", fShowButtonBar);
	settings.AddInt32("UseAccountFrom", fUseAccountFrom);
	settings.AddBool("ColoredQuotes", fColoredQuotes);
	settings.AddString("ReplyPreamble", fReplyPreamble);
	settings.AddBool("AttachAttributes", fAttachAttributes);
	settings.AddBool("WarnAboutUnencodableCharacters",
		fWarnAboutUnencodableCharacters);
	settings.AddBool("StartWithSpellCheck", fStartWithSpellCheckOn);

	BEntry entry;
	status = entry.SetTo(path.Path());
	if (status != B_OK)
		return status;

	status = settings.Flatten(&file);
	if (status == B_OK) {
		// replace original settings file
		status = entry.Rename("Mail Settings", true);
	} else
		entry.Remove();

	return status;
}


status_t
Settings::LoadSettings()
{
	BMailSettings chainSettings;
	fDefaultChain = chainSettings.DefaultOutboundChainID();

	BPath path;
	status_t status = _GetSettingsPath(path);
	if (status != B_OK)
		return status;

	path.Append("Mail Settings");

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK) {
		_GetSettingsPath(path);
		path.Append("BeMail Settings");
		status = file.SetTo(path.Path(), B_READ_ONLY);
		if (status != B_OK)
			return status;
	}

	BMessage settings;
	status = settings.Unflatten(&file);
	if (status < B_OK || settings.what != 'BeMl')
		return status;

	BRect rect;
	if (settings.FindRect("MailWindowSize", &rect) == B_OK)
		fMailWindowFrame = rect;

	int32 int32Value;

	const char *fontFamily;
	if (settings.FindString("FontFamily", &fontFamily) == B_OK) {
		const char *fontStyle;
		if (settings.FindString("FontStyle", &fontStyle) == B_OK) {
			float size;
			if (settings.FindFloat("FontSize", &size) == B_OK) {
				if (size >= 7)
					fContentFont.SetSize(size);

				if (fontFamily[0] && fontStyle[0]) {
					fContentFont.SetFamilyAndStyle(
						fontFamily[0] ? fontFamily : NULL,
						fontStyle[0] ? fontStyle : NULL);
				}
			}
		}
	}

	bool boolValue;
	if (settings.FindBool("WordWrapMode", &boolValue) == B_OK)
		fWrapMode = boolValue;

	if (settings.FindBool("AutoMarkRead", &boolValue) == B_OK)
		fAutoMarkRead = boolValue;

	BString string;
	if (settings.FindString("SignatureText", &string) == B_OK) {
		fSignature = string;
	}

	if (settings.FindInt32("CharacterSet", &int32Value) == B_OK)
		fMailCharacterSet = int32Value;
	if (fMailCharacterSet != B_MAIL_UTF8_CONVERSION
		&& fMailCharacterSet != B_MAIL_US_ASCII_CONVERSION
		&& BCharacterSetRoster::GetCharacterSetByConversionID(fMailCharacterSet)
			== NULL) {
		fMailCharacterSet = B_MS_WINDOWS_CONVERSION;
	}

	int8 int8Value;
	if (settings.FindInt8("ShowButtonBar", &int8Value) == B_OK)
		fShowButtonBar = int8Value;

	if (settings.FindInt32("UseAccountFrom", &int32Value) == B_OK)
		fUseAccountFrom = int32Value;
	if (fUseAccountFrom < ACCOUNT_USE_DEFAULT
		|| fUseAccountFrom > ACCOUNT_FROM_MAIL)
		fUseAccountFrom = ACCOUNT_USE_DEFAULT;

	if (settings.FindBool("ColoredQuotes", &boolValue) == B_OK)
		fColoredQuotes = boolValue;

	if (settings.FindString("ReplyPreamble", &string) == B_OK) {
		fReplyPreamble = string;
	}

	if (settings.FindBool("AttachAttributes", &boolValue) == B_OK)
		fAttachAttributes = boolValue;

	if (settings.FindBool("WarnAboutUnencodableCharacters", &boolValue) == B_OK)
		fWarnAboutUnencodableCharacters = boolValue;

	if (settings.FindBool("StartWithSpellCheck", &boolValue) == B_OK)
		fStartWithSpellCheckOn = boolValue;

	return B_OK;
}


bool
Settings::AutoMarkRead()
{
	BAutolock lock(be_app);
	return fAutoMarkRead;
}


BString
Settings::Signature()
{
	BAutolock lock(be_app);
	return BString(fSignature);
}


BString
Settings::ReplyPreamble()
{
	BAutolock lock(be_app);
	return BString(fReplyPreamble);
}


bool
Settings::WrapMode()
{
	BAutolock lock(be_app);
	return fWrapMode;
}


bool
Settings::AttachAttributes()
{
	BAutolock lock(be_app);
	return fAttachAttributes;
}


bool
Settings::ColoredQuotes()
{
	BAutolock lock(be_app);
	return fColoredQuotes;
}


uint8
Settings::ShowButtonBar()
{
	BAutolock lock(be_app);
	return fShowButtonBar;
}


bool
Settings::WarnAboutUnencodableCharacters()
{
	BAutolock lock(be_app);
	return fWarnAboutUnencodableCharacters;
}


bool
Settings::StartWithSpellCheckOn()
{
	BAutolock lock(be_app);
	return fStartWithSpellCheckOn;
}


void
Settings::SetDefaultChain(uint32 chain)
{
	BAutolock lock(be_app);
	fDefaultChain = chain;
}


uint32
Settings::DefaultChain()
{
	BAutolock lock(be_app);
	return fDefaultChain;
}


int32
Settings::UseAccountFrom()
{
	BAutolock lock(be_app);
	return fUseAccountFrom;
}


uint32
Settings::MailCharacterSet()
{
	BAutolock lock(be_app);
	return fMailCharacterSet;
}


BFont
Settings::ContentFont()
{
	BAutolock lock(be_app);
	return fContentFont;
}


void
Settings::_CheckForSpamFilterExistence()
{
	int32 addonNameIndex;
	const char* addonNamePntr;
	BDirectory inChainDir;
	BPath path;
	BEntry settingsEntry;
	BFile settingsFile;
	BMessage settingsMessage;

	fShowSpamGUI = false;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
	path.Append("Mail/chains/inbound");
	if (inChainDir.SetTo(path.Path()) != B_OK)
		return;

	while (inChainDir.GetNextEntry (&settingsEntry, true) == B_OK) {
		if (!settingsEntry.IsFile())
			continue;
		if (settingsFile.SetTo (&settingsEntry, B_READ_ONLY) != B_OK)
			continue;
		if (settingsMessage.Unflatten (&settingsFile) != B_OK)
			continue;
		for (addonNameIndex = 0;
				B_OK == settingsMessage.FindString("filter_addons",
					addonNameIndex, &addonNamePntr);
				addonNameIndex++) {
			if (strstr(addonNamePntr, "Spam Filter") != NULL) {
				fShowSpamGUI = true;
				return;
			}
		}
	}
}

