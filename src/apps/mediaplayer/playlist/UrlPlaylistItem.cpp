/*
 * Copyright 2016, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "UrlPlaylistItem.h"

#include <MediaFile.h>

#include "MediaFileTrackSupplier.h"


UrlPlaylistItem::UrlPlaylistItem(BUrl url)
	:
	fUrl(url)
{

}


UrlPlaylistItem::UrlPlaylistItem(const UrlPlaylistItem& item)
{
	fUrl = BUrl(item.Url());
}


UrlPlaylistItem::UrlPlaylistItem(const BMessage* archive)
{
}


UrlPlaylistItem::~UrlPlaylistItem()
{
	delete fUrl;
}


PlaylistItem*
UrlPlaylistItem::Clone() const
{
	return new UrlPlaylistItem(fUrl);
}


BArchivable*
UrlPlaylistItem::Instantiate(BMessage* archive)
{
	return new UrlPlaylistItem(archive);
}


status_t
UrlPlaylistItem::Archive(BMessage* into, bool deep) const
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const BString& string)
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, BString& string) const
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const int32& value)
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, int32& value) const
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const int64& value)
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, int64& value) const
{
	return B_ERROR;
}


BString
UrlPlaylistItem::LocationURI() const
{
	return fUrl.UrlString();
}


status_t
UrlPlaylistItem::GetIcon(BBitmap* bitmap, icon_size iconSize) const
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::MoveIntoTrash()
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::RestoreFromTrash()
{
	return B_ERROR;
}


TrackSupplier*
UrlPlaylistItem::CreateTrackSupplier() const
{
	MediaFileTrackSupplier* supplier
		= new(std::nothrow) MediaFileTrackSupplier();
	if (supplier == NULL)
		return NULL;

	BMediaFile* mediaFile = new(std::nothrow) BMediaFile(fUrl);
	if (mediaFile == NULL) {
		delete supplier;
		return NULL;
	}
	if (supplier->AddMediaFile(mediaFile) != B_OK)
			delete mediaFile;

	return supplier;
}


BUrl
UrlPlaylistItem::Url() const
{
	return fUrl;
}
