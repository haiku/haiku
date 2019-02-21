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


#include <Decoder.h>
#include <MediaStreamer.h>
#include <Reader.h>


namespace BCodecKit {


struct stream_info;


class BMediaExtractor {
public:
								BMediaExtractor(BDataIO* source, int32 flags);
								// TODO
								//BMediaExtractor(BMediaStreamer* streamer);
								~BMediaExtractor();

			status_t			InitCheck() const;

			BDataIO*			Source() const;

			void				GetFileFormatInfo(
									media_file_format* fileFormat) const;

			status_t			GetMetaData(BMetaData* data) const;
			status_t			GetStreamMetaData(int32 stream,
									BMetaData* data) const;

			int32				CountStreams() const;

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

			status_t			CreateDecoder(int32 stream, BDecoder** _decoder,
									media_codec_info* codecInfo);

			// TODO: Explore if would be better to add a Start/Stop
			// methods pair to MediaExtractor.
			void				StopProcessing();

private:
			void				_Init(BDataIO* source, int32 flags);

			void				_RecycleLastChunk(stream_info& info);
	static	int32				_ExtractorEntry(void* arg);
			void				_ExtractorThread();

			status_t			fInitStatus;

			sem_id				fExtractorWaitSem;
			thread_id			fExtractorThread;

			BDataIO*			fSource;
			BReader*			fReader;

			stream_info*		fStreamInfo;
			int32				fStreamCount;

			media_file_format	fFileFormat;

			uint32				fReserved[5];
};


} // namespace BCodecKit


#endif	// _MEDIA_EXTRACTOR_H
