/*
 * Copyright 2002-2007, Marcus Overhagen <marcus@overhagen.de>
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marcus Overhagen, marcus@overhagen.de
 */


#include <MediaTrack.h>

#include <CodecRoster.h>
#include <MediaExtractor.h>
#include <MediaWriter.h>
#include <Roster.h>

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace BCodecKit;


//#define TRACE_MEDIA_TRACK
#ifdef TRACE_MEDIA_TRACK
#	ifndef TRACE
#		define TRACE printf
#	endif
#else
#	ifndef TRACE
#		define TRACE(a...)
#	endif
#endif

#define ERROR(a...) fprintf(stderr, a)


#define CONVERT_TO_INT32 0
	// TODO: Test! This triggers a few bugs!

// flags used for workarounds
enum {
	FORCE_RAW_AUDIO 				= 0x0001,
	FORCE_RAW_VIDEO 				= 0x0002,
	FORCE_RAW_AUDIO_INT16_FORMAT 	= 0x0010,
	FORCE_RAW_AUDIO_INT32_FORMAT 	= 0x0020,
	FORCE_RAW_AUDIO_FLOAT_FORMAT 	= 0x0040,
	FORCE_RAW_AUDIO_HOST_ENDIAN 	= 0x0100,
	IGNORE_ENCODED_AUDIO 			= 0x1000,
	IGNORE_ENCODED_VIDEO 			= 0x2000,
};

#define B_MEDIA_DISABLE_FORMAT_TRANSLATION 0x4000
	// TODO: move this (after name change?) to MediaDefs.h


class RawDecoderChunkProvider : public BChunkProvider {
public:
							RawDecoderChunkProvider(BDecoder* decoder,
								int buffer_size, int frame_size);
	virtual					~RawDecoderChunkProvider();

			status_t		GetNextChunk(const void** chunkBuffer,
								size_t* chunkSize,
								media_header* mediaHeader);

private:
			BDecoder*		fDecoder;
			void*			fBuffer;
			int				fBufferSize;
			int				fFrameSize;
};


/*************************************************************
 * protected BMediaTrack
 *************************************************************/

BMediaTrack::~BMediaTrack()
{
	CALLED();

	BCodecRoster::ReleaseDecoder(fRawDecoder);
	BCodecRoster::ReleaseDecoder(fDecoder);
	BCodecRoster::ReleaseEncoder(fEncoder);
}

/*************************************************************
 * public BMediaTrack
 *************************************************************/

status_t
BMediaTrack::InitCheck() const
{
	CALLED();

	return fInitStatus;
}


status_t
BMediaTrack::GetCodecInfo(media_codec_info* _codecInfo) const
{
	CALLED();

	if (fDecoder == NULL)
		return B_NO_INIT;

	*_codecInfo = fCodecInfo;
	strlcpy(_codecInfo->pretty_name, fCodecInfo.pretty_name,
		sizeof(_codecInfo->pretty_name));

	return B_OK;
}


status_t
BMediaTrack::EncodedFormat(media_format* _format) const
{
	CALLED();

	if (_format == NULL)
		return B_BAD_VALUE;

	if (fExtractor == NULL)
		return B_NO_INIT;

	*_format = *fExtractor->EncodedFormat(fStream);

#ifdef TRACE_MEDIA_TRACK
	char s[200];
	string_for_format(*_format, s, sizeof(s));
	printf("BMediaTrack::EncodedFormat: %s\n", s);
#endif

	return B_OK;
}


// for BeOS R5 compatibility
extern "C" status_t DecodedFormat__11BMediaTrackP12media_format(
	BMediaTrack* self, media_format* _format);

status_t DecodedFormat__11BMediaTrackP12media_format(BMediaTrack* self,
	media_format* _format)
{
	return self->DecodedFormat(_format, 0);
}


