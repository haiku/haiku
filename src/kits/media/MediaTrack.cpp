/*
 * Copyright (c) 2002-2004, Marcus Overhagen <marcus@overhagen.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <MediaTrack.h>
#include <Roster.h>
#include <string.h>
#include "MediaExtractor.h"
#include "PluginManager.h"
#include "ReaderPlugin.h"
#include "debug.h"

#define CONVERT_TO_INT32 0 // XXX test! this triggers a few bugs!

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

#define B_MEDIA_DISABLE_FORMAT_TRANSLATION 0x4000 // XXX move this (after name change?) to MediaDefs.h

class RawDecoderChunkProvider : public ChunkProvider
{
public:
				RawDecoderChunkProvider(Decoder *decoder, int buffer_size, int frame_size);
	virtual 	~RawDecoderChunkProvider();
	
	status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize, media_header *mediaHeader);

private:
	Decoder *fDecoder;
	void *fBuffer;
	int	fBufferSize;
	int	fFrameSize;
};


/*************************************************************
 * protected BMediaTrack
 *************************************************************/

BMediaTrack::~BMediaTrack()
{
	CALLED();
	if (fRawDecoder)
		_DestroyDecoder(fRawDecoder);
	if (fDecoder)
		_DestroyDecoder(fDecoder);
}

/*************************************************************
 * public BMediaTrack
 *************************************************************/

status_t
BMediaTrack::InitCheck() const
{
	CALLED();
	return fErr;
}


status_t
BMediaTrack::GetCodecInfo(media_codec_info *mci) const
{
	CALLED();
	if (!fDecoder)
		return B_NO_INIT;

	*mci = fMCI;
	snprintf(mci->pretty_name, sizeof(mci->pretty_name), "Haiku Media Kit:\n%s", fMCI.pretty_name);

	return B_OK;
}


status_t
BMediaTrack::EncodedFormat(media_format *out_format) const
{
	CALLED();
	if (!out_format)
		return B_BAD_VALUE;
	if (!fExtractor)
		return B_NO_INIT;

	*out_format = *fExtractor->EncodedFormat(fStream);

	char s[200];
	string_for_format(*out_format, s, sizeof(s));
	printf("BMediaTrack::EncodedFormat: %s\n", s);  

	return B_OK;
}


// for BeOS R5 compatibilitly
extern "C" status_t DecodedFormat__11BMediaTrackP12media_format(BMediaTrack *self, media_format *inout_format);
status_t DecodedFormat__11BMediaTrackP12media_format(BMediaTrack *self, 
													 media_format *inout_format)
{
	return self->DecodedFormat(inout_format, 0);
}


status_t
BMediaTrack::DecodedFormat(media_format *inout_format, uint32 flags)
{
	CALLED();
	if (!inout_format)
		return B_BAD_VALUE;
	if (!fExtractor || !fDecoder)
		return B_NO_INIT;
		
	if (fRawDecoder) {
		_DestroyDecoder(fRawDecoder);
		fRawDecoder = 0;
	}

	char s[200];

	string_for_format(*inout_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: req1: %s\n", s);  
	
	if ((fWorkaroundFlags & FORCE_RAW_AUDIO) || ((fWorkaroundFlags & IGNORE_ENCODED_AUDIO) && inout_format->type == B_MEDIA_ENCODED_AUDIO)) {
		inout_format->type = B_MEDIA_RAW_AUDIO;
		inout_format->u.raw_audio = media_multi_audio_format::wildcard;
	}
	if ((fWorkaroundFlags & FORCE_RAW_VIDEO) || ((fWorkaroundFlags & IGNORE_ENCODED_VIDEO) && inout_format->type == B_MEDIA_ENCODED_VIDEO)) {
		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = media_raw_video_format::wildcard;
	}
	if (inout_format->type == B_MEDIA_RAW_AUDIO) {
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_HOST_ENDIAN)
			inout_format->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_INT16_FORMAT)
			inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_INT32_FORMAT)
			inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_INT;
		if (fWorkaroundFlags & FORCE_RAW_AUDIO_FLOAT_FORMAT)
			inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	}

	string_for_format(*inout_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: req2: %s\n", s);  

	media_format requested_format = *inout_format;

	status_t res;

	res = fDecoder->NegotiateOutputFormat(inout_format);

	string_for_format(*inout_format, s, sizeof(s));
	printf("BMediaTrack::DecodedFormat: nego: %s\n", s);  
	
	if (inout_format->type == 0)
		debugger("Decoder didn't set output format type");
	if (inout_format->type == B_MEDIA_RAW_AUDIO) {
		if (inout_format->u.raw_audio.byte_order == 0)
			debugger("Decoder didn't set raw audio output byte order");
		if (inout_format->u.raw_audio.format == 0)
			debugger("Decoder didn't set raw audio output sample format");
		if (inout_format->u.raw_audio.buffer_size <= 0)
			debugger("Decoder didn't set raw audio output buffer size");
	}
	if (inout_format->type == B_MEDIA_RAW_VIDEO) {
		if (inout_format->u.raw_video.display.format == 0)
			debugger("Decoder didn't set raw video output color space");
		if (inout_format->u.raw_video.display.line_width == 0)
			debugger("Decoder didn't set raw video output line_width");
		if (inout_format->u.raw_video.display.line_count == 0)
			debugger("Decoder didn't set raw video output line_count");
		if (inout_format->u.raw_video.display.bytes_per_row == 0)
			debugger("Decoder didn't set raw video output bytes_per_row");
	}

	if (0 == (flags & B_MEDIA_DISABLE_FORMAT_TRANSLATION)) {
		if (requested_format.type == B_MEDIA_RAW_AUDIO
			&& inout_format->type == B_MEDIA_RAW_AUDIO
			&& requested_format.u.raw_audio.format != 0
			&& requested_format.u.raw_audio.format != inout_format->u.raw_audio.format) {
				if (SetupFormatTranslation(*inout_format, &requested_format)) {
					*inout_format = requested_format;
				}
		}
	}

//	string_for_format(*inout_format, s, sizeof(s));
//	printf("BMediaTrack::DecodedFormat: res: %s\n", s);  
			
	return res;
}


