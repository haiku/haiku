/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_ENCODER_H
#define _MEDIA_ENCODER_H


#include <MediaFormats.h>


namespace BCodecKit {
	class BEncoder;
}


class BMediaEncoder {
public:
								BMediaEncoder();
								BMediaEncoder(
									const media_format* outputFormat);
								BMediaEncoder(const media_codec_info* info);
	virtual						~BMediaEncoder();

			status_t			InitCheck() const;

			status_t			SetTo(const media_format* outputFormat);
			status_t			SetTo(const media_codec_info* info);
			status_t			SetFormat(media_format* inputFormat,
									media_format* outputFormat,
									media_file_format* fileFormat = NULL);

			status_t			Encode(const void* buffer, int64 frameCount,
									media_encode_info* info);

			status_t			GetEncodeParameters(
									encode_parameters* parameters) const;
			status_t			SetEncodeParameters(
									encode_parameters* parameters);

protected:
	virtual	status_t			WriteChunk(const void* buffer, size_t size,
									media_encode_info* info) = 0;

	virtual	status_t			AddTrackInfo(uint32 code, const char* data,
									size_t size);

	// TODO: Needs Perform() method for FBC!

private:
	// FBC padding and forbidden methods
	virtual	status_t			_Reserved_BMediaEncoder_0(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_1(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_2(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_3(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_4(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_5(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_6(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_7(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_8(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_9(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_10(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_11(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_12(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_13(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_14(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaEncoder_15(int32 arg, ...);

								BMediaEncoder(const BMediaEncoder& other);
			BMediaEncoder&		operator=(const BMediaEncoder& other);

private:
			status_t			_AttachToEncoder();

	static	status_t			write_chunk(void* classPtr,
									const void* buffer, size_t size,
									media_encode_info* info);

			void				Init();
			void				ReleaseEncoder();

			uint32				_reserved_was_fEncoderMgr;
			BCodecKit::BEncoder* fEncoder;

			int32				fEncoderID;
			bool				fFormatValid;
			bool				fEncoderStarted;
			status_t			fInitStatus;

			uint32				_reserved_BMediaEncoder_[32];
};


class BMediaBufferEncoder : public BMediaEncoder {
public:
								BMediaBufferEncoder();
								BMediaBufferEncoder(
									const media_format* outputFormat);
								BMediaBufferEncoder(
									const media_codec_info* info);

			status_t			EncodeToBuffer(void* outputBuffer,
									size_t* _size, const void* inputBuffer,
									int64 frameCount, media_encode_info* info);

protected:
	virtual	status_t			WriteChunk(const void* buffer, size_t size,
									media_encode_info* info);

protected:
			void*				fBuffer;
			int32				fBufferSize;
};

#endif // _MEDIA_ENCODER_H