status_t
BMediaTrack::DecodedFormat(media_format* _format, uint32 flags)
{
	CALLED();

	if (_format == NULL)
		return B_BAD_VALUE;

	if (fExtractor == NULL || fDecoder == NULL)
		return B_NO_INIT;

	BCodecRoster::ReleaseDecoder(fRawDecoder);
	fRawDecoder = NULL;

#ifdef TRACE_MEDIA_TRACK
	char s[200];
	string_for_format(*_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: req1: %s\n", s);
#endif

	if ((fWorkaroundFlags & FORCE_RAW_AUDIO)
		|| ((fWorkaroundFlags & IGNORE_ENCODED_AUDIO)
			&& _format->type == B_MEDIA_ENCODED_AUDIO)) {
		_format->type = B_MEDIA_RAW_AUDIO;
		_format->u.raw_audio = media_multi_audio_format::wildcard;
	}
	if ((fWorkaroundFlags & FORCE_RAW_VIDEO)
		|| ((fWorkaroundFlags & IGNORE_ENCODED_VIDEO)
			&& _format->type == B_MEDIA_ENCODED_VIDEO)) {
		_format->type = B_MEDIA_RAW_VIDEO;
		_format->u.raw_video = media_raw_video_format::wildcard;
	}
	if (_format->type == B_MEDIA_RAW_AUDIO) {
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_HOST_ENDIAN)
			_format->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

		if (fWorkaroundFlags & FORCE_RAW_AUDIO_INT16_FORMAT) {
			_format->u.raw_audio.format
				= media_raw_audio_format::B_AUDIO_SHORT;
		}
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_INT32_FORMAT) {
			_format->u.raw_audio.format
				= media_raw_audio_format::B_AUDIO_INT;
		}
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_FLOAT_FORMAT) {
			_format->u.raw_audio.format
				= media_raw_audio_format::B_AUDIO_FLOAT;
		}
	}

#ifdef TRACE_MEDIA_TRACK
	string_for_format(*_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: req2: %s\n", s);
#endif

	fFormat = *_format;
	status_t result = fDecoder->NegotiateOutputFormat(_format);

#ifdef TRACE_MEDIA_TRACK
	string_for_format(*_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: nego: %s\n", s);
#endif

	if (_format->type == 0) {
#ifdef TRACE_MEDIA_TRACK
		printf("BMediaTrack::DecodedFormat: Decoder didn't set output format "
			"type.\n");
#endif
	}

	if (_format->type == B_MEDIA_RAW_AUDIO) {
		if (_format->u.raw_audio.byte_order == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw audio "
				"output byte order.\n");
#endif
		}
		if (_format->u.raw_audio.format == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw audio "
				"output sample format.\n");
#endif
		}
		if (_format->u.raw_audio.buffer_size <= 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw audio "
				"output buffer size.\n");
#endif
		}
	}

	if (_format->type == B_MEDIA_RAW_VIDEO) {
		if (_format->u.raw_video.display.format == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw video "
				"output color space.\n");
#endif
		}
		if (_format->u.raw_video.display.line_width == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw video "
				"output line_width.\n");
#endif
		}
		if (_format->u.raw_video.display.line_count == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw video "
				"output line_count.\n");
#endif
		}
		if (_format->u.raw_video.display.bytes_per_row == 0) {
#ifdef TRACE_MEDIA_TRACK
			printf("BMediaTrack::DecodedFormat: Decoder didn't set raw video "
				"output bytes_per_row.\n");
#endif
		}
	}

	if ((flags & B_MEDIA_DISABLE_FORMAT_TRANSLATION) == 0) {
		if (fFormat.type == B_MEDIA_RAW_AUDIO
			&& _format->type == B_MEDIA_RAW_AUDIO
			&& fFormat.u.raw_audio.format != 0
			&& fFormat.u.raw_audio.format != _format->u.raw_audio.format) {
			if (SetupFormatTranslation(*_format, &fFormat))
				*_format = fFormat;
		}
	}

	fFormat = *_format;

//	string_for_format(*_format, s, sizeof(s));
//	printf("BMediaTrack::DecodedFormat: status: %s\n", s);
	return result;
}


status_t
BMediaTrack::GetMetaData(BMessage* _data) const
{
	CALLED();

	if (fExtractor == NULL)
		return B_NO_INIT;

	if (_data == NULL)
		return B_BAD_VALUE;

	_data->MakeEmpty();

	BMetaData data;
	if (fExtractor->GetStreamMetaData(fStream, &data) != B_OK) {
		*_data = *data.Message();
		return B_OK;
	}
	return B_ERROR;
}


