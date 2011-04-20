/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		SHINTA
 */
#include "ImageTrackVideoSupplier.h"

#include <string.h>


ImageTrackVideoSupplier::ImageTrackVideoSupplier(BBitmap* bitmap,
		int32 trackIndex, status_t& initStatus)
	:
	VideoTrackSupplier(),
	fPerformanceTime(0),
	fDuration(0),
	fCurrentFrame(0),
	fBitmap(bitmap),
	fTrackIndex(trackIndex)
{
	fFormat.type = B_MEDIA_ENCODED_VIDEO;
	fFormat.u.encoded_video.output.field_rate = 0.0;
	fFormat.u.encoded_video.output.interlace = 1;
	fFormat.u.encoded_video.output.first_active = 0;
	fFormat.u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fFormat.u.encoded_video.output.display.format = B_RGB32;
	fFormat.u.encoded_video.output.pixel_width_aspect
		= fFormat.u.raw_video.display.line_width
		= static_cast<int32>(fBitmap->Bounds().right) + 1;
	fFormat.u.encoded_video.output.pixel_height_aspect
		= fFormat.u.raw_video.display.line_count
		= static_cast<int32>(fBitmap->Bounds().bottom) + 1;
	fFormat.u.encoded_video.output.display.bytes_per_row
		= fFormat.u.raw_video.display.line_width * sizeof(int32);
	fFormat.u.encoded_video.output.display.pixel_offset = 0;
	fFormat.u.encoded_video.output.display.line_offset = 0;
	fFormat.u.encoded_video.output.display.flags = 0;
	fFormat.u.encoded_video.output.last_active
		= fFormat.u.raw_video.display.line_count - 1;
	fFormat.u.encoded_video.avg_bit_rate = 0.0;
	fFormat.u.encoded_video.max_bit_rate = 0.0;
	fFormat.u.encoded_video.encoding = media_encoded_video_format::B_ANY;
	fFormat.u.encoded_video.frame_size
		= fFormat.u.encoded_video.output.display.bytes_per_row *
		fFormat.u.raw_video.display.line_count;
	fFormat.u.encoded_video.forward_history = 0;
	fFormat.u.encoded_video.backward_history = 0;

	initStatus = B_OK;
}


ImageTrackVideoSupplier::~ImageTrackVideoSupplier()
{
}


const media_format&
ImageTrackVideoSupplier::Format() const
{
	return fFormat;
}


status_t
ImageTrackVideoSupplier::GetEncodedFormat(media_format* format) const
{
	*format = fFormat;
	return B_OK;
}


status_t
ImageTrackVideoSupplier::GetCodecInfo(media_codec_info* info) const
{
	strlcpy(info->pretty_name, "Artwork (static image)",
		sizeof(info->pretty_name));
	strlcpy(info->short_name, "Artwork", sizeof(info->short_name));
	info->id = info->sub_id = 0;

	return B_OK;
}


status_t
ImageTrackVideoSupplier::ReadFrame(void* buffer, bigtime_t* performanceTime,
	const media_raw_video_format& format, bool& wasCached)
{
	uint32	size = format.display.bytes_per_row * format.display.line_count;
	memcpy(buffer, fBitmap->Bits(), size);
	return B_OK;
}


status_t
ImageTrackVideoSupplier::FindKeyFrameForFrame(int64* frame)
{
	return B_OK;
}


status_t
ImageTrackVideoSupplier::SeekToTime(bigtime_t* performanceTime)
{
	return B_OK;
}


status_t
ImageTrackVideoSupplier::SeekToFrame(int64* frame)
{
	return B_OK;
}


