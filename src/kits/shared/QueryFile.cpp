/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <QueryFile.h>

#include <fs_attr.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "tracker/MimeTypes.h"
#include "tracker/Utilities.h"


// TODO: add write support
// TODO: let Tracker use it?
// TODO: live query support?


const char*	kAttrQueryString = "_trk/qrystr";
const char* kAttrQueryVolume = "_trk/qryvol1";


BQueryFile::BQueryFile(const entry_ref& ref)
{
	SetTo(ref);
}


BQueryFile::BQueryFile(const BEntry& entry)
{
	SetTo(entry);
}


BQueryFile::BQueryFile(const char* path)
{
	SetTo(path);
}


BQueryFile::BQueryFile(BQuery& query)
{
	SetTo(query);
}


BQueryFile::~BQueryFile()
{
}


status_t
BQueryFile::InitCheck() const
{
	return fStatus;
}


status_t
BQueryFile::SetTo(const entry_ref& ref)
{
	Unset();

	BNode node(&ref);
	fStatus = node.InitCheck();
	if (fStatus != B_OK)
		return fStatus;

	ssize_t bytesRead = node.ReadAttrString(kAttrQueryString, &fPredicate);
	if (bytesRead < 0)
		return fStatus = bytesRead;

	bool searchAllVolumes = true;
	attr_info info;
	if (node.GetAttrInfo(kAttrQueryVolume, &info) == B_OK) {
		void* buffer = malloc(info.size);
		if (buffer == NULL)
			return fStatus = B_NO_MEMORY;

		BMessage message;
		fStatus = message.Unflatten((const char*)buffer);
		if (fStatus == B_OK) {
			for (int32 index = 0; index < 100; index++) {
				BVolume volume;
				status_t status = BPrivate::MatchArchivedVolume(&volume,
					&message, index);
				if (status == B_OK) {
					fStatus = AddVolume(volume);
					if (fStatus != B_OK)
						break;

					searchAllVolumes = false;
				} else if (status != B_DEV_BAD_DRIVE_NUM) {
					// Volume doesn't seem to be mounted
					fStatus = status;
					break;
				}
			}
		}

		free(buffer);
	}

	if (searchAllVolumes) {
		// add all volumes to query
		BVolumeRoster roster;
		BVolume volume;
		while (roster.GetNextVolume(&volume) == B_OK) {
			if (volume.IsPersistent() && volume.KnowsQuery())
				AddVolume(volume);
		}
	}

	return fStatus;
}


status_t
BQueryFile::SetTo(const BEntry& entry)
{
	entry_ref ref;
	fStatus = entry.GetRef(&ref);
	if (fStatus != B_OK)
		return fStatus;

	return SetTo(ref);
}


status_t
BQueryFile::SetTo(const char* path)
{
	entry_ref ref;
	fStatus = get_ref_for_path(path, &ref);
	if (fStatus != B_OK)
		return fStatus;

	return SetTo(ref);
}


status_t
BQueryFile::SetTo(BQuery& query)
{
	Unset();

	BString predicate;
	query.GetPredicate(&predicate);

	fStatus = SetPredicate(predicate.String());
	if (fStatus != B_OK)
		return fStatus;

	return fStatus = AddVolume(query.TargetDevice());
}


void
BQueryFile::Unset()
{
	fStatus = B_NO_INIT;
	fCurrentVolumeIndex = -1;
	fVolumes.MakeEmpty();
	fQuery.Clear();
	fPredicate = "";
}


status_t
BQueryFile::SetPredicate(const char* predicate)
{
	fPredicate = predicate;
	return B_OK;
}


status_t
BQueryFile::AddVolume(const BVolume& volume)
{
	return fVolumes.AddItem((void*)(addr_t)volume.Device()) ? B_OK : B_NO_MEMORY;
}


status_t
BQueryFile::AddVolume(dev_t device)
{
	return fVolumes.AddItem((void*)(addr_t)device) ? B_OK : B_NO_MEMORY;
}


const char*
BQueryFile::Predicate() const
{
	return fPredicate.String();
}


int32
BQueryFile::CountVolumes() const
{
	return fVolumes.CountItems();
}


dev_t
BQueryFile::VolumeAt(int32 index) const
{
	if (index < 0 || index >= fVolumes.CountItems())
		return -1;

	return (dev_t)(addr_t)fVolumes.ItemAt(index);
}


status_t
BQueryFile::WriteTo(const entry_ref& ref)
{
	// TODO: implement
	return B_NOT_SUPPORTED;
}


status_t
BQueryFile::WriteTo(const char* path)
{
	entry_ref ref;
	status_t status = get_ref_for_path(path, &ref);
	if (status != B_OK)
		return status;

	return WriteTo(ref);
}


// #pragma mark - BEntryList implementation


status_t
BQueryFile::GetNextEntry(BEntry* entry, bool traverse)
{
	if (fCurrentVolumeIndex == -1) {
		// Start with first volume
		fCurrentVolumeIndex = 0;

		status_t status = _SetQuery(0);
		if (status != B_OK)
			return status;
	}

	status_t status = B_ENTRY_NOT_FOUND;

	while (fCurrentVolumeIndex < CountVolumes()) {
		status = fQuery.GetNextEntry(entry, traverse);
		if (status != B_ENTRY_NOT_FOUND)
			break;

		// Continue with next volume, if any
		status = _SetQuery(++fCurrentVolumeIndex);
	}

	return status;
}


status_t
BQueryFile::GetNextRef(entry_ref* ref)
{
	if (fCurrentVolumeIndex == -1) {
		// Start with first volume
		fCurrentVolumeIndex = 0;

		status_t status = _SetQuery(0);
		if (status != B_OK)
			return status;
	}

	status_t status = B_ENTRY_NOT_FOUND;

	while (fCurrentVolumeIndex < CountVolumes()) {
		status = fQuery.GetNextRef(ref);
		if (status != B_ENTRY_NOT_FOUND)
			break;

		// Continue with next volume, if any
		status = _SetQuery(++fCurrentVolumeIndex);
	}

	return status;
}


int32
BQueryFile::GetNextDirents(struct dirent* buffer, size_t length, int32 count)
{
	if (fCurrentVolumeIndex == -1) {
		// Start with first volume
		fCurrentVolumeIndex = 0;

		status_t status = _SetQuery(0);
		if (status != B_OK)
			return status;
	}

	status_t status = B_ENTRY_NOT_FOUND;

	while (fCurrentVolumeIndex < CountVolumes()) {
		status = fQuery.GetNextDirents(buffer, length, count);
		if (status != B_ENTRY_NOT_FOUND)
			break;

		// Continue with next volume, if any
		status = _SetQuery(++fCurrentVolumeIndex);
	}

	return status;
}


status_t
BQueryFile::Rewind()
{
	fCurrentVolumeIndex = -1;
	return B_OK;
}


int32
BQueryFile::CountEntries()
{
	// not supported
	return -1;
}


/*static*/ const char*
BQueryFile::MimeType()
{
	return B_QUERY_MIMETYPE;
}


status_t
BQueryFile::_SetQuery(int32 index)
{
	if (fCurrentVolumeIndex >= CountVolumes())
		return B_ENTRY_NOT_FOUND;

	BVolume volume(VolumeAt(fCurrentVolumeIndex));
	fQuery.Clear();
	fQuery.SetPredicate(fPredicate.String());
	fQuery.SetVolume(&volume);

	return fQuery.Fetch();
}