int64
BMediaTrack::CountFrames() const
{
	CALLED();
	int64 frames = fExtractor ? fExtractor->CountFrames(fStream) : 0;
	printf("BMediaTrack::CountFrames: %Ld\n", frames);
	return frames;
}


bigtime_t
BMediaTrack::Duration() const
{
	CALLED();
	bigtime_t duration = fExtractor ? fExtractor->Duration(fStream) : 0;
	printf("BMediaTrack::Duration: %Ld\n", duration);
	return duration;
}


int64
BMediaTrack::CurrentFrame() const
{
	return fCurFrame;
}


bigtime_t
BMediaTrack::CurrentTime() const
{
	return fCurTime;
}


status_t
BMediaTrack::ReadFrames(void *out_buffer,
						int64 *out_frameCount,
						media_header *mh)
{
	return ReadFrames(out_buffer, out_frameCount, mh, 0);
}


status_t
BMediaTrack::ReadFrames(void *out_buffer,
						int64 *out_frameCount,
						media_header *mh /* = 0 */,
						media_decode_info *info /* = 0 */)
{
//	CALLED();
	if (!fDecoder)
		return B_NO_INIT;
	if (!out_buffer || !out_frameCount)
		return B_BAD_VALUE;
	
	status_t result;	
	media_header header;

	memset(&header, 0, sizeof(header)); // always clear it first, as the decoder doesn't set all fields

	if (fRawDecoder)
		result = fRawDecoder->Decode(out_buffer, out_frameCount, &header, info);
	else	
		result = fDecoder->Decode(out_buffer, out_frameCount, &header, info);
	if (result == B_OK) {
		fCurFrame += *out_frameCount;
		fCurTime = header.start_time;
	} else {
		printf("BMediaTrack::ReadFrames: decoder returned error 0x%08lx (%s)\n", result, strerror(result));
		*out_frameCount = 0;
	}
	if (mh)
		*mh = header;

//	printf("BMediaTrack::ReadFrames: %Ld frames, start-time %Ld\n", *out_frameCount, header.start_time);

	return result;
}


status_t
BMediaTrack::ReplaceFrames(const void *in_buffer,
						   int64 *io_frameCount,
						   const media_header *mh)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::SeekToTime(bigtime_t *inout_time,
						int32 flags)
{
	CALLED();
	if (!fDecoder || !fExtractor)
		return B_NO_INIT;
	if (!inout_time)
		return B_BAD_VALUE;

	status_t result;
	uint32 seekTo;
	bigtime_t seekTime;
	
	int64 frame;
	bigtime_t time;
	
	seekTo = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_TIME;
	seekTime = *inout_time;
	
	time = seekTime;
	result = fExtractor->Seek(fStream, seekTo, &frame, &time);
	if (result != B_OK) {
		printf("BMediaTrack::SeekToTime: extractor seek failed\n");
		return result;
	}

	result = fDecoder->Seek(seekTo, 0, &frame, seekTime, &time);
	if (result != B_OK) {
		printf("BMediaTrack::SeekToTime: decoder seek failed\n");
		return result;
	}

	if (fRawDecoder) {
		result = fRawDecoder->Seek(seekTo, 0, &frame, seekTime, &time);
		if (result != B_OK) {
			printf("BMediaTrack::SeekToTime: raw decoder seek failed\n");
			return result;
		}
	}
		
	*inout_time = time;
	fCurFrame = frame;
	fCurTime = time;
	
	printf("BMediaTrack::SeekToTime finished, requested %.6f, result %.6f\n", seekTime / 1000000.0, *inout_time / 1000000.0);

	return B_OK;
}


