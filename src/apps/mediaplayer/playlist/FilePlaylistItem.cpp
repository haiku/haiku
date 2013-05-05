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
#include <TranslationUtils.h>

#include "MediaFileTrackSupplier.h"
#include "SubTitlesSRT.h"

static const char* kPathKey = "path";

FilePlaylistItem::FilePlaylistItem(const entry_ref& ref)
{
	fRefs.push_back(ref);
	fNamesInTrash.push_back("");
}


FilePlaylistItem::FilePlaylistItem(const FilePlaylistItem& other)
	:
	fRefs(other.fRefs),
	fNamesInTrash(other.fNamesInTrash),
	fImageRefs(other.fImageRefs),
	fImageNamesInTrash(other.fImageNamesInTrash)
{
}


FilePlaylistItem::FilePlaylistItem(const BMessage* archive)
{
	const char* path;
	entry_ref	ref;
	if (archive != NULL) {
		int32 i = 0;
		while (archive->FindString(kPathKey, i, &path) == B_OK) {
			if (get_ref_for_path(path, &ref) == B_OK) {
				fRefs.push_back(ref);
			}
			i++;
		}
	}
	if (fRefs.empty()) {
		fRefs.push_back(entry_ref());
	}
	for (vector<entry_ref>::size_type i = 0; i < fRefs.size(); i++) {
		fNamesInTrash.push_back("");
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
	for (vector<entry_ref>::size_type i = 0; i < fRefs.size(); i++) {
		BPath path(&fRefs[i]);
		ret = path.InitCheck();
		if (ret != B_OK)
			return ret;
		ret = into->AddString(kPathKey, path.Path());
		if (ret != B_OK)
			return ret;
	}
	return B_OK;
}


status_t
FilePlaylistItem::SetAttribute(const Attribute& attribute,
	const BString& string)
{
	switch (attribute) {
		case ATTR_STRING_NAME:
		{
			BEntry entry(&fRefs[0], false);
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
		string = fRefs[0].name;
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
	BPath path(&fRefs[0]);
	BString locationURI("file://");
	locationURI << path.Path();
	return locationURI;
}


status_t
FilePlaylistItem::GetIcon(BBitmap* bitmap, icon_size iconSize) const
{
	BNode node(&fRefs[0]);
	BNodeInfo info(&node);
	return info.GetTrackerIcon(bitmap, iconSize);
}


status_t
FilePlaylistItem::MoveIntoTrash()
{
	if (fNamesInTrash[0].Length() != 0) {
		// Already in the trash!
		return B_ERROR;
	}

	status_t err;
	err = _MoveIntoTrash(&fRefs, &fNamesInTrash);
	if (err != B_OK)
		return err;
	
	if (fImageRefs.empty())
		return B_OK;

	err = _MoveIntoTrash(&fImageRefs, &fImageNamesInTrash);
	if (err != B_OK)
		return err;

	return B_OK;
}


status_t
FilePlaylistItem::RestoreFromTrash()
{
	if (fNamesInTrash[0].Length() <= 0) {
		// Not in the trash!
		return B_ERROR;
	}

	status_t err;
	err = _RestoreFromTrash(&fRefs, &fNamesInTrash);
	if (err != B_OK)
		return err;
	
	if (fImageRefs.empty())
		return B_OK;

	err = _RestoreFromTrash(&fImageRefs, &fImageNamesInTrash);
	if (err != B_OK)
		return err;

	return B_OK;
}


// #pragma mark -

TrackSupplier*
FilePlaylistItem::CreateTrackSupplier() const
{
	MediaFileTrackSupplier* supplier
		= new(std::nothrow) MediaFileTrackSupplier();
	if (supplier == NULL)
		return NULL;

	for (vector<entry_ref>::size_type i = 0; i < fRefs.size(); i++) {
		BMediaFile* mediaFile = new(std::nothrow) BMediaFile(&fRefs[i]);
		if (mediaFile == NULL) {
			delete supplier;
			return NULL;
		}
		if (supplier->AddMediaFile(mediaFile) != B_OK)
			delete mediaFile;
	}

	for (vector<entry_ref>::size_type i = 0; i < fImageRefs.size(); i++) {
		BBitmap* bitmap = BTranslationUtils::GetBitmap(&fImageRefs[i]);
		if (bitmap == NULL)
			continue;
		if (supplier->AddBitmap(bitmap) != B_OK)
			delete bitmap;
	}

	// Search for subtitle files in the same folder
	// TODO: Error checking
	BEntry entry(&fRefs[0], true);

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
FilePlaylistItem::AddRef(const entry_ref& ref)
{
	fRefs.push_back(ref);
	fNamesInTrash.push_back("");
	return B_OK;
}


status_t
FilePlaylistItem::AddImageRef(const entry_ref& ref)
{
	fImageRefs.push_back(ref);
	fImageNamesInTrash.push_back("");
	return B_OK;
}


const entry_ref&
FilePlaylistItem::ImageRef() const
{
	static entry_ref ref;

	if (fImageRefs.empty())
		return ref;
	
	return fImageRefs[0];
}


status_t
FilePlaylistItem::_SetAttribute(const char* attrName, type_code type,
	const void* data, size_t size)
{
	BEntry entry(&fRefs[0], true);
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
	BEntry entry(&fRefs[0], true);
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


status_t
FilePlaylistItem::_MoveIntoTrash(vector<entry_ref>* refs,
	vector<BString>* namesInTrash)
{
	char trashPath[B_PATH_NAME_LENGTH];
	status_t err = find_directory(B_TRASH_DIRECTORY, (*refs)[0].device,
		true /*create it*/, trashPath, B_PATH_NAME_LENGTH);
	if (err != B_OK) {
		fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
		return err;
	}

	BDirectory trashDir(trashPath);
	err = trashDir.InitCheck();
	if (err != B_OK) {
		fprintf(stderr, "failed to init BDirectory for %s: %s\n",
			trashPath, strerror(err));
		return err;
	}

	for (vector<entry_ref>::size_type i = 0; i < refs->size(); i++) {
		BEntry entry(&(*refs)[i]);
		err = entry.InitCheck();
		if (err != B_OK) {
			fprintf(stderr, "failed to init BEntry for %s: %s\n",
				(*refs)[i].name, strerror(err));
			return err;
		}
	
		// Find a unique name for the entry in the trash
		(*namesInTrash)[i] = (*refs)[i].name;
		int32 uniqueNameIndex = 1;
		while (true) {
			BEntry test(&trashDir, (*namesInTrash)[i].String());
			if (!test.Exists())
				break;
			(*namesInTrash)[i] = (*refs)[i].name;
			(*namesInTrash)[i] << ' ' << uniqueNameIndex;
			uniqueNameIndex++;
		}
	
		// Remember the original path
		BPath originalPath;
		entry.GetPath(&originalPath);
	
		// Finally, move the entry into the trash
		err = entry.MoveTo(&trashDir, (*namesInTrash)[i].String());
		if (err != B_OK) {
			fprintf(stderr, "failed to move entry into trash %s: %s\n",
				trashPath, strerror(err));
			return err;
		}
	
		// Allow Tracker to restore this entry
		BNode node(&entry);
		BString originalPathString(originalPath.Path());
		node.WriteAttrString("_trk/original_path", &originalPathString);
	}

	return B_OK;
}


status_t
FilePlaylistItem::_RestoreFromTrash(vector<entry_ref>* refs,
	vector<BString>* namesInTrash)
{
	char trashPath[B_PATH_NAME_LENGTH];
	status_t err = find_directory(B_TRASH_DIRECTORY, (*refs)[0].device,
		false /*create it*/, trashPath, B_PATH_NAME_LENGTH);
	if (err != B_OK) {
		fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
		return err;
	}
	
	for (vector<entry_ref>::size_type i = 0; i < refs->size(); i++) {
		// construct the entry to the file in the trash
		// TODO: BEntry(const BDirectory* directory, const char* path) is broken!
		//	BEntry entry(trashPath, (*namesInTrash)[i].String());
		BPath path(trashPath, (*namesInTrash)[i].String());
		BEntry entry(path.Path());
		err = entry.InitCheck();
		if (err != B_OK) {
			fprintf(stderr, "failed to init BEntry for %s: %s\n",
				(*namesInTrash)[i].String(), strerror(err));
			return err;
		}
		//entry.GetPath(&path);
		//printf("moving '%s'\n", path.Path());
	
		// construct the folder of the original entry_ref
		node_ref nodeRef;
		nodeRef.device = (*refs)[i].device;
		nodeRef.node = (*refs)[i].directory;
		BDirectory originalDir(&nodeRef);
		err = originalDir.InitCheck();
		if (err != B_OK) {
			fprintf(stderr, "failed to init original BDirectory for "
				"%s: %s\n", (*refs)[i].name, strerror(err));
			return err;
		}
	
		//path.SetTo(&originalDir, fItems[i].name);
		//printf("as '%s'\n", path.Path());
	
		// Reset the name here, the user may have already moved the entry
		// out of the trash via Tracker for example.
		(*namesInTrash)[i] = "";
	
		// Finally, move the entry back into the original folder
		err = entry.MoveTo(&originalDir, (*refs)[i].name);
		if (err != B_OK) {
			fprintf(stderr, "failed to restore entry from trash "
				"%s: %s\n", (*refs)[i].name, strerror(err));
			return err;
		}
	
		// Remove the attribute that helps Tracker restore the entry.
		BNode node(&entry);
		node.RemoveAttr("_trk/original_path");
	}

	return B_OK;
}

