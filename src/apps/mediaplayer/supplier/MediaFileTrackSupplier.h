/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MEDIA_FILE_TRACK_SUPPLIER_H
#define MEDIA_FILE_TRACK_SUPPLIER_H

#include <vector>

#include <Bitmap.h>
#include <List.h>

#include "TrackSupplier.h"


class BMediaFile;
using std::vector;


class MediaFileTrackSupplier : public TrackSupplier {
public:
								MediaFileTrackSupplier();
	virtual						~MediaFileTrackSupplier();

	virtual	status_t			InitCheck();

	virtual	status_t			GetFileFormatInfo(
									media_file_format* fileFormat);
	virtual	status_t			GetCopyright(BString* copyright);
	virtual	status_t			GetMetaData(BMessage* metaData);

	virtual	int32				CountAudioTracks();
	virtual	int32				CountVideoTracks();
	virtual	int32				CountSubTitleTracks();

	virtual	status_t			GetAudioMetaData(int32 index,
									BMessage* metaData);
	virtual	status_t			GetVideoMetaData(int32 index,
									BMessage* metaData);

	virtual	AudioTrackSupplier*	CreateAudioTrackForIndex(int32 index);
	virtual	VideoTrackSupplier*	CreateVideoTrackForIndex(int32 index);
	virtual	const SubTitles*	SubTitleTrackForIndex(int32 index);

			bool				AddSubTitles(SubTitles* subTitles);

			status_t			AddMediaFile(BMediaFile* mediaFile);

			status_t			AddBitmap(BBitmap* bitmap);

private:
			vector<BMediaFile*>	fMediaFiles;
			vector<BBitmap*>	fBitmaps;
			BList				fAudioTracks;
			BList				fVideoTracks;
			BList				fSubTitleTracks;
};


#endif // MEDIA_FILE_TRACK_SUPPLIER_H