int64
BMediaTrack::CountFrames() const
{
	CALLED();

	int64 frames = fExtractor ? fExtractor->CountFrames(fStream) : 0;
//	printf("BMediaTrack::CountFrames: %lld\n", frames);
	return frames;
}


bigtime_t
BMediaTrack::Duration() const
{
	CALLED();

	bigtime_t duration = fExtractor ? fExtractor->Duration(fStream) : 0;
//	printf("BMediaTrack::Duration: %lld\n", duration);
	return duration;
}


int64
BMediaTrack::CurrentFrame() const
{
	return fCurrentFrame;
}


bigtime_t
BMediaTrack::CurrentTime() const
{
	return fCurrentTime;
}

// BMediaTrack::ReadFrames(char*, long long*, media_header*)
// Compatibility for R5 and below. Required by Corum III and Civ:CTP.
#if __GNUC__ < 3

extern "C" status_t
ReadFrames__11BMediaTrackPcPxP12media_header(BMediaTrack* self,
	char* _buffer, int64* _frameCount, media_header* header)
{
	return self->ReadFrames(_buffer, _frameCount, header, 0);
}

#endif	// __GNUC__ < 3

status_t
BMediaTrack::ReadFrames(void* buffer, int64* _frameCount, media_header* header)
{
	return ReadFrames(buffer, _frameCount, header, NULL);
}


status_t
BMediaTrack::ReadFrames(void* buffer, int64* _frameCount,
	media_header* _header, media_decode_info* info)
{
//	CALLED();

	if (fDecoder == NULL)
		return B_NO_INIT;

	if (buffer == NULL || _frameCount == NULL)
		return B_BAD_VALUE;

	media_header header;
	if (_header == NULL)
		_header = &header;

	// Always clear the header first, as the decoder may not set all fields.
	memset(_header, 0, sizeof(media_header));

	status_t result = fRawDecoder != NULL
		? fRawDecoder->Decode(buffer, _frameCount, _header, info)
		: fDecoder->Decode(buffer, _frameCount, _header, info);

	if (result == B_OK) {
		fCurrentFrame += *_frameCount;
		bigtime_t framesDuration = (bigtime_t)(*_frameCount * 1000000
			/ _FrameRate());
		fCurrentTime = _header->start_time + framesDuration;
#if 0
	// This debug output shows drift between calculated fCurrentFrame and
	// time-based current frame, if there is any.
	if (fFormat.type == B_MEDIA_RAW_AUDIO) {
		printf("current frame: %lld / calculated: %lld (%.2f/%.2f)\r",
			fCurrentFrame,
			int64(fCurrentTime * _FrameRate() / 1000000.0 + 0.5),
			fCurrentTime / 1000000.0, (float)fCurrentFrame / _FrameRate());
		fflush(stdout);
	}
#endif
	} else {
		ERROR("BMediaTrack::ReadFrames: decoder returned error %#" B_PRIx32
			" (%s)\n", result, strerror(result));
		*_frameCount = 0;
	}

#if 0
	PRINT(1, "BMediaTrack::ReadFrames: stream %ld, start-time %5Ld.%06Ld, "
		"%lld frames\n", fStream,  _header->start_time / 1000000,
		_header->start_time % 1000000, *out_frameCount);
#endif

	return result;
}


status_t
BMediaTrack::ReplaceFrames(const void* inBuffer, int64* _frameCount,
	const media_header* header)
{
	UNIMPLEMENTED();

	// TODO: Actually, a file is either open for reading or writing at the
	// moment. Since the chunk size of encoded media data will change,
	// implementing this call will only be possible for raw media tracks.

	return B_NOT_SUPPORTED;
}


