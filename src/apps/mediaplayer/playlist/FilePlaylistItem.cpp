/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FilePlaylistItem.h"

#include <stdio.h>

#include <new>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MediaFile.h>
#include <Path.h>

#include "MediaFileTrackSupplier.h"
#include "SubTitlesSRT.h"


static const char* kPathKey = "path";


FilePlaylistItem::FilePlaylistItem(const entry_ref& ref)
	:
	fRef(ref),
	fNameInTrash("")
{
}


FilePlaylistItem::FilePlaylistItem(const FilePlaylistItem& other)
	:
	fRef(other.fRef),
	fNameInTrash(other.fNameInTrash)
{
}


FilePlaylistItem::FilePlaylistItem(const BMessage* archive)
	:
	fRef(),
	fNameInTrash("")
{
	const char* path;
	if (archive != NULL && archive->FindString(kPathKey, &path) == B_OK) {
		if (get_ref_for_path(path, &fRef) != B_OK)
			fRef = entry_ref();
	}
}


FilePlaylistItem::~FilePlaylistItem()
{
}


PlaylistItem*
FilePlaylistItem::Clone() const
{
	return new (std::nothrow) FilePlaylistItem(*this);
}


BArchivable*
FilePlaylistItem::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "FilePlaylistItem"))
		return new (std::nothrow) FilePlaylistItem(archive);

	return NULL;
}


// #pragma mark -


status_t
FilePlaylistItem::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	if (ret != B_OK)
		return ret;
	BPath path(&fRef);
	ret = path.InitCheck();
	if (ret == B_OK)
		ret = into->AddString(kPathKey, path.Path());
	return ret;
}


status_t
FilePlaylistItem::SetAttribute(const Attribute& attribute,
	const BString& string)
{
	switch (attribute) {
		case ATTR_STRING_NAME:
		{
			BEntry entry(&fRef, false);
			return entry.Rename(string.String(), false);
		}
	
		case ATTR_STRING_KEYWORDS:
			return _SetAttribute("Meta:Keywords", B_STRING_TYPE,
				string.String(), string.Length());

		case ATTR_STRING_ARTIST:
			return _SetAttribute("Audio:Artist", B_STRING_TYPE,
				string.String(), string.Length());
		case ATTR_STRING_AUTHOR:
			return _SetAttribute("Media:Author", B_STRING_TYPE,
				string.String(), string.Length());
		case ATTR_STRING_ALBUM:
			return _SetAttribute("Audio:Album", B_STRING_TYPE,
				string.String(), string.Length());
		case ATTR_STRING_TITLE:
			return _SetAttribute("Media:Title", B_STRING_TYPE,
				string.String(), string.Length());
		case ATTR_STRING_AUDIO_BITRATE:
			return _SetAttribute("Audio:Bitrate", B_STRING_TYPE,
				string.String(), string.Length());
		case ATTR_STRING_VIDEO_BITRATE:
			return _SetAttribute("Video:Bitrate", B_STRING_TYPE,
				string.String(), string.Length());

		default:
			return B_NOT_SUPPORTED;
	}
}


status_t
FilePlaylistItem::GetAttribute(const Attribute& attribute,
	BString& string) const
{
	if (attribute == ATTR_STRING_NAME) {
		string = fRef.name;
		return B_OK;
	}

	return B_NOT_SUPPORTED;
}


status_t
FilePlaylistItem::SetAttribute(const Attribute& attribute,
	const int32& value)
{
	switch (attribute) {
		case ATTR_INT32_TRACK:
			return _SetAttribute("Audio:Track", B_INT32_TYPE, &value,
				sizeof(int32));
		case ATTR_INT32_YEAR:
			return _SetAttribute("Media:Year", B_INT32_TYPE, &value,
				sizeof(int32));
		case ATTR_INT32_RATING:
			return _SetAttribute("Media:Rating", B_INT32_TYPE, &value,
				sizeof(int32));

		default:
			return B_NOT_SUPPORTED;
	}
}


status_t
FilePlaylistItem::GetAttribute(const Attribute& attribute,
	int32& value) const
{
	return B_NOT_SUPPORTED;
}


status_t
FilePlaylistItem::SetAttribute(const Attribute& attribute,
	const int64& value)
{
	return B_NOT_SUPPORTED;
}


status_t
FilePlaylistItem::GetAttribute(const Attribute& attribute,
	int64& value) const
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


BString
FilePlaylistItem::LocationURI() const
{
	BPath path(&fRef);
	BString locationURI("file://");
	locationURI << path.Path();
	return locationURI;
}


status_t
FilePlaylistItem::GetIcon(BBitmap* bitmap, icon_size iconSize) const
{
	BNode node(&fRef);
	BNodeInfo info(&node);
	return info.GetTrackerIcon(bitmap, iconSize);
}


status_t
FilePlaylistItem::MoveIntoTrash()
{
	if (fNameInTrash.Length() != 0) {
		// Already in the trash!
		return B_ERROR;
	}

	char trashPath[B_PATH_NAME_LENGTH];
	status_t err = find_directory(B_TRASH_DIRECTORY, fRef.device,
		true /*create it*/, trashPath, B_PATH_NAME_LENGTH);
	if (err != B_OK) {
		fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
		return err;
	}

	BEntry entry(&fRef);
	err = entry.InitCheck();
	if (err != B_OK) {
		fprintf(stderr, "failed to init BEntry for %s: %s\n",
			fRef.name, strerror(err));
		return err;
	}
	BDirectory trashDir(trashPath);
	if (err != B_OK) {
		fprintf(stderr, "failed to init BDirectory for %s: %s\n",
			trashPath, strerror(err));
		return err;
	}

	// Find a unique name for the entry in the trash
	fNameInTrash = fRef.name;
	int32 uniqueNameIndex = 1;
	while (true) {
		BEntry test(&trashDir, fNameInTrash.String());
		if (!test.Exists())
			break;
		fNameInTrash = fRef.name;
		fNameInTrash << ' ' << uniqueNameIndex;
		uniqueNameIndex++;
	}

	// Remember the original path
	BPath originalPath;
	entry.GetPath(&originalPath);

	// Finally, move the entry into the trash
	err = entry.MoveTo(&trashDir, fNameInTrash.String());
	if (err != B_OK) {
		fprintf(stderr, "failed to move entry into trash %s: %s\n",
			trashPath, strerror(err));
		return err;
	}

	// Allow Tracker to restore this entry
	BNode node(&entry);
	BString originalPathString(originalPath.Path());
	node.WriteAttrString("_trk/original_path", &originalPathString);

	return err;
}



