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


#include "MailApp.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <Autolock.h>
#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Clipboard.h>
#include <Debug.h>
#include <E-mail.h>
#include <InterfaceKit.h>
#include <Locale.h>
#include <Roster.h>
#include <Screen.h>
#include <StorageKit.h>
#include <String.h>
#include <TextView.h>
#include <UTF8.h>

#include <fs_index.h>
#include <fs_info.h>

#include <MailMessage.h>
#include <MailSettings.h>
#include <MailDaemon.h>
#include <mail_util.h>

#include <CharacterSetRoster.h>

using namespace BPrivate ;

#include "ButtonBar.h"
#include "Content.h"
#include "Enclosures.h"
#include "FieldMsg.h"
#include "FindWindow.h"
#include "Header.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"
#include "Prefs.h"
#include "QueryMenu.h"
#include "Signature.h"
#include "Status.h"
#include "String.h"
#include "Utilities.h"
#include "Words.h"


#define B_TRANSLATE_CONTEXT "Mail"


static const char *kDictDirectory = "word_dictionary";
static const char *kIndexDirectory = "word_index";
static const char *kWordsPath = "/boot/optional/goodies/words";
static const char *kExact = ".exact";
static const char *kMetaphone = ".metaphone";


TMailApp::TMailApp()
	:
	BApplication("application/x-vnd.Be-MAIL"),
	fWindowCount(0),
	fPrefsWindow(NULL),
	fSigWindow(NULL),

	fPrintSettings(NULL),
	fPrintHelpAndExit(false),

	fWrapMode(true),
	fAttachAttributes(true),
	fColoredQuotes(true),
	fShowButtonBar(true),
	fWarnAboutUnencodableCharacters(true),
	fStartWithSpellCheckOn(false),
	fShowSpamGUI(true),
	fMailCharacterSet(B_MAIL_UTF8_CONVERSION),
	fContentFont(be_fixed_font)
{
	// set default values
	fContentFont.SetSize(12.0);
	fAutoMarkRead = true;
	fSignature = (char*)malloc(strlen(B_TRANSLATE("None")) + 1);
	strcpy(fSignature, B_TRANSLATE("None"));
	fReplyPreamble = strdup(B_TRANSLATE("%e wrote:\\n"));

	fMailWindowFrame.Set(0, 0, 0, 0);
	fSignatureWindowFrame.Set(6, TITLE_BAR_HEIGHT, 6 + kSigWidth,
		TITLE_BAR_HEIGHT + kSigHeight);
	fPrefsWindowPos.Set(6, TITLE_BAR_HEIGHT);

	const BCharacterSet *defaultComposeEncoding = 
		BCharacterSetRoster::FindCharacterSetByName(
		B_TRANSLATE_COMMENT("UTF-8", "This string is used as a key to set "
		"default message compose encoding. It must be correct IANA name from "
		"http://cgit.haiku-os.org/haiku-tree/src/kits/textencoding"
		"/character_sets.cpp Translate it only if you want to change default "
		"message compose encoding for your locale. If you don't know what is "
		"it and why it may needs changing, just leave \"UTF-8\"."));
	if (defaultComposeEncoding != NULL)
		fMailCharacterSet = defaultComposeEncoding->GetConversionID();

	// Find and read settings file.
	LoadSettings();

	_CheckForSpamFilterExistence();
	fContentFont.SetSpacing(B_BITMAP_SPACING);
	fLastMailWindowFrame = fMailWindowFrame;
}


TMailApp::~TMailApp()
{
}


