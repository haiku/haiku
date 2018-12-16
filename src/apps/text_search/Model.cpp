/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * Distributed under the terms of the MIT License.
 */


#include "Model.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>

#include "GlobalDefs.h"


using std::nothrow;


Model::Model()
	:
	fDirectory(),
	fSelectedFiles((uint32)0),

	fRecurseDirs(true),
	fRecurseLinks(false),
	fSkipDotDirs(true),
	fCaseSensitive(false),
	fRegularExpression(false),
	fTextOnly(true),
	fInvokeEditor(false),

	fFrame(100, 100, 500, 400),

	fState(STATE_IDLE),

	fFilePanelPath(""),

	fShowLines(true),
	fEncoding(0)
{
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK)
		fFilePanelPath = path.Path();
	else
		fFilePanelPath = "/boot/home";
}


status_t
Model::LoadPrefs()
{
	BFile file;
	status_t status = _OpenFile(&file, PREFS_FILE);
	if (status != B_OK)
		return status;

	status = file.Lock();
	if (status != B_OK)
		return status;

	int32 value;

	if (file.ReadAttr("RecurseDirs", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fRecurseDirs = (value != 0);

	if (file.ReadAttr("RecurseLinks", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fRecurseLinks = (value != 0);

	if (file.ReadAttr("SkipDotDirs", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fSkipDotDirs = (value != 0);

	if (file.ReadAttr("CaseSensitive", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fCaseSensitive = (value != 0);

	if (file.ReadAttr("RegularExpression", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fRegularExpression = (value != 0);

	if (file.ReadAttr("TextOnly", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fTextOnly = (value != 0);

	if (file.ReadAttr("InvokeEditor", B_INT32_TYPE, 0, &value, sizeof(int32))
			> 0)
		fInvokeEditor = (value != 0);

	char buffer [B_PATH_NAME_LENGTH+1];
	int32 length = file.ReadAttr("FilePanelPath", B_STRING_TYPE, 0, &buffer,
		sizeof(buffer));
	if (length > 0) {
		buffer[length] = '\0';
		fFilePanelPath = buffer;
	}

	file.ReadAttr("WindowFrame", B_RECT_TYPE, 0, &fFrame, sizeof(BRect));

	if (file.ReadAttr("ShowLines", B_INT32_TYPE, 0, &value,
			sizeof(int32)) > 0)
		fShowLines = (value != 0);

	if (file.ReadAttr("Encoding", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fEncoding = value;

	file.Unlock();

	return B_OK;
}


status_t
Model::SavePrefs()
{
	BFile file;
	status_t status = _OpenFile(&file, PREFS_FILE,
		B_CREATE_FILE | B_WRITE_ONLY);
	if (status != B_OK)
		return status;

	status = file.Lock();
	if (status != B_OK)
		return status;

	int32 value = 2;
	file.WriteAttr("Version", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fRecurseDirs ? 1 : 0;
	file.WriteAttr("RecurseDirs", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fRecurseLinks ? 1 : 0;
	file.WriteAttr("RecurseLinks", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fSkipDotDirs ? 1 : 0;
	file.WriteAttr("SkipDotDirs", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fCaseSensitive ? 1 : 0;
	file.WriteAttr("CaseSensitive", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fRegularExpression ? 1 : 0;
	file.WriteAttr("RegularExpression", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fTextOnly ? 1 : 0;
	file.WriteAttr("TextOnly", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fInvokeEditor ? 1 : 0;
	file.WriteAttr("InvokeEditor", B_INT32_TYPE, 0, &value, sizeof(int32));

	file.WriteAttr("WindowFrame", B_RECT_TYPE, 0, &fFrame, sizeof(BRect));

	file.WriteAttr("FilePanelPath", B_STRING_TYPE, 0, fFilePanelPath.String(),
		fFilePanelPath.Length() + 1);

	value = fShowLines ? 1 : 0;
	file.WriteAttr("ShowLines", B_INT32_TYPE, 0, &value, sizeof(int32));

	file.WriteAttr("Encoding", B_INT32_TYPE, 0, &fEncoding, sizeof(int32));

	file.Sync();
	file.Unlock();

	return B_OK;
}


void
Model::AddToHistory(const char* text)
{
	BList items;
	_LoadHistory(items);

	BString* string = new (nothrow) BString(text);
	if (string == NULL || !items.AddItem(string)) {
		delete string;
		_FreeHistory(items);
		return;
	}

	int32 count = items.CountItems() - 1;
		// don't check last item, since that's the one we just added
	for (int32 t = 0; t < count; ++t) {
		// If the same text is already in the list,
		// then remove it first. Case-sensitive.
		BString* string = static_cast<BString*>(items.ItemAt(t));
		if (*string == text) {
			delete static_cast<BString*>(items.RemoveItem(t));
			break;
		}
	}

	while (items.CountItems() >= HISTORY_LIMIT)
		delete static_cast<BString*>(items.RemoveItem((int32)0));

	_SaveHistory(items);
	_FreeHistory(items);
}


void
Model::FillHistoryMenu(BMenu* menu) const
{
	BList items;
	if (!_LoadHistory(items))
		return;

	for (int32 t = items.CountItems() - 1; t >= 0; --t) {
		BString* item = static_cast<BString*>(items.ItemAtFast(t));
		BMessage* message = new BMessage(MSG_SELECT_HISTORY);
		message->AddString("text", item->String());
		menu->AddItem(new BMenuItem(item->String(), message));
	}

	_FreeHistory(items);
}


// #pragma mark - private


bool
Model::_LoadHistory(BList& items) const
{
	BFile file;
	status_t status = _OpenFile(&file, PREFS_FILE);
	if (status != B_OK)
		return false;

	status = file.Lock();
	if (status != B_OK)
		return false;

	BMessage message;
	status = message.Unflatten(&file);
	if (status != B_OK)
		return false;

	file.Unlock();

	BString string;
	for (int32 x = 0; message.FindString("string", x, &string) == B_OK; x++) {
		BString* copy = new (nothrow) BString(string);
		if (copy == NULL || !items.AddItem(copy)) {
			delete copy;
			break;
		}
	}

	return true;
}


status_t
Model::_SaveHistory(const BList& items) const
{
	BFile file;
	status_t status = _OpenFile(&file, PREFS_FILE,
		B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE,
		B_USER_SETTINGS_DIRECTORY, NULL);
	if (status != B_OK)
		return status;

	status = file.Lock();
	if (status != B_OK)
		return status;

	BMessage message;
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BString* string = static_cast<BString*>(items.ItemAtFast(i));

		if (message.AddString("string", string->String()) != B_OK)
			break;
	}

	status = message.Flatten(&file);
	file.SetSize(message.FlattenedSize());
	file.Sync();
	file.Unlock();

	return status;
}


void
Model::_FreeHistory(const BList& items) const
{
	for (int32 t = items.CountItems() - 1; t >= 0; --t)
		delete static_cast<BString*>((items.ItemAtFast(t)));
}


status_t
Model::_OpenFile(BFile* file, const char* name, uint32 openMode,
	directory_which which, BVolume* volume) const
{
	if (file == NULL)
		return B_BAD_VALUE;

	BPath path;
	status_t status = find_directory(which, &path, true, volume);
	if (status != B_OK)
		return status;

	status = path.Append(PREFS_FILE);
	if (status != B_OK)
		return status;

	status = file->SetTo(path.Path(), openMode);
	if (status != B_OK)
		return status;

	status = file->InitCheck();
	if (status != B_OK)
		return status;

	return B_OK;
}