status_t
BMediaTrack::SeekToTime(bigtime_t* _time, int32 flags)
{
	CALLED();

	if (fDecoder == NULL || fExtractor == NULL)
		return B_NO_INIT;

	if (_time == NULL)
		return B_BAD_VALUE;

	// Make sure flags are valid
	flags = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_TIME;

	#if DEBUG
	bigtime_t requestedTime = *_time;
	#endif
	int64 frame = 0;

	status_t result = fExtractor->Seek(fStream, flags, &frame, _time);
	if (result != B_OK) {
		ERROR("BMediaTrack::SeekToTime: extractor seek failed\n");
		return result;
	}

	result = fDecoder->SeekedTo(frame, *_time);
	if (result != B_OK) {
		ERROR("BMediaTrack::SeekToTime: decoder seek failed\n");
		return result;
	}

	if (fRawDecoder != NULL) {
		result = fRawDecoder->SeekedTo(frame, *_time);
		if (result != B_OK) {
			ERROR("BMediaTrack::SeekToTime: raw decoder seek failed\n");
			return result;
		}
	}

	fCurrentFrame = frame;
	fCurrentTime = *_time;

	PRINT(1, "BMediaTrack::SeekToTime finished, requested %.6f, result %.6f\n",
		requestedTime / 1000000.0, *_time / 1000000.0);

	return B_OK;
}


status_t
BMediaTrack::SeekToFrame(int64* _frame, int32 flags)
{
	CALLED();

	if (fDecoder == NULL || fExtractor == NULL)
		return B_NO_INIT;

	if (_frame == NULL)
		return B_BAD_VALUE;

	// Make sure flags are valid
	flags = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_FRAME;

	#if DEBUG
	int64 requestedFrame = *_frame;
	#endif
	bigtime_t time = 0;

	status_t result = fExtractor->Seek(fStream, flags, _frame, &time);
	if (result != B_OK) {
		ERROR("BMediaTrack::SeekToFrame: extractor seek failed\n");
		return result;
	}

	result = fDecoder->SeekedTo(*_frame, time);
	if (result != B_OK) {
		ERROR("BMediaTrack::SeekToFrame: decoder seek failed\n");
		return result;
	}

	if (fRawDecoder != NULL) {
		result = fRawDecoder->SeekedTo(*_frame, time);
		if (result != B_OK) {
			ERROR("BMediaTrack::SeekToFrame: raw decoder seek failed\n");
			return result;
		}
	}

	fCurrentFrame = *_frame;
	fCurrentTime = time;

	PRINT(1, "BMediaTrack::SeekToTime SeekToFrame, requested %lld, "
		"result %lld\n", requestedFrame, *_frame);

	return B_OK;
}


status_t
BMediaTrack::FindKeyFrameForTime(bigtime_t* _time, int32 flags) const
{
	CALLED();

	if (fExtractor == NULL)
		return B_NO_INIT;

	if (_time == NULL)
		return B_BAD_VALUE;

	// Make sure flags are valid
	flags = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_TIME;

	int64 frame = 0;
		// dummy frame, will be ignored because of flags
	status_t result = fExtractor->FindKeyFrame(fStream, flags, &frame, _time);
	if (result != B_OK) {
		ERROR("BMediaTrack::FindKeyFrameForTime: extractor seek failed: %s\n",
			strerror(result));
	}

	return result;
}


status_t
BMediaTrack::FindKeyFrameForFrame(int64* _frame, int32 flags) const
{
	CALLED();

	if (fExtractor == NULL)
		return B_NO_INIT;

	if (_frame == NULL)
		return B_BAD_VALUE;

	// Make sure flags are valid
	flags = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_FRAME;

	bigtime_t time = 0;
		// dummy time, will be ignored because of flags
	status_t result = fExtractor->FindKeyFrame(fStream, flags, _frame, &time);
	if (result != B_OK) {
		ERROR("BMediaTrack::FindKeyFrameForFrame: extractor seek failed: %s\n",
			strerror(result));
	}

	return result;
}


status_t
BMediaTrack::ReadChunk(char** _buffer, int32* _size, media_header* _header)
{
	CALLED();

	if (fExtractor == NULL)
		return B_NO_INIT;

	if (_buffer == NULL || _size == NULL)
		return B_BAD_VALUE;

	media_header header;
	if (_header == NULL)
		_header = &header;

	// Always clear the header first, as the extractor may not set all fields.
	memset(_header, 0, sizeof(media_header));

	const void* buffer;
	size_t size;
	status_t result = fExtractor->GetNextChunk(fStream, &buffer, &size,
		_header);

	if (result == B_OK) {
		*_buffer = const_cast<char*>(static_cast<const char*>(buffer));
			// TODO: Change the pointer type when we break the API.
		*_size = size;
		// TODO: This changes the meaning of fCurrentTime from pointing
		// to the next chunk start time (i.e. after seeking) to the start time
		// of the last chunk. Asking the extractor for the current time will
		// not work so well because of the chunk cache. But providing a
		// "duration" field in the media_header could be useful.
		fCurrentTime = _header->start_time;
		fCurrentFrame = (int64)(fCurrentTime * _FrameRate() / 1000000LL);

	}

	return result;
}


