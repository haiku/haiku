/*	SoundPlayer.h	*/
/*	Copyright 1998 Be Incorporated. All rights reserved.	*/

#if !defined(_SOUND_PLAYER_H)
#define _SOUND_PLAYER_H


#include <MediaDefs.h>
#include <BufferProducer.h>
#include <Locker.h>
#include <exception>


class BSound;
class sound_error : public exception {
	const char * m_str_const;
public:
	sound_error(const char * str);
	const char * what() const;
};

class BSoundPlayer {
	friend class _SoundPlayNode;
public:
		enum sound_player_notification {
			B_STARTED = 1,
			B_STOPPED,
			B_SOUND_DONE
		};

		BSoundPlayer(
				const char * name = NULL,
				void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format) = NULL,
				void (*Notifier)(void *, sound_player_notification what, ...) = NULL,
				void * cookie = NULL);
		BSoundPlayer(
				const media_raw_audio_format * format, 
				const char * name = NULL,
				void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format) = NULL,
				void (*Notifier)(void *, sound_player_notification what, ...) = NULL,
				void * cookie = NULL);
		BSoundPlayer(
				const media_node & toNode,
				const media_multi_audio_format * format = NULL, 
				const char * name = NULL,
				const media_input * input = NULL,
				void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format) = NULL,
				void (*Notifier)(void *, sound_player_notification what, ...) = NULL,
				void * cookie = NULL);
virtual	~BSoundPlayer();

		status_t InitCheck();	//	new in R4.1
		media_raw_audio_format Format() const;	//	new in R4.1

		status_t Start();
		void Stop(
				bool block = true, 
				bool flush = true);

		typedef void (*BufferPlayerFunc)(void *, void *, size_t, const media_raw_audio_format &);
		BufferPlayerFunc BufferPlayer() const;
		void SetBufferPlayer(
			void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format));
		typedef void (*EventNotifierFunc)(void *, sound_player_notification what, ...);
		EventNotifierFunc EventNotifier() const;
		void SetNotifier(
			void (*Notifier)(void *, sound_player_notification what, ...));
		void * Cookie() const;
		void SetCookie(
			void * cookie);
		void SetCallbacks(		//	atomic change
				void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format) = NULL,
				void (*Notifier)(void *, sound_player_notification what, ...) = NULL,
				void * cookie = NULL);

		typedef int32 play_id;

		bigtime_t	CurrentTime();
		bigtime_t	PerformanceTime();
		status_t	Preroll();
		play_id		StartPlaying(
						BSound * sound,
						bigtime_t at_time = 0);
		play_id 	StartPlaying(
						BSound * sound,
						bigtime_t at_time,
						float with_volume);
		status_t 	SetSoundVolume(
						play_id sound,		//	update only non-NULL values
						float new_volume);
		bool 		IsPlaying(
						play_id id);
		status_t 	StopPlaying(
						play_id id);
		status_t 	WaitForSound(
						play_id id);

		float 		Volume();			/* 0 - 1.0 */
		void 		SetVolume(			/* 0 - 1.0 */
						float new_volume);
		float		VolumeDB(			/* -xx - +xx (see GetVolumeInfo()) */
						bool forcePoll = false);	/* if false, cached value will be used if new enough */
		void		SetVolumeDB(
						float volume_dB);
		status_t	GetVolumeInfo(
						media_node * out_node,
						int32 * out_parameter,
						float * out_min_dB,
						float * out_max_dB);
		bigtime_t	Latency();

virtual	bool HasData();
		void SetHasData(		//	this does more than just set a flag
				bool has_data);

protected:

		void SetInitError(		//	new in R4.1
				status_t in_error);

private:

virtual	status_t _Reserved_SoundPlayer_0(void *, ...);
virtual	status_t _Reserved_SoundPlayer_1(void *, ...);
virtual	status_t _Reserved_SoundPlayer_2(void *, ...);
virtual	status_t _Reserved_SoundPlayer_3(void *, ...);
virtual	status_t _Reserved_SoundPlayer_4(void *, ...);
virtual	status_t _Reserved_SoundPlayer_5(void *, ...);
virtual	status_t _Reserved_SoundPlayer_6(void *, ...);
virtual	status_t _Reserved_SoundPlayer_7(void *, ...);
 
		_SoundPlayNode * _m_node;
		struct _playing_sound {
			_playing_sound * next;
			off_t cur_offset;
			BSound * sound;
			play_id id;
			int32 delta;
			int32 rate;
			float volume;
		};
		_playing_sound * _m_sounds;
		struct _waiting_sound {
			_waiting_sound * next;
			bigtime_t start_time;
			BSound * sound;
			play_id id;
			int32 rate;
			float volume;
		};
		_waiting_sound * _m_waiting;
		void (*_PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format);
		void (*_Notifier)(void * cookie, sound_player_notification what, ...);
		BLocker _m_lock;
		float _m_volume;
		media_input m_input;
		media_output m_output;
		float * _m_mix_buffer;
		size_t _m_mix_buffer_size;
		void * _m_cookie;
		void * _m_buf;
		size_t _m_bufsize;
		int32 _m_has_data;

		status_t _m_init_err;		//	new in R4.1
		bigtime_t _m_perfTime;
		BContinuousParameter * _m_volumeSlider;
		bigtime_t _m_gotVolume;
		uint32 _m_reserved[10];

		void NotifySoundDone(
				play_id sound,
				bool got_to_play);

		void get_volume_slider();
		void Init(
				const media_node * node,
				const media_multi_audio_format * format, 
				const char * name,
				const media_input * input,
				void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
				void (*Notifier)(void *, sound_player_notification what, ...),
				void * cookie);
		//	for B_STARTED and B_STOPPED, the argument is BSoundPlayer* (this)
		//	for B_SOUND_DONE, the arguments are play_id and true/false for whether it got to play data at all
virtual	void Notify(
				sound_player_notification what,
				...);
		//	get data into the buffer to play -- this version will use the queued BSound system
virtual	void PlayBuffer(
				void * buffer,
				size_t size,
				const media_raw_audio_format & format);
};


#endif /* _SOUND_PLAYER_H */