void
TMailApp::ArgvReceived(int32 argc, char **argv)
{
	BEntry entry;
	BString names;
	BString ccNames;
	BString bccNames;
	BString subject;
	BString body;
	BMessage enclosure(B_REFS_RECEIVED);
	// a "mailto:" with no name should open an empty window
	// so remember if we got a "mailto:" even if there isn't a name
	// that goes along with it (this allows deskbar replicant to open
	// an empty message even when Mail is already running)
	bool gotmailto = false;

	for (int32 loop = 1; loop < argc; loop++)
	{
		if (strcmp(argv[loop], "-h") == 0
			|| strcmp(argv[loop], "--help") == 0)
		{
			printf(" usage: %s [ mailto:<address> ] [ -subject \"<text>\" ] [ ccto:<address> ] [ bccto:<address> ] "
				"[ -body \"<body text>\" ] [ enclosure:<path> ] [ <message to read> ...] \n",
				argv[0]);
			fPrintHelpAndExit = true;
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		}
		else if (strncmp(argv[loop], "mailto:", 7) == 0)
		{
			if (names.Length())
				names += ", ";
			char *options;
			if ((options = strchr(argv[loop],'?')) != NULL)
			{
				names.Append(argv[loop] + 7, options - argv[loop] - 7);
				if (!strncmp(++options,"subject=",8))
					subject = options + 8;
			}
			else
				names += argv[loop] + 7;
			gotmailto = true;
		}
		else if (strncmp(argv[loop], "ccto:", 5) == 0)
		{
			if (ccNames.Length())
				ccNames += ", ";
			ccNames += argv[loop] + 5;
		}
		else if (strncmp(argv[loop], "bccto:", 6) == 0)
		{
			if (bccNames.Length())
				bccNames += ", ";
			bccNames += argv[loop] + 6;
		}
		else if (strcmp(argv[loop], "-subject") == 0)
			subject = argv[++loop];
		else if (strcmp(argv[loop], "-body") == 0 && argv[loop + 1])
			body = argv[++loop];
		else if (strncmp(argv[loop], "enclosure:", 10) == 0)
		{
			BEntry tmp(argv[loop] + 10, true);
			if (tmp.InitCheck() == B_OK && tmp.Exists())
			{
				entry_ref ref;
				tmp.GetRef(&ref);
				enclosure.AddRef("refs", &ref);
			}
		}
		else if (entry.SetTo(argv[loop]) == B_NO_ERROR)
		{
			BMessage msg(B_REFS_RECEIVED);
			entry_ref ref;
			entry.GetRef(&ref);
			msg.AddRef("refs", &ref);
			RefsReceived(&msg);
		}
	}

	if (gotmailto || names.Length() || ccNames.Length() || bccNames.Length() || subject.Length()
		|| body.Length() || enclosure.HasRef("refs"))
	{
		TMailWindow	*window = NewWindow(NULL, names.String());
		window->SetTo(names.String(), subject.String(), ccNames.String(), bccNames.String(),
			&body, &enclosure);
		window->Show();
	}
}


