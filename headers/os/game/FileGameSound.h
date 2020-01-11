/*
 *  Copyright 2020, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 *
 * Author:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */

#ifndef _FILEGAMESOUND_H
#define _FILEGAMESOUND_H


#include <StreamingGameSound.h>
#include <DataIO.h>


struct entry_ref;
struct _gs_media_tracker;
struct _gs_ramp;


class BFileGameSound : public BStreamingGameSound {
public:

								BFileGameSound(const entry_ref* file,
									bool looping = true,
									BGameSoundDevice* device = NULL);

								BFileGameSound(const char* file,
									bool looping = true,
									BGameSoundDevice* device = NULL);

								BFileGameSound(BDataIO* data,
									bool looping = true,
									BGameSoundDevice* device = NULL);

	virtual						~BFileGameSound();

	virtual	BGameSound*			Clone() const;

	virtual	status_t			StartPlaying();
	virtual	status_t			StopPlaying();
	status_t					Preload();

	virtual	void				FillBuffer(void* buffer, size_t byteCount);
	virtual	status_t 			Perform(int32 selector, void* data);
	virtual	status_t			SetPaused(bool isPaused, bigtime_t rampTime);

			enum {
				B_NOT_PAUSED,
				B_PAUSE_IN_PROGRESS,
				B_PAUSED
			};

			int32				IsPaused();
private:
								BFileGameSound();
								BFileGameSound(const BFileGameSound& other);

			BFileGameSound&		operator=(const BFileGameSound& other);

			status_t			Init(BDataIO* data);

			bool				Load();
			bool				Read(void* buffer, size_t bytes);

			status_t			_Reserved_BFileGameSound_0(int32 arg, ...);
								// SetPaused(bool paused, bigtime_t ramp);
	virtual	status_t			_Reserved_BFileGameSound_1(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_2(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_3(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_4(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_5(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_6(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_7(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_8(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_9(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_10(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_11(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_12(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_13(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_14(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_15(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_16(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_17(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_18(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_19(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_20(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_21(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_22(int32 arg, ...);
	virtual	status_t			_Reserved_BFileGameSound_23(int32 arg, ...);
private:
			_gs_media_tracker*	fAudioStream;

			bool				fStopping;
			bool				fLooping;
			char				fReserved;
			char*				fBuffer;

			size_t				fFrameSize;
			size_t				fBufferSize;
			size_t				fPlayPosition;

			_gs_ramp*			fPausing;
			bool				fPaused;
			float				fPauseGain;

			BDataIO*			fDataSource;

#ifdef B_HAIKU_64_BIT
			uint32				_reserved[9];
#else
			uint32				_reserved[10];
#endif
};

#endif
