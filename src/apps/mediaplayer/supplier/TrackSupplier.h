/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRACK_SUPPLIER_H
#define TRACK_SUPPLIER_H


#include <MediaDefs.h>
#include <MediaFormats.h>

#include "AudioTrackSupplier.h"
#include "SubTitles.h"
#include "VideoTrackSupplier.h"


class BMessage;
class BString;


class TrackSupplier {
public:
								TrackSupplier();
	virtual						~TrackSupplier();

	virtual	status_t			InitCheck() = 0;

	virtual	status_t			GetFileFormatInfo(
									media_file_format* fileFormat) = 0;
	virtual	status_t			GetCopyright(BString* copyright) = 0;
	virtual	status_t			GetMetaData(BMessage* metaData) = 0;

	virtual	int32				CountAudioTracks() = 0;
	virtual	int32				CountVideoTracks() = 0;
	virtual	int32				CountSubTitleTracks() = 0;

	virtual	status_t			GetAudioMetaData(int32 index,
									BMessage* metaData) = 0;
	virtual	status_t			GetVideoMetaData(int32 index,
									BMessage* metaData) = 0;

	virtual	AudioTrackSupplier*	CreateAudioTrackForIndex(int32 index) = 0;
	virtual	VideoTrackSupplier*	CreateVideoTrackForIndex(int32 index) = 0;
	virtual	const SubTitles*	SubTitleTrackForIndex(int32 index) = 0;
};


#endif // TRACK_SUPPLIER_H