void
TMailApp::MessageReceived(BMessage *msg)
{
	TMailWindow	*window = NULL;
	entry_ref ref;

	switch (msg->what) {
		case M_NEW:
		{
			int32 type;
			msg->FindInt32("type", &type);
			switch (type) {
				case M_NEW:
					window = NewWindow();
					break;

				case M_RESEND:
				{
					msg->FindRef("ref", &ref);
					BNode file(&ref);
					BString string;

					if (file.InitCheck() == B_OK)
						file.ReadAttrString(B_MAIL_ATTR_TO, &string);

					window = NewWindow(&ref, string.String(), true);
					break;
				}
				case M_FORWARD:
				case M_FORWARD_WITHOUT_ATTACHMENTS:
				{
					TMailWindow	*sourceWindow;
					if (msg->FindPointer("window", (void **)&sourceWindow) < B_OK
						|| !sourceWindow->Lock())
						break;

					msg->FindRef("ref", &ref);
					window = NewWindow();
					if (window->Lock()) {
						window->Forward(&ref, sourceWindow, type == M_FORWARD);
						window->Unlock();
					}
					sourceWindow->Unlock();
					break;
				}

				case M_REPLY:
				case M_REPLY_TO_SENDER:
				case M_REPLY_ALL:
				case M_COPY_TO_NEW:
				{
					TMailWindow	*sourceWindow;
					if (msg->FindPointer("window", (void **)&sourceWindow) < B_OK
						|| !sourceWindow->Lock())
						break;
					msg->FindRef("ref", &ref);
					window = NewWindow();
					if (window->Lock()) {
						if (type == M_COPY_TO_NEW)
							window->CopyMessage(&ref, sourceWindow);
						else
							window->Reply(&ref, sourceWindow, type);
						window->Unlock();
					}
					sourceWindow->Unlock();
					break;
				}
			}
			if (window)
				window->Show();
			break;
		}

		case M_PREFS:
			if (fPrefsWindow)
				fPrefsWindow->Activate(true);
			else {
				fPrefsWindow = new TPrefsWindow(BRect(fPrefsWindowPos.x,
						fPrefsWindowPos.y, fPrefsWindowPos.x + PREF_WIDTH,
						fPrefsWindowPos.y + PREF_HEIGHT),
						&fContentFont, NULL, &fWrapMode, &fAttachAttributes,
						&fColoredQuotes, &fDefaultAccount, &fUseAccountFrom,
						&fReplyPreamble, &fSignature, &fMailCharacterSet,
						&fWarnAboutUnencodableCharacters,
						&fStartWithSpellCheckOn, &fAutoMarkRead,
						&fShowButtonBar);
				fPrefsWindow->Show();
			}
			break;

		case PREFS_CHANGED:
		{
			// Notify all Mail windows
			TMailWindow	*window;
			for (int32 i = 0; (window=(TMailWindow *)fWindowList.ItemAt(i))
				!= NULL; i++)
			{
				window->Lock();
				window->UpdatePreferences();
				window->UpdateViews();
				window->Unlock();
			}
			break;
		}

		case M_ACCOUNTS:
			be_roster->Launch("application/x-vnd.Haiku-Mail");
			break;

		case M_EDIT_SIGNATURE:
			if (fSigWindow)
				fSigWindow->Activate(true);
			else {
				fSigWindow = new TSignatureWindow(fSignatureWindowFrame);
				fSigWindow->Show();
			}
			break;

		case M_FONT:
			FontChange();
			break;

		case REFS_RECEIVED:
			if (msg->HasPointer("window")) {
				msg->FindPointer("window", (void **)&window);
				BMessage message(*msg);
				window->PostMessage(&message, window);
			}
			break;

		case WINDOW_CLOSED:
			switch (msg->FindInt32("kind")) {
				case MAIL_WINDOW:
				{
					TMailWindow	*window;
					if( msg->FindPointer( "window", (void **)&window ) == B_OK )
						fWindowList.RemoveItem(window);
					fWindowCount--;
					break;
				}

				case PREFS_WINDOW:
					fPrefsWindow = NULL;
					msg->FindPoint("window pos", &fPrefsWindowPos);
					break;

				case SIG_WINDOW:
					fSigWindow = NULL;
					msg->FindRect("window frame", &fSignatureWindowFrame);
					break;
			}

			if (!fWindowCount && !fSigWindow && !fPrefsWindow)
				be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		case B_PRINTER_CHANGED:
			_ClearPrintSettings();
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}


bool
TMailApp::QuitRequested()
{
	if (!BApplication::QuitRequested())
		return false;

    fMailWindowFrame = fLastMailWindowFrame;
    	// Last closed window becomes standard window size.

	// Shut down the spam server if it's still running. If the user has trained it on a message, it will stay
	// open. This is actually a good thing if there's quite a bit of spam -- no waiting for the thing to start
	// up for each message, but it has no business staying that way if the user isn't doing anything with e-mail. :)
	if (be_roster->IsRunning(kSpamServerSignature)) {
		team_id serverTeam = be_roster->TeamFor(kSpamServerSignature);
		if (serverTeam >= 0) {
			int32 errorCode = B_SERVER_NOT_FOUND;
			BMessenger messengerToSpamServer(kSpamServerSignature, serverTeam, &errorCode);
			if (messengerToSpamServer.IsValid()) {
				BMessage quitMessage(B_QUIT_REQUESTED);
				messengerToSpamServer.SendMessage(&quitMessage);
			}
		}

	}

	SaveSettings();
	return true;
}


void
TMailApp::ReadyToRun()
{
	// Create needed indices for META:group, META:email, MAIL:draft,
	// INDEX_SIGNATURE, INDEX_STATUS on the boot volume

	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);

	fs_create_index(volume.Device(), "META:group", B_STRING_TYPE, 0);
	fs_create_index(volume.Device(), "META:email", B_STRING_TYPE, 0);
	fs_create_index(volume.Device(), "MAIL:draft", B_INT32_TYPE, 0);
	fs_create_index(volume.Device(), INDEX_SIGNATURE, B_STRING_TYPE, 0);
	fs_create_index(volume.Device(), INDEX_STATUS, B_STRING_TYPE, 0);

	// Load dictionaries
	BPath indexDir;
	BPath dictionaryDir;
	BPath dataPath;
	BPath indexPath;
	BDirectory directory;
	BEntry entry;

	// Locate user settings directory
	find_directory(B_SYSTEM_DATA_DIRECTORY, &indexDir, true);
	indexDir.Append("spell_check");
	dictionaryDir = indexDir;

	// Setup directory paths
	indexDir.Append(kIndexDirectory);
	dictionaryDir.Append(kDictDirectory);

	// Create directories if needed
	directory.CreateDirectory(indexDir.Path(), NULL);
	directory.CreateDirectory(dictionaryDir.Path(), NULL);

	dataPath = dictionaryDir;
	dataPath.Append("words");

	// Only Load if Words Dictionary
	if (BEntry(kWordsPath).Exists() || BEntry(dataPath.Path()).Exists()) {
		// If "/boot/optional/goodies/words" exists but there is no
		// system dictionary, copy words
		if (!BEntry(dataPath.Path()).Exists() && BEntry(kWordsPath).Exists()) {
			BFile words(kWordsPath, B_READ_ONLY);
			BFile copy(dataPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
			char buffer[4096];
			ssize_t size;

			while ((size = words.Read( buffer, 4096)) > 0)
				copy.Write(buffer, size);
			BNodeInfo(&copy).SetType("text/plain");
		}

		// Create user dictionary if it does not exist
		dataPath = dictionaryDir;
		dataPath.Append("user");
		if (!BEntry(dataPath.Path()).Exists()) {
			BFile user(dataPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
			BNodeInfo(&user).SetType("text/plain");
		}

		// Load dictionaries
		directory.SetTo(dictionaryDir.Path());

		BString leafName;
		gUserDict = -1;

		while (gDictCount < MAX_DICTIONARIES
			&& directory.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			dataPath.SetTo(&entry);

			// Identify the user dictionary
			if (strcmp("user", dataPath.Leaf()) == 0) {
				gUserDictFile = new BFile(dataPath.Path(), B_WRITE_ONLY | B_OPEN_AT_END);
				gUserDict = gDictCount;
			}

			indexPath = indexDir;
			leafName.SetTo(dataPath.Leaf());
			leafName.Append(kMetaphone);
			indexPath.Append(leafName.String());
			gWords[gDictCount] = new Words(dataPath.Path(), indexPath.Path(), true);

			indexPath = indexDir;
			leafName.SetTo(dataPath.Leaf());
			leafName.Append(kExact);
			indexPath.Append(leafName.String());
			gExactWords[gDictCount] = new Words(dataPath.Path(), indexPath.Path(), false);
			gDictCount++;
		}
	}

	// Create a new window if starting up without any extra arguments.

	if (!fPrintHelpAndExit && !fWindowCount) {
		TMailWindow	*window;
		window = NewWindow();
		window->Show();
	}
}


void
TMailApp::RefsReceived(BMessage *msg)
{
	bool		have_names = false;
	BString		names;
	char		type[B_FILE_NAME_LENGTH];
	int32		item = 0;
	BFile		file;
	TMailWindow	*window;
	entry_ref	ref;

	//
	// If a tracker window opened me, get a messenger from it.
	//
	BMessenger messenger;
	if (msg->HasMessenger("TrackerViewToken"))
		msg->FindMessenger("TrackerViewToken", &messenger);

	while (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		if ((window = FindWindow(ref)) != NULL)
			window->Activate(true);
		else {
			file.SetTo(&ref, O_RDONLY);
			if (file.InitCheck() == B_NO_ERROR) {
				BNodeInfo	node(&file);
				node.GetType(type);
				if (strcmp(type, B_MAIL_TYPE) == 0
					|| strcmp(type, B_PARTIAL_MAIL_TYPE) == 0) {
					window = NewWindow(&ref, NULL, false, &messenger);
					window->Show();
				} else if(strcmp(type, "application/x-person") == 0) {
					/* Got a People contact info file, see if it has an Email address. */
					BString name;
					BString email;
					attr_info	info;
					char *attrib;

					if (file.GetAttrInfo("META:email", &info) == B_NO_ERROR) {
						attrib = (char *) malloc(info.size + 1);
						file.ReadAttr("META:email", B_STRING_TYPE, 0, attrib, info.size);
						attrib[info.size] = 0; // Just in case it wasn't NUL terminated.
						email << attrib;
						free(attrib);

						/* we got something... */
						if (email.Length() > 0) {
							/* see if we can get a username as well */
							if(file.GetAttrInfo("META:name", &info) == B_NO_ERROR) {
								attrib = (char *) malloc(info.size + 1);
								file.ReadAttr("META:name", B_STRING_TYPE, 0, attrib, info.size);
								attrib[info.size] = 0; // Just in case it wasn't NUL terminated.
								name << "\"" << attrib << "\" ";
								email.Prepend("<");
								email.Append(">");
								free(attrib);
							}

							if (names.Length() == 0) {
								names << name << email;
							} else {
								names << ", " << name << email;
							}
							have_names = true;
							email.SetTo("");
							name.SetTo("");
						}
					}
				}
				else if (strcmp(type, kDraftType) == 0) {
					window = NewWindow();

					// If it's a draft message, open it
					window->OpenMessage(&ref);
					window->Show();
				}
			} /* end of else(file.InitCheck() == B_NO_ERROR */
		}
	}

	if (have_names) {
		window = NewWindow(NULL, names.String());
		window->Show();
	}
}


TMailWindow *
TMailApp::FindWindow(const entry_ref &ref)
{
	BEntry entry(&ref);
	if (entry.InitCheck() < B_OK)
		return NULL;

	node_ref nodeRef;
	if (entry.GetNodeRef(&nodeRef) < B_OK)
		return NULL;

	BWindow	*window;
	int32 index = 0;
	while ((window = WindowAt(index++)) != NULL) {
		TMailWindow *mailWindow = dynamic_cast<TMailWindow *>(window);
		if (mailWindow == NULL)
			continue;

		node_ref mailNodeRef;
		if (mailWindow->GetMailNodeRef(mailNodeRef) == B_OK
			&& mailNodeRef == nodeRef)
			return mailWindow;
	}

	return NULL;
}


void
TMailApp::_CheckForSpamFilterExistence()
{
	// Looks at the filter settings to see if the user is using a spam filter.
	// If there is one there, set fShowSpamGUI to TRUE, otherwise to FALSE.

	int32 addonNameIndex;
	const char *addonNamePntr;
	BDirectory inChainDir;
	BPath path;
	BEntry settingsEntry;
	BFile settingsFile;
	BMessage settingsMessage;

	fShowSpamGUI = false;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
	// TODO use new settings
	path.Append("Mail/chains/inbound");
	if (inChainDir.SetTo(path.Path()) != B_OK)
		return;

	while (inChainDir.GetNextEntry (&settingsEntry, true /* traverse */) == B_OK) {
		if (!settingsEntry.IsFile())
			continue;
		if (settingsFile.SetTo (&settingsEntry, B_READ_ONLY) != B_OK)
			continue;
		if (settingsMessage.Unflatten (&settingsFile) != B_OK)
			continue;
		for (addonNameIndex = 0; B_OK == settingsMessage.FindString (
			"filter_addons", addonNameIndex, &addonNamePntr);
			addonNameIndex++) {
			if (strstr (addonNamePntr, "Spam Filter") != NULL) {
				fShowSpamGUI = true; // Found it!
				return;
			}
		}
	}
}


void
TMailApp::SetPrintSettings(const BMessage* printSettings)
{
	BAutolock _(this);

	if (printSettings == fPrintSettings)
		return;

	delete fPrintSettings;
	if (printSettings)
		fPrintSettings = new BMessage(*printSettings);
	else
		fPrintSettings = NULL;
}


bool
TMailApp::HasPrintSettings()
{
	BAutolock _(this);
	return fPrintSettings != NULL;
}


BMessage
TMailApp::PrintSettings()
{
	BAutolock _(this);
	return BMessage(*fPrintSettings);
}


void
TMailApp::_ClearPrintSettings()
{
	delete fPrintSettings;
	fPrintSettings = NULL;
}


void
TMailApp::SetLastWindowFrame(BRect frame)
{
	BAutolock _(this);
	fLastMailWindowFrame = frame;
}


status_t
TMailApp::GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("Mail");
	return create_directory(path.Path(), 0755);
}


status_t
TMailApp::LoadOldSettings()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("Mail_data");

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	file.Read(&fMailWindowFrame, sizeof(BRect));
//	file.Read(&level, sizeof(level));

	font_family	fontFamily;
	font_style	fontStyle;
	float size;
	file.Read(&fontFamily, sizeof(font_family));
	file.Read(&fontStyle, sizeof(font_style));
	file.Read(&size, sizeof(float));
	if (size >= 9)
		fContentFont.SetSize(size);

	if (fontFamily[0] && fontStyle[0])
		fContentFont.SetFamilyAndStyle(fontFamily, fontStyle);

	file.Read(&fSignatureWindowFrame, sizeof(BRect));
	file.Seek(1, SEEK_CUR);	// ignore (bool) show header
	file.Read(&fWrapMode, sizeof(bool));
	file.Read(&fPrefsWindowPos, sizeof(BPoint));

	int32 length;
	if (file.Read(&length, sizeof(int32)) < (ssize_t)sizeof(int32))
		return B_IO_ERROR;

	free(fSignature);
	fSignature = NULL;

	if (length > 0) {
		fSignature = (char *)malloc(length);
		if (fSignature == NULL)
			return B_NO_MEMORY;

		file.Read(fSignature, length);
	}

	file.Read(&fMailCharacterSet, sizeof(int32));
	if (fMailCharacterSet != B_MAIL_UTF8_CONVERSION
		&& fMailCharacterSet != B_MAIL_US_ASCII_CONVERSION
		&& BCharacterSetRoster::GetCharacterSetByConversionID(fMailCharacterSet) == NULL)
		fMailCharacterSet = B_MS_WINDOWS_CONVERSION;

	if (file.Read(&length, sizeof(int32)) == (ssize_t)sizeof(int32)) {
		char *findString = (char *)malloc(length + 1);
		if (findString == NULL)
			return B_NO_MEMORY;

		file.Read(findString, length);
		findString[length] = '\0';
		FindWindow::SetFindString(findString);
		free(findString);
	}
	if (file.Read(&fShowButtonBar, sizeof(uint8)) < (ssize_t)sizeof(uint8))
		fShowButtonBar = true;
	if (file.Read(&fUseAccountFrom, sizeof(int32)) < (ssize_t)sizeof(int32)
		|| fUseAccountFrom < ACCOUNT_USE_DEFAULT
		|| fUseAccountFrom > ACCOUNT_FROM_MAIL)
		fUseAccountFrom = ACCOUNT_USE_DEFAULT;
	if (file.Read(&fColoredQuotes, sizeof(bool)) < (ssize_t)sizeof(bool))
		fColoredQuotes = true;

	if (file.Read(&length, sizeof(int32)) == (ssize_t)sizeof(int32)) {
		free(fReplyPreamble);
		fReplyPreamble = (char *)malloc(length + 1);
		if (fReplyPreamble == NULL)
			return B_NO_MEMORY;

		file.Read(fReplyPreamble, length);
		fReplyPreamble[length] = '\0';
	}

	file.Read(&fAttachAttributes, sizeof(bool));
	file.Read(&fWarnAboutUnencodableCharacters, sizeof(bool));

	return B_OK;
}


