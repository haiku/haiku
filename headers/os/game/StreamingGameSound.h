/*******************************************************************************
/
/	File:			StreamingGameSound.h
/
/   Description:    BStreamingGameSound is a class for all kinds of streaming
/					(data not known beforehand) game sounds.
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined(_STREAMING_GAME_SOUND_H)
#define _STREAMING_GAME_SOUND_H

#include <SupportDefs.h>
#include <GameSound.h>
#include <Locker.h>

class BStreamingGameSound : public BGameSound
{
public:
							BStreamingGameSound(
									size_t inBufferFrameCount,
									const gs_audio_format * format,
									size_t inBufferCount = 2,
									BGameSoundDevice * device = NULL);

virtual						~BStreamingGameSound();

virtual	BGameSound *		Clone() const;

virtual	status_t			SetStreamHook(
									void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
									void * cookie);
virtual	void				FillBuffer(			//	default is to call stream hook, if any
									void * inBuffer,
									size_t inByteCount);

virtual	status_t			SetAttributes(
									gs_attribute * inAttributes,
									size_t inAttributeCount);

virtual	status_t Perform(int32 selector, void * data);

protected:

							BStreamingGameSound(
									BGameSoundDevice * device);
virtual	status_t			SetParameters(			//	will allocate handle and call Init()
									size_t inBufferFrameCount,
									const gs_audio_format * format,
									size_t inBufferCount);

		bool				Lock();
		void				Unlock();

private:

		void				(*_m_streamHook)(void * cookie, void * buffer, size_t bytes, BStreamingGameSound * sound);
		void *				_m_streamCookie;
		BLocker				_m_lock;

static	void				stream_callback(void * cookie, gs_id handle, void * buffer, int32 offset, int32 size, size_t bufSize);

	/* leave these declarations private unless you plan on actually implementing and using them. */
	BStreamingGameSound();
	BStreamingGameSound(const BStreamingGameSound&);
	BStreamingGameSound& operator=(const BStreamingGameSound&);

	/* fbc data and virtuals */

	uint32 _reserved_BStreamingGameSound_[12];

virtual	status_t _Reserved_BStreamingGameSound_0(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BStreamingGameSound_23(int32 arg, ...);

};

#endif	//	_STREAMING_GAME_SOUND_H
