/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FILE_PLAYLIST_ITEM_H
#define FILE_PLAYLIST_ITEM_H

#include "PlaylistItem.h"

#include <vector>

#include <Entry.h>

using std::vector;


class FilePlaylistItem : public PlaylistItem {
public:
								FilePlaylistItem(const entry_ref& ref);
								FilePlaylistItem(const FilePlaylistItem& item);
								FilePlaylistItem(const BMessage* archive);
	virtual						~FilePlaylistItem();

	virtual	PlaylistItem*		Clone() const;

	// archiving
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	// attributes
	virtual	status_t			SetAttribute(const Attribute& attribute,
									const BString& string);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									BString& string) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int32& value);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int32& value) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int64& value);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int64& value) const;

	// methods
	virtual	BString				LocationURI() const;
	virtual	status_t			GetIcon(BBitmap* bitmap,
									icon_size iconSize) const;

	virtual	status_t			MoveIntoTrash();
	virtual	status_t			RestoreFromTrash();

	// playback
	virtual	TrackSupplier*		CreateTrackSupplier() const;

			status_t			AddRef(const entry_ref& ref);
			const entry_ref&	Ref() const { return fRefs[0]; }

			status_t			AddImageRef(const entry_ref& ref);
			const entry_ref&	ImageRef() const;

private:
			status_t			_SetAttribute(const char* attrName,
									type_code type, const void* data,
									size_t size);
			status_t			_GetAttribute(const char* attrName,
									type_code type, void* data,
									size_t size);
			status_t			_MoveIntoTrash(vector<entry_ref>* refs,
									vector<BString>* namesInTrash);
			status_t			_RestoreFromTrash(vector<entry_ref>* refs,
									vector<BString>* namesInTrash);

private:
	// always fRefs.size() == fNamesInTrash.size()
			vector<entry_ref>	fRefs;
			vector<BString>		fNamesInTrash;
	// always fImageRefs.size() == fImageNamesInTrash.size()
			vector<entry_ref>	fImageRefs;
			vector<BString>		fImageNamesInTrash;
};

#endif // FILE_PLAYLIST_ITEM_H