status_t
BMediaTrack::AddCopyright(const char* copyright)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	BMetaData* data = new BMetaData();
	data->SetString(kCopyright, copyright);
	return fWriter->SetMetaData(data);
}


status_t
BMediaTrack::AddTrackInfo(uint32 code, const void* data, size_t size,
	uint32 flags)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->AddTrackInfo(fStream, code, data, size, flags);
}


status_t
BMediaTrack::WriteFrames(const void* data, int32 frameCount, int32 flags)
{
	media_encode_info encodeInfo;
	encodeInfo.flags = flags;

	return WriteFrames(data, frameCount, &encodeInfo);
}


status_t
BMediaTrack::WriteFrames(const void* data, int64 frameCount,
	media_encode_info* info)
{
	if (fEncoder == NULL)
		return B_NO_INIT;

	return fEncoder->Encode(data, frameCount, info);
}


status_t
BMediaTrack::WriteChunk(const void* data, size_t size, uint32 flags)
{
	media_encode_info encodeInfo;
	encodeInfo.flags = flags;

	return WriteChunk(data, size, &encodeInfo);
}


status_t
BMediaTrack::WriteChunk(const void* data, size_t size, media_encode_info* info)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->WriteChunk(fStream, data, size, info);
}


status_t
BMediaTrack::Flush()
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->Flush();
}


// deprecated BeOS R5 API
BParameterWeb*
BMediaTrack::Web()
{
	BParameterWeb* web;
	if (GetParameterWeb(&web) == B_OK)
		return web;

	return NULL;
}


status_t
BMediaTrack::GetParameterWeb(BParameterWeb** outWeb)
{
	if (outWeb == NULL)
		return B_BAD_VALUE;

	if (fEncoder == NULL)
		return B_NO_INIT;

	// TODO: This method is new in Haiku. The header mentions it returns a
	// copy. But how could it even do that? How can one clone a web and make
	// it point to the same BControllable?
	*outWeb = fEncoder->ParameterWeb();
	if (*outWeb != NULL)
		return B_OK;

	return B_NOT_SUPPORTED;
}


status_t
BMediaTrack::GetParameterValue(int32 id, void* value, size_t* size)
{
	if (value == NULL || size == NULL)
		return B_BAD_VALUE;

	if (fEncoder == NULL)
		return B_NO_INIT;

	return fEncoder->GetParameterValue(id, value, size);
}


status_t
BMediaTrack::SetParameterValue(int32 id, const void* value, size_t size)
{
	if (value == NULL || size == 0)
		return B_BAD_VALUE;

	if (fEncoder == NULL)
		return B_NO_INIT;

	return fEncoder->SetParameterValue(id, value, size);
}


BView*
BMediaTrack::GetParameterView()
{
	if (fEncoder == NULL)
		return NULL;

	return fEncoder->ParameterView();
}


status_t
BMediaTrack::GetQuality(float* quality)
{
	if (quality == NULL)
		return B_BAD_VALUE;

	encode_parameters parameters;
	status_t result = GetEncodeParameters(&parameters);
	if (result != B_OK)
		return result;

	*quality = parameters.quality;

	return B_OK;
}


status_t
BMediaTrack::SetQuality(float quality)
{
	encode_parameters parameters;
	status_t result = GetEncodeParameters(&parameters);
	if (result != B_OK)
		return result;

	if (quality < 0.0f)
		quality = 0.0f;

	if (quality > 1.0f)
		quality = 1.0f;

	parameters.quality = quality;

	return SetEncodeParameters(&parameters);
}


status_t
BMediaTrack::GetEncodeParameters(encode_parameters* parameters) const
{
	if (parameters == NULL)
		return B_BAD_VALUE;

	if (fEncoder == NULL)
		return B_NO_INIT;

	return fEncoder->GetEncodeParameters(parameters);
}


