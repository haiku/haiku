// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// Copyright 2010, Haiku, Inc. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "MediaFileInfo.h"

#include <MediaTrack.h>


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
		ret = track->InitCheck();
		if (ret != B_OK)
			return ret;

		if (track != NULL) {
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

				video.details << (int32)format.Width() << "x"
					<< (int32)format.Height() << " "
					<< (int32)(rvf->field_rate / rvf->interlace)
					<< " fps / "  << videoFrames << " frames";

				videoDone = true;

			} else if (format.IsAudio()) {
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				ret = track->DecodedFormat(&format);
				if (ret != B_OK)
					return ret;

				media_raw_audio_format *raf = &(format.u.raw_audio);
				char bytesPerSample = (char)(raf->format & 0xf);
				if (bytesPerSample == 1) {
					audio.details << "8 bit ";
				} else if (bytesPerSample == 2) {
					audio.details << "16 bit ";
				} else {
					audio.details << bytesPerSample << "byte ";
				}

				ret = track->GetCodecInfo(&codecInfo);
				if (ret != B_OK)
					return ret;

				audio.format << codecInfo.pretty_name;
				audioDuration = track->Duration();
				audioFrames = track->CountFrames();

				audio.details << (float)(raf->frame_rate / 1000.0f) << " kHz";
				if (raf->channel_count == 1) {
					audio.details << " mono / ";
				} else if (raf->channel_count == 2) {
					audio.details << " stereo / ";
				} else {
					audio.details << (int32)raf->channel_count
						<< " channel / " ;
				}
				audio.details << audioFrames << " frames";
				audioDone = true;
			}
			ret = file->ReleaseTrack(track);
			if (ret != B_OK)
				return ret;
		}
	}

	useconds = MAX(audioDuration, videoDuration);
	duration << (int32)(useconds / 1000000)
			  << " seconds";

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
