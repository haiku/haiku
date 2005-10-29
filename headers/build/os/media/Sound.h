/*	BSound.h	*/
/*	Copyright 1998 Be Incorporated. All rights reserved.	*/

#if !defined(_SOUND_H)
#define _SOUND_H

#include <MediaDefs.h>

class BSoundFile;

namespace BPrivate {
	class BTrackReader;
}

class BSound {
public:
		BSound(
				void * data,
				size_t size,
				const media_raw_audio_format & format,
				bool free_when_done = false);
		BSound(
				const entry_ref * sound_file,
				bool load_into_memory = false);

		status_t InitCheck();
		BSound * AcquireRef();
		bool ReleaseRef();
		int32 RefCount() const;	// unreliable!

virtual	bigtime_t Duration() const;
virtual	const media_raw_audio_format & Format() const;
virtual	const void * Data() const;	/* returns NULL for files */
virtual	off_t Size() const;

virtual	bool GetDataAt(
				off_t offset,
				void * into_buffer,
				size_t buffer_size,
				size_t * out_used);

protected:

		BSound(
				const media_raw_audio_format & format);

virtual	status_t Perform(
				int32 code,
				...);

private:

		BSound(const BSound &);	//	unimplemented
		BSound & operator=(const BSound &);	//	unimplemented

		friend class BSoundPlayer;
		friend class _HostApp;

		void Reset();

virtual	~BSound();

		void free_data();
static	status_t load_entry(void * arg);
		void loader_thread();
		bool check_stop();

public:

virtual	status_t BindTo(
				BSoundPlayer * player,
				const media_raw_audio_format & format);
virtual	status_t UnbindFrom(
				BSoundPlayer * player);

private:
		status_t _Reserved_Sound_0(void *);	//	BindTo
		status_t _Reserved_Sound_1(void *);	//	UnbindFrom
virtual	status_t _Reserved_Sound_2(void *);
virtual	status_t _Reserved_Sound_3(void *);
virtual	status_t _Reserved_Sound_4(void *);
virtual	status_t _Reserved_Sound_5(void *);

		void * _m_data;
		BMediaFile * _m_file;
		int32 _m_ref_count;
		status_t _m_error;
		size_t _m_size;
		media_raw_audio_format _m_format;
		bool _m_free_when_done;
		bool _m_checkStopped;
		bool _m_reserved[2];
		area_id _m_area;
		sem_id _m_avail_sem;
		sem_id _m_free_sem;
		thread_id _m_loader_thread;
		size_t _m_read_pos;
		sem_id _m_check_token;
		int32 _m_prev_sem_count;

		BSoundPlayer * _m_bound_player;
		int32 _m_bind_flags;

		BPrivate::BTrackReader * _m_trackReader;
		char m_tname[32];
		uint32 _reserved_[1];

};


#endif	/* _SOUND_H */
