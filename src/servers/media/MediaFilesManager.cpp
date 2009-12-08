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


static const char* kSettingsDirectory = "Media";
static const char* kSettingsFile = "MediaFiles";
static const uint32 kSettingsWhat = 'mfil';


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

	_LoadState();
#if DEBUG >=3
	Dump();
#endif
}


MediaFilesManager::~MediaFilesManager()
{
	CALLED();
	delete fSaveTimerRunner;
}


status_t
MediaFilesManager::SaveState()
{
	CALLED();
	BMessage settings(kSettingsWhat);
	status_t status;

	TypeMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++) {
		const BString& type = iterator->first;
		FileMap& fileMap = iterator->second;

		BMessage items;
		status = items.AddString("type", type.String());
		if (status != B_OK)
			return status;

		FileMap::iterator fileIterator = fileMap.begin();
		for (; fileIterator != fileMap.end(); fileIterator++) {
			const BString& item = fileIterator->first;
			BPath path(&fileIterator->second);

			status = items.AddString("item", item.String());
			if (status == B_OK) {
				status = items.AddString("path",
					path.Path() ? path.Path() : "");
			}
			if (status != B_OK)
				return status;
		}

		status = settings.AddMessage("type items", &items);
		if (status != B_OK)
			return status;
	}

	BFile file;
	status = _OpenSettingsFile(file,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	return settings.Flatten(&file);
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


status_t
MediaFilesManager::_OpenSettingsFile(BFile& file, int mode)
{
	bool createFile = (mode & O_ACCMODE) != O_RDONLY;
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path,
		createFile);
	if (status == B_OK)
		status = path.Append(kSettingsDirectory);
	if (status == B_OK && createFile) {
		status = create_directory(path.Path(),
			S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	}
	if (status == B_OK)
		status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	return file.SetTo(path.Path(), mode);
}


//! This is called by the media_server *before* any add-ons have been loaded.
status_t
MediaFilesManager::_LoadState()
{
	CALLED();

	BFile file;
	status_t status = _OpenSettingsFile(file, B_READ_ONLY);
	if (status != B_OK)
		return status;

	BMessage settings;
	status = settings.Unflatten(&file);
	if (status != B_OK)
		return status;

	if (settings.what != kSettingsWhat)
		return B_BAD_TYPE;

	BMessage items;
	for (int32 i = 0; settings.FindMessage("type items", i, &items) == B_OK;
			i++) {
		const char* type;
		if (items.FindString("type", &type) != B_OK)
			continue;

		const char* item;
		for (int32 j = 0; items.FindString("item", j, &item) == B_OK; j++) {
			const char* path;
			if (items.FindString("path", j, &path) != B_OK)
				return B_BAD_DATA;

			entry_ref ref;
			get_ref_for_path(path, &ref);
				// it's okay for this to fail

			SetRefFor(type, item, ref, false);
		}
	}

	return B_OK;
}
