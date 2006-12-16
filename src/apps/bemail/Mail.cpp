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
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//--------------------------------------------------------------------
//
//	Mail.cpp
//
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <Clipboard.h>
#include <E-mail.h>
#include <InterfaceKit.h>
#include <Roster.h>
#include <Screen.h>
#include <StorageKit.h>
#include <String.h>
#include <UTF8.h>
#include <Debug.h>
#include <Autolock.h>

#include <fs_index.h>
#include <fs_info.h>

#include <MailMessage.h>
#include <MailSettings.h>
#include <MailDaemon.h>
#include <mail_util.h>
#include <MDRLanguage.h>

#include <CharacterSetRoster.h>

using namespace BPrivate ;

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	#include <netdb.h>
#endif

#include "Mail.h"
#include "Header.h"
#include "Content.h"
#include "Enclosures.h"
#include "Prefs.h"
#include "Signature.h"
#include "Status.h"
#include "String.h"
#include "FindWindow.h"
#include "Utilities.h"
#include "ButtonBar.h"
#include "QueryMenu.h"
#include "FieldMsg.h"
#include "Words.h"


const char *kUndoStrings[] = {
	MDR_DIALECT_CHOICE ("Undo","Z) 取り消し"),
	MDR_DIALECT_CHOICE ("Undo Typing","Z) 取り消し（入力）"),
	MDR_DIALECT_CHOICE ("Undo Cut","Z) 取り消し（切り取り）"),
	MDR_DIALECT_CHOICE ("Undo Paste","Z) 取り消し（貼り付け）"),
	MDR_DIALECT_CHOICE ("Undo Clear","Z) 取り消し（消去）"),
	MDR_DIALECT_CHOICE ("Undo Drop","Z) 取り消し（ドロップ）")
};

const char *kRedoStrings[] = {
	MDR_DIALECT_CHOICE ("Redo", "Z) やり直し"),
	MDR_DIALECT_CHOICE ("Redo Typing", "Z) やり直し（入力）"),
	MDR_DIALECT_CHOICE ("Redo Cut", "Z) やり直し（切り取り）"),
	MDR_DIALECT_CHOICE ("Redo Paste", "Z) やり直し（貼り付け）"),
	MDR_DIALECT_CHOICE ("Redo Clear", "Z) やり直し（消去）"),
	MDR_DIALECT_CHOICE ("Redo Drop", "Z) やり直し（ドロップ）")
};

// Spam related globals.
static bool			 gShowSpamGUI = true;
static BMessenger	 gMessengerToSpamServer;
static const char	*kSpamServerSignature = "application/x-vnd.agmsmith.spamdbm";

static const char *kDraftPath = "mail/draft";
static const char *kDraftType = "text/x-vnd.Be-MailDraft";
static const char *kMailFolder = "mail";
static const char *kMailboxFolder = "mail/mailbox";

static const char *kDictDirectory = "word_dictionary";
static const char *kIndexDirectory = "word_index";
static const char *kWordsPath = "/boot/optional/goodies/words";
static const char *kExact = ".exact";
static const char *kMetaphone = ".metaphone";

/*
	// No longer needed if the popup is disabled
// Text for both the main menu and the pop-up menu.
static const char *kSpamMenuItemTextArray[] = {
	"Mark as Spam and Move to Trash",		// M_TRAIN_SPAM_AND_DELETE
	"Mark as Spam",							// M_TRAIN_SPAM
	"Unmark this Message",					// M_UNTRAIN
	"Mark as Genuine"						// M_TRAIN_GENUINE
};
*/

// global variables
bool		gHelpOnly = false;
bool		header_flag = false;
static bool	sWrapMode = true;
bool		attachAttributes_mode = true;
bool		gColoredQuotes = true;
static uint8 sShowButtonBar = true;
char		*gReplyPreamble;
char		*signature;
// int32		level = L_BEGINNER;
entry_ref	open_dir;
BMessage	*print_settings = NULL;
BPoint		prefs_window;
BRect		signature_window;
BRect		mail_window;
BRect		last_window;
uint32		gMailCharacterSet = B_MS_WINDOWS_CONVERSION;
bool		gWarnAboutUnencodableCharacters = true;
Words 		*gWords[MAX_DICTIONARIES], *gExactWords[MAX_DICTIONARIES];
int32 		gUserDict;
BFile 		*gUserDictFile;
int32 		gDictCount = 0;
bool		gStartWithSpellCheckOn = false;
uint32		gDefaultChain;
int32		gUseAccountFrom;

// static list for tracking of Windows
BList TMailWindow::sWindowList;
BLocker TMailWindow::sWindowListLock;


//====================================================================

int
main()
{
	TMailApp().Run();
	return B_NO_ERROR;
}


int32
header_len(BFile *file)
{
	char	*buffer;
	int32	length;
	int32	result = 0;
	off_t	size;

	if (file->ReadAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &result, sizeof(int32)) != sizeof(int32))
	{
		file->GetSize(&size);
		buffer = (char *)malloc(size);
		if (buffer)
		{
			file->Seek(0, 0);
			if (file->Read(buffer, size) == size)
			{
				while ((length = linelen(buffer + result, size - result, true)) > 2)
					result += length;

				result += length;
			}
			free(buffer);
			file->WriteAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &result, sizeof(int32));
		}
	}
	return result;
}


//	#pragma mark -


TMailApp::TMailApp()
	:	BApplication("application/x-vnd.Be-MAIL"),
	fFont(*be_plain_font),
	fWindowCount(0),
	fPrefsWindow(NULL),
	fSigWindow(NULL)
{
	// set default values
	fFont.SetSize(FONT_SIZE);
	signature = (char *)malloc(strlen(SIG_NONE) + 1);
	strcpy(signature, SIG_NONE);
	gReplyPreamble = (char *)malloc(1);
	gReplyPreamble[0] = '\0';

	mail_window.Set(0, 0, 0, 0);
	signature_window.Set(6, TITLE_BAR_HEIGHT, 6 + kSigWidth, TITLE_BAR_HEIGHT + kSigHeight);
	prefs_window.Set(6, TITLE_BAR_HEIGHT);

	// Find and read settings file.
	LoadSettings();

	CheckForSpamFilterExistence();
	fFont.SetSpacing(B_BITMAP_SPACING);
	last_window = mail_window;
}


TMailApp::~TMailApp()
{
}


