/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					This file handles video playback for ViewObject base class.
*/

#include <stdio.h>
#include <Bitmap.h>
#include <Screen.h>

#include "ViewObject.h"
#include "Video.h"

/*	FUNCTION:		Video :: Video
	ARGUMENTS:		ref
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Video :: Video(entry_ref *ref)
{
	fMediaFile = 0;
	fVideoTrack = 0;
	fBitmap = 0;
	fVideoThread = 0;
	
	fMediaFile = new BMediaFile(ref, B_MEDIA_FILE_BIG_BUFFERS);
	fStatus = fMediaFile->InitCheck();
	if (fStatus != B_OK)
		return;
		
	int32 num_tracks = fMediaFile->CountTracks();
	for (int32 i=0; i < num_tracks; i++)
	{
		BMediaTrack *track = fMediaFile->TrackAt(i);
		if (track == NULL)
		{
			fMediaFile->ReleaseAllTracks();
			printf("Media file claims to have %ld tracks, cannot find track %ld\n", num_tracks, i);
			fVideoTrack = 0;
			return;
		}
		
		media_format mf;
		fStatus = track->EncodedFormat(&mf);
		if (fStatus == B_OK)
		{
			switch (mf.type)
			{
				case B_MEDIA_ENCODED_VIDEO:
				case B_MEDIA_RAW_VIDEO:
					if (fVideoTrack == 0)
					{
						fVideoTrack = track;
						InitPlayer(&mf);
					}
					else
						printf("Multiple video tracks not supported\n");
					break;
				default:
					fStatus = B_ERROR;
			}
		}
		
		if (fStatus != B_OK)
			fMediaFile->ReleaseTrack(track);
	}
	
	if (fVideoTrack)
		fStatus = B_OK;
}

/*	FUNCTION:		Video :: ~Video
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Video :: ~Video()
{
	if (fVideoThread > 0)
		kill_thread(fVideoThread);
	delete fMediaFile;
	delete fBitmap;
}

/*	FUNCTION:		Video :: InitPlayer
	ARGUMENTS:		format
	RETURN:			n/a
	DESCRIPTION:	Create frame buffer and init decoder
*/
void Video :: InitPlayer(media_format *format)
{
	BRect frame(0, 0,
				format->u.encoded_video.output.display.line_width - 1.0f,
				format->u.encoded_video.output.display.line_count - 1.0f);
	
	BScreen screen;
	color_space cs = screen.ColorSpace();

	//	Loop asking the track for a format we can deal with
	for (;;)
	{
		fBitmap = new BBitmap(frame, cs);
		
		media_format mf, old_mf;
		memset(&mf, 0, sizeof(media_format));
		media_raw_video_format  *rvf = &mf.u.raw_video;
		rvf->last_active = (uint32)(frame.Height() - 1.0f);
		rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
		rvf->pixel_width_aspect = 1;
		rvf->pixel_height_aspect = 3;
		rvf->display.format = cs;
		rvf->display.line_width = (int32)frame.Width();
		rvf->display.line_count = (int32)frame.Height();
		rvf->display.bytes_per_row = fBitmap->BytesPerRow();
		
		old_mf = mf;
		fVideoTrack->DecodedFormat(&mf);
		//	check if match found
		if (old_mf.u.raw_video.display.format == mf.u.raw_video.display.format)
			break;
		
		//	otherwise, change colour space
		cs = mf.u.raw_video.display.format;
		delete fBitmap;
	}
	
	media_header mh;
	fVideoTrack->SeekToTime(0);
	int64 dummy_num_frames;
	fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &dummy_num_frames, &mh);
	fVideoTrack->SeekToTime(0);
}
	
/*	FUNCTION:		Video :: Start
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Start playing video
*/
void Video :: Start()
{
	fVideoThread = spawn_thread(Video::VideoThread, "Video thread", B_NORMAL_PRIORITY, this);
	if (fVideoThread > 0)
		resume_thread(fVideoThread);	
}

/*	FUNCTION:		Video :: ShowNextFrame
	ARGUMENTS:		none
	RETURN:			status
	DESCRIPTION:	Read next frame, update texture
*/
status_t Video :: ShowNextFrame()
{
	status_t 		err;
	media_header	mh;
	int64			dummy = 0;
	
	fBitmap->LockBits();
	err = fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &dummy, &mh);
	fBitmap->UnlockBits();
	
	if (err != B_OK)
	{
		//	restart video
		fVideoTrack->SeekToTime(0);
		return B_OK;
	}
	
	fMediaSource->mOwner->UpdateFrame(fMediaSource);
	return B_OK;
}

/*	FUNCTION:		Video :: VideoThread
	ARGUMENTS:		cookie
	RETURN:			thread exit status
	DESCRIPTION:	Video playback thread
*/
int32 Video :: VideoThread(void *cookie)
{
	Video *video = (Video *) cookie;
	if (video->fVideoTrack == NULL)
	{
		exit_thread(B_ERROR);
		return B_ERROR;
	}
	
	float frames_per_second = (float)video->fVideoTrack->CountFrames() / (float)video->fVideoTrack->Duration() * 1000000.0f;
	bigtime_t frame_time = (bigtime_t) (1000000.0f / frames_per_second);
	video->fPerformanceTime = real_time_clock_usecs() + frame_time;
	status_t err = B_OK;
	printf("frame_rate = %f\n", frames_per_second);
	
	while (1)
	{
		err = video->ShowNextFrame();
		bigtime_t zzz = video->fPerformanceTime - real_time_clock_usecs();
		if (zzz < 0)
			zzz = 1;
		video->fPerformanceTime += frame_time;
		snooze(zzz);
	} 
	exit_thread(err);
	return err;
}
	
	
	
	