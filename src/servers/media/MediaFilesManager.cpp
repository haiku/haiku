/*
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MediaFilesManager.h"

#include <string.h>

#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <MediaFiles.h>
#include <Path.h>

#include <debug.h>
#include <MediaSounds.h>


const char* kMediaFilesManagerSettingsDirectory = "Media";
const char* kMediaFilesManagerSettingsFile = "MediaFilesManager";

const uint32 kHeader[] = {0xac00150c, 0x18723462, 0x00000001};


MediaFilesManager::MediaFilesManager()
	:
	BLocker("media files manager"),
	fSaveTimerRunner(NULL)
{
	CALLED();
	entry_ref ref;
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_BEEP, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_STARTUP, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_DOWN, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_REPEAT, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_UP, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_MOUSE_DOWN, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_MOUSE_UP, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_ACTIVATED, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_CLOSE, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_MINIMIZED, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_OPEN, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_RESTORED, ref, false);
	SetRefFor(MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_ZOOMED, ref, false);

	LoadState();
#if DEBUG >=3
	Dump();
#endif
}


MediaFilesManager::~MediaFilesManager()
{
	CALLED();
	delete fSaveTimerRunner;
}


//! This is called by the media_server *before* any add-ons have been loaded.
status_t
MediaFilesManager::LoadState()
{
	CALLED();
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append(kMediaFilesManagerSettingsDirectory);
	if (status == B_OK)
		status = path.Append(kMediaFilesManagerSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	uint32 header[3];
	if (file.Read(header, sizeof(header)) < (int32)sizeof(header))
		return B_ERROR;
	if (memcmp(header, kHeader, sizeof(kHeader)) != 0)
		return B_ERROR;

	uint32 categoryCount;
	if (file.Read(&categoryCount, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;

	while (categoryCount--) {
		BString type;
		ssize_t length = _ReadSettingsString(file, type);
		if (length < 0)
			return length;
		if (length == 0)
			break;

		TRACE("%s {\n", type.String());

		do {
			BString key;
			length = _ReadSettingsString(file, key);
			if (length < 0)
				return length;
			if (length == 0)
				break;

			BString value;
			length = _ReadSettingsString(file, value);

			entry_ref ref;
			if (length > 1)
				get_ref_for_path(value.String(), &ref);

			SetRefFor(type, key.String(), ref, false);
		} while (true);

		TRACE("}\n");
	}

	return B_OK;
}


status_t
MediaFilesManager::SaveState()
{
	CALLED();
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append(kMediaFilesManagerSettingsDirectory);
	if (status == B_OK) {
		status = create_directory(path.Path(),
			S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	}
	if (status == B_OK)
		status = path.Append(kMediaFilesManagerSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (status != B_OK)
		return status;

	if (file.Write(kHeader, sizeof(kHeader)) < (ssize_t)sizeof(kHeader))
		return B_ERROR;

	BAutolock _(this);

	uint32 categoryCount = fMap.size();
	if (file.Write(&categoryCount, sizeof(uint32)) < (ssize_t)sizeof(uint32))
		return B_ERROR;

	uint32 zero = 0;

	TypeMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++) {
		const BString& type = iterator->first;
		FileMap& fileMap = iterator->second;

		_WriteSettingsString(file, type.String());

		FileMap::iterator fileIterator = fileMap.begin();
		for (; fileIterator != fileMap.end(); fileIterator++) {
			const BString& item = fileIterator->first;
			BPath path(&fileIterator->second);

			_WriteSettingsString(file, item.String());
			_WriteSettingsString(file, path.Path() ? path.Path() : "");
		}

		file.Write(&zero, sizeof(uint32));
	}
	file.Write(&zero, sizeof(uint32));

	return B_OK;
}


void
MediaFilesManager::Dump()
{
	BAutolock _(this);

	printf("MediaFilesManager: types follow\n");

	TypeMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++) {
		const BString& type = iterator->first;
		FileMap& fileMap = iterator->second;

		FileMap::iterator fileIterator = fileMap.begin();
		for (; fileIterator != fileMap.end(); fileIterator++) {
			const BString& item = fileIterator->first;
			BPath path(&fileIterator->second);

			printf(" type \"%s\", item \"%s\", path \"%s\"\n",
				type.String(), item.String(),
				path.InitCheck() == B_OK ? path.Path() : "INVALID");
		}
	}

	printf("MediaFilesManager: list end\n");
}


area_id
MediaFilesManager::GetTypesArea(int32& count)
{
	CALLED();
	BAutolock _(this);

	count = fMap.size();

	size_t size = (count * B_MEDIA_NAME_LENGTH + B_PAGE_SIZE - 1)
		& ~(B_PAGE_SIZE - 1);

	char* start;
	area_id area = create_area("media types", (void**)&start, B_ANY_ADDRESS,
		size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		ERROR("MediaFilesManager::GetTypesArea(): failed to create area: %s\n",
			strerror(area));
		count = 0;
		return area;
	}

	TypeMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++, start += B_MEDIA_NAME_LENGTH) {
		const BString& type = iterator->first;
		strncpy(start, type.String(), B_MEDIA_NAME_LENGTH);
	}

	return area;
}


area_id
MediaFilesManager::GetItemsArea(const char* type, int32& count)
{
	CALLED();
	if (type == NULL)
		return B_BAD_VALUE;

	BAutolock _(this);

	TypeMap::iterator found = fMap.find(BString(type));
	if (found == fMap.end()) {
		count = 0;
		return B_NAME_NOT_FOUND;
	}

	FileMap& fileMap = found->second;
	count = fileMap.size();

	size_t size = (count * B_MEDIA_NAME_LENGTH + B_PAGE_SIZE - 1)
		& ~(B_PAGE_SIZE - 1);

	char* start;
	area_id area = create_area("media refs", (void**)&start, B_ANY_ADDRESS,
		size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		ERROR("MediaFilesManager::GetRefsArea(): failed to create area: %s\n",
			strerror(area));
		count = 0;
		return area;
	}

	FileMap::iterator iterator = fileMap.begin();
	for (; iterator != fileMap.end();
			iterator++, start += B_MEDIA_NAME_LENGTH) {
		const BString& item = iterator->first;
		strncpy(start, item.String(), B_MEDIA_NAME_LENGTH);
	}

	return area;
}


status_t
MediaFilesManager::GetRefFor(const char* type, const char* item,
	entry_ref** _ref)
{
	CALLED();
	BAutolock _(this);

	TypeMap::iterator found = fMap.find(BString(type));
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	FileMap::iterator foundFile = found->second.find(item);
	if (foundFile == found->second.end())
		return B_NAME_NOT_FOUND;

	*_ref = &foundFile->second;
	return B_OK;
}


status_t
MediaFilesManager::SetRefFor(const char* _type, const char* _item,
	const entry_ref& ref, bool save)
{
	CALLED();
	TRACE("MediaFilesManager::SetRefFor %s %s\n", _type, _item);

	BString type(_type);
	type.Truncate(B_MEDIA_NAME_LENGTH);
	BString item(_item);
	item.Truncate(B_MEDIA_NAME_LENGTH);

	BAutolock _(this);

	try {
		TypeMap::iterator found = fMap.find(type);
		if (found == fMap.end()) {
			// add new type
			FileMap fileMap;
			// TODO: For some reason, this does not work:
			//found = fMap.insert(TypeMap::value_type(type, fileMap));
			fMap[type] = fileMap;
			found = fMap.find(type);
		}

		FileMap& fileMap = found->second;
		fileMap[item] = ref;
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	if (save)
		_LaunchTimer();

	return B_OK;
}


status_t
MediaFilesManager::InvalidateRefFor(const char* type, const char* item)
{
	CALLED();
	BAutolock _(this);

	TypeMap::iterator found = fMap.find(type);
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	FileMap& fileMap = found->second;

	entry_ref emptyRef;
	fileMap[item] = emptyRef;

	_LaunchTimer();
	return B_OK;
}


status_t
MediaFilesManager::RemoveItem(const char *type, const char *item)
{
	CALLED();
	BAutolock _(this);

	TypeMap::iterator found = fMap.find(type);
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	found->second.erase(item);
	if (found->second.empty())
		fMap.erase(found);

	_LaunchTimer();
	return B_OK;
}


void
MediaFilesManager::TimerMessage()
{
	SaveState();

	delete fSaveTimerRunner;
	fSaveTimerRunner = NULL;
}


void
MediaFilesManager::HandleAddSystemBeepEvent(BMessage* message)
{
	const char* name;
	const char* type;
	uint32 flags;
	if (message->FindString(MEDIA_NAME_KEY, &name) != B_OK
		|| message->FindString(MEDIA_TYPE_KEY, &type) != B_OK
		|| message->FindInt32(MEDIA_FLAGS_KEY, (int32 *)&flags) != B_OK) {
		message->SendReply(B_BAD_VALUE);
		return;
	}

	entry_ref* ref = NULL;
	if (GetRefFor(type, name, &ref) == B_ENTRY_NOT_FOUND) {
		entry_ref newRef;
		SetRefFor(type, name, newRef);
	}
}


void
MediaFilesManager::_LaunchTimer()
{
	if (fSaveTimerRunner == NULL) {
		BMessage timer(MEDIA_FILES_MANAGER_SAVE_TIMER);
		fSaveTimerRunner = new BMessageRunner(be_app, &timer, 3 * 1000000LL, 1);
	}
}


/*static*/ int32
MediaFilesManager::_ReadSettingsString(BFile& file, BString& string)
{
	uint32 length;
	if (file.Read(&length, 4) < 4)
		return -1;

	if (length == 0) {
		string.Truncate(0);
		return 0;
	}

	char* buffer = string.LockBuffer(length);
	if (buffer == NULL)
		return -1;

	ssize_t bytesRead = file.Read(buffer, length);
	string.UnlockBuffer(length);

	if (bytesRead < (ssize_t)length)
		return -1;

	return length;
}


status_t
MediaFilesManager::_WriteSettingsString(BFile& file, const char* string)
{
	if (string == NULL)
		return B_BAD_VALUE;

	uint32 length = strlen(string);
	if (file.Write(&length, 4) < 4)
		return B_ERROR;

	if (length == 0)
		return B_OK;

	if (file.Write(string, length) < (ssize_t)length)
		return B_ERROR;

	return B_OK;
}