status_t
TMailApp::SaveSettings()
{
	BMailSettings accountSettings;

	if (fDefaultAccount != ~0L) {
		accountSettings.SetDefaultOutboundAccount(fDefaultAccount);
		accountSettings.Save();
	}

	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	path.Append("BeMail Settings~");

	BFile file;
	status = file.SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings('BeMl');
	settings.AddRect("MailWindowSize", fMailWindowFrame);
//	settings.AddInt32("ExperienceLevel", level);

	font_family fontFamily;
	font_style fontStyle;
	fContentFont.GetFamilyAndStyle(&fontFamily, &fontStyle);

	settings.AddString("FontFamily", fontFamily);
	settings.AddString("FontStyle", fontStyle);
	settings.AddFloat("FontSize", fContentFont.Size());

	settings.AddRect("SignatureWindowSize", fSignatureWindowFrame);
	settings.AddBool("WordWrapMode", fWrapMode);
	settings.AddPoint("PreferencesWindowLocation", fPrefsWindowPos);
	settings.AddBool("AutoMarkRead", fAutoMarkRead);
	settings.AddString("SignatureText", fSignature);
	settings.AddInt32("CharacterSet", fMailCharacterSet);
	settings.AddString("FindString", FindWindow::GetFindString());
	settings.AddInt8("ShowButtonBar", fShowButtonBar);
	settings.AddInt32("UseAccountFrom", fUseAccountFrom);
	settings.AddBool("ColoredQuotes", fColoredQuotes);
	settings.AddString("ReplyPreamble", fReplyPreamble);
	settings.AddBool("AttachAttributes", fAttachAttributes);
	settings.AddBool("WarnAboutUnencodableCharacters", fWarnAboutUnencodableCharacters);
	settings.AddBool("StartWithSpellCheck", fStartWithSpellCheckOn);

	BEntry entry;
	status = entry.SetTo(path.Path());
	if (status != B_OK)
		return status;

	status = settings.Flatten(&file);
	if (status == B_OK) {
		// replace original settings file
		status = entry.Rename("BeMail Settings", true);
	} else
		entry.Remove();

	return status;
}


