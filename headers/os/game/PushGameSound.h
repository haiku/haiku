/*******************************************************************************
/
/	File:			PushGameSound.h
/
/   Description:    BPushGameSound is a class for games that want to push data
/					at the system, rather than have a callback get called.
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined(_PUSH_GAME_SOUND_H)
#define _PUSH_GAME_SOUND_H

#include <StreamingGameSound.h>

class BPushGameSound : public BStreamingGameSound
{
public:
							BPushGameSound(
									size_t inBufferFrameCount,
									const gs_audio_format * format,
									size_t inBufferCount = 2,
									BGameSoundDevice * device = NULL);

virtual						~BPushGameSound();

		enum lock_status {
			lock_failed = -1,		//	not yet time to do more
			lock_ok = 0,			//	do more
			lock_ok_frames_dropped	//	you may have missed some buffers
		};

virtual	lock_status		LockNextPage(
									void ** out_pagePtr,
									size_t * out_pageSize);
virtual	status_t			UnlockPage(
									void * in_pagePtr);

virtual	lock_status		LockForCyclic(
									void ** out_basePtr,
									size_t * out_size);
virtual	status_t			UnlockCyclic();
virtual	size_t				CurrentPosition();

virtual	BGameSound *		Clone() const;

virtual	status_t Perform(int32 selector, void * data);

protected:

virtual	status_t			SetParameters(			//	will allocate handle and call Init()
									size_t inBufferFrameCount,
									const gs_audio_format * format,
									size_t inBufferCount);

virtual	status_t			SetStreamHook(
									void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
									void * cookie);
virtual	void				FillBuffer(			//	default is to call stream hook, if any
									void * inBuffer,
									size_t inByteCount);


private:

	/* leave these declarations private unless you plan on actually implementing and using them. */
	BPushGameSound();
	BPushGameSound(const BPushGameSound&);
	BPushGameSound& operator=(const BPushGameSound&);

		int32				_m_benCount;
		sem_id				_m_benSem;
		char *				_m_lockedPtr;
		char *				_m_playedPtr;
		size_t				_m_pageSize;
		int32				_m_pageCount;
		size_t				_m_bufSize;
		char *				_m_buffer;

	/* fbc data and virtuals */

	uint32 _reserved_BPushGameSound_[12];

virtual	status_t _Reserved_BPushGameSound_0(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BPushGameSound_23(int32 arg, ...);

};

#endif	//	_PUSH_GAME_SOUND_H
