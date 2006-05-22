/*
 * Controller.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <Locker.h>
#include <String.h>

class BBitmap;
class BMediaFile;
class BMediaTrack;
class SoundOutput;
class VideoView;
class ControllerView;

class Controller
{
public:
							Controller();
	virtual 				~Controller();
	
	status_t				SetTo(const entry_ref &ref);
	void					GetSize(int *width, int *height);

	int						AudioTrackCount();
	int						VideoTrackCount();
	
	status_t				SelectAudioTrack(int n);
	status_t				SelectVideoTrack(int n);

	bigtime_t				Duration();
	bigtime_t				Position();

	status_t				Seek(bigtime_t pos);

	void					Stop();
	void					Play();
	void					Pause();
	bool					IsPaused();
	bool					IsStopped();

	void					SetVideoView(VideoView *view);
	void					SetControllerView(ControllerView *view);
	
	bool					IsOverlayActive();

	void					LockBitmap();
	void					UnlockBitmap();
	BBitmap *				Bitmap();
	
	void					VolumeUp();
	void					VolumeDown();
	
	void					SetVolume(float value);
	
	void					SetPosition(float value);
	
	void					UpdateVolume(float value);
	void					UpdatePosition(float value);

private:
	static int32			audio_decode_thread(void *self);
	static int32			video_decode_thread(void *self);
	static int32			audio_play_thread(void *self);
	static int32			video_play_thread(void *self);

	void					AudioDecodeThread();
	void					VideoDecodeThread();
	void					AudioPlayThread();
	void					VideoPlayThread();
		
	void					StartThreads();
	void					StopThreads();
	
private:

	enum {
		MAX_AUDIO_BUFFERS = 8,
		MAX_VIDEO_BUFFERS = 3,
	};
	
	struct buffer_info {
		char *	 		buffer;
		BBitmap *		bitmap;
		size_t			sizeUsed;
		size_t			sizeMax;
		bigtime_t		startTime;
		bool			formatChanged;
		media_format	mediaFormat;
	};

	VideoView *				fVideoView;
	ControllerView *		fControllerView;
	BString					fName;
	volatile bool			fPaused;
	volatile bool			fStopped;
	BMediaFile *			fMediaFile;
	BMediaTrack *			fAudioTrack;
	BMediaTrack *			fVideoTrack;
	
	BLocker					fAudioTrackLock;
	BLocker					fVideoTrackLock;
	
	BList *					fAudioTrackList;
	BList *					fVideoTrackList;
	bigtime_t				fPosition;
	media_format			fAudioFormat;
	media_format			fVideoFormat;

	sem_id					fAudioDecodeSem;
	sem_id					fVideoDecodeSem;
	sem_id					fAudioPlaySem;
	sem_id					fVideoPlaySem;
	sem_id					fThreadWaitSem;
	thread_id				fAudioDecodeThread;
	thread_id				fVideoDecodeThread;
	thread_id				fAudioPlayThread;
	thread_id				fVideoPlayThread;

	SoundOutput	*			fSoundOutput;
	volatile bool			fSeekAudio;
	volatile bool			fSeekVideo;
	volatile bigtime_t		fSeekPosition;
	bigtime_t				fDuration;

	int32					fAudioBufferCount;
	int32					fAudioBufferReadIndex;
	int32					fAudioBufferWriteIndex;
	int32					fVideoBufferCount;
	int32					fVideoBufferReadIndex;
	int32					fVideoBufferWriteIndex;
	
	buffer_info				fAudioBuffer[MAX_AUDIO_BUFFERS];
	buffer_info				fVideoBuffer[MAX_VIDEO_BUFFERS];

	BLocker					fTimeSourceLock;
	bigtime_t				fTimeSourceSysTime;
	bigtime_t				fTimeSourcePerfTime;
	bool 					fAutoplay;
	
	BBitmap *				fCurrentBitmap;

};


#endif