status_t
TMailApp::LoadSettings()
{
	BMailSettings accountSettings;
	fDefaultAccount = accountSettings.DefaultOutboundAccount();

	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	path.Append("BeMail Settings");

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return LoadOldSettings();

	BMessage settings;
	status = settings.Unflatten(&file);
	if (status < B_OK || settings.what != 'BeMl') {
		// the current settings are corrupted, try old ones
		return LoadOldSettings();
	}

	BRect rect;
	if (settings.FindRect("MailWindowSize", &rect) == B_OK)
		fMailWindowFrame = rect;

	int32 int32Value;
//	if (settings.FindInt32("ExperienceLevel", &int32Value) == B_OK)
//		level = int32Value;

	const char *fontFamily;
	if (settings.FindString("FontFamily", &fontFamily) == B_OK) {
		const char *fontStyle;
		if (settings.FindString("FontStyle", &fontStyle) == B_OK) {
			float size;
			if (settings.FindFloat("FontSize", &size) == B_OK) {
				if (size >= 7)
					fContentFont.SetSize(size);

				if (fontFamily[0] && fontStyle[0]) {
					fContentFont.SetFamilyAndStyle(fontFamily[0] ? fontFamily : NULL,
						fontStyle[0] ? fontStyle : NULL);
				}
			}
		}
	}

	if (settings.FindRect("SignatureWindowSize", &rect) == B_OK)
		fSignatureWindowFrame = rect;

	bool boolValue;
	if (settings.FindBool("WordWrapMode", &boolValue) == B_OK)
		fWrapMode = boolValue;

	BPoint point;
	if (settings.FindPoint("PreferencesWindowLocation", &point) == B_OK)
		fPrefsWindowPos = point;

	if (settings.FindBool("AutoMarkRead", &boolValue) == B_OK)
		fAutoMarkRead = boolValue;

	const char *string;
	if (settings.FindString("SignatureText", &string) == B_OK) {
		free(fSignature);
		fSignature = strdup(string);
	}

	if (settings.FindInt32("CharacterSet", &int32Value) == B_OK)
		fMailCharacterSet = int32Value;
	if (fMailCharacterSet != B_MAIL_UTF8_CONVERSION
		&& fMailCharacterSet != B_MAIL_US_ASCII_CONVERSION
		&& BCharacterSetRoster::GetCharacterSetByConversionID(fMailCharacterSet) == NULL)
		fMailCharacterSet = B_MS_WINDOWS_CONVERSION;

	if (settings.FindString("FindString", &string) == B_OK)
		FindWindow::SetFindString(string);

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
		free(fReplyPreamble);
		fReplyPreamble = strdup(string);
	}

	if (settings.FindBool("AttachAttributes", &boolValue) == B_OK)
		fAttachAttributes = boolValue;

	if (settings.FindBool("WarnAboutUnencodableCharacters", &boolValue) == B_OK)
		fWarnAboutUnencodableCharacters = boolValue;

	if (settings.FindBool("StartWithSpellCheck", &boolValue) == B_OK)
		fStartWithSpellCheckOn = boolValue;

	return B_OK;
}


