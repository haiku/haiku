/*
 * Copyright 2016, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "UrlPlaylistItem.h"

#include <MediaFile.h>
#include <MediaTrack.h>

#include "MediaFileTrackSupplier.h"


UrlPlaylistItem::UrlPlaylistItem(BUrl url)
	:
	fUrl(url),
	fDuration(-1)
{
}


UrlPlaylistItem::UrlPlaylistItem(const UrlPlaylistItem& item)
{
	fUrl = BUrl(item.Url());
}


UrlPlaylistItem::UrlPlaylistItem(const BMessage* archive)
{
	const char* url = NULL;
	if (archive->FindString("mediaplayer:url", &url) == B_OK)
		fUrl = BUrl(url, true);
}


UrlPlaylistItem::~UrlPlaylistItem()
{
}


PlaylistItem*
UrlPlaylistItem::Clone() const
{
	return new UrlPlaylistItem(fUrl);
}


BArchivable*
UrlPlaylistItem::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "UrlPlaylistItem"))
		return new (std::nothrow) UrlPlaylistItem(archive);

	return NULL;
}


status_t
UrlPlaylistItem::Archive(BMessage* into, bool deep) const
{
	return into->AddString("mediaplayer:url", fUrl.UrlString());
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const BString& string)
{
	return B_ERROR;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, BString& string) const
{
	if (attribute == ATTR_STRING_NAME) {
		string = fUrl.UrlString();
		return B_OK;
	}

	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const int32& value)
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, int32& value) const
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const int64& value)
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, int64& value) const
{
	if (attribute == ATTR_INT64_DURATION && fDuration >= 0) {
		value = fDuration;
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::SetAttribute(const Attribute& attribute, const float& value)
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::GetAttribute(const Attribute& attribute, float& value) const
{
	return B_NOT_SUPPORTED;
}


BString
UrlPlaylistItem::LocationURI() const
{
	return fUrl.UrlString();
}


status_t
UrlPlaylistItem::GetIcon(BBitmap* bitmap, icon_size iconSize) const
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::MoveIntoTrash()
{
	return B_NOT_SUPPORTED;
}


status_t
UrlPlaylistItem::RestoreFromTrash()
{
	return B_NOT_SUPPORTED;
}


bigtime_t
UrlPlaylistItem::_CalculateDuration()
{
	if (fDuration < 0) {
		BMediaFile mediaFile(fUrl);

		if (mediaFile.InitCheck() != B_OK || mediaFile.CountTracks() < 1)
			return 0;
		fDuration = mediaFile.TrackAt(0)->Duration();
	}
	return fDuration;
}


TrackSupplier*
UrlPlaylistItem::_CreateTrackSupplier() const
{
	MediaFileTrackSupplier* supplier
		= new(std::nothrow) MediaFileTrackSupplier();
	if (supplier == NULL)
		return NULL;

	BMediaFile* mediaFile = new(std::nothrow) BMediaFile(fUrl);
	if (mediaFile == NULL || supplier->AddMediaFile(mediaFile) != B_OK) {
		delete mediaFile;
		delete supplier;
		return NULL;
	}

	return supplier;
}


BUrl
UrlPlaylistItem::Url() const
{
	return fUrl;
}
