/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MEDIA_FILE_TRACK_SUPPLIER_H
#define MEDIA_FILE_TRACK_SUPPLIER_H


#include <List.h>

#include "TrackSupplier.h"


class BMediaFile;


class MediaFileTrackSupplier : public TrackSupplier {
public:
								MediaFileTrackSupplier(BMediaFile* mediaFile);
	virtual						~MediaFileTrackSupplier();

	virtual	status_t			InitCheck();

	virtual	status_t			GetFileFormatInfo(
									media_file_format* fileFormat);
	virtual	status_t			GetCopyright(BString* copyright);
	virtual	status_t			GetMetaData(BMessage* metaData);

	virtual	int32				CountAudioTracks();
	virtual	int32				CountVideoTracks();

	virtual	status_t			GetAudioMetaData(int32 index,
									BMessage* metaData);
	virtual	status_t			GetVideoMetaData(int32 index,
									BMessage* metaData);

	virtual	AudioTrackSupplier*	CreateAudioTrackForIndex(int32 index);
	virtual	VideoTrackSupplier*	CreateVideoTrackForIndex(int32 index);

private:
			BMediaFile*			fMediaFile;
			BList				fAudioTracks;
			BList				fVideoTracks;
};


#endif // MEDIA_FILE_TRACK_SUPPLIER_H
