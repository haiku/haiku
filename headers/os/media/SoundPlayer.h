/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
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
class _SoundPlayNode;


class sound_error : public std::exception {
			const char*			m_str_const;
public:
								sound_error(const char* string);
			const char*			what() const throw();
};


class BSoundPlayer {
	friend class _SoundPlayNode;
public:
			enum sound_player_notification {
				B_STARTED = 1,
				B_STOPPED,
				B_SOUND_DONE
			};

								BSoundPlayer(const char* name = NULL,
									void (*PlayBuffer)(void*, void* buffer,
										size_t size,
										const media_raw_audio_format& format)
										= NULL,
									void (*Notifier)(void*,
										sound_player_notification what, ...)
										= NULL,
									void* cookie = NULL);
								BSoundPlayer(
									const media_raw_audio_format* format,
									const char* name = NULL,
									void (*PlayBuffer)(void*, void* buffer,
										size_t size,
										const media_raw_audio_format& format)
										= NULL,
									void (*Notifier)(void*,
										sound_player_notification what, ...)
										= NULL,
									void* cookie = NULL);
								BSoundPlayer(
									const media_node& toNode,
									const media_multi_audio_format* format
										= NULL,
									const char* name = NULL,
									const media_input* input = NULL,
									void (*PlayBuffer)(void*, void* buffer,
										size_t size,
										const media_raw_audio_format& format)
										= NULL,
									void (*Notifier)(void*,
										sound_player_notification what, ...)
										= NULL,
									void* cookie = NULL);
	virtual						~BSoundPlayer();

			status_t			InitCheck();
			media_raw_audio_format Format() const;

			status_t			Start();
			void				Stop(bool block = true, bool flush = true);

	typedef void (*BufferPlayerFunc)(void*, void*, size_t,
		const media_raw_audio_format&);

			BufferPlayerFunc	BufferPlayer() const;
			void				SetBufferPlayer(void (*PlayBuffer)(void*,
									void* buffer, size_t size,
									const media_raw_audio_format& format));
	typedef void (*EventNotifierFunc)(void*, sound_player_notification what,
		...);
			EventNotifierFunc	EventNotifier() const;
			void				SetNotifier(void (*Notifier)(void*,
									sound_player_notification what, ...));
			void*				Cookie() const;
			void				SetCookie(void* cookie);
			void				SetCallbacks(void (*PlayBuffer)(void*,
									void* buffer, size_t size,
									const media_raw_audio_format& format)
										= NULL,
									void (*Notifier)(void*,
										sound_player_notification what, ...)
										= NULL,
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

			void				SetInitError(status_t inError);

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

private:
			_SoundPlayNode*		fPlayerNode;

			struct _playing_sound {
				_playing_sound*	next;
				off_t			current_offset;
				BSound*			sound;
				play_id			id;
				int32			delta;
				int32			rate;
				sem_id			wait_sem;
				float			volume;
			};

			_playing_sound*		fPlayingSounds;

			struct _waiting_sound {
				_waiting_sound*	next;
				bigtime_t		start_time;
				BSound*			sound;
				play_id			id;
				int32			rate;
				float			volume;
			};

			_waiting_sound*		fWaitingSounds;

			void (*fPlayBufferFunc)(void* cookie, void* buffer,
				size_t size, const media_raw_audio_format& format);
			void (*fNotifierFunc)(void* cookie, sound_player_notification what,
				...);

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

			uint32				_m_reserved[15];

private:
			void				NotifySoundDone(play_id sound, bool gotToPlay);

			void				get_volume_slider();
			void				Init(const media_node* node,
									const media_multi_audio_format* format,
									const char* name,
									const media_input* input,
									void (*PlayBuffer)(void*, void* buffer,
										size_t size,
										const media_raw_audio_format& format),
									void (*Notifier)(void*,
										sound_player_notification what, ...),
									void* cookie);
	// For B_STARTED and B_STOPPED, the argument is BSoundPlayer* (this).
	// For B_SOUND_DONE, the arguments are play_id and true/false for
	// whether it got to play data at all.
	virtual	void				Notify(sound_player_notification what,
									...);
	// Get data into the buffer to play -- this version will use the
	// queued BSound system.
	virtual	void				PlayBuffer(void* buffer, size_t size,
									const media_raw_audio_format& format);
};

#endif // _SOUND_PLAYER_H
