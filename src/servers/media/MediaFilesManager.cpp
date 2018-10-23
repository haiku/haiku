/*
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
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

#include <MediaDebug.h>
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

	static const struct {
		const char* type;
		const char* item;
	} kInitialItems[] = {
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_BEEP},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_STARTUP},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_DOWN},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_REPEAT},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_KEY_UP},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_MOUSE_DOWN},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_MOUSE_UP},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_ACTIVATED},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_CLOSE},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_MINIMIZED},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_OPEN},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_RESTORED},
		{MEDIA_TYPE_SOUNDS, MEDIA_SOUNDS_WINDOW_ZOOMED}
	};

	for (size_t i = 0; i < sizeof(kInitialItems) / sizeof(kInitialItems[0]);
			i++) {
		_SetItem(kInitialItems[i].type, kInitialItems[i].item);
	}

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
		ItemMap& itemMap = iterator->second;

		BMessage items;
		status = items.AddString("type", type.String());
		if (status != B_OK)
			return status;

		ItemMap::iterator itemIterator = itemMap.begin();
		for (; itemIterator != itemMap.end(); itemIterator++) {
			const BString& item = itemIterator->first;
			item_info& info = itemIterator->second;

			status = items.AddString("item", item.String());
			if (status == B_OK) {
				BPath path(&info.ref);
				status = items.AddString("path",
					path.Path() ? path.Path() : "");
			}
			if (status == B_OK)
				status = items.AddFloat("gain", info.gain);
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
		ItemMap& itemMap = iterator->second;

		ItemMap::iterator fileIterator = itemMap.begin();
		for (; fileIterator != itemMap.end(); fileIterator++) {
			const BString& item = fileIterator->first;
			const item_info& info = fileIterator->second;

			BPath path(&info.ref);

			printf(" type \"%s\", item \"%s\", path \"%s\", gain %g\n",
				type.String(), item.String(),
				path.InitCheck() == B_OK ? path.Path() : "INVALID",
				info.gain);
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

	ItemMap& itemMap = found->second;
	count = itemMap.size();

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

	ItemMap::iterator iterator = itemMap.begin();
	for (; iterator != itemMap.end();
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

	item_info* info;
	status_t status = _GetItem(type, item, info);
	if (status == B_OK)
		*_ref = &info->ref;

	return status;
}


status_t
MediaFilesManager::GetAudioGainFor(const char* type, const char* item,
	float* _gain)
{
	CALLED();
	BAutolock _(this);

	item_info* info;
	status_t status = _GetItem(type, item, info);
	if (status == B_OK)
		*_gain = info->gain;

	return status;
}


status_t
MediaFilesManager::SetRefFor(const char* type, const char* item,
	const entry_ref& ref)
{
	CALLED();
	TRACE("MediaFilesManager::SetRefFor %s %s\n", type, item);

	BAutolock _(this);

	status_t status = _SetItem(type, item, &ref);
	if (status == B_OK)
		_LaunchTimer();

	return status;
}


status_t
MediaFilesManager::SetAudioGainFor(const char* type, const char* item,
	float gain)
{
	CALLED();
	TRACE("MediaFilesManager::SetAudioGainFor %s %s %g\n", type, item, gain);

	BAutolock _(this);

	status_t status = _SetItem(type, item, NULL, &gain);
	if (status == B_OK)
		_LaunchTimer();

	return status;
}


status_t
MediaFilesManager::InvalidateItem(const char* type, const char* item)
{
	CALLED();
	BAutolock _(this);

	TypeMap::iterator found = fMap.find(type);
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	ItemMap& itemMap = found->second;
	itemMap[item] = item_info();

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
	if (GetRefFor(type, name, &ref) != B_OK) {
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


/*!	You need to have the manager locked when calling this method.
*/
status_t
MediaFilesManager::_GetItem(const char* type, const char* item,
	item_info*& info)
{
	ASSERT(IsLocked());

	TypeMap::iterator found = fMap.find(type);
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	ItemMap::iterator foundFile = found->second.find(item);
	if (foundFile == found->second.end())
		return B_NAME_NOT_FOUND;

	info = &foundFile->second;
	return B_OK;
}


/*!	You need to have the manager locked when calling this method after
	launch.
*/
status_t
MediaFilesManager::_SetItem(const char* _type, const char* _item,
	const entry_ref* ref, const float* gain)
{
	CALLED();
	TRACE("MediaFilesManager::_SetItem(%s, %s)\n", _type, _item);

	BString type(_type);
	type.Truncate(B_MEDIA_NAME_LENGTH);
	BString item(_item);
	item.Truncate(B_MEDIA_NAME_LENGTH);

	try {
		TypeMap::iterator found = fMap.find(type);
		if (found == fMap.end()) {
			// add new type
			ItemMap itemMap;
			// TODO: For some reason, this does not work:
			//found = fMap.insert(TypeMap::value_type(type, itemMap));
			fMap[type] = itemMap;
			found = fMap.find(type);
		}

		ItemMap& itemMap = found->second;
		item_info info = itemMap[item];

		// only update what we've got
		if (gain != NULL)
			info.gain = *gain;
		if (ref != NULL)
			info.ref = *ref;

		itemMap[item] = info;
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	return B_OK;
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

			float gain;
			if (items.FindFloat("gain", j, &gain) != B_OK)
				gain = 1.0f;

			entry_ref ref;
			get_ref_for_path(path, &ref);
				// it's okay for this to fail

			_SetItem(type, item, &ref, &gain);
		}
	}

	return B_OK;
}
