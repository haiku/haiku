/*
 * Copyright 2009-2010, Stephan Aßmus <supertippi@gmx.de>. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, Maurice Kalinowski. All rights reserved.
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_EXTRACTOR_H
#define _MEDIA_EXTRACTOR_H


#include "ReaderPlugin.h"
#include "DecoderPlugin.h"


namespace BPrivate {
namespace media {


class ChunkCache;
struct chunk_buffer;


struct stream_info {
	status_t		status;
	void*			cookie;
	bool			hasCookie;
	const void*		infoBuffer;
	size_t			infoBufferSize;
	ChunkCache*		chunkCache;
	chunk_buffer*	lastChunk;
	media_format	encodedFormat;
};


class MediaExtractor {
public:
								MediaExtractor(BDataIO* source, int32 flags);

								~MediaExtractor();

			status_t			InitCheck();

			void				GetFileFormatInfo(
									media_file_format* fileFormat) const;
			status_t			GetMetaData(BMessage* _data) const;

			int32				StreamCount();

			const char*			Copyright();

			const media_format*	EncodedFormat(int32 stream);
			int64				CountFrames(int32 stream) const;
			bigtime_t			Duration(int32 stream) const;

			status_t			Seek(int32 stream, uint32 seekTo,
									int64* _frame, bigtime_t* _time);
			status_t			FindKeyFrame(int32 stream, uint32 seekTo,
									int64* _frame, bigtime_t* _time) const;

			status_t			GetNextChunk(int32 stream,
									const void** _chunkBuffer,
									size_t* _chunkSize,
									media_header* mediaHeader);

			status_t			CreateDecoder(int32 stream, Decoder** _decoder,
									media_codec_info* codecInfo);

			status_t			GetStreamMetaData(int32 stream,
									BMessage* _data) const;

			void				StopProcessing();


private:
			void				_Init(BDataIO* source, int32 flags);

			void				_RecycleLastChunk(stream_info& info);
	static	int32				_ExtractorEntry(void* arg);
			void				_ExtractorThread();

private:
			status_t			fInitStatus;

			sem_id				fExtractorWaitSem;
			thread_id			fExtractorThread;

			BDataIO*			fSource;
			Reader*				fReader;

			stream_info*		fStreamInfo;
			int32				fStreamCount;

			media_file_format	fFileFormat;
};

} // namespace media
} // namespace BPrivate

using namespace BPrivate::media;

#endif	// _MEDIA_EXTRACTOR_H
