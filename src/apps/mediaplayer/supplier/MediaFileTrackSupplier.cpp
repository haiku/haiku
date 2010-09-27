/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
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


MediaFileTrackSupplier::MediaFileTrackSupplier(BMediaFile* mediaFile)
	:
	TrackSupplier(),
	fMediaFile(mediaFile)
{
	if (fMediaFile->InitCheck() != B_OK)
		return;
	int trackCount = fMediaFile->CountTracks();
	if (trackCount <= 0)
		return;

	for (int i = 0; i < trackCount; i++) {
		BMediaTrack* track = fMediaFile->TrackAt(i);
		media_format format;
		status_t status = track->EncodedFormat(&format);
		if (status != B_OK) {
			fprintf(stderr, "MediaFileTrackSupplier: EncodedFormat failed for "
				"track index %d, error: %s\n", i, strerror(status));
			fMediaFile->ReleaseTrack(track);
			continue;
		}

		if (track->Duration() <= 0) {
			fprintf(stderr, "MediaFileTrackSupplier: warning! track index %d "
				"has no duration\n", i);
		}

		if (format.IsAudio()) {
			if (!fAudioTracks.AddItem(track)) {
				fMediaFile->ReleaseTrack(track);
				return;
			}
		} else if (format.IsVideo()) {
			if (!fVideoTracks.AddItem(track)) {
				fMediaFile->ReleaseTrack(track);
				return;
			}
		} else {
			printf("MediaFileTrackSupplier: track index %d has unknown "
				"type\n", i);
			fMediaFile->ReleaseTrack(track);
		}
	}
}


MediaFileTrackSupplier::~MediaFileTrackSupplier()
{
	delete fMediaFile;
		// BMediaFile destructor will call ReleaseAllTracks()
 	for (int32 i = fSubTitleTracks.CountItems() - 1; i >= 0; i--)
 		delete reinterpret_cast<SubTitles*>(fSubTitleTracks.ItemAtFast(i));
 }


status_t
MediaFileTrackSupplier::InitCheck()
{
	return fMediaFile->InitCheck();
}


status_t
MediaFileTrackSupplier::GetFileFormatInfo(media_file_format* fileFormat)
{
	return fMediaFile->GetFileFormatInfo(fileFormat);
}


status_t
MediaFileTrackSupplier::GetCopyright(BString* copyright)
{
	*copyright = fMediaFile->Copyright();
	return B_OK;
}


status_t
MediaFileTrackSupplier::GetMetaData(BMessage* metaData)
{
	return fMediaFile->GetMetaData(metaData);
}


int32
MediaFileTrackSupplier::CountAudioTracks()
{
	return fAudioTracks.CountItems();
}


int32
MediaFileTrackSupplier::CountVideoTracks()
{
	return fVideoTracks.CountItems();
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
	BMediaTrack* track = (BMediaTrack*)fVideoTracks.ItemAt(index);
	if (track == NULL)
		return NULL;

	status_t initStatus;
	MediaTrackVideoSupplier* supplier
		= new(std::nothrow) MediaTrackVideoSupplier(track, index, initStatus);
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

