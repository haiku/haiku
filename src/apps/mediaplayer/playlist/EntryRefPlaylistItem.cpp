/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "EntryRefPlaylistItem.h"

#include <new>

#include <MediaFile.h>


EntryRefPlaylistItem::EntryRefPlaylistItem(const entry_ref& ref)
	:
	fRef(ref)
{
}


EntryRefPlaylistItem::~EntryRefPlaylistItem()
{
}


status_t
EntryRefPlaylistItem::SetName(const BString& name)
{
	BEntry entry(&fRef);

	status_t ret = entry.Rename(name.String(), false);
	if (ret != B_OK)
		return ret;

	entry.GetRef(&fRef);
	_NotifyListeners();
	return B_OK;
}



status_t
EntryRefPlaylistItem::GetName(BString& name) const
{
	name = fRef.name;
	return B_OK;
}


status_t
EntryRefPlaylistItem::SetTitle(const BString& title)
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetTitle(BString& title) const
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::SetAuthor(const BString& author)
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetAuthor(BString& author) const
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::SetAlbum(const BString& album)
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetAlbum(BString& album) const
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::SetTrackNumber(int32 trackNumber)
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetTrackNumber(int32& trackNumber) const
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::SetBitRate(int32 bitRate)
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetBitRate(int32& bitRate) const
{
	return B_NOT_SUPPORTED;
}


status_t
EntryRefPlaylistItem::GetDuration(bigtime_t& duration) const
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


status_t
EntryRefPlaylistItem::MoveIntoTrash()
{
	return B_NOT_SUPPORTED;
}



status_t
EntryRefPlaylistItem::RestoreFromTrash()
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


BMediaFile*
EntryRefPlaylistItem::CreateMediaFile() const
{
	return new (std::nothrow) BMediaFile(&fRef);
}

