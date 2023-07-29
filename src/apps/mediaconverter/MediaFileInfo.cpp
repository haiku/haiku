// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// Copyright 2010, Haiku, Inc. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "MediaFileInfo.h"

#include <Catalog.h>
#include <MediaTrack.h>
#include <StringFormat.h>
#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaFileInfo"

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
	format.Clear();
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

		static BStringFormat frameFormat(B_TRANSLATE(
			"{0, plural, one{# frame} other{# frames}}"));

		if (format.IsVideo()) {
			format.Clear();
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
			details.SetToFormat(
				B_TRANSLATE_COMMENT("%u × %u, %.2ffps", "Width × Height, fps;"
				"The '×' is the Unicode multiplication sign U+00D7"),
				format.Width(), format.Height(),
				rvf->field_rate / rvf->interlace);

			details << " / ";
			frameFormat.Format(details, videoFrames);

			video.details << details;
			videoDone = true;

		} else if (format.IsAudio()) {
			format.Clear();
			format.type = B_MEDIA_RAW_AUDIO;
			ret = track->DecodedFormat(&format);
			if (ret != B_OK)
				return ret;
			media_raw_audio_format *raf = &(format.u.raw_audio);
			char bytesPerSample = (char)(raf->format & 0xf);

			BString details;
			if (bytesPerSample == 1 || bytesPerSample == 2) {
				static BStringFormat bitFormat(
					B_TRANSLATE("{0, plural, one{# bit} other{# bits}}"));
				bitFormat.Format(details, bytesPerSample * 8);
				details.SetToFormat(B_TRANSLATE("%d bit "), bytesPerSample * 8);
			} else {
				static BStringFormat bitFormat(
					B_TRANSLATE("{0, plural, one{# byte} other{# bytes}}"));
				bitFormat.Format(details, bytesPerSample);
			}
			audio.details << details;
			audio.details << " ";

			ret = track->GetCodecInfo(&codecInfo);
			if (ret != B_OK)
				return ret;

			audio.format << codecInfo.pretty_name;
			audioDuration = track->Duration();
			audioFrames = track->CountFrames();
			BString channels;
			if (raf->channel_count == 1) {
				channels.SetToFormat(B_TRANSLATE("%.1f kHz mono"),
					raf->frame_rate / 1000.f);
			} else if (raf->channel_count == 2) {
				channels.SetToFormat(B_TRANSLATE("%.1f kHz stereo"),
					raf->frame_rate / 1000.f);
			} else {
				channels.SetToFormat(B_TRANSLATE("%.1f kHz %ld channel"),
					raf->frame_rate / 1000.f, raf->channel_count);
			}

			channels << " / ";
			frameFormat.Format(channels, audioFrames);

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