status_t
BMediaTrack::SetEncodeParameters(encode_parameters* parameters)
{
	if (parameters == NULL)
		return B_BAD_VALUE;

	if (fEncoder == NULL)
		return B_NO_INIT;

	return fEncoder->SetEncodeParameters(parameters);
}


status_t
BMediaTrack::Perform(int32 selector, void* data)
{
	return B_OK;
}

// #pragma mark - private


BMediaTrack::BMediaTrack(BCodecKit::BMediaExtractor* extractor,
	int32 stream)
{
	CALLED();

	fWorkaroundFlags = 0;
	fDecoder = NULL;
	fRawDecoder = NULL;
	fExtractor = extractor;
	fStream = stream;
	fInitStatus = B_OK;

	SetupWorkaround();

	status_t ret = fExtractor->CreateDecoder(fStream, &fDecoder, &fCodecInfo);
	if (ret != B_OK) {
		TRACE("BMediaTrack::BMediaTrack: Error: creating decoder failed: "
			"%s\n", strerror(ret));
		// We do not set fInitStatus here, because ReadChunk should still work.
		fDecoder = NULL;
	}

	fCurrentFrame = 0;
	fCurrentTime = 0;

	// not used:
	fEncoder = NULL;
	fEncoderID = 0;
	fWriter = NULL;
}


BMediaTrack::BMediaTrack(BCodecKit::BMediaWriter* writer,
	int32 streamIndex, media_format* format,
	const media_codec_info* codecInfo)
{
	CALLED();

	fWorkaroundFlags = 0;
	fEncoder = NULL;
	fEncoderID = -1;
		// TODO: Not yet sure what this was needed for...
	fWriter = writer;
	fStream = streamIndex;
	fInitStatus = B_OK;

	SetupWorkaround();

	if (codecInfo != NULL) {
		status_t ret = fWriter->CreateEncoder(&fEncoder, codecInfo, format);
		if (ret != B_OK) {
			TRACE("BMediaTrack::BMediaTrack: Error: creating decoder failed: "
				"%s\n", strerror(ret));
			// We do not set fInitStatus here, because WriteChunk should still
			// work.
			fEncoder = NULL;
		} else {
			fCodecInfo = *codecInfo;
			fInitStatus = fEncoder->SetUp(format);
		}
	}

	fFormat = *format;

	// not used:
	fCurrentFrame = 0;
	fCurrentTime = 0;
	fDecoder = NULL;
	fRawDecoder = NULL;
	fExtractor = NULL;
}


// Does nothing, returns B_ERROR, for Zeta compatiblity only
status_t
BMediaTrack::ControlCodec(int32 selector, void* io_data, size_t size)
{
	return B_ERROR;
}


void
BMediaTrack::SetupWorkaround()
{
	app_info	ainfo;
	thread_info	tinfo;

	get_thread_info(find_thread(0), &tinfo);
	be_roster->GetRunningAppInfo(tinfo.team, &ainfo);

	if (strcmp(ainfo.signature, "application/x-vnd.marcone-soundplay") == 0) {
		fWorkaroundFlags = FORCE_RAW_AUDIO | FORCE_RAW_AUDIO_INT16_FORMAT
			| FORCE_RAW_AUDIO_HOST_ENDIAN;
		printf("BMediaTrack::SetupWorkaround: SoundPlay workaround active\n");
	}
	if (strcmp(ainfo.signature, "application/x-vnd.Be.MediaPlayer") == 0) {
		fWorkaroundFlags = IGNORE_ENCODED_AUDIO | IGNORE_ENCODED_VIDEO;
		printf("BMediaTrack::SetupWorkaround: MediaPlayer workaround active\n");
	}

#if CONVERT_TO_INT32
	// TODO: Test
	if (!(fWorkaroundFlags & FORCE_RAW_AUDIO_INT16_FORMAT))
		fWorkaroundFlags |= FORCE_RAW_AUDIO_INT32_FORMAT;
#endif
}


