/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaTrack.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaTrack.h>
#include <string.h>
#include "MediaExtractor.h"
#include "PluginManager.h"
#include "ReaderPlugin.h"
#include "debug.h"

/*************************************************************
 * protected BMediaTrack
 *************************************************************/

BMediaTrack::~BMediaTrack()
{
	CALLED();
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

	return B_OK;
}


status_t
BMediaTrack::DecodedFormat(media_format *inout_format)
{
	CALLED();
	if (!inout_format)
		return B_BAD_VALUE;
	if (!fExtractor || !fDecoder)
		return B_NO_INIT;
		
	return fDecoder->NegotiateOutputFormat(inout_format);
}


int64
BMediaTrack::CountFrames() const
{
	CALLED();
	return fExtractor ? fExtractor->CountFrames(fStream) : 0;
}


bigtime_t
BMediaTrack::Duration() const
{
	CALLED();
	return fExtractor ? fExtractor->Duration(fStream) : 0;
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

	media_header temp_header;
	if (!mh)
		mh = &temp_header;
	
	result = fDecoder->Decode(out_buffer, out_frameCount, mh, info);
	if (result == B_OK) {
		fCurFrame += *out_frameCount;
		fCurTime = mh->start_time;
	} else {
		printf("BMediaTrack::ReadFrames: decoder returned error 0x%08lx (%s)\n", result, strerror(result));
		*out_frameCount = 0;
	}
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
	media_header temp_header;
	if (!mh)
		mh = &temp_header;

	result = fExtractor->GetNextChunk(fStream, (void **)out_buffer, out_size, mh);

	fCurTime = mh->start_time;

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


BParameterWeb *
BMediaTrack::Web()
{
	UNIMPLEMENTED();
	return NULL;
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
	fExtractor = extractor;
	fStream = stream;
	fErr = B_OK;
	
	if (fExtractor->CreateDecoder(fStream, &fDecoder, &fMCI) != B_OK) {
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

