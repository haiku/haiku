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


#include "MailSupport.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <Autolock.h>
#include <Clipboard.h>
#include <Collator.h>
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

#include "Utilities.h"


using namespace BPrivate ;

#include "Words.h"

// global variables
Words*	gWords[MAX_DICTIONARIES];
Words*	gExactWords[MAX_DICTIONARIES];

int32	gUserDict;
BFile*	gUserDictFile;
int32 	gDictCount = 0;

// int32			level = L_BEGINNER;

const char* kSpamServerSignature =
	"application/x-vnd.agmsmith.spamdbm";
const char* kDraftPath = "mail/draft";
const char* kDraftType = "text/x-vnd.Be-MailDraft";
const char* kMailFolder = "mail";
const char* kMailboxSymlink = "Mail/mailbox";
	// relative to B_USER_SETTINGS_DIRECTORY

BCollator collator;


int32
header_len(BFile *file)
{
	char	*buffer;
	int32	length;
	int32	result = 0;
	off_t	size;

	if (file->ReadAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &result,
		sizeof(int32)) != sizeof(int32)) {
		file->GetSize(&size);
		buffer = (char *)malloc(size);
		if (buffer) {
			file->Seek(0, 0);
			if (file->Read(buffer, size) == size) {
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


static int
compare_menu_items(const BMenuItem* a, const BMenuItem* b)
{
	return collator.Compare(a->Label(), b->Label());
}

status_t
create_label_file(const char* label)
{
	status_t result;
	BPath path;

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append("Mail");
	path.Append("labels");
	if (create_directory(path.Path(), 777) != B_OK)
		return B_ERROR;
	path.Append(label);

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (file.InitCheck() != B_OK)
		return B_ERROR;

	BString string;
	string.SetToFormat(B_MAIL_ATTR_LABEL "==\"%s\"", label);
	file.WriteAttrString("_trk/qrystr", &string);
	string = "E-mail";
	file.WriteAttrString("_trk/qryinitmime", &string);
	result = BNodeInfo(&file).SetType("application/x-vnd.Be-query");

	return result;
}


int32
add_folder_menu_items(BMenu* menu, const char* folder, uint32 what)
{
	BPath path;
	BPath label_path;
	BDirectory directory;
	BEntry entry;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;
	path.Append("Mail");
	path.Append(folder);
	directory.SetTo(path.Path());
	if (directory.InitCheck() != B_OK)
		return B_ERROR;

	BObjectList<BMenuItem> items;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		if (entry.InitCheck() != B_OK)
			break;

		entry.GetPath(&label_path);
		const char* label = label_path.Leaf();

		// Make sure the label file has the correct query formula,
		// in case it wasn't created with the Mail app
		BFile file(label_path.Path(), B_READ_WRITE);
		if (file.InitCheck() != B_OK)
			continue;
		BString string;
		BString formula;
		formula.SetToFormat(B_MAIL_ATTR_LABEL "==\"%s\"", label);
		file.ReadAttrString("_trk/qrystr", &string);
		if (string != formula) {
			file.WriteAttrString("_trk/qrystr", &formula);
			string = "E-mail";
			file.WriteAttrString("_trk/qryinitmime", &string);
			BNodeInfo(&file).SetType("application/x-vnd.Be-query");
		}

		BMessage* message = new BMessage(what);
		message->AddString("label", label);
		items.AddItem(new BMenuItem(label, message));
	}

	if (!items.IsEmpty()) {
		if (BLocale::Default()->GetCollator(&collator) == B_OK) {
			collator.SetNumericSorting(true);
			items.SortItems(compare_menu_items);
		}
	}
	int32 itemCount = items.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BMenuItem* item = items.ItemAt(i);
		if (i < 9)
			item->SetShortcut('1' + i, B_COMMAND_KEY);
		menu->AddItem(item);
	}

	return itemCount;
}


int32
add_query_menu_items(BMenu* menu, const char* attribute, uint32 what,
	const char* format, bool popup)
{
	BVolume	volume;
	BVolumeRoster().GetBootVolume(&volume);

	BQuery query;
	query.SetVolume(&volume);
	query.PushAttr(attribute);
	query.PushString("*");
	query.PushOp(B_EQ);
	query.Fetch();

	int32 index = 0;
	BEntry entry;
	while (query.GetNextEntry(&entry) == B_OK) {
		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BMessage* message = new BMessage(what);

			entry_ref ref;
			entry.GetRef(&ref);
			message->AddRef("ref", &ref);

			BString value;
			if (file.ReadAttrString(attribute, &value) < B_OK)
				continue;

			message->AddString("attribute", value.String());

			BString name;
			if (format != NULL)
				name.SetToFormat(format, value.String());
			else
				name = value;

			if (index < 9 && !popup)
				menu->AddItem(new BMenuItem(name, message, '1' + index));
			else
				menu->AddItem(new BMenuItem(name, message));
			index++;
		}
	}

	return index;
}