void
TMailApp::AboutRequested()
{
	(new BAlert("",
		"BeMail\nBy Robert Polic\n\n"
		"Enhanced by Axel Dörfler and the Dr. Zoidberg crew\n\n"
		"Compiled on " __DATE__ " at " __TIME__ ".",
		"OK"))->Go();
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
	// an empty message even when BeMail is already running)
	bool gotmailto = false;

	for (int32 loop = 1; loop < argc; loop++)
	{
		if (strcmp(argv[loop], "-h") == 0
			|| strcmp(argv[loop], "--help") == 0)
		{
			printf(" usage: %s [ mailto:<address> ] [ -subject \"<text>\" ] [ ccto:<address> ] [ bccto:<address> ] "
				"[ -body \"<body text\" ] [ enclosure:<path> ] [ <message to read> ...] \n",
				argv[0]);
			gHelpOnly = true;
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
	entry_ref	ref;

	switch (msg->what)
	{
		case M_NEW:
		{
			int32 type;
			msg->FindInt32("type", &type);
			switch (type)
			{
				case M_NEW:
					window = NewWindow();
					break;

				case M_RESEND:
				{
					msg->FindRef("ref", &ref);
					BNode file(&ref);
					BString string = "";

					if (file.InitCheck() == B_OK)
						ReadAttrString(&file, B_MAIL_ATTR_TO, &string);

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
			else
			{
				fPrefsWindow = new TPrefsWindow(BRect(prefs_window.x,
						prefs_window.y, prefs_window.x + PREF_WIDTH,
						prefs_window.y + PREF_HEIGHT),
						&fFont, NULL, &sWrapMode, &attachAttributes_mode,
						&gColoredQuotes, &gDefaultChain, &gUseAccountFrom,
						&gReplyPreamble, &signature, &gMailCharacterSet,
						&gWarnAboutUnencodableCharacters,
						&gStartWithSpellCheckOn, &sShowButtonBar);
				fPrefsWindow->Show();
				fPreviousShowButtonBar = sShowButtonBar;
			}
			break;

		case PREFS_CHANGED:
		{
			// Do we need to update the state of the button bars?
			if (fPreviousShowButtonBar != sShowButtonBar) {
				// Notify all BeMail windows
				TMailWindow	*window;
				for (int32 i = 0; (window=(TMailWindow *)fWindowList.ItemAt(i)) != NULL; i++) {
					window->Lock();
					window->UpdateViews();
					window->Unlock();
				}
				fPreviousShowButtonBar = sShowButtonBar;
			}
			break;
		}

		case M_EDIT_SIGNATURE:
			if (fSigWindow)
				fSigWindow->Activate(true);
			else {
				fSigWindow = new TSignatureWindow(signature_window);
				fSigWindow->Show();
			}
			break;

		case M_FONT:
			FontChange();
			break;

/*		case M_BEGINNER:
		case M_EXPERT:
			level = msg->what - M_BEGINNER;
			break;
*/
		case REFS_RECEIVED:
			if (msg->HasPointer("window"))
			{
				msg->FindPointer("window", (void **)&window);
				BMessage message(*msg);
				window->PostMessage(&message, window);
			}
			break;

		case WINDOW_CLOSED:
			switch (msg->FindInt32("kind"))
			{
				case MAIL_WINDOW:
				{
					TMailWindow	*window;
					if( msg->FindPointer( "window", (void **)&window ) == B_OK )
						fWindowList.RemoveItem( window );
					fWindowCount--;
					break;
				}

				case PREFS_WINDOW:
					fPrefsWindow = NULL;
					break;

				case SIG_WINDOW:
					fSigWindow = NULL;
					break;
			}

			if (!fWindowCount && !fSigWindow && !fPrefsWindow)
				be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		case B_PRINTER_CHANGED:
			ClearPrintSettings();
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

    mail_window = last_window;
    	// Last closed window becomes standard window size.

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
	find_directory(B_BEOS_ETC_DIRECTORY, &indexDir, true);
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
	if (BEntry(kWordsPath).Exists() || BEntry(dataPath.Path()).Exists())
	{
		// If "/boot/optional/goodies/words" exists but there is no system dictionary, copy words
		if (!BEntry(dataPath.Path()).Exists() && BEntry(kWordsPath).Exists())
		{
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
		if (!BEntry(dataPath.Path()).Exists())
		{
			BFile user(dataPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
			BNodeInfo(&user).SetType("text/plain");
		}

		// Load dictionaries
		directory.SetTo(dictionaryDir.Path());

		BString leafName;
		gUserDict = -1;

		while (gDictCount < MAX_DICTIONARIES
			&& directory.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND)
		{
			dataPath.SetTo(&entry);

			// Identify the user dictionary
			if (strcmp("user", dataPath.Leaf()) == 0)
			{
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

	if (!gHelpOnly && !fWindowCount)
	{
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
				if (!strcmp(type, B_MAIL_TYPE)) {
					window = NewWindow(&ref, NULL, false, &messenger);
					window->Show();
				} else if(!strcmp(type, "application/x-person")) {
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
				else if (!strcmp(type, kDraftType))
				{
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
TMailApp::CheckForSpamFilterExistence()
{
	// Looks at the filter settings to see if the user is using a spam filter.
	// If there is one there, set gShowSpamGUI to TRUE, otherwise to FALSE.

	int32		addonNameIndex;
	const char *addonNamePntr;
	BDirectory	inChainDir;
	BPath		path;
	BEntry		settingsEntry;
	BFile		settingsFile;
	BMessage	settingsMessage;

	gShowSpamGUI = false;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
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
				gShowSpamGUI = true; // Found it!
				return;
			}
		}
	}
}


void
TMailApp::ClearPrintSettings()
{
	delete print_settings;
	print_settings = NULL;
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

	file.Read(&mail_window, sizeof(BRect));
//	file.Read(&level, sizeof(level));

	font_family	fontFamily;
	font_style	fontStyle;
	float size;
	file.Read(&fontFamily, sizeof(font_family));
	file.Read(&fontStyle, sizeof(font_style));
	file.Read(&size, sizeof(float));
	if (size >= 9)
		fFont.SetSize(size);

	if (fontFamily[0] && fontStyle[0])
		fFont.SetFamilyAndStyle(fontFamily, fontStyle);

	file.Read(&signature_window, sizeof(BRect));
	file.Read(&header_flag, sizeof(bool));
	file.Read(&sWrapMode, sizeof(bool));
	file.Read(&prefs_window, sizeof(BPoint));

	int32 length;
	if (file.Read(&length, sizeof(int32)) < (ssize_t)sizeof(int32))
		return B_IO_ERROR;

	free(signature);
	signature = NULL;

	if (length > 0) {
		signature = (char *)malloc(length);
		if (signature == NULL)
			return B_NO_MEMORY;

		file.Read(signature, length);
	}

	file.Read(&gMailCharacterSet, sizeof(int32));
	if (gMailCharacterSet != B_MAIL_UTF8_CONVERSION
		&& gMailCharacterSet != B_MAIL_US_ASCII_CONVERSION
		&& BCharacterSetRoster::GetCharacterSetByConversionID(gMailCharacterSet) == NULL)
		gMailCharacterSet = B_MS_WINDOWS_CONVERSION;

	if (file.Read(&length, sizeof(int32)) == (ssize_t)sizeof(int32)) {
		char *findString = (char *)malloc(length + 1);
		if (findString == NULL)
			return B_NO_MEMORY;

		file.Read(findString, length);
		findString[length] = '\0';
		FindWindow::SetFindString(findString);
		free(findString);
	}
	if (file.Read(&sShowButtonBar, sizeof(uint8)) < (ssize_t)sizeof(uint8))
		sShowButtonBar = true;
	if (file.Read(&gUseAccountFrom, sizeof(int32)) < (ssize_t)sizeof(int32)
		|| gUseAccountFrom < ACCOUNT_USE_DEFAULT
		|| gUseAccountFrom > ACCOUNT_FROM_MAIL)
		gUseAccountFrom = ACCOUNT_USE_DEFAULT;
	if (file.Read(&gColoredQuotes, sizeof(bool)) < (ssize_t)sizeof(bool))
		gColoredQuotes = true;

	if (file.Read(&length, sizeof(int32)) == (ssize_t)sizeof(int32)) {
		free(gReplyPreamble);
		gReplyPreamble = (char *)malloc(length + 1);
		if (gReplyPreamble == NULL)
			return B_NO_MEMORY;

		file.Read(gReplyPreamble, length);
		gReplyPreamble[length] = '\0';
	}

	file.Read(&attachAttributes_mode, sizeof(bool));
	file.Read(&gWarnAboutUnencodableCharacters, sizeof(bool));

	return B_OK;
}


status_t
TMailApp::SaveSettings()
{
	BMailSettings chainSettings;

	if (gDefaultChain != ~0UL) {
		chainSettings.SetDefaultOutboundChainID(gDefaultChain);
		chainSettings.Save();
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
	settings.AddRect("MailWindowSize", mail_window);
//	settings.AddInt32("ExperienceLevel", level);

	font_family fontFamily;
	font_style fontStyle;
	fFont.GetFamilyAndStyle(&fontFamily, &fontStyle);

	settings.AddString("FontFamily", fontFamily);
	settings.AddString("FontStyle", fontStyle);
	settings.AddFloat("FontSize", fFont.Size());

	settings.AddRect("SignatureWindowSize", signature_window);
	settings.AddBool("ShowHeadersMode", header_flag);
	settings.AddBool("WordWrapMode", sWrapMode);
	settings.AddPoint("PreferencesWindowLocation", prefs_window);
	settings.AddString("SignatureText", signature);
	settings.AddInt32("CharacterSet", gMailCharacterSet);
	settings.AddString("FindString", FindWindow::GetFindString());
	settings.AddInt8("ShowButtonBar", sShowButtonBar);
	settings.AddInt32("UseAccountFrom", gUseAccountFrom);
	settings.AddBool("ColoredQuotes", gColoredQuotes);
	settings.AddString("ReplyPreamble", gReplyPreamble);
	settings.AddBool("AttachAttributes", attachAttributes_mode);
	settings.AddBool("WarnAboutUnencodableCharacters", gWarnAboutUnencodableCharacters);
	settings.AddBool("StartWithSpellCheck", gStartWithSpellCheckOn);

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
	BMailSettings chainSettings;
	gDefaultChain = chainSettings.DefaultOutboundChainID();

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
		mail_window = rect;

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
					fFont.SetSize(size);

				if (fontFamily[0] && fontStyle[0]) {
					fFont.SetFamilyAndStyle(fontFamily[0] ? fontFamily : NULL,
						fontStyle[0] ? fontStyle : NULL);
				}
			}
		}
	}

	if (settings.FindRect("SignatureWindowSize", &rect) == B_OK)
		signature_window = rect;

	bool boolValue;
	if (settings.FindBool("ShowHeadersMode", &boolValue) == B_OK)
		header_flag = boolValue;

	if (settings.FindBool("WordWrapMode", &boolValue) == B_OK)
		sWrapMode = boolValue;

	BPoint point;
	if (settings.FindPoint("PreferencesWindowLocation", &point) == B_OK)
		prefs_window = point;

	const char *string;
	if (settings.FindString("SignatureText", &string) == B_OK) {
		free(signature);
		signature = strdup(string);
	}

	if (settings.FindInt32("CharacterSet", &int32Value) == B_OK)
		gMailCharacterSet = int32Value;
	if (gMailCharacterSet != B_MAIL_UTF8_CONVERSION
		&& gMailCharacterSet != B_MAIL_US_ASCII_CONVERSION
		&& BCharacterSetRoster::GetCharacterSetByConversionID(gMailCharacterSet) == NULL)
		gMailCharacterSet = B_MS_WINDOWS_CONVERSION;

	if (settings.FindString("FindString", &string) == B_OK)
		FindWindow::SetFindString(string);

	int8 int8Value;
	if (settings.FindInt8("ShowButtonBar", &int8Value) == B_OK)
		sShowButtonBar = int8Value;

	if (settings.FindInt32("UseAccountFrom", &int32Value) == B_OK)
		gUseAccountFrom = int32Value;
	if (gUseAccountFrom < ACCOUNT_USE_DEFAULT
		|| gUseAccountFrom > ACCOUNT_FROM_MAIL)
		gUseAccountFrom = ACCOUNT_USE_DEFAULT;

	if (settings.FindBool("ColoredQuotes", &boolValue) == B_OK)
		gColoredQuotes = boolValue;

	if (settings.FindString("ReplyPreamble", &string) == B_OK) {
		free(gReplyPreamble);
		gReplyPreamble = strdup(string);
	}

	if (settings.FindBool("AttachAttributes", &boolValue) == B_OK)
		attachAttributes_mode = boolValue;

	if (settings.FindBool("WarnAboutUnencodableCharacters", &boolValue) == B_OK)
		gWarnAboutUnencodableCharacters = boolValue;

	if (settings.FindBool("StartWithSpellCheck", &boolValue) == B_OK)
		gStartWithSpellCheckOn = boolValue;

	return B_OK;
}


void
TMailApp::FontChange()
{
	int32		index = 0;
	BMessage	msg;
	BWindow		*window;

	msg.what = CHANGE_FONT;
	msg.AddPointer("font", &fFont);

	for (;;) {
		window = WindowAt(index++);
		if (!window)
			break;

		window->PostMessage(&msg);
	}
}


TMailWindow *
TMailApp::NewWindow(const entry_ref *ref, const char *to, bool resend,
	BMessenger *trackerMessenger)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect screen_frame = screen.Frame();

	BRect r;
	if ((mail_window.Width() > 1) && (mail_window.Height() > 1))
		r = mail_window;
	else
		r.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);

	r.OffsetBy(fWindowCount * 20, fWindowCount * 20);

	if ((r.left - 6) < screen_frame.left)
		r.OffsetTo(screen_frame.left + 8, r.top);

	if ((r.left + 20) > screen_frame.right)
		r.OffsetTo(6, r.top);

	if ((r.top - 26) < screen_frame.top)
		r.OffsetTo(r.left, screen_frame.top + 26);

	if ((r.top + 20) > screen_frame.bottom)
		r.OffsetTo(r.left, TITLE_BAR_HEIGHT);

	if (r.Width() < WIND_WIDTH)
		r.right = r.left + WIND_WIDTH;

	fWindowCount++;

	BString title;
	BFile file;
	if (!resend && ref && file.SetTo(ref, O_RDONLY) == B_NO_ERROR) {
		BString name;
		if (ReadAttrString(&file, B_MAIL_ATTR_NAME, &name) == B_NO_ERROR) {
			title << name;
			BString subject;
			if (ReadAttrString(&file, B_MAIL_ATTR_SUBJECT, &subject) == B_NO_ERROR)
				title << " -> " << subject;
		}
	}
	if (title == "")
		title = "BeMail";

	TMailWindow *window = new TMailWindow(r, title.String(), ref, to, &fFont, resend,
								trackerMessenger);
	fWindowList.AddItem(window);

	return window;
}


//====================================================================
//	#pragma mark -


TMailWindow::TMailWindow(BRect rect, const char *title, const entry_ref *ref, const char *to,
						 const BFont *font, bool resending, BMessenger *messenger)
		:	BWindow(rect, title, B_DOCUMENT_WINDOW, 0),
		fFieldState(0),
		fPanel(NULL),
		fSendButton(NULL),
		fSaveButton(NULL),
		fPrintButton(NULL),
		fSigButton(NULL),
		fZoom(rect),
		fEnclosuresView(NULL),
		fPrevTrackerPositionSaved(false),
		fNextTrackerPositionSaved(false),
		fSigAdded(false),
		fReplying(false),
		fResending(resending),
		fSent(false),
		fDraft(false),
		fChanged(false),
		fStartingText(NULL),
		fOriginatingWindow(NULL)
{
	if (messenger != NULL)
		fTrackerMessenger = *messenger;

	char		str[256];
	char		status[272];
	uint32		message;
	float		height;
	BMenu		*menu;
	BMenu		*subMenu;
	BMenuBar	*menu_bar;
	BMenuItem	*item;
	BMessage	*msg;
	attr_info	info;
	BFile file(ref, B_READ_ONLY);

	if (ref) {
		fRef = new entry_ref(*ref);
		fMail = new BEmailMessage(fRef);
		fIncoming = true;
	} else {
		fRef = NULL;
		fMail = NULL;
		fIncoming = false;
	}

	BRect r(0, 0, RIGHT_BOUNDARY, 15);

	// Create real menu bar
	fMenuBar = menu_bar = new BMenuBar(r, "");
	
	// Program Menu
	
	menu = new BMenu(MDR_DIALECT_CHOICE ("Program","TRANSLATE ME"));
	menu->AddItem(item = new BMenuItem(
		MDR_DIALECT_CHOICE ("About BeMail", "A) BeMailについて") B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(
		MDR_DIALECT_CHOICE ("Options","O) BeMailの設定") B_UTF8_ELLIPSIS,
		new BMessage(M_PREFS),','));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(
		MDR_DIALECT_CHOICE ("Signatures","S) 署名の編集") B_UTF8_ELLIPSIS,
		new BMessage(M_EDIT_SIGNATURE)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(
		MDR_DIALECT_CHOICE ("Quit", "Q) 終了"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	menu_bar->AddItem(menu);
	
	//
	//	File Menu
	//
	menu = new BMenu(MDR_DIALECT_CHOICE ("File","F) ファイル"));

	msg = new BMessage(M_NEW);
	msg->AddInt32("type", M_NEW);
	menu->AddItem(item = new BMenuItem(MDR_DIALECT_CHOICE (
		"New Mail Message", "N) 新規メッセージ作成"), msg, 'N'));
	item->SetTarget(be_app);

	// Cheap hack - only show the drafts menu when composing messages.  Insert
	// a "true || " in the following IF statement if you want the old BeMail
	// behaviour.  The difference is that without live draft menu updating you
	// can open around 100 e-mails (the BeOS maximum number of open files)
	// rather than merely around 20, since each open draft-monitoring query
	// sucks up one file handle per mounted BFS disk volume.  Plus mail file
	// opening speed is noticably improved!  ToDo: change this to populate the
	// Draft menu with the file names on demand - when the user clicks on it;
	// don't need a live query since the menu isn't staying up for more than a
	// few seconds.

	if (!fIncoming) {
		QueryMenu *queryMenu;
		queryMenu = new QueryMenu(MDR_DIALECT_CHOICE ("Open Draft", "O) ドラフトを開く"), false);
		queryMenu->SetTargetForItems(be_app);

		queryMenu->SetPredicate("MAIL:draft==1");
		menu->AddItem(queryMenu);
	}
	
	if(!fIncoming || resending) {
		menu->AddItem(fSendLater = new BMenuItem(
			MDR_DIALECT_CHOICE ("Save as Draft", "S)ドラフトとして保存"),
			new BMessage(M_SAVE_AS_DRAFT), 'S'));
	}
	
	menu->AddSeparatorItem();
	menu->AddItem(fPrint = new BMenuItem(
		MDR_DIALECT_CHOICE ("Page Setup", "G) ページ設定") B_UTF8_ELLIPSIS,
		new BMessage(M_PRINT_SETUP)));
	menu->AddItem(fPrint = new BMenuItem(
		MDR_DIALECT_CHOICE ("Print", "P) 印刷") B_UTF8_ELLIPSIS,
		new BMessage(M_PRINT), 'P'));
	menu_bar->AddItem(menu);
	
	if(!resending && fIncoming) {	
		menu->AddSeparatorItem();
		
		subMenu = new BMenu(MDR_DIALECT_CHOICE ("Close","C) 閉じる"));
		if (file.GetAttrInfo(B_MAIL_ATTR_STATUS, &info) == B_NO_ERROR)
			file.ReadAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, str, info.size);
		else
			str[0] = 0;

		//if( (strcmp(str, "Pending")==0)||(strcmp(str, "Sent")==0) )
		//	canResend = true;

		if (!strcmp(str, "New")) {
			subMenu->AddItem(item = new BMenuItem(
				MDR_DIALECT_CHOICE ("Leave as New", "N) 新規<New>のままにする"),
				new BMessage(M_CLOSE_SAME), 'W', B_SHIFT_KEY));
			subMenu->AddItem(item = new BMenuItem(
				MDR_DIALECT_CHOICE ("Set to Read", "R) 開封済<Read>に設定"),
				new BMessage(M_CLOSE_READ), 'W'));
			message = M_CLOSE_READ;
		} else {
			if (strlen(str))
				sprintf(status, MDR_DIALECT_CHOICE ("Leave as '%s'","W) 属性を<%s>にする"), str);
			else
				sprintf(status, MDR_DIALECT_CHOICE ("Leave same","W) 属性はそのまま"));
			subMenu->AddItem(item = new BMenuItem(status,
							new BMessage(M_CLOSE_SAME), 'W'));
			message = M_CLOSE_SAME;
			AddShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(M_CLOSE_SAME));
		}

		subMenu->AddItem(new BMenuItem(
			MDR_DIALECT_CHOICE ("Set to Saved", "S) 属性を<Saved>に設定"),
			new BMessage(M_CLOSE_SAVED), 'W', B_CONTROL_KEY));
		subMenu->AddItem(new BMenuItem(new TMenu(
			MDR_DIALECT_CHOICE ("Set to", "X) 他の属性に変更")B_UTF8_ELLIPSIS,
			INDEX_STATUS, M_STATUS, false, false), new BMessage(M_CLOSE_CUSTOM)));
		menu->AddItem(subMenu);
	}
	else
	{
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(
			MDR_DIALECT_CHOICE ("Close", "W) 閉じる"),
			new BMessage(B_CLOSE_REQUESTED), 'W'));
	}

	//
	//	Edit Menu
	//
	menu = new BMenu(MDR_DIALECT_CHOICE ("Edit","E) 編集"));
	menu->AddItem(fUndo = new BMenuItem(MDR_DIALECT_CHOICE ("Undo","Z) 元に戻す"), new BMessage(B_UNDO), 'Z', 0));
	fUndo->SetTarget(NULL, this);
	menu->AddItem(fRedo = new BMenuItem(MDR_DIALECT_CHOICE ("Redo","Z) やり直し"), new BMessage(M_REDO), 'Z', B_SHIFT_KEY));
	fRedo->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem(MDR_DIALECT_CHOICE ("Cut","X) 切り取り"), new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem(MDR_DIALECT_CHOICE ("Copy","C) コピー"), new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem(MDR_DIALECT_CHOICE ("Paste","V) 貼り付け"), new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(MDR_DIALECT_CHOICE ("Select All", "A) 全文選択"), new BMessage(M_SELECT), 'A'));
	menu->AddSeparatorItem();
	item->SetTarget(NULL, this);
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Find", "F) 検索") B_UTF8_ELLIPSIS, new BMessage(M_FIND), 'F'));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Find Again", "G) 次を検索"), new BMessage(M_FIND_AGAIN), 'G'));
	if (!fIncoming)
	{
		menu->AddSeparatorItem();
		menu->AddItem(fQuote =new BMenuItem(
			MDR_DIALECT_CHOICE ("Quote","Q) 引用符をつける"),
			new BMessage(M_QUOTE), B_RIGHT_ARROW));
		menu->AddItem(fRemoveQuote = new BMenuItem(
			MDR_DIALECT_CHOICE ("Remove Quote","R) 引用符を削除"),
			new BMessage(M_REMOVE_QUOTE), B_LEFT_ARROW));
		menu->AddSeparatorItem();
		fSpelling = new BMenuItem(
			MDR_DIALECT_CHOICE ("Check Spelling","H) スペルチェック"),
			new BMessage( M_CHECK_SPELLING ), ';' );
		menu->AddItem(fSpelling);
		if (gStartWithSpellCheckOn)
			PostMessage (M_CHECK_SPELLING);
	}
	menu_bar->AddItem(menu);
	
	// View Menu
	
	if (!resending && fIncoming) {
		menu = new BMenu("View");
		menu->AddItem(fHeader = new BMenuItem(MDR_DIALECT_CHOICE ("Show Header","H) ヘッダーを表示"),	new BMessage(M_HEADER), 'H'));
		if (header_flag)
			fHeader->SetMarked(true);
		menu->AddItem(fRaw = new BMenuItem(MDR_DIALECT_CHOICE ("Show Raw Message","   メッセージを生で表示"), new BMessage(M_RAW)));
		menu_bar->AddItem(menu);
	}
	
	//
	//	Message Menu
	//
	menu = new BMenu(MDR_DIALECT_CHOICE ("Message", "M) メッセージ"));

	if (!resending && fIncoming) {
		BMenuItem *menuItem;
		menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply","R) 返信"), new BMessage(M_REPLY),'R'));
		menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply to Sender","S) 送信者に返信"), new BMessage(M_REPLY_TO_SENDER),'R',B_OPTION_KEY));
		menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply to All","P) 全員に返信"), new BMessage(M_REPLY_ALL), 'R', B_SHIFT_KEY));

		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Forward","J) 転送"), new BMessage(M_FORWARD), 'J'));
		menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Forward without Attachments","The opposite: F) 添付ファイルを含めて転送"), new BMessage(M_FORWARD_WITHOUT_ATTACHMENTS)));
		menu->AddItem(menuItem = new BMenuItem(MDR_DIALECT_CHOICE ("Resend","   再送信"), new BMessage(M_RESEND)));
		menu->AddItem(menuItem = new BMenuItem(MDR_DIALECT_CHOICE ("Copy to New","D) 新規メッセージへコピー"), new BMessage(M_COPY_TO_NEW), 'D'));

		menu->AddSeparatorItem();
		fDeleteNext = new BMenuItem(MDR_DIALECT_CHOICE ("Move to Trash","T) 削除"), new BMessage(M_DELETE_NEXT), 'T');
		menu->AddItem(fDeleteNext);
		menu->AddSeparatorItem();

		fPrevMsg = new BMenuItem(MDR_DIALECT_CHOICE ("Previous Message","B) 前のメッセージ"), new BMessage(M_PREVMSG),
		 B_UP_ARROW);
		menu->AddItem(fPrevMsg);
		fNextMsg = new BMenuItem(MDR_DIALECT_CHOICE ("Next Message","N) 次のメッセージ"), new BMessage(M_NEXTMSG),
		  B_DOWN_ARROW);
		menu->AddItem(fNextMsg);
		menu->AddSeparatorItem();
		fSaveAddrMenu = subMenu = new BMenu(MDR_DIALECT_CHOICE ("Save Address", "   アドレスを保存"));

		// create the list of addresses

		BList addressList;
		get_address_list(addressList, fMail->To(), extract_address);
		get_address_list(addressList, fMail->CC(), extract_address);
		get_address_list(addressList, fMail->From(), extract_address);
		get_address_list(addressList, fMail->ReplyTo(), extract_address);

		for (int32 i = addressList.CountItems(); i-- > 0;) {
			char *address = (char *)addressList.RemoveItem(0L);

			// insert the new address in alphabetical order
			int32 index = 0;
			while ((item = subMenu->ItemAt(index)) != NULL) {
				if (!strcmp(address, item->Label())) {
					// item already in list
					goto skip;
				}

				if (strcmp(address, item->Label()) < 0)
					break;

				index++;
			}

			msg = new BMessage(M_SAVE);
			msg->AddString("address", address);
			subMenu->AddItem(new BMenuItem(address, msg), index);

		skip:
			free(address);
		}

		menu->AddItem(subMenu);
		menu_bar->AddItem(menu);

		// Spam Menu
		
		if (gShowSpamGUI) {
			menu = new BMenu("Spam Filtering");
			menu->AddItem(new BMenuItem("Mark as Spam and Move to Trash",
										new BMessage(M_TRAIN_SPAM_AND_DELETE), 'K'));
			menu->AddItem(new BMenuItem("Mark as Spam",
										new BMessage(M_TRAIN_SPAM), 'K', B_OPTION_KEY));
			menu->AddSeparatorItem();
			menu->AddItem(new BMenuItem("Unmark this Message",
										new BMessage(M_UNTRAIN)));
			menu->AddSeparatorItem();
			menu->AddItem(new BMenuItem("Mark as Genuine",
										new BMessage(M_TRAIN_GENUINE), 'K', B_SHIFT_KEY));
			menu_bar->AddItem(menu);
		}
	} else {
		menu->AddItem(fSendNow = new BMenuItem(
			MDR_DIALECT_CHOICE ("Send Message", "M) メッセージを送信"),
			new BMessage(M_SEND_NOW), 'M'));
		
		if(!fIncoming) {
			menu->AddSeparatorItem();
			fSignature = new TMenu(
				MDR_DIALECT_CHOICE ("Add Signature", "D) 署名を追加"),
				INDEX_SIGNATURE, M_SIGNATURE);
			menu->AddItem(new BMenuItem(fSignature));
			menu->AddSeparatorItem();
			menu->AddItem(fAdd = new BMenuItem(MDR_DIALECT_CHOICE ("Add Enclosure","E) 追加")B_UTF8_ELLIPSIS, new BMessage(M_ADD), 'E'));
			menu->AddItem(fRemove = new BMenuItem(MDR_DIALECT_CHOICE ("Remove Enclosure","T) 削除"), new BMessage(M_REMOVE), 'T'));
		}
		menu_bar->AddItem(menu);
	}
	
	Lock();
	AddChild(menu_bar);
	height = menu_bar->Bounds().bottom + 1;
	Unlock();

	//
	// Button Bar
	//
	float bbwidth = 0, bbheight = 0;

	if (sShowButtonBar) {
		BuildButtonBar();
		fButtonBar->ShowLabels(sShowButtonBar == 1);
		fButtonBar->Arrange(/* True for all buttons same size, false to just fit */
			MDR_DIALECT_CHOICE (true, true));
		fButtonBar->GetPreferredSize(&bbwidth, &bbheight);
		fButtonBar->ResizeTo(Bounds().right+3, bbheight+1);
		fButtonBar->MoveTo(-1, height-1);
		fButtonBar->Show();
	} else
		fButtonBar = NULL;

	r.top = r.bottom = height + bbheight + 1;
	fHeaderView = new THeaderView (r, rect, fIncoming, fMail, resending,
		(resending || !fIncoming)
		? gMailCharacterSet // Use preferences setting for composing mail.
		: B_MAIL_NULL_CONVERSION); // Default is automatic selection for reading mail.

	r = Frame();
	r.OffsetTo(0, 0);
	r.top = fHeaderView->Frame().bottom - 1;
	fContentView = new TContentView(r, fIncoming, fMail, const_cast<BFont *>(font));
		// TContentView needs to be properly const, for now cast away constness

	Lock();
	AddChild(fHeaderView);
	if (fEnclosuresView)
		AddChild(fEnclosuresView);
	AddChild(fContentView);
	Unlock();

	if (to) {
		Lock();
		fHeaderView->fTo->SetText(to);
		Unlock();
	}

	SetSizeLimits(WIND_WIDTH, RIGHT_BOUNDARY,
				  fHeaderView->Bounds().Height() + ENCLOSURES_HEIGHT + height + 60,
				  RIGHT_BOUNDARY);

	AddShortcut('n', B_COMMAND_KEY, new BMessage(M_NEW));

	//
	// 	If auto-signature, add signature to the text here.
	//

	if (!fIncoming && strcmp(signature, SIG_NONE) != 0) {
		if (strcmp(signature, SIG_RANDOM) == 0)
			PostMessage(M_RANDOM_SIG);
		else {
			//
			//	Create a query to find this signature
			//
			BVolume volume;
			BVolumeRoster().GetBootVolume(&volume);

			BQuery query;
			query.SetVolume(&volume);
			query.PushAttr(INDEX_SIGNATURE);
			query.PushString(signature);
			query.PushOp(B_EQ);
			query.Fetch();

			//
			//	If we find the named query, add it to the text.
			//
			BEntry entry;
			if (query.GetNextEntry(&entry) == B_NO_ERROR) {
				off_t size;
				BFile file;
				file.SetTo(&entry, O_RDWR);
				if (file.InitCheck() == B_NO_ERROR) {
					file.GetSize(&size);
					char *str = (char *)malloc(size);
					size = file.Read(str, size);

					fContentView->fTextView->Insert(str, size);
					fContentView->fTextView->GoToLine(0);
					fContentView->fTextView->ScrollToSelection();

					fStartingText = (char *)malloc(size = strlen(fContentView->fTextView->Text()) + 1);
					if (fStartingText != NULL)
						strcpy(fStartingText, fContentView->fTextView->Text());
				}
			} else {
				char tempString [2048];
				query.GetPredicate (tempString, sizeof (tempString));
				printf ("Query failed, was looking for: %s\n", tempString);
			}
		}
	}

	if (fRef)
		SetTitleForMessage();
}


void
TMailWindow::BuildButtonBar()
{
	ButtonBar *bbar;

	bbar = new ButtonBar(BRect(0, 0, 100, 100), "ButtonBar", 2, 3, 0, 1, 10, 2);
	bbar->AddButton(MDR_DIALECT_CHOICE ("New","新規"), 28, new BMessage(M_NEW));
	bbar->AddDivider(5);

	if (fResending)
	{
		fSendButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Send","送信"), 8, new BMessage(M_SEND_NOW));
		bbar->AddDivider(5);
	}
	else if (!fIncoming)
	{
		fSendButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Send","送信"), 8, new BMessage(M_SEND_NOW));
		fSendButton->SetEnabled(false);
		fSigButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Signature","署名"), 4, new BMessage(M_SIG_MENU));
		fSigButton->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		fSaveButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Save","保存"), 44, new BMessage(M_SAVE_AS_DRAFT));
		fSaveButton->SetEnabled(false);
		bbar->AddDivider(5);
		fPrintButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Print","印刷"), 16, new BMessage(M_PRINT));
		fPrintButton->SetEnabled(false);
		bbar->AddButton(MDR_DIALECT_CHOICE ("Trash","削除"), 0, new BMessage(M_DELETE));
	}
	else
	{
		BmapButton *button = bbar->AddButton(MDR_DIALECT_CHOICE ("Reply","返信"), 12, new BMessage(M_REPLY));
		button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		button = bbar->AddButton(MDR_DIALECT_CHOICE ("Forward","転送"), 40, new BMessage(M_FORWARD));
		button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		fPrintButton = bbar->AddButton(MDR_DIALECT_CHOICE ("Print","印刷"), 16, new BMessage(M_PRINT));
		bbar->AddButton(MDR_DIALECT_CHOICE ("Trash","削除"), 0, new BMessage(M_DELETE_NEXT));
		if (gShowSpamGUI) {
			button = bbar->AddButton("Spam", 48, new BMessage(M_SPAM_BUTTON));
			button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		}
		bbar->AddDivider(5);
		bbar->AddButton(MDR_DIALECT_CHOICE ("Next","次へ"), 24, new BMessage(M_NEXTMSG));
		bbar->AddButton(MDR_DIALECT_CHOICE ("Previous","前へ"), 20, new BMessage(M_PREVMSG));
	}
	bbar->AddButton(MDR_DIALECT_CHOICE ("Inbox","受信箱"), 36, new BMessage(M_OPEN_MAIL_BOX));
	bbar->AddButton(MDR_DIALECT_CHOICE ("Mail","メール"), 32, new BMessage(M_OPEN_MAIL_FOLDER));
	bbar->AddDivider(5);

	bbar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	bbar->Hide();
	AddChild(bbar);
	fButtonBar = bbar;
}


void
TMailWindow::UpdateViews()
{
	float bbwidth = 0, bbheight = 0;
	float nextY = fMenuBar->Frame().bottom+1;

	// Show/Hide Button Bar
	if (sShowButtonBar) {
		// Create the Button Bar if needed
		if (!fButtonBar)
			BuildButtonBar();
		fButtonBar->ShowLabels(sShowButtonBar == 1);
		fButtonBar->Arrange(/* True for all buttons same size, false to just fit */
			MDR_DIALECT_CHOICE (true, true));
		fButtonBar->GetPreferredSize( &bbwidth, &bbheight);
		fButtonBar->ResizeTo(Bounds().right+3, bbheight+1);
		fButtonBar->MoveTo(-1, nextY-1);
		nextY += bbheight + 1;
		if (fButtonBar->IsHidden())
			fButtonBar->Show();
		else
			fButtonBar->Invalidate();
	} else if (fButtonBar)
		fButtonBar->Hide();

	// Arange other views to match
	fHeaderView->MoveTo(0, nextY);
	nextY = fHeaderView->Frame().bottom;
	if (fEnclosuresView) {
		fEnclosuresView->MoveTo(0, nextY);
		nextY = fEnclosuresView->Frame().bottom+1;
	}
	BRect bounds(Bounds());
	fContentView->MoveTo(0, nextY-1);
	fContentView->ResizeTo(bounds.right-bounds.left, bounds.bottom-nextY+1);
}


TMailWindow::~TMailWindow()
{
	delete fMail;
	last_window = Frame();
	delete fPanel;
	delete fOriginatingWindow;

	BAutolock locker(sWindowListLock);
	sWindowList.RemoveItem(this);
}


status_t
TMailWindow::GetMailNodeRef(node_ref &nodeRef) const
{
	if (fRef == NULL)
		return B_ERROR;

	BNode node(fRef);
	return node.GetNodeRef(&nodeRef);
}


bool
TMailWindow::GetTrackerWindowFile(entry_ref *ref, bool next) const
{
	// Position was already saved
	if (next && fNextTrackerPositionSaved)
	{
		*ref = fNextRef;
		return true;
	}
	if (!next && fPrevTrackerPositionSaved)
	{
		*ref = fPrevRef;
		return true;
	}

	if (!fTrackerMessenger.IsValid())
		return false;

	//
	//	Ask the tracker what the next/prev file in the window is.
	//	Continue asking for the next reference until a valid
	//	email file is found (ignoring other types).
	//
	entry_ref nextRef = *ref;
	bool foundRef = false;
	while (!foundRef)
	{
		BMessage request(B_GET_PROPERTY);
		BMessage spc;
		if (next)
			spc.what = 'snxt';
		else
			spc.what = 'sprv';

		spc.AddString("property", "Entry");
		spc.AddRef("data", &nextRef);

		request.AddSpecifier(&spc);
		BMessage reply;
		if (fTrackerMessenger.SendMessage(&request, &reply) != B_OK)
			return false;

		if (reply.FindRef("result", &nextRef) != B_OK)
			return false;

		char fileType[256];
		BNode node(&nextRef);
		if (node.InitCheck() != B_OK)
			return false;

		if (BNodeInfo(&node).GetType(fileType) != B_OK)
			return false;

		if (strcasecmp(fileType,"text/x-email") == 0)
			foundRef = true;
	}

	*ref = nextRef;
	return foundRef;
}


void
TMailWindow::SaveTrackerPosition(entry_ref *ref)
{
	// if only one of them is saved, we're not going to do it again
	if (fNextTrackerPositionSaved || fPrevTrackerPositionSaved)
		return;

	fNextRef = fPrevRef = *ref;

	fNextTrackerPositionSaved = GetTrackerWindowFile(&fNextRef, true);
	fPrevTrackerPositionSaved = GetTrackerWindowFile(&fPrevRef, false);
}


void
TMailWindow::SetOriginatingWindow(BWindow *window)
{
	delete fOriginatingWindow;
	fOriginatingWindow = new BMessenger(window);
}


void
TMailWindow::SetTrackerSelectionToCurrent()
{
	BMessage setSelection(B_SET_PROPERTY);
	setSelection.AddSpecifier("Selection");
	setSelection.AddRef("data", fRef);

	fTrackerMessenger.SendMessage(&setSelection);
}


void
TMailWindow::SetCurrentMessageRead()
{
	BNode node(fRef);
	if (node.InitCheck() == B_NO_ERROR)
	{
		BString status;
		if (ReadAttrString(&node, B_MAIL_ATTR_STATUS, &status) == B_NO_ERROR
			&& !status.ICompare("New"))
		{
			node.RemoveAttr(B_MAIL_ATTR_STATUS);
			WriteAttrString(&node, B_MAIL_ATTR_STATUS, "Read");
		}
	}
}


void
TMailWindow::FrameResized(float width, float height)
{
	fContentView->FrameResized(width, height);
}


void
TMailWindow::MenusBeginning()
{
	bool		enable;
	int32		finish = 0;
	int32		start = 0;
	BTextView	*textView;

	if (!fIncoming)
	{
		enable = strlen(fHeaderView->fTo->Text()) ||
				 strlen(fHeaderView->fBcc->Text());
		fSendNow->SetEnabled(enable);
		fSendLater->SetEnabled(enable);

		be_clipboard->Lock();
		fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE) &&
						   ((fEnclosuresView == NULL) || !fEnclosuresView->fList->IsFocus()));
		be_clipboard->Unlock();

		fQuote->SetEnabled(false);
		fRemoveQuote->SetEnabled(false);

		fAdd->SetEnabled(true);
		fRemove->SetEnabled((fEnclosuresView != NULL) &&
							(fEnclosuresView->fList->CurrentSelection() >= 0));
	}
	else
	{
		if (fResending)
		{
			enable = strlen(fHeaderView->fTo->Text());
			fSendNow->SetEnabled(enable);
			// fSendLater->SetEnabled(enable);

			if (fHeaderView->fTo->HasFocus())
			{
				textView = fHeaderView->fTo->TextView();
				textView->GetSelection(&start, &finish);

				fCut->SetEnabled(start != finish);
				be_clipboard->Lock();
				fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
				be_clipboard->Unlock();
			}
			else
			{
				fCut->SetEnabled(false);
				fPaste->SetEnabled(false);
			}
		}
		else
		{
			fCut->SetEnabled(false);
			fPaste->SetEnabled(false);

			if (!fTrackerMessenger.IsValid()) {
				fNextMsg->SetEnabled(false);
				fPrevMsg->SetEnabled(false);
			}
		}
	}

	fPrint->SetEnabled(fContentView->fTextView->TextLength());

	textView = dynamic_cast<BTextView *>(CurrentFocus());
	if ((NULL != textView) && (dynamic_cast<TTextControl *>(textView->Parent()) != NULL))
	{
		// one of To:, Subject:, Account:, Cc:, Bcc:
		textView->GetSelection(&start, &finish);
	}
	else if (fContentView->fTextView->IsFocus())
	{
		fContentView->fTextView->GetSelection(&start, &finish);
		if (!fIncoming)
		{
			fQuote->SetEnabled(true);
			fRemoveQuote->SetEnabled(true);
		}
	}

	fCopy->SetEnabled(start != finish);
	if (!fIncoming)
		fCut->SetEnabled(start != finish);

	// Undo stuff
	bool isRedo = false;
	undo_state	undoState = B_UNDO_UNAVAILABLE;

	BTextView *focusTextView = dynamic_cast<BTextView *>(CurrentFocus());
	if (focusTextView != NULL)
		undoState = focusTextView->UndoState(&isRedo);

//	fUndo->SetLabel((isRedo) ? kRedoStrings[undoState] : kUndoStrings[undoState]);
	fUndo->SetEnabled(undoState != B_UNDO_UNAVAILABLE);
}


