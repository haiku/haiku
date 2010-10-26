/*
	Copyright 2010, Haiku, Inc. All Rights Reserved.
	This file may be used under the terms of the MIT License.
*/
#ifndef MEDIA_FILE_INFO_H
#define MEDIA_FILE_INFO_H


#include <MediaFile.h>
#include <String.h>


struct MediaInfo {
	BString format;
	BString details;
};


struct MediaFileInfo {
				MediaFileInfo(BMediaFile* file = NULL);
	status_t	LoadInfo(BMediaFile* file);

	MediaInfo	audio;
	MediaInfo	video;
	BString		duration;
	bigtime_t	useconds;

private:
	void		_Reset();
};

#endif // MEDIA_FILE_INFO_H
