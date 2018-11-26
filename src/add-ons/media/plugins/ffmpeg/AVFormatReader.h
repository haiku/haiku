/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */
#ifndef AV_FORMAT_READER_H
#define AV_FORMAT_READER_H


#include <Reader.h>
#include <Locker.h>
#include <String.h>

using namespace BCodecKit;


class AVFormatReader : public BReader {
public:
								AVFormatReader();
								~AVFormatReader();

	virtual	status_t			Sniff(int32* streamCount);

	virtual	void				GetFileFormatInfo(media_file_format* mff);
	virtual	status_t			GetMetaData(BMetaData* data);

	virtual	status_t			AllocateCookie(int32 streamNumber,
									void** cookie);
	virtual	status_t			FreeCookie(void* cookie);

	virtual	status_t			GetStreamInfo(void* cookie, int64* frameCount,
									bigtime_t* duration, media_format* format,
									const void** infoBuffer,
									size_t* infoSize);

	virtual	status_t			GetStreamMetaData(void* cookie,
									BMetaData* data);

	virtual	status_t			Seek(void* cookie, uint32 flags, int64* frame,
									bigtime_t* time);
	virtual	status_t			FindKeyFrame(void* cookie, uint32 flags,
									int64* frame, bigtime_t* time);

	virtual	status_t			GetNextChunk(void* cookie,
									const void** chunkBuffer,
									size_t* chunkSize,
									media_header* mediaHeader);

private:
			class Stream;

			BString				fCopyright;
			Stream**			fStreams;
			BLocker				fSourceLock;
};


#endif // AV_FORMAT_READER_H
