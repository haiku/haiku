/*
 * Copyright 2010-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 * 		SHINTA
 */
#include "MediaFileTrackSupplier.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <MediaFile.h>
#include <MediaTrack.h>
#include <String.h>

#include "MediaTrackAudioSupplier.h"
#include "MediaTrackVideoSupplier.h"
#include "ImageTrackVideoSupplier.h"


MediaFileTrackSupplier::MediaFileTrackSupplier()
	:
	TrackSupplier()
{
}


MediaFileTrackSupplier::~MediaFileTrackSupplier()
{
	for (vector<BMediaFile*>::size_type i = fMediaFiles.size() - 1;
		static_cast<int32>(i) >= 0; i--) {
		delete fMediaFiles[i];
		// BMediaFile destructor will call ReleaseAllTracks()
	}

	for (vector<BBitmap*>::size_type i = fBitmaps.size() - 1;
		static_cast<int32>(i) >= 0; i--) {
		delete fBitmaps[i];
	}

 	for (int32 i = fSubTitleTracks.CountItems() - 1; i >= 0; i--)
 		delete reinterpret_cast<SubTitles*>(fSubTitleTracks.ItemAtFast(i));
 }


status_t
MediaFileTrackSupplier::InitCheck()
{
	if (fMediaFiles.empty())
		return B_ERROR;

	return fMediaFiles[0]->InitCheck();
}


status_t
MediaFileTrackSupplier::GetFileFormatInfo(media_file_format* fileFormat)
{
	if (fMediaFiles.empty())
		return B_ERROR;

	return fMediaFiles[0]->GetFileFormatInfo(fileFormat);
}


status_t
MediaFileTrackSupplier::GetCopyright(BString* copyright)
{
	if (fMediaFiles.empty())
		return B_ERROR;

	*copyright = fMediaFiles[0]->Copyright();
	return B_OK;
}


status_t
MediaFileTrackSupplier::GetMetaData(BMessage* metaData)
{
	if (fMediaFiles.empty())
		return B_ERROR;

	return fMediaFiles[0]->GetMetaData(metaData);
}


int32
MediaFileTrackSupplier::CountAudioTracks()
{
	return fAudioTracks.CountItems();
}


int32
MediaFileTrackSupplier::CountVideoTracks()
{
	return fVideoTracks.CountItems() + fBitmaps.size();
}


int32
MediaFileTrackSupplier::CountSubTitleTracks()
{
	return fSubTitleTracks.CountItems();
}


status_t
MediaFileTrackSupplier::GetAudioMetaData(int32 index, BMessage* metaData)
{
	BMediaTrack* track = (BMediaTrack*)fAudioTracks.ItemAt(index);
	if (track == NULL)
		return B_BAD_INDEX;

	return track->GetMetaData(metaData);
}


status_t
MediaFileTrackSupplier::GetVideoMetaData(int32 index, BMessage* metaData)
{
	BMediaTrack* track = (BMediaTrack*)fVideoTracks.ItemAt(index);
	if (track == NULL)
		return B_BAD_INDEX;

	return track->GetMetaData(metaData);
}


AudioTrackSupplier*
MediaFileTrackSupplier::CreateAudioTrackForIndex(int32 index)
{
	BMediaTrack* track = (BMediaTrack*)fAudioTracks.ItemAt(index);
	if (track == NULL)
		return NULL;

	return new(std::nothrow) MediaTrackAudioSupplier(track, index);
}


VideoTrackSupplier*
MediaFileTrackSupplier::CreateVideoTrackForIndex(int32 index)
{
	VideoTrackSupplier* supplier;
	status_t initStatus;

	if (fVideoTracks.CountItems() <= index
			&& index < fVideoTracks.CountItems() + static_cast<int32>(fBitmaps.size())) {
		supplier = new(std::nothrow) ImageTrackVideoSupplier(
			fBitmaps[index - fVideoTracks.CountItems()], index, initStatus);
	} else {
		BMediaTrack* track = (BMediaTrack*)fVideoTracks.ItemAt(index);
		if (track == NULL)
			return NULL;

		supplier = new(std::nothrow) MediaTrackVideoSupplier(track, index,
			initStatus);
	}

	if (initStatus != B_OK) {
		delete supplier;
		return NULL;
	}
	return supplier;
}


const SubTitles*
MediaFileTrackSupplier::SubTitleTrackForIndex(int32 index)
{
	return reinterpret_cast<SubTitles*>(fSubTitleTracks.ItemAt(index));
}


bool
MediaFileTrackSupplier::AddSubTitles(SubTitles* subTitles)
{
	return fSubTitleTracks.AddItem(subTitles);
}


status_t
MediaFileTrackSupplier::AddMediaFile(BMediaFile* mediaFile)
{
	if (mediaFile->InitCheck() != B_OK)
		return B_ERROR;
	int trackCount = mediaFile->CountTracks();
	if (trackCount <= 0)
		return B_ERROR;

	status_t	funcStatus = B_ERROR;
	for (int i = 0; i < trackCount; i++) {
		BMediaTrack* track = mediaFile->TrackAt(i);
		media_format format;
		status_t status = track->EncodedFormat(&format);
		if (status != B_OK) {
			fprintf(stderr, "MediaFileTrackSupplier: EncodedFormat failed for "
				"track index %d, error: %s\n", i, strerror(status));
			mediaFile->ReleaseTrack(track);
			continue;
		}

		if (track->Duration() <= 0) {
			fprintf(stderr, "MediaFileTrackSupplier: warning! track index %d "
				"has no duration\n", i);
		}

		if (format.IsAudio()) {
			if (fAudioTracks.AddItem(track)) {
				funcStatus = B_OK;
			} else {
				mediaFile->ReleaseTrack(track);
			}
		} else if (format.IsVideo()) {
			if (fVideoTracks.AddItem(track)) {
				funcStatus = B_OK;
			} else {
				mediaFile->ReleaseTrack(track);
			}
		} else {
			printf("MediaFileTrackSupplier: track index %d has unknown "
				"type\n", i);
			mediaFile->ReleaseTrack(track);
		}
	}
	if (funcStatus == B_OK)
		fMediaFiles.push_back(mediaFile);
	return funcStatus;
}


status_t
MediaFileTrackSupplier::AddBitmap(BBitmap* bitmap)
{
	fBitmaps.push_back(bitmap);
	return B_OK;
}


