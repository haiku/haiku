/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaTrack.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaTrack.h>
#include "debug.h"

/*************************************************************
 * protected BMediaTrack
 *************************************************************/

BMediaTrack::~BMediaTrack()
{
	UNIMPLEMENTED();
}

/*************************************************************
 * public BMediaTrack
 *************************************************************/

status_t
BMediaTrack::InitCheck() const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::GetCodecInfo(media_codec_info *mci) const
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BMediaTrack::EncodedFormat(media_format *out_format) const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::DecodedFormat(media_format *inout_format)
{
	UNIMPLEMENTED();

	return B_OK;
}


int64
BMediaTrack::CountFrames() const
{
	UNIMPLEMENTED();

	return 100000;
}


bigtime_t
BMediaTrack::Duration() const
{
	UNIMPLEMENTED();

	return 1000000;
}


int64
BMediaTrack::CurrentFrame() const
{
	UNIMPLEMENTED();

	return 0;
}


bigtime_t
BMediaTrack::CurrentTime() const
{
	UNIMPLEMENTED();

	return 0;
}


status_t
BMediaTrack::ReadFrames(void *out_buffer,
						int64 *out_frameCount,
						media_header *mh)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::ReadFrames(void *out_buffer,
						int64 *out_frameCount,
						media_header *mh,
						media_decode_info *info)
{
	UNIMPLEMENTED();

	return B_OK;
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
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BMediaTrack::SeekToFrame(int64 *inout_frame,
						 int32 flags)
{
	UNIMPLEMENTED();

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
					   media_header *mh)
{
	UNIMPLEMENTED();

	return B_OK;
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
	status_t dummy;

	return dummy;
}


status_t
BMediaTrack::SetParameterValue(int32 id,
							   const void *valu,
							   size_t size)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
}


status_t
BMediaTrack::SetQuality(float quality)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BMediaTrack::GetEncodeParameters(encode_parameters *parameters) const
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BMediaTrack::SetEncodeParameters(encode_parameters *parameters)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


status_t
BMediaTrack::Perform(int32 selector,
					 void *data)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}

/*************************************************************
 * private BMediaTrack
 *************************************************************/

BMediaTrack::BMediaTrack(BPrivate::MediaExtractor *extractor,
						 int32 stream)
{
	UNIMPLEMENTED();
}


BMediaTrack::BMediaTrack(BPrivate::MediaWriter *writer,
						 int32 stream_num,
						 media_format *in_format,
						 BPrivate::Encoder *encoder,
						 media_codec_info *mci)
{
	UNIMPLEMENTED();
}


status_t
BMediaTrack::TrackInfo(media_format *out_format,
					   void **out_info,
					   int32 *out_infoSize)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}

/*
// unimplemented
BMediaTrack::BMediaTrack()
BMediaTrack::BMediaTrack(const BMediaTrack &)
BMediaTrack &BMediaTrack::operator=(const BMediaTrack &)
*/

BPrivate::Decoder *
BMediaTrack::find_decoder(BMediaTrack *track,
						  int32 *id)
{
	UNIMPLEMENTED();
	return NULL;
}


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

