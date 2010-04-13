/*
 * Copyright 2009-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H


#include <exception>

#include <BufferProducer.h>
#include <Locker.h>
#include <MediaDefs.h>


class BContinuousParameter;
class BParameterWeb;
class BSound;
namespace BPrivate {
	class SoundPlayNode;
}


class sound_error : public std::exception {
			const char*			m_str_const;
public:
								sound_error(const char* string);
			const char*			what() const throw();
};


class BSoundPlayer {
public:
	enum sound_player_notification {
		B_STARTED = 1,
		B_STOPPED,
		B_SOUND_DONE
	};

	typedef void (*BufferPlayerFunc)(void*, void* buffer, size_t size,
		const media_raw_audio_format& format);
	typedef void (*EventNotifierFunc)(void*, sound_player_notification what,
		...);

public:
								BSoundPlayer(const char* name = NULL,
									BufferPlayerFunc playerFunction = NULL,
									EventNotifierFunc
										eventNotifierFunction = NULL,
									void* cookie = NULL);
								BSoundPlayer(
									const media_raw_audio_format* format,
									const char* name = NULL,
									BufferPlayerFunc playerFunction = NULL,
									EventNotifierFunc
										eventNotifierFunction = NULL,
									void* cookie = NULL);
								BSoundPlayer(
									const media_node& toNode,
									const media_multi_audio_format*
										format = NULL,
									const char* name = NULL,
									const media_input* input = NULL,
									BufferPlayerFunc playerFunction = NULL,
									EventNotifierFunc
										eventNotifierFunction = NULL,
									void* cookie = NULL);
	virtual						~BSoundPlayer();

			status_t			InitCheck();
			media_raw_audio_format Format() const;

			status_t			Start();
			void				Stop(bool block = true, bool flush = true);

			BufferPlayerFunc	BufferPlayer() const;
			void				SetBufferPlayer(void (*PlayBuffer)(void*,
									void* buffer, size_t size,
									const media_raw_audio_format& format));

			EventNotifierFunc	EventNotifier() const;
			void				SetNotifier(
									EventNotifierFunc eventNotifierFunction);
			void*				Cookie() const;
			void				SetCookie(void* cookie);
			void				SetCallbacks(
									BufferPlayerFunc playerFunction = NULL,
									EventNotifierFunc
										eventNotifierFunction = NULL,
									void* cookie = NULL);

	typedef int32 play_id;

			bigtime_t			CurrentTime();
			bigtime_t			PerformanceTime();
			status_t			Preroll();
			play_id				StartPlaying(BSound* sound,
									bigtime_t atTime = 0);
			play_id 			StartPlaying(BSound* sound,
									bigtime_t atTime,
									float withVolume);
			status_t 			SetSoundVolume(play_id sound, float newVolume);
			bool 				IsPlaying(play_id id);
			status_t 			StopPlaying(play_id id);
			status_t		 	WaitForSound(play_id id);

			// On [0..1]
			float 				Volume();
			void 				SetVolume(float volume);

			// -xx - +xx (see GetVolumeInfo())
			// If 'forcePoll' is false, a cached value will be used if new
			// enough.
			float				VolumeDB(bool forcePoll = false);
			void				SetVolumeDB(float dB);
			status_t			GetVolumeInfo(media_node* _node,
									int32* _parameterID, float* _minDB,
									float* _maxDB);
			bigtime_t			Latency();

	virtual	bool				HasData();
			void				SetHasData(bool hasData);

	// TODO: Needs Perform() method for FBC!

protected:

			void				SetInitError(status_t error);

private:
	static	void				_SoundPlayBufferFunc(void* cookie,
									void* buffer, size_t size,
									const media_raw_audio_format& format);

	// FBC padding
	virtual	status_t			_Reserved_SoundPlayer_0(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_1(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_2(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_3(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_4(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_5(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_6(void*, ...);
	virtual	status_t			_Reserved_SoundPlayer_7(void*, ...);

			void				_Init(const media_node* node,
									const media_multi_audio_format* format,
									const char* name, const media_input* input,
									BufferPlayerFunc playerFunction,
									EventNotifierFunc eventNotifierFunction,
									void* cookie);
			void				_GetVolumeSlider();

			void				_NotifySoundDone(play_id sound, bool gotToPlay);

	// TODO: those two shouldn't be virtual
	virtual	void				Notify(sound_player_notification what, ...);
	virtual	void				PlayBuffer(void* buffer, size_t size,
									const media_raw_audio_format& format);

private:
	friend class BPrivate::SoundPlayNode;

	struct playing_sound {
		playing_sound*	next;
		off_t			current_offset;
		BSound*			sound;
		play_id			id;
		int32			delta;
		int32			rate;
		sem_id			wait_sem;
		float			volume;
	};

	struct waiting_sound {
		waiting_sound*	next;
		bigtime_t		start_time;
		BSound*			sound;
		play_id			id;
		int32			rate;
		float			volume;
	};

			BPrivate::SoundPlayNode* fPlayerNode;

			playing_sound*		fPlayingSounds;
			waiting_sound*		fWaitingSounds;

			BufferPlayerFunc	fPlayBufferFunc;
			EventNotifierFunc	fNotifierFunc;

			BLocker				fLocker;
			float				fVolumeDB;
			media_input			fMediaInput;
				// Usually the system mixer
			media_output		fMediaOutput;
				// The playback node
			void*				fCookie;
				// Opaque handle passed to hooks
			int32				fFlags;

			status_t			fInitStatus;
			BContinuousParameter* fVolumeSlider;
			bigtime_t			fLastVolumeUpdate;
			BParameterWeb*		fParameterWeb;

			uint32				_reserved[15];
};

#endif // _SOUND_PLAYER_H
