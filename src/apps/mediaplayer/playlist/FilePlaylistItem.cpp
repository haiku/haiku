/*
 * Copyright © 2009 Stephan Aßmus <superstippi@gmx.de>
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
	return B_NOT_SUPPORTED;
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
	return B_NOT_SUPPORTED;
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


BMediaFile*
FilePlaylistItem::CreateMediaFile() const
{
	return new (std::nothrow) BMediaFile(&fRef);
}

