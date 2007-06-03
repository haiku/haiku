/*
 * Controller.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
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

#include <Entry.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <List.h>
#include <Locker.h>
#include <String.h>

class AudioSupplier;
class BBitmap;
class BMediaFile;
class BMediaTrack;
class SoundOutput;
class VideoSupplier;
class VideoView;

class Controller {
public:

	class Listener {
	public:
							Listener();
		virtual				~Listener();

		virtual	void		FileFinished();
		virtual	void		FileChanged();

		virtual	void		VideoTrackChanged(int32 index);
		virtual	void		AudioTrackChanged(int32 index);

		virtual	void		VideoStatsChanged();
		virtual	void		AudioStatsChanged();

		virtual	void		PlaybackStateChanged(uint32 state);
		virtual	void		PositionChanged(float position);
		virtual	void		VolumeChanged(float volume);
	};

							Controller();
	virtual 				~Controller();

	bool					Lock();
	status_t				LockWithTimeout(bigtime_t timeout);
	void					Unlock();

	status_t				SetTo(const entry_ref &ref);
	void					GetSize(int *width, int *height);

	int						AudioTrackCount();
	int						VideoTrackCount();
	
	status_t				SelectAudioTrack(int n);
	status_t				SelectVideoTrack(int n);

	void					Stop();
	void					Play();
	void					Pause();
	void					TogglePlaying();

	bool					IsPaused() const;
	bool					IsStopped() const;
	uint32					PlaybackState() const;

	bigtime_t				Duration();
	bigtime_t				Position();

	void					SetVolume(float value);
	float					Volume() const;
	void					SetPosition(float value);
	
	// video view
	void					SetVideoView(VideoView *view);
	
	bool					IsOverlayActive();

	bool					LockBitmap();
	void					UnlockBitmap();
	BBitmap *				Bitmap();
	
	// notification support
	bool					AddListener(Listener* listener);
	void					RemoveListener(Listener* listener);

private:
	void					_AudioDecodeThread();
	void					_AudioPlayThread();

	void					_VideoDecodeThread();
	void					_VideoPlayThread();

	void					_StartThreads();
	void					_StopThreads();

	void					_EndOfStreamReached(bool isVideo = false);
	void					_UpdatePosition(bigtime_t position,
								bool isVideoPosition = false,
								bool force = false);

	static int32			_VideoDecodeThreadEntry(void *self);
	static int32			_VideoPlayThreadEntry(void *self);
	static int32			_AudioDecodeThreadEntry(void *self);
	static int32			_AudioPlayThreadEntry(void *self);

private:
	void					_NotifyFileChanged();
	void					_NotifyFileFinished();
	void					_NotifyVideoTrackChanged(int32 index);
	void					_NotifyAudioTrackChanged(int32 index);

	void					_NotifyVideoStatsChanged();
	void					_NotifyAudioStatsChanged();

	void					_NotifyPlaybackStateChanged();
	void					_NotifyPositionChanged(float position);
	void					_NotifyVolumeChanged(float volume);

	friend class InfoWin;

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
		bool			endOfStream;
		media_format	mediaFormat;
	};

	VideoView *				fVideoView;
	BString					fName;
	volatile bool			fPaused;
	volatile bool			fStopped;
	float					fVolume;

	entry_ref				fRef;
	BMediaFile *			fMediaFile;
BMediaTrack *			fAudioTrack;
BMediaTrack *			fVideoTrack;
	mutable BLocker			fDataLock;

	VideoSupplier*			fVideoSupplier;
	AudioSupplier*			fAudioSupplier;

	BLocker					fVideoSupplierLock;
	BLocker					fAudioSupplierLock;

	BList *					fAudioTrackList;
	BList *					fVideoTrackList;
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
	bigtime_t				fPosition;
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
	volatile bool			fPauseAtEndOfStream;
	volatile bool			fSeekToStartAfterPause;
	
	BBitmap *				fCurrentBitmap;
	BLocker					fBitmapLock;

	BList					fListeners;
};


#endif