status_t
BMediaTrack::SeekToFrame(int64 *inout_frame,
						 int32 flags)
{
	CALLED();
	if (!fDecoder || !fExtractor)
		return B_NO_INIT;
	if (!inout_frame)
		return B_BAD_VALUE;

	status_t result;
	uint32 seekTo;
	int64 seekFrame;
	
	int64 frame;
	bigtime_t time;
	
	seekTo = (flags & B_MEDIA_SEEK_DIRECTION_MASK) | B_MEDIA_SEEK_TO_FRAME;
	seekFrame = *inout_frame;
	
	frame = seekFrame;
	result = fExtractor->Seek(fStream, seekTo, &frame, &time);
	if (result != B_OK) {
		printf("BMediaTrack::SeekToFrame: extractor seek failed\n");
		return result;
	}
		
	result = fDecoder->Seek(seekTo, seekFrame, &frame, 0, &time);
	if (result != B_OK) {
		return result;
		printf("BMediaTrack::SeekToFrame: decoder seek failed\n");
	}		

	if (fRawDecoder) {
		result = fRawDecoder->Seek(seekTo, seekFrame, &frame, 0, &time);
		if (result != B_OK) {
			printf("BMediaTrack::SeekToFrame: raw decoder seek failed\n");
			return result;
		}
	}
		
	*inout_frame = frame;
	fCurFrame = frame;
	fCurTime = time;

	printf("BMediaTrack::SeekToTime SeekToFrame, requested %Ld, result %Ld\n", seekFrame, *inout_frame);

	return B_OK;
}


status_t
BMediaTrack::FindKeyFrameForTime(bigtime_t *inout_time,
								 int32 flags) const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::FindKeyFrameForFrame(int64 *inout_frame,
								  int32 flags) const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::ReadChunk(char **out_buffer,
					   int32 *out_size,
					   media_header *mh /* = 0 */)
{
	CALLED();
	if (!fExtractor)
		return B_NO_INIT;
	if (!out_buffer || !out_size)
		return B_BAD_VALUE;

	status_t result;
	media_header header;

	memset(&header, 0, sizeof(header)); // always clear it first, as the reader doesn't set all fields

	result = fExtractor->GetNextChunk(fStream, (void **)out_buffer, out_size, &header);

	fCurTime = header.start_time;
	if (mh)
		*mh = header;

	return result;
}


status_t
BMediaTrack::AddCopyright(const char *data)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::AddTrackInfo(uint32 code,
						  const void *data,
						  size_t size,
						  uint32 flags)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::WriteFrames(const void *data,
						 int32 num_frames,
						 int32 flags)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::WriteFrames(const void *data,
						 int64 num_frames,
						 media_encode_info *info)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::WriteChunk(const void *data,
						size_t size,
						uint32 flags)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::WriteChunk(const void *data,
						size_t size,
						media_encode_info *info)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::Flush()
{
	UNIMPLEMENTED();

	return B_OK;
}


// deprecated BeOS R5 API
BParameterWeb *
BMediaTrack::Web()
{
	UNIMPLEMENTED();
	return NULL;
}


// returns a copy of the parameter web
status_t
BMediaTrack::GetParameterWeb(BParameterWeb** outWeb)
{
}


status_t
BMediaTrack::GetParameterValue(int32 id,
							   void *valu,
							   size_t *size)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::SetParameterValue(int32 id,
							   const void *valu,
							   size_t size)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


BView *
BMediaTrack::GetParameterView()
{
	UNIMPLEMENTED();
	return NULL;
}


status_t
BMediaTrack::GetQuality(float *quality)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::SetQuality(float quality)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::GetEncodeParameters(encode_parameters *parameters) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::SetEncodeParameters(encode_parameters *parameters)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::Perform(int32 selector,
					 void *data)
{
	UNIMPLEMENTED();

	return B_ERROR;
}

/*************************************************************
 * private BMediaTrack
 *************************************************************/

