/*
 *  Copyright 2020 Haiku, Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 *
 * Author:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */

#ifndef _STREAMINGGAMESOUND_H
#define _STREAMINGGAMESOUND_H


#include <GameSound.h>
#include <Locker.h>
#include <SupportDefs.h>


class BStreamingGameSound : public BGameSound
{
public:
						BStreamingGameSound(size_t bufferFrameCount,
							const gs_audio_format* format,
							size_t bufferCount = 2,
							BGameSoundDevice* device = NULL);

	virtual				~BStreamingGameSound();

	virtual	BGameSound*	Clone() const;

	typedef	void		(*hook)(void* cookie, void* buffer, size_t byteCount,
							BStreamingGameSound* me);

	virtual	status_t	SetStreamHook(hook h, void* cookie);

	virtual	void		FillBuffer(void* buffer, size_t byteCount);

	virtual	status_t	SetAttributes(gs_attribute* attributes,
							size_t attributeCount);

	virtual	status_t	Perform(int32 selector, void* data);

protected:
						BStreamingGameSound(BGameSoundDevice* device);

	virtual	status_t	SetParameters(size_t bufferFrameCount,
							const gs_audio_format* format,
							size_t bufferCount);

			bool		Lock();
			void		Unlock();

private:
						BStreamingGameSound();
						BStreamingGameSound(const BStreamingGameSound& other);
						BStreamingGameSound&
							operator=(const BStreamingGameSound& other);

	virtual	status_t	_Reserved_BStreamingGameSound_0(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_1(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_2(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_3(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_4(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_5(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_6(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_7(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_8(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_9(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_10(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_11(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_12(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_13(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_14(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_15(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_16(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_17(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_18(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_19(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_20(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_21(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_22(int32 arg, ...);
	virtual	status_t	_Reserved_BStreamingGameSound_23(int32 arg, ...);

private:
			hook		fStreamHook;
			void*		fStreamCookie;
			BLocker		fLock;

			uint32		_reserved[12];
};

#endif