void
TMailWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case FIELD_CHANGED:
		{
			int32 prevState = fFieldState, fieldMask = msg->FindInt32("bitmask");
			void *source;

			if (msg->FindPointer("source", &source) == B_OK)
			{
				int32 length;

				if (fieldMask == FIELD_BODY)
					length = ((TTextView *)source)->TextLength();
				else
					length = ((BComboBox *)source)->TextView()->TextLength();

				if (length)
					fFieldState |= fieldMask;
				else
					fFieldState &= ~fieldMask;
			}

			// Has anything changed?
			if (prevState != fFieldState || !fChanged)
			{
				// Change Buttons to reflect this
				if (fSaveButton)
					fSaveButton->SetEnabled(fFieldState);
				if (fPrintButton)
					fPrintButton->SetEnabled(fFieldState);
				if (fSendButton)
					fSendButton->SetEnabled((fFieldState & FIELD_TO) || (fFieldState & FIELD_BCC));
			}
			fChanged = true;

			// Update title bar if "subject" has changed
			if (!fIncoming && fieldMask & FIELD_SUBJECT)
			{
				// If no subject, set to "BeMail"
				if (!fHeaderView->fSubject->TextView()->TextLength())
					SetTitle("BeMail");
				else
					SetTitle(fHeaderView->fSubject->Text());
			}
			break;
		}
		case LIST_INVOKED:
			PostMessage(msg, fEnclosuresView);
			break;

		case CHANGE_FONT:
			PostMessage(msg, fContentView);
			break;

		case M_NEW:
		{
			BMessage message(M_NEW);
			message.AddInt32("type", msg->what);
			be_app->PostMessage(&message);
			break;
		}

		case M_SPAM_BUTTON:
		{
			/*
				A popup from a button is good only when the behavior has some consistency and
				there is some visual indication that a menu will be shown when clicked. A
				workable implementation would have an extra button attached to the main one
				which has a downward-pointing arrow. Mozilla Thunderbird's 'Get Mail' button
				is a good example of this.
				
				Until someone is willing to retool the toolbar class or otherwise do something
				like this, it is better to just disable the menu for consistency's sake.
			*/
/*			uint32 buttons;
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON)
			{
				BPopUpMenu menu("Spam Actions", false, false);
				for (int i = 0; i < 4; i++)
					menu.AddItem(new BMenuItem(kSpamMenuItemTextArray[i], new BMessage(M_TRAIN_SPAM_AND_DELETE + i)));

				BPoint where;
				msg->FindPoint("where", &where);
				BMenuItem *item;
				if ((item = menu.Go(where, false, false)) != NULL)
					PostMessage(item->Message());
				break;
			} else // Default action for left clicking on the spam button.
				PostMessage (new BMessage (M_TRAIN_SPAM_AND_DELETE));
*/
			PostMessage (new BMessage (M_TRAIN_SPAM_AND_DELETE));
			break;
		}

		case M_TRAIN_SPAM_AND_DELETE:
			PostMessage (M_DELETE_NEXT);
		case M_TRAIN_SPAM:
			TrainMessageAs ("Spam");
			break;

		case M_UNTRAIN:
			TrainMessageAs ("Uncertain");
			break;

		case M_TRAIN_GENUINE:
			TrainMessageAs ("Genuine");
			break;

		case M_REPLY:
		{
			// Disabled for usability reasons - see listing for spam button
/*			uint32 buttons;
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON)
			{
				BPopUpMenu menu("Reply To", false, false);
				menu.AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply","R) 返信"),new BMessage(M_REPLY)));
				menu.AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply to Sender","S) 送信者に返信"),new BMessage(M_REPLY_TO_SENDER)));
				menu.AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply to All","P) 全員に返信"),new BMessage(M_REPLY_ALL)));

				BPoint where;
				msg->FindPoint("where", &where);

				BMenuItem *item;
				if ((item = menu.Go(where, false, false)) != NULL)
				{
					item->SetTarget(this);
					PostMessage(item->Message());
				}
				break;
			}
*/
			// Fall through
		}
		case M_FORWARD:
		{
			// Disabled for usability reasons - see listing for spam button
/*			uint32 buttons;
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Forward", false, false);
				menu.AddItem(new BMenuItem(MDR_DIALECT_CHOICE("Forward", "J) 転送"),
					new BMessage(M_FORWARD)));
				menu.AddItem(new BMenuItem(MDR_DIALECT_CHOICE("Forward without Attachments",
					"The opposite: F) 添付ファイルを含む転送"),
					new BMessage(M_FORWARD_WITHOUT_ATTACHMENTS)));

				BPoint where;
				msg->FindPoint("where", &where);

				BMenuItem *item;
				if ((item = menu.Go(where, false, false)) != NULL) {
					item->SetTarget(this);
					PostMessage(item->Message());
				}
				break;
			}
*/
		}

		// Fall Through
		case M_REPLY_ALL:
		case M_REPLY_TO_SENDER:
		case M_FORWARD_WITHOUT_ATTACHMENTS:
		case M_RESEND:
		case M_COPY_TO_NEW:
		{
			BMessage message(M_NEW);
			message.AddRef("ref", fRef);
			message.AddPointer("window", this);
			message.AddInt32("type", msg->what);
			be_app->PostMessage(&message);
			break;
		}
		case M_DELETE:
		case M_DELETE_PREV:
		case M_DELETE_NEXT:
		{
			// The Trash already provides a level of Undo to prevent loss of data. This
			// alert is a lot like the infamous "Are you sure?" confirmation dialogs
			// from Windows 98
/*			if (level == L_BEGINNER)
			{
				beep();
				if (!(new BAlert("", MDR_DIALECT_CHOICE (
					"Are you sure you want to move this message to the trash?",
					"このメッセージを削除してもよろしいですか？"),
					MDR_DIALECT_CHOICE ("Cancel","中止"),
					MDR_DIALECT_CHOICE ("Trash","削除"),
					NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
					B_WARNING_ALERT))->Go())
					break;
			}
*/
			if (msg->what == M_DELETE_NEXT && (modifiers() & B_SHIFT_KEY))
				msg->what = M_DELETE_PREV;

			bool foundRef = false;
			entry_ref nextRef;
			if ((msg->what == M_DELETE_PREV || msg->what == M_DELETE_NEXT) && fRef)
			{
				//
				//	Find the next message that should be displayed
				//
				nextRef = *fRef;
				foundRef = GetTrackerWindowFile(&nextRef, msg->what ==
				  M_DELETE_NEXT);
			}
			if (fIncoming)
				SetCurrentMessageRead();

			if (!fTrackerMessenger.IsValid() || !fIncoming) {
				//
				//	Not associated with a tracker window.  Create a new
				//	messenger and ask the tracker to delete this entry
				//
				if (fDraft || fIncoming) {
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid()) {
						BMessage msg('Ttrs');
						msg.AddRef("refs", fRef);
						tracker.SendMessage(&msg);
					} else {
						(new BAlert("",
							MDR_DIALECT_CHOICE ( "Need tracker to move items to trash",
							"削除するにはTrackerが必要です。"),
							 MDR_DIALECT_CHOICE ("sorry","削除できませんでした。")))->Go();
					}
				}
			} else {
				//
				// This is associated with a tracker window.  Ask the
				// window to delete this entry.  Do it this way if we
				// can instead of the above way because it doesn't reset
				// the selection (even though we set selection below, this
				// still causes problems).
				//
				BMessage delmsg(B_DELETE_PROPERTY);
				BMessage entryspec('sref');
				entryspec.AddRef("refs", fRef);
				entryspec.AddString("property", "Entry");
				delmsg.AddSpecifier(&entryspec);
				fTrackerMessenger.SendMessage(&delmsg);
			}

			//
			// 	If the next file was found, open it.  If it was not,
			//	we have no choice but to close this window.
			//
			if (foundRef) {
				TMailWindow *window = static_cast<TMailApp *>(be_app)->FindWindow(nextRef);
				if (window == NULL)
					OpenMessage(&nextRef, fHeaderView->fCharacterSetUserSees);
				else
					window->Activate();

				SetTrackerSelectionToCurrent();

				if (window == NULL)
					break;
			}

			fSent = true;
			BMessage msg(B_CLOSE_REQUESTED);
			PostMessage(&msg);
			break;
		}

		case M_CLOSE_READ:
		{
			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", "Read");
			PostMessage(&message);
			break;
		}
		case M_CLOSE_SAVED:
		{
			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", "Saved");
			PostMessage(&message);
			break;
		}
		case M_CLOSE_SAME:
		{
			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", "");
			message.AddString("same", "");
			PostMessage(&message);
			break;
		}
		case M_CLOSE_CUSTOM:
			if (msg->HasString("status"))
			{
				const char *str;
				msg->FindString("status", (const char**) &str);
				BMessage message(B_CLOSE_REQUESTED);
				message.AddString("status", str);
				PostMessage(&message);
			}
			else
			{
				BRect r = Frame();
				r.left += ((r.Width() - STATUS_WIDTH) / 2);
				r.right = r.left + STATUS_WIDTH;
				r.top += 40;
				r.bottom = r.top + STATUS_HEIGHT;

				BString string = "could not read";
				BNode node(fRef);
				if (node.InitCheck() == B_OK)
					ReadAttrString(&node, B_MAIL_ATTR_STATUS, &string);

				new TStatusWindow(r, this, string.String());
			}
			break;

		case M_STATUS:
		{
			BMenuItem *menu;
			msg->FindPointer("source", (void **)&menu);
			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", menu->Label());
			PostMessage(&message);
			break;
		}
		case M_HEADER:
		{
			header_flag = !fHeader->IsMarked();
			fHeader->SetMarked(header_flag);

			BMessage message(M_HEADER);
			message.AddBool("header", header_flag);
			PostMessage(&message, fContentView->fTextView);
			break;
		}
		case M_RAW:
		{
			bool raw = !(fRaw->IsMarked());
			fRaw->SetMarked(raw);
			BMessage message(M_RAW);
			message.AddBool("raw", raw);
			PostMessage(&message, fContentView->fTextView);
			break;
		}
		case M_SEND_NOW:
		case M_SAVE_AS_DRAFT:
			Send(msg->what == M_SEND_NOW);
			break;

		case M_SAVE:
		{
			char *str;
			if (msg->FindString("address", (const char **)&str) == B_NO_ERROR)
			{
				char *arg = (char *)malloc(strlen("META:email ") + strlen(str) + 1);
				BVolumeRoster volumeRoster;
				BVolume volume;
				volumeRoster.GetBootVolume(&volume);

				BQuery query;
				query.SetVolume(&volume);
				sprintf(arg, "META:email=%s", str);
				query.SetPredicate(arg);
				query.Fetch();

				BEntry entry;
				if (query.GetNextEntry(&entry) == B_NO_ERROR)
				{
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid())
					{
						entry_ref ref;
						entry.GetRef(&ref);

						BMessage open(B_REFS_RECEIVED);
						open.AddRef("refs", &ref);
						tracker.SendMessage(&open);
					}
				}
				else
				{
					sprintf(arg, "META:email %s", str);
					status_t result = be_roster->Launch("application/x-person", 1, &arg);
					if (result != B_NO_ERROR)
						(new BAlert("",	MDR_DIALECT_CHOICE (
							"Sorry, could not find an application that supports the 'Person' data type.",
							"Peopleデータ形式をサポートするアプリケーションが見つかりませんでした。"),
							MDR_DIALECT_CHOICE ("OK","了解")))->Go();
				}
				free(arg);
			}
			break;
		}

		case M_PRINT_SETUP:
			PrintSetup();
			break;

		case M_PRINT:
			Print();
			break;

		case M_SELECT:
			break;

		case M_FIND:
			FindWindow::Find(this);
			break;

		case M_FIND_AGAIN:
			FindWindow::FindAgain(this);
			break;

		case M_QUOTE:
		case M_REMOVE_QUOTE:
			PostMessage(msg->what, fContentView);
			break;

		case M_RANDOM_SIG:
		{
			BList		sigList;
			BMessage	*message;

			BVolume volume;
			BVolumeRoster().GetBootVolume(&volume);

			BQuery query;
			query.SetVolume(&volume);

			char predicate[128];
			sprintf(predicate, "%s = *", INDEX_SIGNATURE);
			query.SetPredicate(predicate);
			query.Fetch();

			BEntry entry;
			while (query.GetNextEntry(&entry) == B_NO_ERROR)
			{
				BFile file(&entry, O_RDONLY);
				if (file.InitCheck() == B_NO_ERROR)
				{
					entry_ref ref;
					entry.GetRef(&ref);

					message = new BMessage(M_SIGNATURE);
					message->AddRef("ref", &ref);
					sigList.AddItem(message);
				}
			}
			if (sigList.CountItems() > 0)
			{
				srand(time(0));
				PostMessage((BMessage *)sigList.ItemAt(rand() % sigList.CountItems()));

				for (int32 i = 0; (message = (BMessage *)sigList.ItemAt(i)) != NULL; i++)
					delete message;
			}
			break;
		}
		case M_SIGNATURE:
		{
			BMessage message(*msg);
			PostMessage(&message, fContentView);
			fSigAdded = true;
			break;
		}
		case M_SIG_MENU:
		{
			TMenu *menu;
			BMenuItem *item;
			menu = new TMenu( "Add Signature", INDEX_SIGNATURE, M_SIGNATURE, true );

			BPoint	where;
			bool open_anyway = true;

			if (msg->FindPoint("where", &where) != B_OK)
			{
				BRect	bounds;
				bounds = fSigButton->Bounds();
				where = fSigButton->ConvertToScreen(BPoint((bounds.right-bounds.left)/2,
														   (bounds.bottom-bounds.top)/2));
			}
			else if (msg->FindInt32("buttons") == B_SECONDARY_MOUSE_BUTTON)
				open_anyway = false;

			if ((item = menu->Go(where, false, open_anyway)) != NULL)
			{
				item->SetTarget(this);
				(dynamic_cast<BInvoker *>(item))->Invoke();
			}
			delete menu;
			break;
		}

		case M_ADD:
			if (!fPanel)
			{
				BMessenger me(this);
				BMessage msg(REFS_RECEIVED);
				fPanel = new BFilePanel(B_OPEN_PANEL, &me, &open_dir, false, true, &msg);
			}
			else if (!fPanel->Window()->IsHidden())
				fPanel->Window()->Activate();

			if (fPanel->Window()->IsHidden())
				fPanel->Window()->Show();
			break;

		case M_REMOVE:
			PostMessage(msg->what, fEnclosuresView);
			break;

		case CHARSET_CHOICE_MADE:
			if (fIncoming && !fResending) {
				// The user wants to see the message they are reading (not
				// composing) displayed with a different kind of character set
				// for decoding.  Reload the whole message and redisplay.  For
				// messages which are being composed, the character set is
				// retrieved from the header view when it is needed.

				entry_ref fileRef = *fRef;
				int32 characterSet;
				msg->FindInt32("charset", &characterSet);
				OpenMessage(&fileRef, characterSet);
			}
			break;

		case REFS_RECEIVED:
			AddEnclosure(msg);
			break;

		//
		//	Navigation Messages
		//
		case M_PREVMSG:
		case M_NEXTMSG:
			if (fRef)
			{
				entry_ref nextRef = *fRef;
				if (GetTrackerWindowFile(&nextRef, (msg->what == M_NEXTMSG))) {
					TMailWindow *window = static_cast<TMailApp *>(be_app)->FindWindow(nextRef);
					if (window == NULL) {
						SetCurrentMessageRead();
						OpenMessage(&nextRef, fHeaderView->fCharacterSetUserSees);
					} else {
						window->Activate();

						//fSent = true;
						BMessage msg(B_CLOSE_REQUESTED);
						PostMessage(&msg);
					}

					SetTrackerSelectionToCurrent();
				}
				else
					beep();
			}
			break;
		case M_SAVE_POSITION:
			if (fRef)
				SaveTrackerPosition(fRef);
			break;

		case M_OPEN_MAIL_FOLDER:
		case M_OPEN_MAIL_BOX:
		{
			BEntry folderEntry;
			BPath path;
			// Get the user home directory
			if (find_directory(B_USER_DIRECTORY, &path) != B_OK)
				break;
			if (msg->what == M_OPEN_MAIL_FOLDER)
				path.Append(kMailFolder);
			else
				path.Append(kMailboxFolder);
			if (folderEntry.SetTo(path.Path()) == B_OK && folderEntry.Exists())
			{
				BMessage thePackage(B_REFS_RECEIVED);
				BMessenger tracker("application/x-vnd.Be-TRAK");

				entry_ref ref;
				folderEntry.GetRef(&ref);
				thePackage.AddRef("refs", &ref);
				tracker.SendMessage(&thePackage);
			}
			break;
		}
		case RESET_BUTTONS:
			fChanged = false;
			fFieldState = 0;
			if (fHeaderView->fTo->TextView()->TextLength())
				fFieldState |= FIELD_TO;
			if (fHeaderView->fSubject->TextView()->TextLength())
				fFieldState |= FIELD_SUBJECT;
			if (fHeaderView->fCc->TextView()->TextLength())
				fFieldState |= FIELD_CC;
			if (fHeaderView->fBcc->TextView()->TextLength())
				fFieldState |= FIELD_BCC;
			if (fContentView->fTextView->TextLength())
				fFieldState |= FIELD_BODY;

			if (fSaveButton)
				fSaveButton->SetEnabled(false);
			if (fPrintButton)
				fPrintButton->SetEnabled(fFieldState);
			if (fSendButton)
				fSendButton->SetEnabled((fFieldState & FIELD_TO) || (fFieldState & FIELD_BCC));
			break;

		case M_CHECK_SPELLING:
			if (gDictCount == 0)
				// Give the application time to initialise and load the dictionaries.
				snooze (1500000);
			if (!gDictCount)
			{
				beep();
				(new BAlert("",
					MDR_DIALECT_CHOICE (
					"The spell check feature requires the optional \"words\" file on your BeOS CD.",
					"スペルチェク機能はBeOS CDの optional \"words\" ファイルが必要です"),
					MDR_DIALECT_CHOICE ("OK","了解"),
					NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
					B_STOP_ALERT))->Go();
			}
			else
			{
				fSpelling->SetMarked(!fSpelling->IsMarked());
				fContentView->fTextView->EnableSpellCheck(fSpelling->IsMarked());
			}
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
TMailWindow::AddEnclosure(BMessage *msg)
{
	if (fEnclosuresView == NULL && !fIncoming)
	{
		BRect r;
		r.left = 0;
		r.top = fHeaderView->Frame().bottom - 1;
		r.right = Frame().Width() + 2;
		r.bottom = r.top + ENCLOSURES_HEIGHT;

		fEnclosuresView = new TEnclosuresView(r, Frame());
		AddChild(fEnclosuresView, fContentView);
		fContentView->ResizeBy(0, -ENCLOSURES_HEIGHT);
		fContentView->MoveBy(0, ENCLOSURES_HEIGHT);
	}

	if (fEnclosuresView == NULL)
		return;

	if (msg && msg->HasRef("refs"))
	{
		// Add enclosure to view
		PostMessage(msg, fEnclosuresView);

		fChanged = true;
		BEntry entry;
		entry_ref ref;
		msg->FindRef("refs", &ref);
		entry.SetTo(&ref);
		entry.GetParent(&entry);
		entry.GetRef(&open_dir);
	}
}


