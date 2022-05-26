/*
 * Copyright 2007-2008, Haiku. Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "MediaTrackVideoSupplier.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <MediaTrack.h>

#include "ColorSpaceToString.h"

using std::nothrow;

#define DEBUG_DECODED_FRAME 0
#if DEBUG_DECODED_FRAME
#  include <Bitmap.h>
#  include <BitmapStream.h>
#  include <File.h>
#  include <TranslatorRoster.h>
#endif // DEBUG_DECODED_FRAME

// constructor
MediaTrackVideoSupplier::MediaTrackVideoSupplier(BMediaTrack* track,
		int32 trackIndex, status_t& initStatus)
	:
	VideoTrackSupplier(),
	fVideoTrack(track),

	fPerformanceTime(0),
	fDuration(0),
	fCurrentFrame(0),

	fTrackIndex(trackIndex)
{
	if (!fVideoTrack) {
		printf("MediaTrackVideoSupplier() - no video track\n");
		return;
	}

	initStatus = _SwitchFormat(B_NO_COLOR_SPACE, 0);

	fDuration = fVideoTrack->Duration();

//	for (bigtime_t time = 0; time < fDuration; time += 10000) {
//		bigtime_t keyFrameTime = time;
//		fVideoTrack->FindKeyFrameForTime(&keyFrameTime,
//			B_MEDIA_SEEK_CLOSEST_BACKWARD);
//		printf("keyframe time for time: %lld -> %lld\n", time, keyFrameTime);
//	}
}

// destructor
MediaTrackVideoSupplier::~MediaTrackVideoSupplier()
{
}


const media_format&
MediaTrackVideoSupplier::Format() const
{
	return fFormat;
}


status_t
MediaTrackVideoSupplier::GetEncodedFormat(media_format* format) const
{
	if (!fVideoTrack)
		return B_NO_INIT;
	return fVideoTrack->EncodedFormat(format);
}


status_t
MediaTrackVideoSupplier::GetCodecInfo(media_codec_info* info) const
{
	if (!fVideoTrack)
		return B_NO_INIT;
	return fVideoTrack->GetCodecInfo(info);
}


status_t
MediaTrackVideoSupplier::ReadFrame(void* buffer, bigtime_t* performanceTime,
	const media_raw_video_format& format, bool& wasCached)
{
	if (!fVideoTrack)
		return B_NO_INIT;
	if (!buffer)
		return B_BAD_VALUE;

	// Hack for single frame video (cover art). Media player really wants a
	// video track and will not seek in the middle of a video frame. So we
	// pretend to be a 25fps stream and keep rendering the same frame over
	// and over again.
	if (fVideoTrack->CountFrames() < 2) {
		static int already = false;
		if (already) {
			wasCached = true;
			return B_OK;
		}
		already = true;
	}

	status_t ret = B_OK;
	if (format.display.format
			!= fFormat.u.raw_video.display.format
		|| fFormat.u.raw_video.display.bytes_per_row
			!= format.display.bytes_per_row) {
		ret = _SwitchFormat(format.display.format,
			format.display.bytes_per_row);
		if (ret < B_OK) {
			fprintf(stderr, "MediaTrackVideoSupplier::ReadFrame() - "
				"unable to switch media format: %s\n", strerror(ret));
			return ret;
		}
	}

	// read a frame
	int64 frameCount = 1;
	// TODO: how does this work for interlaced video (field count > 1)?
	media_header mediaHeader;
	ret = fVideoTrack->ReadFrames(buffer, &frameCount, &mediaHeader);

	if (ret < B_OK && ret != B_LAST_BUFFER_ERROR) {
		fprintf(stderr, "MediaTrackVideoSupplier::ReadFrame() - "
			"error while reading frame of track: %s\n", strerror(ret));
	} else
		fPerformanceTime = mediaHeader.start_time;

	fCurrentFrame = fVideoTrack->CurrentFrame();
	if (performanceTime)
		*performanceTime = fPerformanceTime;

#if DEBUG_DECODED_FRAME
if (modifiers() & B_SHIFT_KEY) {
BFile fileStream("/boot/home/Desktop/decoded.png", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
BTranslatorRoster* roster = BTranslatorRoster::Default();
BBitmap* bitmap = new BBitmap(Bounds(), 0, ColorSpace(), BytesPerRow());
memcpy(bitmap->Bits(), buffer, bitmap->BitsLength());
BBitmapStream bitmapStream(bitmap);
roster->Translate(&bitmapStream, NULL, NULL, &fileStream, B_PNG_FORMAT, 0);
bitmapStream.DetachBitmap(&bitmap);
delete bitmap;
}
#endif // DEBUG_DECODED_FRAME

	if (ret == B_LAST_BUFFER_ERROR && fVideoTrack->CountFrames() < 2)
		return B_OK;
	return ret;
}


status_t
MediaTrackVideoSupplier::FindKeyFrameForFrame(int64* frame)
{
	if (!fVideoTrack)
		return B_NO_INIT;

	if (fVideoTrack->CountFrames() < 2)
		return B_OK;

//int64 wantedFrame = *frame;
	status_t ret = fVideoTrack->FindKeyFrameForFrame(frame,
		B_MEDIA_SEEK_CLOSEST_BACKWARD);
//printf("found keyframe for frame %lld -> %lld\n", wantedFrame, *frame);
	return ret;
}


status_t
MediaTrackVideoSupplier::SeekToTime(bigtime_t* performanceTime)
{
	if (!fVideoTrack)
		return B_NO_INIT;

	if (fVideoTrack->CountFrames() < 2)
		return B_OK;

	bigtime_t _performanceTime = *performanceTime;
	status_t ret = fVideoTrack->FindKeyFrameForTime(performanceTime,
		B_MEDIA_SEEK_CLOSEST_BACKWARD);
	if (ret < B_OK)
		return ret;

	ret = fVideoTrack->SeekToTime(performanceTime);
	if (ret == B_OK) {
		if (_performanceTime != *performanceTime) {
			printf("seeked by time: %" B_PRIdBIGTIME " -> %" B_PRIdBIGTIME
				"\n", _performanceTime, *performanceTime);
		}
		fPerformanceTime = *performanceTime;
		fCurrentFrame = fVideoTrack->CurrentFrame();
	}

	return ret;
}


status_t
MediaTrackVideoSupplier::SeekToFrame(int64* frame)
{
	if (!fVideoTrack)
		return B_NO_INIT;

	if (fVideoTrack->CountFrames() < 2)
		return B_OK;

	int64 wantFrame = *frame;

	if (wantFrame == fCurrentFrame)
		return B_OK;

	status_t ret = fVideoTrack->FindKeyFrameForFrame(frame,
		B_MEDIA_SEEK_CLOSEST_BACKWARD);
	if (ret != B_OK)
		return ret;
	if (wantFrame > *frame) {
		// Work around a rounding problem with some extractors and
		// converting frames <-> time <-> internal time.
		int64 nextWantFrame = wantFrame + 1;
		if (fVideoTrack->FindKeyFrameForFrame(&nextWantFrame,
			B_MEDIA_SEEK_CLOSEST_BACKWARD) == B_OK) {
			if (nextWantFrame == wantFrame) {
				wantFrame++;
				*frame = wantFrame;
			}
		}
	}

//if (wantFrame != *frame) {
//	printf("keyframe for frame: %lld -> %lld\n", wantFrame, *frame);
//}

	if (*frame <= fCurrentFrame && wantFrame >= fCurrentFrame) {
		// The current frame is already closer to the wanted frame
		// than the next keyframe before it.
		*frame = fCurrentFrame;
		return B_OK;
	}

	ret = fVideoTrack->SeekToFrame(frame);
	if (ret != B_OK)
		return ret;

//if (wantFrame != *frame) {
//	printf("seeked by frame: %lld -> %lld, was %lld\n", wantFrame, *frame,
//		fCurrentFrame);
//}

	fCurrentFrame = *frame;
	fPerformanceTime = fVideoTrack->CurrentTime();

	return ret;
}


// #pragma mark -


BRect
MediaTrackVideoSupplier::Bounds() const
{
	return BRect(0, 0, 	fFormat.u.raw_video.display.line_width - 1,
		fFormat.u.raw_video.display.line_count - 1);
}


color_space
MediaTrackVideoSupplier::ColorSpace() const
{
	return fFormat.u.raw_video.display.format;
}


uint32
MediaTrackVideoSupplier::BytesPerRow() const
{
	return fFormat.u.raw_video.display.bytes_per_row;
}


// #pragma mark -


status_t
MediaTrackVideoSupplier::_SwitchFormat(color_space format, uint32 bytesPerRow)
{
	// get the encoded format
	fFormat.Clear();
	status_t ret = fVideoTrack->EncodedFormat(&fFormat);
	if (ret < B_OK) {
		printf("MediaTrackVideoSupplier::_SwitchFormat() - "
			"fVideoTrack->EncodedFormat(): %s\n", strerror(ret));
		return ret;
	}

	// get ouput video frame size
	uint32 width = fFormat.u.encoded_video.output.display.line_width;
	uint32 height = fFormat.u.encoded_video.output.display.line_count;
	if (format == B_NO_COLOR_SPACE) {
		format = fFormat.u.encoded_video.output.display.format;
		if (format == B_NO_COLOR_SPACE) {
			// if still no preferred format, try the most commonly
			// supported overlay format
			format = B_YCbCr422;
		} else {
			printf("MediaTrackVideoSupplier::_SwitchFormat() - "
				"preferred color space: %s\n",
				color_space_to_string(format));
		}
	}

	uint32 minBytesPerRow;
	if (format == B_YCbCr422)
		minBytesPerRow = ((width * 2 + 3) / 4) * 4;
	else
		minBytesPerRow = width * 4;
	bytesPerRow = max_c(bytesPerRow, minBytesPerRow);

	ret = _SetDecodedFormat(width, height, format, bytesPerRow);
	if (ret < B_OK) {
		printf("MediaTrackVideoSupplier::_SwitchFormat() - "
			"fVideoTrack->DecodedFormat(): %s - retrying with B_RGB32\n",
			strerror(ret));
		format = B_RGB32;
		bytesPerRow = max_c(bytesPerRow, width * 4);

		ret = _SetDecodedFormat(width, height, format, bytesPerRow);
		if (ret < B_OK) {
			printf("MediaTrackVideoSupplier::_SwitchFormat() - "
				"fVideoTrack->DecodedFormat(): %s - giving up\n",
				strerror(ret));
			return ret;
		}
	}

	if (fFormat.u.raw_video.display.format != format) {
		printf("MediaTrackVideoSupplier::_SwitchFormat() - "
			" codec changed colorspace of decoded format (%s -> %s)!\n",
			color_space_to_string(format),
			color_space_to_string(fFormat.u.raw_video.display.format));
		// check if the codec forgot to adjust bytes_per_row
		format = fFormat.u.raw_video.display.format;
		if (format == B_YCbCr422)
			minBytesPerRow = ((width * 2 + 3) / 4) * 4;
		else
			minBytesPerRow = width * 4;
		if (minBytesPerRow > fFormat.u.raw_video.display.bytes_per_row) {
			printf("  -> stupid codec forgot to adjust bytes_per_row!\n");

			ret = _SetDecodedFormat(width, height, format, minBytesPerRow);
		}
	}

	if (fFormat.u.raw_video.last_active != height - 1) {
		printf("should skip %" B_PRId32 " lines at bottom!\n", 
			(height - 1) - fFormat.u.raw_video.last_active);
	}

	return ret;
}


status_t
MediaTrackVideoSupplier::_SetDecodedFormat(uint32 width, uint32 height,
	color_space format, uint32 bytesPerRow)
{
	// specifiy the decoded format. we derive this information from
	// the encoded format (width & height).
	fFormat.Clear();
//	fFormat.u.raw_video.last_active = height - 1;
//	fFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
//	fFormat.u.raw_video.pixel_width_aspect = 1;
//	fFormat.u.raw_video.pixel_height_aspect = 1;
	fFormat.u.raw_video.display.format = format;
	fFormat.u.raw_video.display.line_width = width;
	fFormat.u.raw_video.display.line_count = height;
	fFormat.u.raw_video.display.bytes_per_row = bytesPerRow;

	return fVideoTrack->DecodedFormat(&fFormat);
}