status_t
FilePlaylistItem::RestoreFromTrash()
{
	if (fNameInTrash.Length() <= 0) {
		// Not in the trash!
		return B_ERROR;
	}

	char trashPath[B_PATH_NAME_LENGTH];
	status_t err = find_directory(B_TRASH_DIRECTORY, fRef.device,
		false /*create it*/, trashPath, B_PATH_NAME_LENGTH);
	if (err != B_OK) {
		fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
		return err;
	}
	// construct the entry to the file in the trash
// TODO: BEntry(const BDirectory* directory, const char* path) is broken!
//	BEntry entry(trashPath, fNamesInTrash[i].String());
BPath path(trashPath, fNameInTrash.String());
BEntry entry(path.Path());
	err = entry.InitCheck();
	if (err != B_OK) {
		fprintf(stderr, "failed to init BEntry for %s: %s\n",
			fNameInTrash.String(), strerror(err));
		return err;
	}
//entry.GetPath(&path);
//printf("moving '%s'\n", path.Path());

	// construct the folder of the original entry_ref
	node_ref nodeRef;
	nodeRef.device = fRef.device;
	nodeRef.node = fRef.directory;
	BDirectory originalDir(&nodeRef);
	err = originalDir.InitCheck();
	if (err != B_OK) {
		fprintf(stderr, "failed to init original BDirectory for "
			"%s: %s\n", fRef.name, strerror(err));
		return err;
	}

//path.SetTo(&originalDir, fItems[i].name);
//printf("as '%s'\n", path.Path());

	// Reset the name here, the user may have already moved the entry
	// out of the trash via Tracker for example.
	fNameInTrash = "";

	// Finally, move the entry back into the original folder
	err = entry.MoveTo(&originalDir, fRef.name);
	if (err != B_OK) {
		fprintf(stderr, "failed to restore entry from trash "
			"%s: %s\n", fRef.name, strerror(err));
		return err;
	}

	// Remove the attribute that helps Tracker restore the entry.
	BNode node(&entry);
	node.RemoveAttr("_trk/original_path");

	return err;
}


// #pragma mark -


TrackSupplier*
FilePlaylistItem::CreateTrackSupplier() const
{
	BMediaFile* mediaFile = new(std::nothrow) BMediaFile(&fRef);
	if (mediaFile == NULL)
		return NULL;
	MediaFileTrackSupplier* supplier
		= new(std::nothrow) MediaFileTrackSupplier(mediaFile);
	if (supplier == NULL) {
		delete mediaFile;
		return NULL;
	}

	// Search for subtitle files in the same folder
	// TODO: Error checking
	BEntry entry(&fRef, true);

	char originalName[B_FILE_NAME_LENGTH];
	entry.GetName(originalName);
	BString nameWithoutExtension(originalName);
	int32 extension = nameWithoutExtension.FindLast('.');
	if (extension > 0)
		nameWithoutExtension.Truncate(extension);

	BPath path;
	entry.GetPath(&path);
	path.GetParent(&path);
	BDirectory directory(path.Path());
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) != B_OK)
			continue;
		BString nameString(name);
		if (nameString == originalName)
			continue;
		if (nameString.IFindFirst(nameWithoutExtension) < 0)
			continue;

		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			continue;

		int32 pos = nameString.FindLast('.');
		if (pos < 0)
			continue;

		BString extensionString(nameString.String() + pos + 1);
		extensionString.ToLower();

		BString language = "default";
		if (pos > 1) {
			int32 end = pos;
			while (pos > 0 && *(nameString.String() + pos - 1) != '.')
				pos--;
			language.SetTo(nameString.String() + pos, end - pos);
		}

		if (extensionString == "srt") {
			SubTitles* subTitles
				= new(std::nothrow) SubTitlesSRT(&file, language.String());
			if (subTitles != NULL && !supplier->AddSubTitles(subTitles))
				delete subTitles;
		}
	}

	return supplier;
}


status_t
FilePlaylistItem::_SetAttribute(const char* attrName, type_code type,
	const void* data, size_t size)
{
	BEntry entry(&fRef, true);
	BNode node(&entry);
	if (node.InitCheck() != B_OK)
		return node.InitCheck();

	ssize_t written = node.WriteAttr(attrName, type, 0, data, size);
	if (written != (ssize_t)size) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}
	return B_OK;
}


status_t
FilePlaylistItem::_GetAttribute(const char* attrName, type_code type,
	void* data, size_t size)
{
	BEntry entry(&fRef, true);
	BNode node(&entry);
	if (node.InitCheck() != B_OK)
		return node.InitCheck();

	ssize_t read = node.ReadAttr(attrName, type, 0, data, size);
	if (read != (ssize_t)size) {
		if (read < 0)
			return (status_t)read;
		return B_IO_ERROR;
	}
	return B_OK;
}