bool
BMediaTrack::SetupFormatTranslation(const media_format &from, media_format* to)
{
	BCodecRoster::ReleaseDecoder(fRawDecoder);
	fRawDecoder = NULL;

#ifdef TRACE_MEDIA_TRACK
	char s[200];
	string_for_format(from, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation: from: %s\n", s);
#endif

	status_t result = BCodecRoster::InstantiateDecoder(&fRawDecoder, from);
	if (result != B_OK) {
		ERROR("BMediaTrack::SetupFormatTranslation: CreateDecoder failed\n");
		return false;
	}

	// XXX video?
	int buffer_size = from.u.raw_audio.buffer_size;
	int frame_size = (from.u.raw_audio.format & 15)
		* from.u.raw_audio.channel_count;
	media_format fromNotConst = from;

	BChunkProvider* chunkProvider
		= new (std::nothrow) RawDecoderChunkProvider(fDecoder, buffer_size,
			frame_size);
	if (chunkProvider == NULL) {
		ERROR("BMediaTrack::SetupFormatTranslation: can't create chunk "
			"provider\n");
		goto error;
	}
	fRawDecoder->SetChunkProvider(chunkProvider);

	result = fRawDecoder->Setup(&fromNotConst, 0, 0);
	if (result != B_OK) {
		ERROR("BMediaTrack::SetupFormatTranslation: Setup failed\n");
		goto error;
	}

#ifdef TRACE_MEDIA_TRACK
	string_for_format(*to, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation:   to: %s\n", s);
#endif

	result = fRawDecoder->NegotiateOutputFormat(to);
	if (result != B_OK) {
		ERROR("BMediaTrack::SetupFormatTranslation: NegotiateOutputFormat "
			"failed\n");
		goto error;
	}

#ifdef TRACE_MEDIA_TRACK
	string_for_format(*to, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation:  result: %s\n", s);
#endif

	return true;

error:
	BCodecRoster::ReleaseDecoder(fRawDecoder);
	fRawDecoder = NULL;
	return false;
}


double
BMediaTrack::_FrameRate() const
{
	switch (fFormat.type) {
		case B_MEDIA_RAW_VIDEO:
			return fFormat.u.raw_video.field_rate;
		case B_MEDIA_ENCODED_VIDEO:
			return fFormat.u.encoded_video.output.field_rate;
		case B_MEDIA_RAW_AUDIO:
			return fFormat.u.raw_audio.frame_rate;
		case B_MEDIA_ENCODED_AUDIO:
			return fFormat.u.encoded_audio.output.frame_rate;
		default:
			return 1.0;
	}
}

#if 0
// unimplemented
BMediaTrack::BMediaTrack()
BMediaTrack::BMediaTrack(const BMediaTrack &)
BMediaTrack &BMediaTrack::operator=(const BMediaTrack &)
#endif

status_t BMediaTrack::_Reserved_BMediaTrack_0(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_1(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_2(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_3(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_4(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_5(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_6(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_7(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_8(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_9(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_10(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_11(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_12(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_13(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_14(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_15(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_16(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_17(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_18(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_19(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_20(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_21(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_22(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_23(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_24(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_25(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_26(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_27(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_28(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_29(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_30(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_31(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_32(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_33(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_34(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_35(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_36(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_37(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_38(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_39(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_40(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_41(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_42(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_43(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_44(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_45(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_46(int32 arg, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_47(int32 arg, ...) { return B_ERROR; }


RawDecoderChunkProvider::RawDecoderChunkProvider(BDecoder* decoder,
	int buffer_size, int frame_size)
{
//	printf("RawDecoderChunkProvider: buffer_size %d, frame_size %d\n",
//		buffer_size, frame_size);
	fDecoder = decoder;
	fFrameSize = frame_size;
	fBufferSize = buffer_size;
	fBuffer = malloc(buffer_size);
}


RawDecoderChunkProvider::~RawDecoderChunkProvider()
{
	free(fBuffer);
}


status_t
RawDecoderChunkProvider::GetNextChunk(const void** chunkBuffer,
	size_t* chunkSize, media_header* header)
{
	int64 frames;
	media_decode_info info;
	status_t result = fDecoder->Decode(fBuffer, &frames, header, &info);
	if (result == B_OK) {
		*chunkBuffer = fBuffer;
		*chunkSize = frames * fFrameSize;
//		printf("RawDecoderChunkProvider::GetNextChunk, %lld frames, "
//			"%ld bytes, start-time %lld\n", frames, *chunkSize,
//			header->start_time);
	} else
		ERROR("RawDecoderChunkProvider::GetNextChunk failed\n");

	return result;
}