void
TMailApp::FontChange()
{
	int32		index = 0;
	BMessage	msg;
	BWindow		*window;

	msg.what = CHANGE_FONT;
	msg.AddPointer("font", &fContentFont);

	for (;;) {
		window = WindowAt(index++);
		if (!window)
			break;

		window->PostMessage(&msg);
	}
}


TMailWindow*
TMailApp::NewWindow(const entry_ref* ref, const char* to, bool resend,
	BMessenger* trackerMessenger)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect screenFrame = screen.Frame();

	BRect r;
	if (fMailWindowFrame.Width() < 64 || fMailWindowFrame.Height() < 20) {
		// default size
		r.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH,
			TITLE_BAR_HEIGHT + WIND_HEIGHT);
	} else
		r = fMailWindowFrame;

	// make sure the window is not larger than the screen space
	if (r.Height() > screenFrame.Height())
		r.bottom = r.top + screenFrame.Height();
	if (r.Width() > screenFrame.Width())
		r.bottom = r.top + screenFrame.Width();

	// cascading windows
	r.OffsetBy(((fWindowCount + 5) % 10) * 15 - 75,
		((fWindowCount + 5) % 10) * 15 - 75);

	// make sure the window is still on screen
	if (r.left - 6 < screenFrame.left)
		r.OffsetTo(screenFrame.left + 8, r.top);

	if (r.left + 20 > screenFrame.right)
		r.OffsetTo(6, r.top);

	if (r.top - 26 < screenFrame.top)
		r.OffsetTo(r.left, screenFrame.top + 26);

	if (r.top + 20 > screenFrame.bottom)
		r.OffsetTo(r.left, TITLE_BAR_HEIGHT);

	if (r.Width() < WIND_WIDTH)
		r.right = r.left + WIND_WIDTH;

	fWindowCount++;

	BString title;
	BFile file;
	if (!resend && ref && file.SetTo(ref, O_RDONLY) == B_OK) {
		BString name;
		if (file.ReadAttrString(B_MAIL_ATTR_NAME, &name) == B_OK) {
			title << name;
			BString subject;
			if (file.ReadAttrString(B_MAIL_ATTR_SUBJECT, &subject) == B_OK)
				title << " -> " << subject;
		}
	}
	if (title == "")
		title = B_TRANSLATE_SYSTEM_NAME("Mail");

	TMailWindow* window = new TMailWindow(r, title.String(), this, ref, to,
		&fContentFont, resend, trackerMessenger);
	fWindowList.AddItem(window);

	return window;
}


