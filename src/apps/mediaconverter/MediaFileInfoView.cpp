// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "MediaFileInfoView.h"

#include <string.h>

#include <MediaFile.h>
#include <MediaTrack.h>
#include <String.h>

#include "Strings.h"


MediaFileInfoView::MediaFileInfoView(BRect frame, uint32 resizingMode)
	: BView(frame, "MediaFileInfoView", resizingMode,
			B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW)
	, fRef()
	, fMediaFile(NULL)
	, fDuration(0)
{
}


MediaFileInfoView::~MediaFileInfoView()
{
}


void 
MediaFileInfoView::Draw(BRect /*update*/)
{
	font_height fh;
	GetFontHeight(&fh);
	BPoint p(2, fh.ascent + fh.leading);
	BFont font;
	GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(12);
	SetFont(&font);

	if (fMediaFile == NULL) {
		DrawString(NO_FILE_LABEL, p);
		return;
	}

	BString aFmt, vFmt, aDetails, vDetails, duration;
	_GetFileInfo(&aFmt, &vFmt, &aDetails, &vDetails, &duration);
	
	// draw filename
	DrawString(fRef.name, p);
	float lineHeight = fh.ascent + fh.descent + fh.leading;
	p.y += (float)ceil(lineHeight * 1.5);
	
	float durLen = StringWidth(DURATION_LABEL) + 5;
	float audLen = StringWidth(AUDIO_INFO_LABEL) + 5;
	float vidLen = StringWidth(VIDEO_INFO_LABEL) + 5;
	float maxLen = MAX(durLen, audLen);
	maxLen = MAX(maxLen, vidLen);
			
	// draw labels
	DrawString(AUDIO_INFO_LABEL, p + BPoint(maxLen - audLen, 0));
	BPoint p2 = p;
	p2.x += maxLen + 4;
	p.y += lineHeight * 2;
	DrawString(VIDEO_INFO_LABEL, p + BPoint(maxLen - vidLen, 0));
	p.y += lineHeight * 2;
	DrawString(DURATION_LABEL, p + BPoint(maxLen - durLen, 0));

	// draw audio/video/duration info
	font.SetFace(B_REGULAR_FACE);
	font.SetSize(10);
	SetFont(&font);
	
	DrawString(aFmt.String(), p2);
	p2.y += lineHeight;
	DrawString(aDetails.String(), p2);
	p2.y += lineHeight;
	DrawString(vFmt.String(), p2);
	p2.y += lineHeight;
	DrawString(vDetails.String(), p2);
	p2.y += lineHeight;
	DrawString(duration.String(), p2);
}


void 
MediaFileInfoView::AttachedToWindow()
{
	rgb_color c = Parent()->LowColor();
	SetViewColor(c);
	SetLowColor(c);
}


void 
MediaFileInfoView::Update(BMediaFile* file, entry_ref* ref)
{
	if (fMediaFile == file)
		return;

	fMediaFile = file;

	if (file != NULL && ref != NULL)
		fRef = *ref;
	else
		fRef = entry_ref();

	Invalidate();
}


// #pragma mark -


void 
MediaFileInfoView::_GetFileInfo(BString* audioFormat, BString* videoFormat,
	BString* audioDetails, BString* videoDetails, BString* duration)
{
	fDuration = 0;
	if (fMediaFile == NULL)
		return;
	
	BMediaTrack* track;
	media_format format;
	memset(&format, 0, sizeof(format));
	media_codec_info codecInfo;
	bool audioDone(false), videoDone(false);
	bigtime_t audioDuration = 0;
	bigtime_t videoDuration = 0;
	int32 tracks = fMediaFile->CountTracks();
	int64 videoFrames = 0;
	int64 audioFrames = 0;
	for (int32 i = 0; i < tracks && (!audioDone || !videoDone); i++) {
		track = fMediaFile->TrackAt(i);
		if (track != NULL) {
			track->EncodedFormat(&format);
			if (format.IsVideo()) {
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_VIDEO;
				track->DecodedFormat(&format);
				media_raw_video_format *rvf = &(format.u.raw_video);

				track->GetCodecInfo(&codecInfo);
				*videoFormat << codecInfo.pretty_name;
				videoDuration = track->Duration();
				videoFrames = track->CountFrames();

				*videoDetails << (int32)format.Width() << "x" << (int32)format.Height()
							 << " " << (int32)(rvf->field_rate / rvf->interlace)
							 << " fps / "  << videoFrames << " frames";

				videoDone = true;

			} else if (format.IsAudio()) {
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				track->DecodedFormat(&format);
				media_raw_audio_format *raf = &(format.u.raw_audio);
				char bytesPerSample = (char)(raf->format & 0xf);
				if (bytesPerSample == 1) {
					*audioDetails << "8 bit ";
				} else if (bytesPerSample == 2) {
					*audioDetails << "16 bit ";
				} else {
					*audioDetails << bytesPerSample << "byte ";
				}

				track->GetCodecInfo(&codecInfo);
				*audioFormat << codecInfo.pretty_name;
				audioDuration = track->Duration();
				audioFrames = track->CountFrames();

				*audioDetails << (float)(raf->frame_rate / 1000.0f) << " kHz";
				if (raf->channel_count == 1) {
					*audioDetails << " mono / ";
				} else if (raf->channel_count == 2) {
					*audioDetails << " stereo / ";
				} else {
					*audioDetails << (int32)raf->channel_count << " channel / " ;
				}
				*audioDetails << audioFrames << " frames";
				audioDone = true;
			}
			fMediaFile->ReleaseTrack(track);
		}	
	}

	fDuration = MAX(audioDuration, videoDuration);
	*duration << (int32)(fDuration / 1000000)
			  << " seconds";
}


