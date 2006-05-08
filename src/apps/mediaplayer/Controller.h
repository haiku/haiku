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
#include <String.h>

class BBitmap;
class BMediaFile;
class BMediaTrack;
class SoundOutput;
class VideoView;

class Controller
{
public:
							Controller();
	virtual 				~Controller();
	
	status_t				SetTo(const entry_ref &ref);

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

	void					SetVideoView(VideoView *view);
	
	bool					IsOverlayActive();

	void					LockBitmap();
	void					UnlockBitmap();
	BBitmap *				Bitmap();
	
	void					VolumeUp();
	void					VolumeDown();
	
	void					SetVolume(float value);

private:
	static int32			audio_play_thread(void *self);
	static int32			video_play_thread(void *self);

	void					AudioPlayThread();
	void					VideoPlayThread();
	
	void					StartAudioPlayback();
	void					StartVideoPlayback();
	void					StopAudioPlayback();
	void					StopVideoPlayback();

private:
	VideoView *				fVideoView;
	BString					fName;
	bool					fPaused;
	bool					fStopped;
	BMediaFile *			fMediaFile;
	BMediaTrack *			fAudioTrack;
	BMediaTrack *			fVideoTrack;
	BList *					fAudioTrackList;
	BList *					fVideoTrackList;
	bigtime_t				fPosition;
	media_format			fAudioFormat;
	media_format			fVideoFormat;
	sem_id					fAudioPlaySem;
	sem_id					fVideoPlaySem;
	thread_id				fAudioPlayThread;
	thread_id				fVideoPlayThread;
	volatile bool			fStopAudioPlayback;
	volatile bool			fStopVideoPlayback;
	SoundOutput	*			fSoundOutput;
};


#endif