bool
TMailWindow::QuitRequested()
{
	int32 result;

	if ((!fIncoming || (fIncoming && fResending)) && fChanged && !fSent
		&& (strlen(fHeaderView->fTo->Text())
			|| strlen(fHeaderView->fSubject->Text())
			|| (fHeaderView->fCc && strlen(fHeaderView->fCc->Text()))
			|| (fHeaderView->fBcc && strlen(fHeaderView->fBcc->Text()))
			|| (strlen(fContentView->fTextView->Text()) && (!fStartingText || fStartingText && strcmp(fContentView->fTextView->Text(), fStartingText)))
			|| (fEnclosuresView != NULL && fEnclosuresView->fList->CountItems())))
	{
		if (fResending) {
			BAlert *alert = new BAlert("",
				MDR_DIALECT_CHOICE (
				"Do you wish to send this message before closing?",
				"閉じる前に送信しますか？"),
				MDR_DIALECT_CHOICE ("Discard","無視"),
				MDR_DIALECT_CHOICE ("Cancel","中止"),
				MDR_DIALECT_CHOICE ("Send","送信"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT);
			alert->SetShortcut(0,'d');
			alert->SetShortcut(1,B_ESCAPE);
			result = alert->Go();

			switch (result) {
				case 0:	// Discard
					break;
				case 1:	// Cancel
					return false;
				case 2:	// Send
					Send(true);
					break;
			}
		} else {
			BAlert *alert = new BAlert("",
				MDR_DIALECT_CHOICE (
				"Do you wish to save this message as a draft before closing?",
				"閉じる前に保存しますか？"),
				MDR_DIALECT_CHOICE ("Don't Save","保存しない"),
				MDR_DIALECT_CHOICE ("Cancel","中止"),
				MDR_DIALECT_CHOICE ("Save","保存"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT);
			alert->SetShortcut(0,'d');
			alert->SetShortcut(1,B_ESCAPE);
			result = alert->Go();
			switch (result) {
				case 0:	// Don't Save
					break;
				case 1:	// Cancel
					return false;
				case 2:	// Save
					Send(false);
					break;
			}
		}
	}

	BMessage message(WINDOW_CLOSED);
	message.AddInt32("kind", MAIL_WINDOW);
	message.AddPointer( "window", this );
	be_app->PostMessage(&message);

	if ((CurrentMessage()) && (CurrentMessage()->HasString("status"))) {
		//	User explicitly requests a status to set this message to.
		if (!CurrentMessage()->HasString("same")) {
			const char *status = CurrentMessage()->FindString("status");
			if (status != NULL) {
				BNode node(fRef);
				if (node.InitCheck() == B_NO_ERROR) {
					node.RemoveAttr(B_MAIL_ATTR_STATUS);
					WriteAttrString(&node, B_MAIL_ATTR_STATUS, status);
				}
			}
		}
	} else if (fRef) {
		//	...Otherwise just set the message read
		SetCurrentMessageRead();
	}

	return true;
}


void
TMailWindow::Show()
{
	if (Lock()) {
		if (!fResending && (fIncoming || fReplying))
			fContentView->fTextView->MakeFocus(true);
		else
		{
			BTextView *textView = fHeaderView->fTo->TextView();
			fHeaderView->fTo->MakeFocus(true);
			textView->Select(0, textView->TextLength());
		}
		Unlock();
	}
	BWindow::Show();
}


void
TMailWindow::Zoom(BPoint /*pos*/, float /*x*/, float /*y*/)
{
	float		height;
	float		width;
	BScreen		screen(this);
	BRect		r;
	BRect		s_frame = screen.Frame();

	r = Frame();
	width = 80 * ((TMailApp*)be_app)->fFont.StringWidth("M") +
			(r.Width() - fContentView->fTextView->Bounds().Width() + 6);
	if (width > (s_frame.Width() - 8))
		width = s_frame.Width() - 8;

	height = max_c(fContentView->fTextView->CountLines(), 20) *
			  fContentView->fTextView->LineHeight(0) +
			  (r.Height() - fContentView->fTextView->Bounds().Height());
	if (height > (s_frame.Height() - 29))
		height = s_frame.Height() - 29;

	r.right = r.left + width;
	r.bottom = r.top + height;

	if (abs((int)(Frame().Width() - r.Width())) < 5
		&& abs((int)(Frame().Height() - r.Height())) < 5)
		r = fZoom;
	else
	{
		fZoom = Frame();
		s_frame.InsetBy(6, 6);

		if (r.Width() > s_frame.Width())
			r.right = r.left + s_frame.Width();
		if (r.Height() > s_frame.Height())
			r.bottom = r.top + s_frame.Height();

		if (r.right > s_frame.right)
		{
			r.left -= r.right - s_frame.right;
			r.right = s_frame.right;
		}
		if (r.bottom > s_frame.bottom)
		{
			r.top -= r.bottom - s_frame.bottom;
			r.bottom = s_frame.bottom;
		}
		if (r.left < s_frame.left)
		{
			r.right += s_frame.left - r.left;
			r.left = s_frame.left;
		}
		if (r.top < s_frame.top)
		{
			r.bottom += s_frame.top - r.top;
			r.top = s_frame.top;
		}
	}

	ResizeTo(r.Width(), r.Height());
	MoveTo(r.LeftTop());
}


void
TMailWindow::WindowActivated(bool status)
{
	if (status) {
		BAutolock locker(sWindowListLock);
		sWindowList.RemoveItem(this);
		sWindowList.AddItem(this, 0);
	}
}


void
TMailWindow::Forward(entry_ref *ref, TMailWindow *window, bool includeAttachments)
{
	BEmailMessage *mail = window->Mail();
	if (mail == NULL)
		return;

	fMail = mail->ForwardMessage(gUseAccountFrom == ACCOUNT_FROM_MAIL, includeAttachments);

	BFile file(ref, O_RDONLY);
	if (file.InitCheck() < B_NO_ERROR)
		return;

	fHeaderView->fSubject->SetText(fMail->Subject());

	// set mail account

	if (gUseAccountFrom == ACCOUNT_FROM_MAIL) {
		fHeaderView->fChain = fMail->Account();

		BMenu *menu = fHeaderView->fAccountMenu;
		for (int32 i = menu->CountItems(); i-- > 0;) {
			BMenuItem *item = menu->ItemAt(i);
			BMessage *msg;
			if (item && (msg = item->Message()) != NULL
				&& msg->FindInt32("id") == fHeaderView->fChain)
				item->SetMarked(true);
		}
	}

	if (fMail->CountComponents() > 1) {
		// if there are any enclosures to be added, first add the enclosures
		// view to the window
		AddEnclosure(NULL);
		if (fEnclosuresView)
			fEnclosuresView->AddEnclosuresFromMail(fMail);
	}

	fContentView->fTextView->LoadMessage(fMail, false, NULL);
	fChanged = false;
	fFieldState = 0;
}


class HorizontalLine : public BView {
	public:
		HorizontalLine(BRect rect) : BView (rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW) {}
		virtual void Draw(BRect rect)
		{
			FillRect(rect,B_SOLID_HIGH);
		}
};


void
TMailWindow::Print()
{
	BPrintJob print(Title());

	if (!print_settings)
	{
		if (print.Settings()) {
			print_settings = print.Settings();
		} else {
			PrintSetup();
			if (!print_settings)
				return;
		}
	}

	print.SetSettings(new BMessage(*print_settings));

	if (print.ConfigJob() == B_NO_ERROR)
	{
		int32 curPage = 1;
		int32 lastLine = 0;
		BTextView header_view(print.PrintableRect(),"header",print.PrintableRect().OffsetByCopy(BPoint(-print.PrintableRect().left,-print.PrintableRect().top)),B_FOLLOW_ALL_SIDES);

		//---------Init the header fields
		#define add_header_field(field)			{/*header_view.SetFontAndColor(be_bold_font);*/ \
												header_view.Insert(fHeaderView->field->Label()); \
												header_view.Insert(" ");\
												/*header_view.SetFontAndColor(be_plain_font);*/ \
												header_view.Insert(fHeaderView->field->Text()); \
												header_view.Insert("\n");}
		add_header_field(fSubject);
		add_header_field(fTo);
		if ((fHeaderView->fCc != NULL) && (strcmp(fHeaderView->fCc->Text(),"") != 0))
			add_header_field(fCc);
		header_view.Insert(fHeaderView->fDate->Text());

		int32 maxLine = fContentView->fTextView->CountLines();
		BRect pageRect = print.PrintableRect();
		BRect curPageRect = pageRect;

		print.BeginJob();
		float header_height = header_view.TextHeight(0,header_view.CountLines());
		BBitmap bmap(BRect(0,0,pageRect.Width(),header_height),B_BITMAP_ACCEPTS_VIEWS,B_RGBA32);
		bmap.Lock();
		bmap.AddChild(&header_view);
		print.DrawView(&header_view,BRect(0,0,pageRect.Width(),header_height),BPoint(0.0,0.0));
		HorizontalLine line(BRect(0,0,pageRect.right,0));
		bmap.AddChild(&line);
		print.DrawView(&line,line.Bounds(),BPoint(0,header_height+1));
		bmap.Unlock();
		header_height += 5;

		do
		{
			int32 lineOffset = fContentView->fTextView->OffsetAt(lastLine);
			curPageRect.OffsetTo(0, fContentView->fTextView->PointAt(lineOffset).y);

			int32 fromLine = lastLine;
			lastLine = fContentView->fTextView->LineAt(BPoint(0.0, curPageRect.bottom - ((curPage == 1) ? header_height : 0)));

			float curPageHeight = fContentView->fTextView->TextHeight(fromLine, lastLine) + ((curPage == 1) ? header_height : 0);
			if(curPageHeight > pageRect.Height())
				curPageHeight = fContentView->fTextView->TextHeight(fromLine, --lastLine) + ((curPage == 1) ? header_height : 0);

			curPageRect.bottom = curPageRect.top + curPageHeight - 1.0;

			if((curPage >= print.FirstPage()) &&
				(curPage <= print.LastPage()))
			{
				print.DrawView(fContentView->fTextView, curPageRect, BPoint(0.0, (curPage == 1) ? header_height : 0.0));
				print.SpoolPage();
			}

			curPageRect = pageRect;
			lastLine++;
			curPage++;

		} while (print.CanContinue() && lastLine < maxLine);

		print.CommitJob();
		bmap.RemoveChild(&header_view);
		bmap.RemoveChild(&line);
	}
}


void
TMailWindow::PrintSetup()
{
	BPrintJob	print("mail_print");
	status_t	result;

	if (print_settings)
		print.SetSettings(new BMessage(*print_settings));

	if ((result = print.ConfigPage()) == B_NO_ERROR)
	{
		delete print_settings;
		print_settings = print.Settings();
	}
}


void
TMailWindow::SetTo(const char *mailTo, const char *subject, const char *ccTo,
	const char *bccTo, const BString *body, BMessage *enclosures)
{
	Lock();

	if (mailTo && mailTo[0])
		fHeaderView->fTo->SetText(mailTo);
	if (subject && subject[0])
		fHeaderView->fSubject->SetText(subject);
	if (ccTo && ccTo[0])
		fHeaderView->fCc->SetText(ccTo);
	if (bccTo && bccTo[0])
		fHeaderView->fBcc->SetText(bccTo);

	if (body && body->Length())
	{
		fContentView->fTextView->SetText(body->String(), body->Length());
		fContentView->fTextView->GoToLine(0);
	}

	if (enclosures && enclosures->HasRef("refs"))
		AddEnclosure(enclosures);

	Unlock();
}


void
TMailWindow::CopyMessage(entry_ref *ref, TMailWindow *src)
{
	BNode file(ref);
	if (file.InitCheck() == B_OK) {
		BString string;
		if (fHeaderView->fTo && ReadAttrString(&file, B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->fTo->SetText(string.String());

		if (fHeaderView->fSubject && ReadAttrString(&file, B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->fSubject->SetText(string.String());

		if (fHeaderView->fCc && ReadAttrString(&file, B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->fCc->SetText(string.String());
	}

	TTextView *text = src->fContentView->fTextView;
	text_run_array *style = text->RunArray(0, text->TextLength());

	fContentView->fTextView->SetText(text->Text(), text->TextLength(), style);

	free(style);
}


void
TMailWindow::Reply(entry_ref *ref, TMailWindow *window, uint32 type)
{
	const char *notImplementedString = "<Not Yet Implemented>";

	fRepliedMail = *ref;
	SetOriginatingWindow(window);

	BEmailMessage *mail = window->Mail();
	if (mail == NULL)
		return;

	if (type == M_REPLY_ALL)
		type = B_MAIL_REPLY_TO_ALL;
	else if (type == M_REPLY_TO_SENDER)
		type = B_MAIL_REPLY_TO_SENDER;
	else
		type = B_MAIL_REPLY_TO;

	fMail = mail->ReplyMessage(mail_reply_to_mode(type),
		gUseAccountFrom == ACCOUNT_FROM_MAIL, QUOTE);

	// set header fields
	fHeaderView->fTo->SetText(fMail->To());
	fHeaderView->fCc->SetText(fMail->CC());
	fHeaderView->fSubject->SetText(fMail->Subject());

	int32 chainID;
	BFile file(window->fRef, B_READ_ONLY);
	if (file.ReadAttr("MAIL:reply_with", B_INT32_TYPE, 0, &chainID, 4) < B_OK)
		chainID = -1;

	// set mail account

	if ((gUseAccountFrom == ACCOUNT_FROM_MAIL) || (chainID > -1)) {
		if (gUseAccountFrom == ACCOUNT_FROM_MAIL)
			fHeaderView->fChain = fMail->Account();
		else
			fHeaderView->fChain = chainID;

		BMenu *menu = fHeaderView->fAccountMenu;
		for (int32 i = menu->CountItems(); i-- > 0;) {
			BMenuItem *item = menu->ItemAt(i);
			BMessage *msg;
			if (item && (msg = item->Message()) != NULL
				&& msg->FindInt32("id") == fHeaderView->fChain)
				item->SetMarked(true);
		}
	}

	// create preamble string

	char preamble[1024], *from = gReplyPreamble, *to = preamble;
	while (*from) {
		if (*from == '%') {
			// insert special content
			int32 length;

			switch (*++from) {
				case 'n':	// full name
				{
					BString fullName(mail->From());
					if (fullName.Length() <= 0)
						fullName = "No-From-Address-Available";

					extract_address_name(fullName);
					length = fullName.Length();
					memcpy(to, fullName.String(), length);
					to += length;
					break;
				}

				case 'e':	// eMail address
				{
					const char *address = mail->From();
					if (address == NULL)
						address = "<unknown>";
					length = strlen(address);
					memcpy(to, address, length);
					to += length;
					break;
				}

				case 'd':	// date
				{
					const char *date = mail->Date();
					if (date == NULL)
						date = "No-Date-Available";
					length = strlen(date);
					memcpy(to, date, length);
					to += length;
					break;
				}

				// ToDo: parse stuff!
				case 'f':	// first name
				case 'l':	// last name
					length = strlen(notImplementedString);
					memcpy(to, notImplementedString, length);
					to += length;
					break;

				default: // Sometimes a % is just a %.
					*to++ = *from;
			}
		} else if (*from == '\\') {
			switch (*++from) {
				case 'n':
					*to++ = '\n';
					break;

				default:
					*to++ = *from;
			}
		} else
			*to++ = *from;

		from++;
	}
	*to = '\0';

	// insert (if selection) or load (if whole mail) message text into text view

	int32 finish, start;
	window->fContentView->fTextView->GetSelection(&start, &finish);
	if (start != finish) {
		char *text = (char *)malloc(finish - start + 1);
		if (text == NULL)
			return;

		window->fContentView->fTextView->GetText(start, finish - start, text);
		if (text[strlen(text) - 1] != '\n') {
			text[strlen(text)] = '\n';
			finish++;
		}
		fContentView->fTextView->SetText(text, finish - start);
		free(text);

		finish = fContentView->fTextView->CountLines() - 1;
		for (int32 loop = 0; loop < finish; loop++) {
			fContentView->fTextView->GoToLine(loop);
			fContentView->fTextView->Insert((const char *)QUOTE);
		}

		if (gColoredQuotes) {
			const BFont *font = fContentView->fTextView->Font();
			int32 length = fContentView->fTextView->TextLength();

			TextRunArray style(length / 8 + 8);

			FillInQuoteTextRuns(fContentView->fTextView, fContentView->fTextView->Text(),
				length, font, &style.Array(), style.MaxEntries());

			fContentView->fTextView->SetRunArray(0, length, &style.Array());
		}

		fContentView->fTextView->GoToLine(0);
		if (strlen(preamble) > 0)
			fContentView->fTextView->Insert(preamble);
	}
	else
		fContentView->fTextView->LoadMessage(mail, true, preamble);

	fReplying = true;
}


status_t
TMailWindow::Send(bool now)
{
	uint32 characterSetToUse = gMailCharacterSet;
	mail_encoding encodingForBody = quoted_printable;
	mail_encoding encodingForHeaders = quoted_printable;

	if (!now) {
		status_t status;

		if ((status = SaveAsDraft()) != B_OK) {
			beep();
			(new BAlert("",
				MDR_DIALECT_CHOICE ("E-mail draft could not be saved!","ドラフトは保存できませんでした。"),
				MDR_DIALECT_CHOICE ("OK","了解")))->Go();
		}
		return status;
	}

	if (fHeaderView != NULL)
		characterSetToUse = fHeaderView->fCharacterSetUserSees;

	// Set up the encoding to use for converting binary to printable ASCII.
	// Normally this will be quoted printable, but for some old software,
	// particularly Japanese stuff, they only understand base64.  They also
	// prefer it for the smaller size.  Later on this will be reduced to 7bit
	// if the encoded text is just 7bit characters.
	if (characterSetToUse == B_SJIS_CONVERSION ||
		characterSetToUse == B_EUC_CONVERSION)
		encodingForBody = base64;
	else if (characterSetToUse == B_JIS_CONVERSION ||
		characterSetToUse == B_MAIL_US_ASCII_CONVERSION ||
		characterSetToUse == B_ISO1_CONVERSION ||
		characterSetToUse == B_EUC_KR_CONVERSION)
		encodingForBody = eight_bit;

	// Using quoted printable headers on almost completely non-ASCII Japanese
	// is a waste of time.  Besides, some stupid cell phone services need
	// base64 in the headers.
	if (characterSetToUse == B_SJIS_CONVERSION ||
		characterSetToUse == B_EUC_CONVERSION ||
		characterSetToUse == B_JIS_CONVERSION ||
		characterSetToUse == B_EUC_KR_CONVERSION)
		encodingForHeaders = base64;

	// Count the number of characters in the message body which aren't in the
	// currently selected character set.  Also see if the resulting encoded
	// text can safely use 7 bit characters.
	if (fContentView->fTextView->TextLength() > 0) {
		// First do a trial encoding with the user's character set.
		int32	converterState = 0;
		int32	originalLength;
		BString	tempString;
		int32	tempStringLength;
		char	*tempStringPntr;
		originalLength = fContentView->fTextView->TextLength();
		tempStringLength = originalLength *
			6 /* Some character sets bloat up on escape codes */;
		tempStringPntr = tempString.LockBuffer (tempStringLength);
		if (tempStringPntr != NULL &&
			B_OK == mail_convert_from_utf8 (
			characterSetToUse,
			fContentView->fTextView->Text(), &originalLength,
			tempStringPntr, &tempStringLength, &converterState,
			0x1A /* The code to substitute for unknown characters */)) {

			// Check for any characters which don't fit in a 7 bit encoding.
			int		i;
			bool	has8Bit = false;
			for (i = 0; i < tempStringLength; i++)
				if (tempString[i] == 0 || (tempString[i] & 0x80)) {
					has8Bit = true;
					break;
				}
			if (!has8Bit)
				encodingForBody = seven_bit;
			tempString.UnlockBuffer (tempStringLength);

			// Count up the number of unencoded characters and warn the user about them.
			if (gWarnAboutUnencodableCharacters) {
				int32	offset = 0;
				int		count = 0;
				while (offset >= 0) {
					offset = tempString.FindFirst (0x1A, offset);
					if (offset >= 0) {
						count++;
						offset++; // Don't get stuck finding the same character again.
					}
				}
				if (count > 0) {
					int32	userAnswer;
					BString	messageString;
					MDR_DIALECT_CHOICE (
						messageString << "Your main text contains " << count <<
							" unencodable characters.  Perhaps a different character "
							"set would work better?  Hit Send to send it anyway "
							"(a substitute character will be used in place of "
							"the unencodable ones), or choose Cancel to go back "
							"and try fixing it up."
						,
						messageString << "送信メールの本文には " << count <<
							" 個のエンコードできない文字があります。"
							"違う文字セットを使うほうがよい可能性があります。"
							"このまま送信の場合は「送信」ボタンを押してください。"
							"その場合、代用文字がUnicode化可能な文字に代わって使われます。"
							"文字セットを変更する場合は「中止」ボタンを押して下さい。"
						);
					userAnswer = (new BAlert ("Question", messageString.String(),
						MDR_DIALECT_CHOICE ("Send","送信"),
						MDR_DIALECT_CHOICE ("Cancel","中止"), // Default is cancel.
						NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))
						->Go();
					if (userAnswer == 1)
						return -1; // Cancel was picked.
				}
			}
		}
	}

	status_t result;

	if (fResending) {
		BFile file(fRef, O_RDONLY);
		result = file.InitCheck();
		if (result == B_OK)
		{
			BEmailMessage mail(&file);
			mail.SetTo(fHeaderView->fTo->Text(), characterSetToUse, encodingForHeaders);

			if (fHeaderView->fChain != ~0L)
				mail.SendViaAccount(fHeaderView->fChain);

			result = mail.Send(now);
		}
	} else {
		if (fMail == NULL)
			// the mail will be deleted when the window is closed
			fMail = new BEmailMessage;

		// Had an embarrassing bug where replying to a message and clearing the
		// CC field meant that it got sent out anyway, so pass in empty strings
		// when changing the header to force it to remove the header.

		fMail->SetTo(fHeaderView->fTo->Text(), characterSetToUse, encodingForHeaders);
		fMail->SetSubject(fHeaderView->fSubject->Text(), characterSetToUse, encodingForHeaders);
		fMail->SetCC(fHeaderView->fCc->Text(), characterSetToUse, encodingForHeaders);
		fMail->SetBCC(fHeaderView->fBcc->Text());

		//--- Add X-Mailer field
		{
			// get app version
			version_info versionInfo;
			memset(&versionInfo, 0, sizeof(version_info));

			app_info appInfo;
			if (be_app->GetAppInfo(&appInfo) == B_OK) {
				BFile file(&appInfo.ref, B_READ_ONLY);
				if (file.InitCheck() == B_OK) {
					BAppFileInfo info(&file);
					if (info.InitCheck() == B_OK)
						info.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND);
				}
			}
			// prepare version variety string
			const char *varietyStrings[] = {
				"Development", "Alpha", "Beta",
				"Gamma", "Golden master", "Final"
			};
			char varietyString[32];
			strcpy(varietyString, varietyStrings[versionInfo.variety % 6]);
			if (versionInfo.variety < 5)
				sprintf(varietyString + strlen(varietyString), "/%li", versionInfo.internal);

			char versionString[255];
			sprintf(versionString,
				"BeMail - Mail Daemon Replacement %ld.%ld.%ld %s",
				versionInfo.major, versionInfo.middle, versionInfo.minor, varietyString);
			fMail->SetHeaderField("X-Mailer", versionString);
		}

		/****/

		// the content text is always added to make sure there is a mail body
		fMail->SetBodyTextTo("");
		fContentView->fTextView->AddAsContent(fMail, sWrapMode, characterSetToUse,
			encodingForBody);

		if (fEnclosuresView != NULL) {
			TListItem *item;
			int32 index = 0;
			while ((item = (TListItem *)fEnclosuresView->fList->ItemAt(index++)) != NULL) {
				if (item->Component())
					continue;

				// leave out missing enclosures
				BEntry entry(item->Ref());
				if (!entry.Exists())
					continue;

				fMail->Attach(item->Ref(), attachAttributes_mode);
			}
		}
		if (fHeaderView->fChain != ~0L)
			fMail->SendViaAccount(fHeaderView->fChain);

		result = fMail->Send(now);

		if (fReplying) {
			// Set status of the replied mail

			BNode node(&fRepliedMail);
			if (node.InitCheck() >= B_OK) {
				if (fOriginatingWindow) {
					BMessage msg(M_SAVE_POSITION), reply;
					fOriginatingWindow->SendMessage(&msg, &reply);
				}
				WriteAttrString(&node, B_MAIL_ATTR_STATUS, "Replied");
			}
		}
	}

	bool close = false;
	char errorMessage[256];

	switch (result) {
		case B_NO_ERROR:
			close = true;
			fSent = true;

			// If it's a draft, remove the draft file
			if (fDraft) {
				BEntry entry(fRef);
				entry.Remove();
			}
			break;

		case B_MAIL_NO_DAEMON:
		{
			close = true;
			fSent = true;

			int32 start = (new BAlert("no daemon",
				MDR_DIALECT_CHOICE ("The mail_daemon is not running.  "
				"The message is queued and will be sent when the mail_daemon is started.",
				"mail_daemon が開始されていません "
				"このメッセージは処理待ちとなり、mail_daemon 開始後に処理されます"),
				MDR_DIALECT_CHOICE ("Start Now","ただちに開始する"),
				MDR_DIALECT_CHOICE ("Ok","了解")))->Go();

			if (start == 0) {
				result = be_roster->Launch("application/x-vnd.Be-POST");
				if (result == B_OK)
					BMailDaemon::SendQueuedMail();
				else
					sprintf(errorMessage,"The mail_daemon could not be started:\n  (0x%.8lx) %s",
							result,strerror(result));
			}
			break;
		}

//		case B_MAIL_UNKNOWN_HOST:
//		case B_MAIL_ACCESS_ERROR:
//			sprintf(errorMessage, "An error occurred trying to connect with the SMTP "
//				"host.  Check your SMTP host name.");
//			break;
//
//		case B_MAIL_NO_RECIPIENT:
//			sprintf(errorMessage, "You must have either a \"To\" or \"Bcc\" recipient.");
//			break;

		default:
			sprintf(errorMessage, "An error occurred trying to send mail (0x%.8lx): %s",
							result,strerror(result));
	}

	if (result != B_NO_ERROR && result != B_MAIL_NO_DAEMON) {
		beep();
		(new BAlert("", errorMessage, "Ok"))->Go();
	}
	if (close)
		PostMessage(B_QUIT_REQUESTED);

	return result;
}


status_t
TMailWindow::SaveAsDraft()
{
	status_t	status;
	BPath		draftPath;
	BDirectory	dir;
	BFile		draft;
	uint32		flags = 0;

	if (fDraft) {
		if ((status = draft.SetTo(fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) != B_OK)
			return status;
	} else {
		// Get the user home directory
		if ((status = find_directory(B_USER_DIRECTORY, &draftPath)) != B_OK)
			return status;

		// Append the relative path of the draft directory
		draftPath.Append(kDraftPath);

		// Create the file
		status = dir.SetTo(draftPath.Path());
		switch (status) {
			// Create the directory if it does not exist
			case B_ENTRY_NOT_FOUND:
				if ((status = dir.CreateDirectory(draftPath.Path(), &dir)) != B_OK)
					return status;
			case B_OK:
			{
				char fileName[512], *eofn;
				int32 i;

				// save as some version of the message's subject
				strncpy(fileName, fHeaderView->fSubject->Text(), sizeof(fileName)-10);
				fileName[sizeof(fileName)-10]='\0';  // terminate like strncpy doesn't
				eofn = fileName + strlen(fileName);

				// convert /, \ and : to -
				for (char *bad = fileName; (bad = strchr(bad, '/')) != NULL; ++bad) *bad = '-';
				for (char *bad = fileName; (bad = strchr(bad, '\\')) != NULL;++bad) *bad = '-';
				for (char *bad = fileName; (bad = strchr(bad, ':')) != NULL; ++bad) *bad = '-';

				// Create the file; if the name exists, find a unique name
				flags = B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS;
				for (i = 1; (status = draft.SetTo(&dir, fileName, flags )) != B_OK; i++) {
					if( status != B_FILE_EXISTS )
						return status;
					sprintf(eofn, "%ld", i );
				}

				// Cache the ref
				delete fRef;
				BEntry entry(&dir, fileName);
				fRef = new entry_ref;
				entry.GetRef(fRef);
				break;
			}
			default:
				return status;
		}
	}

	// Write the content of the message
	draft.Write(fContentView->fTextView->Text(), fContentView->fTextView->TextLength());

	//
	// Add the header stuff as attributes
	//
	WriteAttrString(&draft, B_MAIL_ATTR_NAME, fHeaderView->fTo->Text());
	WriteAttrString(&draft, B_MAIL_ATTR_TO, fHeaderView->fTo->Text());
	WriteAttrString(&draft, B_MAIL_ATTR_SUBJECT, fHeaderView->fSubject->Text());
	if (fHeaderView->fCc != NULL)
		WriteAttrString(&draft, B_MAIL_ATTR_CC, fHeaderView->fCc->Text());
	if (fHeaderView->fBcc != NULL)
		WriteAttrString(&draft, "MAIL:bcc", fHeaderView->fBcc->Text());

	// Add the draft attribute for indexing
	uint32 draftAttr = true;
	draft.WriteAttr( "MAIL:draft", B_INT32_TYPE, 0, &draftAttr, sizeof(uint32) );

	// Add Attachment paths in attribute
	if (fEnclosuresView != NULL) {
		TListItem *item;
		BPath path;
		BString pathStr;

		for (int32 i = 0; (item = (TListItem *)fEnclosuresView->fList->ItemAt(i)) != NULL; i++) {
			if (i > 0)
				pathStr.Append(":");

			BEntry entry(item->Ref(), true);
			if (!entry.Exists())
				continue;

			entry.GetPath(&path);
			pathStr.Append(path.Path());
		}
		if (pathStr.Length())
			WriteAttrString(&draft, "MAIL:attachments", pathStr.String());
	}

	// Set the MIME Type of the file
	BNodeInfo info(&draft);
	info.SetType(kDraftType);

	fSent = true;
	fDraft = true;
	fChanged = false;

	return B_OK;
}


status_t
TMailWindow::TrainMessageAs(const char *CommandWord)
{
	status_t	errorCode = -1;
	char		errorString[1500];
	BEntry		fileEntry;
	BPath		filePath;
	BMessage	replyMessage;
	BMessage	scriptingMessage;
	team_id		serverTeam;

	if (fRef == NULL)
		goto ErrorExit; // Need to have a real file and name.
	errorCode = fileEntry.SetTo(fRef, true /* traverse */);
	if (errorCode != B_OK)
		goto ErrorExit;
	errorCode = fileEntry.GetPath(&filePath);
	if (errorCode != B_OK)
		goto ErrorExit;
	fileEntry.Unset();

	// Get a connection to the spam database server.  Launch if needed.

	if (!gMessengerToSpamServer.IsValid()) {
		// Make sure the server is running.
		if (!be_roster->IsRunning (kSpamServerSignature)) {
			errorCode = be_roster->Launch (kSpamServerSignature);
			if (errorCode != B_OK) {
				BPath path;
				entry_ref ref;
				directory_which places[] = {B_COMMON_BIN_DIRECTORY,B_BEOS_BIN_DIRECTORY};
				for (int32 i = 0; i < 2; i++) {
					find_directory(places[i],&path);
					path.Append("spamdbm");
					if (!BEntry(path.Path()).Exists())
						continue;
					get_ref_for_path(path.Path(),&ref);
					if ((errorCode =  be_roster->Launch (&ref)) == B_OK)
						break;
				}
				if (errorCode != B_OK)
					goto ErrorExit;
			}
		}

		// Set up the messenger to the database server.
		errorCode = B_SERVER_NOT_FOUND;
		serverTeam = be_roster->TeamFor(kSpamServerSignature);
		if (serverTeam < 0)
			goto ErrorExit;

		gMessengerToSpamServer = BMessenger (kSpamServerSignature, serverTeam, &errorCode);
		if (!gMessengerToSpamServer.IsValid())
			goto ErrorExit;
	}

	// Ask the server to train on the message.  Give it the command word and
	// the absolute path name to use.

	scriptingMessage.MakeEmpty();
	scriptingMessage.what = B_SET_PROPERTY;
	scriptingMessage.AddSpecifier(CommandWord);
	errorCode = scriptingMessage.AddData("data", B_STRING_TYPE,
		filePath.Path(), strlen(filePath.Path()) + 1, false /* fixed size */);
	if (errorCode != B_OK)
		goto ErrorExit;
	replyMessage.MakeEmpty();
	errorCode = gMessengerToSpamServer.SendMessage(&scriptingMessage,
		&replyMessage);
	if (errorCode != B_OK
		|| replyMessage.FindInt32("error", &errorCode) != B_OK
		|| errorCode != B_OK)
		goto ErrorExit; // Classification failed in one of many ways.

	SetTitleForMessage(); // Update window title to show new spam classification.
	return B_OK;

ErrorExit:
	beep();
	sprintf(errorString, "Unable to train the message file \"%s\" as %s.  "
		"Possibly useful error code: %s (%ld).",
		filePath.Path(), CommandWord, strerror (errorCode), errorCode);
	(new BAlert("", errorString,
		MDR_DIALECT_CHOICE("Ok","了解")))->Go();
	return errorCode;
}


void
TMailWindow::SetTitleForMessage()
{
	//
	//	Figure out the title of this message and set the title bar
	//
	BString title = "BeMail";

	if (fIncoming)
	{
		if (fMail->GetName(&title) == B_OK)
			title << ": \"" << fMail->Subject() << "\"";
		else
			title = fMail->Subject();

		if (gShowSpamGUI && fRef != NULL) {
			BString	classification;
			BNode	node (fRef);
			char	numberString [30];
			BString oldTitle (title);
			float	spamRatio;
			if (node.InitCheck() != B_OK || node.ReadAttrString
				("MAIL:classification", &classification) != B_OK)
				classification = "Unrated";
			if (classification != "Spam" && classification != "Genuine") {
				// Uncertain, Unrated and other unknown classes, show the ratio.
				if (node.InitCheck() == B_OK && sizeof (spamRatio) ==
					node.ReadAttr("MAIL:ratio_spam", B_FLOAT_TYPE, 0,
					&spamRatio, sizeof (spamRatio))) {
					sprintf (numberString, "%.4f", spamRatio);
					classification << " " << numberString;
				}
			}
			title = "";
			title << "[" << classification << "] " << oldTitle;
		}
	}
	SetTitle(title.String());
}


//
//	Open *another* message in the existing mail window.  Some code here is
//	duplicated from various constructors.
//	The duplicated code should be in a private initializer method -- axeld.
//

status_t
TMailWindow::OpenMessage(entry_ref *ref, uint32 characterSetForDecoding)
{
	//
	//	Set some references to the email file
	//
	if (fRef)
		delete fRef;
	fRef = new entry_ref(*ref);

	if (fStartingText)
	{
		free(fStartingText);
		fStartingText = NULL;
	}
	fPrevTrackerPositionSaved = false;
	fNextTrackerPositionSaved = false;

	fContentView->fTextView->StopLoad();
	delete fMail;

	BFile file(fRef, B_READ_ONLY);
	status_t err = file.InitCheck();
	if (err != B_OK)
		return err;

	char mimeType[256];
	BNodeInfo fileInfo(&file);
	fileInfo.GetType(mimeType);

	// Check if it's a draft file, which contains only the text, and has the
	// from, to, bcc, attachments listed as attributes.
	if (!strcmp(kDraftType, mimeType))
	{
		BNode node(fRef);
		off_t size;
		BString string;

		fMail = new BEmailMessage; // Not really used much, but still needed.

		// Load the raw UTF-8 text from the file.
		file.GetSize(&size);
		fContentView->fTextView->SetText(&file, 0, size);

		// Restore Fields from attributes
		if (ReadAttrString(&node, B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->fTo->SetText(string.String());
		if (ReadAttrString(&node, B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->fSubject->SetText(string.String());
		if (ReadAttrString(&node, B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->fCc->SetText(string.String());
		if (ReadAttrString(&node, "MAIL:bcc", &string) == B_OK)
			fHeaderView->fBcc->SetText(string.String());

		// Restore attachments
		if (ReadAttrString(&node, "MAIL:attachments", &string) == B_OK)
		{
			BMessage msg(REFS_RECEIVED);
			entry_ref enc_ref;

			char *s = strtok((char *)string.String(), ":");
			while (s)
			{
				BEntry entry(s, true);
				if (entry.Exists())
				{
					entry.GetRef(&enc_ref);
					msg.AddRef("refs", &enc_ref);
				}
				s = strtok(NULL, ":");
			}
			AddEnclosure(&msg);
		}
		PostMessage(RESET_BUTTONS);
		fIncoming = false;
		fDraft = true;
	}
	else // A real mail message, parse its headers to get from, to, etc.
	{
		fMail = new BEmailMessage(fRef, characterSetForDecoding);
		fIncoming = true;
		fHeaderView->LoadMessage(fMail);
	}

	err = fMail->InitCheck();
	if (err < B_OK)
	{
		delete fMail;
		fMail = NULL;
		return err;
	}

	SetTitleForMessage();

	if (fIncoming)
	{
		//
		//	Put the addresses in the 'Save Address' Menu
		//
		BMenuItem *item;
		while ((item = fSaveAddrMenu->RemoveItem(0L)) != NULL)
			delete item;

		// create the list of addresses

		BList addressList;
		get_address_list(addressList, fMail->To(), extract_address);
		get_address_list(addressList, fMail->CC(), extract_address);
		get_address_list(addressList, fMail->From(), extract_address);
		get_address_list(addressList, fMail->ReplyTo(), extract_address);

		BMessage *msg;

		for (int32 i = addressList.CountItems(); i-- > 0;) {
			char *address = (char *)addressList.RemoveItem(0L);

			// insert the new address in alphabetical order
			int32 index = 0;
			while ((item = fSaveAddrMenu->ItemAt(index)) != NULL) {
				if (!strcmp(address, item->Label())) {
					// item already in list
					goto skip;
				}

				if (strcmp(address, item->Label()) < 0)
					break;

				index++;
			}

			msg = new BMessage(M_SAVE);
			msg->AddString("address", address);
			fSaveAddrMenu->AddItem(new BMenuItem(address, msg), index);

		skip:
			free(address);
		}

		//
		// Clear out existing contents of text view.
		//
		fContentView->fTextView->SetText("", (int32)0);

		fContentView->fTextView->LoadMessage(fMail, false, NULL);
	}

	return B_OK;
}


TMailWindow *
TMailWindow::FrontmostWindow()
{
	BAutolock locker(sWindowListLock);
	if (sWindowList.CountItems() > 0)
		return (TMailWindow *)sWindowList.ItemAt(0);

	return NULL;
}


//====================================================================
//	#pragma mark -


TMenu::TMenu(const char *name, const char *attribute, int32 message, bool popup, bool addRandom)
	:	BPopUpMenu(name, false, false),
	fPopup(popup),
	fAddRandom(addRandom),
	fMessage(message)
{
	fAttribute = (char *)malloc(strlen(attribute) + 1);
	strcpy(fAttribute, attribute);
	fPredicate = (char *)malloc(strlen(fAttribute) + 5);
	sprintf(fPredicate, "%s = *", fAttribute);

	BuildMenu();
}


TMenu::~TMenu()
{
	free(fAttribute);
	free(fPredicate);
}


void
TMenu::AttachedToWindow()
{
	BuildMenu();
	BPopUpMenu::AttachedToWindow();
}


BPoint
TMenu::ScreenLocation(void)
{
	if (fPopup)
		return BPopUpMenu::ScreenLocation();

	return BMenu::ScreenLocation();
}


void
TMenu::BuildMenu()
{
	BMenuItem *item;
	while ((item = RemoveItem((int32)0)) != NULL)
		delete item;

	BVolume	volume;
	BVolumeRoster().GetBootVolume(&volume);

	BQuery query;
	query.SetVolume(&volume);
	query.SetPredicate(fPredicate);
	query.Fetch();

	int32 index = 0;
	BEntry entry;
	while (query.GetNextEntry(&entry) == B_NO_ERROR)
	{
		BFile file(&entry, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR)
		{
			BMessage *msg = new BMessage(fMessage);

			entry_ref ref;
			entry.GetRef(&ref);
			msg->AddRef("ref", &ref);

			char name[B_FILE_NAME_LENGTH];
			file.ReadAttr(fAttribute, B_STRING_TYPE, 0, name, sizeof(name));

			if (index < 9 && !fPopup)
				AddItem(new BMenuItem(name, msg, '1' + index));
			else
				AddItem(new BMenuItem(name, msg));
			index++;
		}
	}
	if (fAddRandom && CountItems())
	{
		AddItem(new BSeparatorItem(), 0);
		//AddSeparatorItem();
		BMessage *msg = new BMessage(M_RANDOM_SIG);
		if (!fPopup)
			AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Random","R) 自動決定"), msg, '0'), 0);
		else
			AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Random","R) 自動決定"), msg), 0);
	}
}

