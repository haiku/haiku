// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// Copyright 2010, Haiku, Inc. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "MediaFileInfo.h"

#include <Catalog.h>
#include <MediaTrack.h>
#include <stdio.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaFileInfo"

MediaFileInfo::MediaFileInfo(BMediaFile* file)
{
	LoadInfo(file);
}


status_t
MediaFileInfo::LoadInfo(BMediaFile* file)
{
	_Reset();
	if (!file)
		return B_BAD_VALUE;
	
	BMediaTrack* track;
	media_format format;
	memset(&format, 0, sizeof(format));
	media_codec_info codecInfo;
	bool audioDone(false), videoDone(false);
	bigtime_t audioDuration = 0;
	bigtime_t videoDuration = 0;
	int32 tracks = file->CountTracks();
	int64 videoFrames = 0;
	int64 audioFrames = 0;
	status_t ret = B_OK;

	for (int32 i = 0; i < tracks && (!audioDone || !videoDone); i++) {
		track = file->TrackAt(i);
		if (track == NULL)
			return B_ERROR;

		ret = track->InitCheck();
		if (ret != B_OK)
			return ret;

		ret = track->EncodedFormat(&format);
		if (ret != B_OK)
			return ret;

		if (format.IsVideo()) {
			memset(&format, 0, sizeof(format));
			format.type = B_MEDIA_RAW_VIDEO;

			ret = track->DecodedFormat(&format);
			if (ret != B_OK)
				return ret;

			media_raw_video_format *rvf = &(format.u.raw_video);

			ret = track->GetCodecInfo(&codecInfo);
			if (ret != B_OK)
				return ret;

			video.format << codecInfo.pretty_name;
			videoDuration = track->Duration();
			videoFrames = track->CountFrames();

			BString details;
			snprintf(details.LockBuffer(256), 256,
					B_TRANSLATE_COMMENT("%u x %u, %.2ffps / %Ld frames",
					"Width x Height, fps / frames"),
					format.Width(), format.Height(),
					rvf->field_rate / rvf->interlace, videoFrames);
			details.UnlockBuffer();
			video.details << details;
			videoDone = true;

		} else if (format.IsAudio()) {
			memset(&format, 0, sizeof(format));
			format.type = B_MEDIA_RAW_AUDIO;
			ret = track->DecodedFormat(&format);
			if (ret != B_OK)
				return ret;
			media_raw_audio_format *raf = &(format.u.raw_audio);
			char bytesPerSample = (char)(raf->format & 0xf);

			BString details;
			if (bytesPerSample == 1 || bytesPerSample == 2) {
				snprintf(details.LockBuffer(16), 16,
						B_TRANSLATE("%d bit "), bytesPerSample * 8);
			} else {
				snprintf(details.LockBuffer(16), 16,
						B_TRANSLATE("%d byte "), bytesPerSample);
			}
			details.UnlockBuffer();
			audio.details << details;

			ret = track->GetCodecInfo(&codecInfo);
			if (ret != B_OK)
				return ret;

			audio.format << codecInfo.pretty_name;
			audioDuration = track->Duration();
			audioFrames = track->CountFrames();
			BString channels;
			if (raf->channel_count == 1) {
				snprintf(channels.LockBuffer(64), 64,
				B_TRANSLATE("%.1f kHz mono / %lld frames"), 
				raf->frame_rate / 1000.f, audioFrames);
			} else if (raf->channel_count == 2) {
				snprintf(channels.LockBuffer(64), 64,
				B_TRANSLATE("%.1f kHz stereo / %lld frames"), 
				raf->frame_rate / 1000.f, audioFrames);
			} else {
				snprintf(channels.LockBuffer(64), 64,
				B_TRANSLATE("%.1f kHz %ld channel / %lld frames"), 
				raf->frame_rate / 1000.f, raf->channel_count, audioFrames);
			}
			channels.UnlockBuffer();
			audio.details << channels;

			audioDone = true;
		}
		ret = file->ReleaseTrack(track);
		if (ret != B_OK)
			return ret;
	}

	useconds = MAX(audioDuration, videoDuration);
	duration << (int32)(useconds / 1000000)
			  << B_TRANSLATE(" seconds");

	return B_OK;
}


void
MediaFileInfo::_Reset()
{
	audio.details.SetTo("");
	audio.format.SetTo("");
	video.details.SetTo("");
	video.format.SetTo("");
	useconds = 0;
	duration.SetTo("");
}