BMediaTrack::BMediaTrack(BPrivate::media::MediaExtractor *extractor,
						 int32 stream)
{
	CALLED();
	fWorkaroundFlags = 0;
	fRawDecoder = 0;
	fExtractor = extractor;
	fStream = stream;
	fErr = B_OK;
	
	SetupWorkaround();
	
	if (fExtractor->CreateDecoder(fStream, &fDecoder, &fMCI) != B_OK) {
		ERROR("BMediaTrack::BMediaTrack: Error: creating decoder failed\n");
		// we do not set fErr here, because ReadChunk should still work
		fDecoder = 0;
		return;
	}

	fCurFrame = 0;
	fCurTime = 0;
	
	// not used:
	fEncoder = 0;
	fEncoderID = 0;
	fWriter = 0;
}


BMediaTrack::BMediaTrack(BPrivate::MediaWriter *writer,
						 int32 stream_num,
						 media_format *in_format,
						 BPrivate::media::Encoder *encoder,
						 media_codec_info *mci)
{
	UNIMPLEMENTED();
}


// Does nothing, returns B_ERROR, for Zeta compatiblity only	
status_t
BMediaTrack::ControlCodec(int32 selector, void *io_data, size_t size)
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
		fWorkaroundFlags = FORCE_RAW_AUDIO | FORCE_RAW_AUDIO_INT16_FORMAT | FORCE_RAW_AUDIO_HOST_ENDIAN;
		printf("BMediaTrack::SetupWorkaround: SoundPlay workaround active\n"); 
	}
	if (strcmp(ainfo.signature, "application/x-vnd.Be.MediaPlayer") == 0) {
		fWorkaroundFlags = IGNORE_ENCODED_AUDIO | IGNORE_ENCODED_VIDEO;
		printf("BMediaTrack::SetupWorkaround: MediaPlayer workaround active\n"); 
	}
	
#if CONVERT_TO_INT32 // XXX test
	if (!(fWorkaroundFlags & FORCE_RAW_AUDIO_INT16_FORMAT))
		fWorkaroundFlags |= FORCE_RAW_AUDIO_INT32_FORMAT;
#endif
}

bool
BMediaTrack::SetupFormatTranslation(const media_format &from, media_format *to)
{
	char s[200];
	status_t res;
	
	string_for_format(from, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation: from: %s\n", s);  

	res = _CreateDecoder(&fRawDecoder, from);
	if (res != B_OK) {
		printf("BMediaTrack::SetupFormatTranslation: _CreateDecoder failed\n");
		return false;
	}

	// XXX video?
	int buffer_size = from.u.raw_audio.buffer_size;
	int frame_size = (from.u.raw_audio.format & 15) * from.u.raw_audio.channel_count;

	fRawDecoder->Setup(new RawDecoderChunkProvider(fDecoder, buffer_size, frame_size));

	media_format from_temp = from;
	res = fRawDecoder->Setup(&from_temp, 0, 0);
	if (res != B_OK) {
		printf("BMediaTrack::SetupFormatTranslation: Setup failed\n");
		goto error;
	}

	string_for_format(*to, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation:   to: %s\n", s);  

	res = fRawDecoder->NegotiateOutputFormat(to);
	if (res != B_OK) {
		printf("BMediaTrack::SetupFormatTranslation: NegotiateOutputFormat failed\n");
		goto error;
	}

	string_for_format(*to, s, sizeof(s));
	printf("BMediaTrack::SetupFormatTranslation:  res: %s\n", s);  

	return true;

error:
	_DestroyDecoder(fRawDecoder);
	fRawDecoder = 0;
	return false;
}

/*
// unimplemented
BMediaTrack::BMediaTrack()
BMediaTrack::BMediaTrack(const BMediaTrack &)
BMediaTrack &BMediaTrack::operator=(const BMediaTrack &)
*/

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


RawDecoderChunkProvider::RawDecoderChunkProvider(Decoder *decoder, int buffer_size, int frame_size)
{
//	printf("RawDecoderChunkProvider: buffer_size %d, frame_size %d\n", buffer_size, frame_size);
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
RawDecoderChunkProvider::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
                                      media_header *mediaHeader)
{
	int64 frames;
	media_decode_info info;
	status_t res = fDecoder->Decode(fBuffer, &frames, mediaHeader, &info);
	if (res == B_OK) {
		*chunkBuffer = fBuffer;
		*chunkSize = frames * fFrameSize;
//		printf("RawDecoderChunkProvider::GetNextChunk, %Ld frames, %ld bytes, start-time %Ld\n", frames, *chunkSize, mediaHeader->start_time);
	} else {
		printf("RawDecoderChunkProvider::GetNextChunk failed\n");
	}
	return res;
}
