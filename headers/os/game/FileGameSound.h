/*******************************************************************************
/
/	File:			FileGameSound.h
/
/   Description:    BFileGameSound is a class that streams data out of a file.
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_FILE_GAME_SOUND_H)
#define _FILE_GAME_SOUND_H

#include <StreamingGameSound.h>

class BMediaFile;
namespace BPrivate {
	class BTrackReader;
}

class BFileGameSound : public BStreamingGameSound
{
public:
							BFileGameSound(
									const entry_ref * file,
									bool looping = true,
									BGameSoundDevice * device = NULL);
							BFileGameSound(
									const char * file,
									bool looping = true,
									BGameSoundDevice * device = NULL);

virtual						~BFileGameSound();

virtual	BGameSound *		Clone() const;

virtual	status_t			StartPlaying();
virtual	status_t			StopPlaying();
		status_t			Preload();		//	if you have stopped and want to start quickly again

virtual	void				FillBuffer(
									void * inBuffer,
									size_t inByteCount);

virtual	status_t Perform(int32 selector, void * data);

virtual	status_t			SetPaused(
									bool isPaused,
									bigtime_t rampTime);
		enum {
			B_NOT_PAUSED,
			B_PAUSE_IN_PROGRESS,
			B_PAUSED
		};
		int32				IsPaused();

private:

		BMediaFile *		_m_soundFile;
		BPrivate::BTrackReader *	_m_trackReader;
		bool				_m_looping;
		bool				_m_stopping;
		uchar				_m_zero;
		uchar				_m_reserved_u;

		//	cruft for read-ahead goes here
		thread_id			_m_streamThread;
		port_id				_m_streamPort;
		enum {
			cmdStartPlaying = 1,
			cmdStopPlaying,
			cmdUsedBytes,
			cmdPreload
		};
		int32				_m_loopCount;
		char *				_m_buffer;
		size_t				_m_bufferSize;
		int32				_m_stopCount;
		int32				_m_pauseState;
		float				_m_pauseGain;
		float				_m_pausePan;

static	status_t			stream_thread(
									void * that);
		void				LoadFunc();
		size_t				Load(
									size_t & offset,
									size_t bytes);
		void				read_data(
									char * dest,
									size_t bytes);

	/* leave these declarations private unless you plan on actually implementing and using them. */
	BFileGameSound();
	BFileGameSound(const BFileGameSound&);
	BFileGameSound& operator=(const BFileGameSound&);

	/* fbc data and virtuals */

	uint32 _reserved_BFileGameSound_[8];

		status_t _Reserved_BFileGameSound_0(int32 arg, ...);	/* SetPaused(bool paused, bigtime_t ramp); */
virtual	status_t _Reserved_BFileGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BFileGameSound_23(int32 arg, ...);
};

#endif
