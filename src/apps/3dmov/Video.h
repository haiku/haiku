/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Media playback support.
*/

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <MediaKit.h>

class BBitmap;
class MediaSource;

/*****************************
	Video class handles media playback
******************************/
class Video
{
public:
				Video(entry_ref *ref);
				~Video();
			
	status_t	GetStatus() {return fStatus;}
	BBitmap		*GetBitmap() {return fBitmap;}
	void		Start();
	void		SetMediaSource(MediaSource *source) {fMediaSource = source;}

private:
	MediaSource		*fMediaSource;
	status_t		fStatus;
	BMediaFile		*fMediaFile;
	BMediaTrack		*fVideoTrack;
	BBitmap			*fBitmap;
	thread_id		fVideoThread;
	bigtime_t		fPerformanceTime;
	
	void			InitPlayer(media_format *format);
	status_t		ShowNextFrame();
	static int32	VideoThread(void *arg);		// one thread per object is spawned	
};

#endif	//#ifndef _VIDEO_H_

