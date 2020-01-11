/*
 *  Copyright 2020, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License.
 */
#ifndef _SIMPLEGAMESOUND_H
#define _SIMPLEGAMESOUND_H


#include <GameSound.h>
#include <GameSoundDefs.h>

struct entry_ref;

class BSimpleGameSound : public BGameSound {
public:
							BSimpleGameSound(const entry_ref* file,
								BGameSoundDevice* device = NULL);

							BSimpleGameSound(const char* file,
								BGameSoundDevice* device = NULL);

							BSimpleGameSound(const void* data,
								size_t frameCount,
								const gs_audio_format* format,
								BGameSoundDevice* device = NULL);

							BSimpleGameSound(const BSimpleGameSound& other);

	virtual					~BSimpleGameSound();

	virtual	BGameSound*		Clone() const;
	virtual	status_t 		Perform(int32 selector, void* data);
			status_t		SetIsLooping(bool looping);
			bool			IsLooping() const;
private:
							BSimpleGameSound();

	BSimpleGameSound&		operator=(const BSimpleGameSound& other);

			status_t		Init(const entry_ref* file);
			status_t 		Init(const void* data, int64 frameCount,
								const gs_audio_format* format);

	virtual	status_t		_Reserved_BSimpleGameSound_0(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_1(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_2(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_3(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_4(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_5(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_6(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_7(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_8(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_9(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_10(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_11(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_12(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_13(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_14(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_15(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_16(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_17(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_18(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_19(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_20(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_21(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_22(int32 arg, ...);
	virtual	status_t		_Reserved_BSimpleGameSound_23(int32 arg, ...);
private:
			uint32			_reserved[12];
};

#endif