// #pragma mark - settings


bool
TMailApp::AutoMarkRead()
{
	BAutolock _(this);
	return fAutoMarkRead;
}


BString
TMailApp::Signature()
{
	BAutolock _(this);
	return BString(fSignature);
}


BString
TMailApp::ReplyPreamble()
{
	BAutolock _(this);
	return BString(fReplyPreamble);
}


bool
TMailApp::WrapMode()
{
	BAutolock _(this);
	return fWrapMode;
}


bool
TMailApp::AttachAttributes()
{
	BAutolock _(this);
	return fAttachAttributes;
}


bool
TMailApp::ColoredQuotes()
{
	BAutolock _(this);
	return fColoredQuotes;
}


uint8
TMailApp::ShowButtonBar()
{
	BAutolock _(this);
	return fShowButtonBar;
}


bool
TMailApp::WarnAboutUnencodableCharacters()
{
	BAutolock _(this);
	return fWarnAboutUnencodableCharacters;
}


bool
TMailApp::StartWithSpellCheckOn()
{
	BAutolock _(this);
	return fStartWithSpellCheckOn;
}


void
TMailApp::SetDefaultAccount(int32 account)
{
	BAutolock _(this);
	fDefaultAccount = account;
}


int32
TMailApp::DefaultAccount()
{
	BAutolock _(this);
	return fDefaultAccount;
}


int32
TMailApp::UseAccountFrom()
{
	BAutolock _(this);
	return fUseAccountFrom;
}


uint32
TMailApp::MailCharacterSet()
{
	BAutolock _(this);
	return fMailCharacterSet;
}


BFont
TMailApp::ContentFont()
{
	BAutolock _(this);
	return fContentFont;
}


//	#pragma mark -


int
main()
{
	tzset();

	TMailApp().Run();
	return B_OK;
}

