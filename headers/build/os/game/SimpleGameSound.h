//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		SimpleGameSound.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BSimpleGameSound is a class for sound effects that are 
//					short, and consists of non-changing samples in memory.
//------------------------------------------------------------------------------


#ifndef _SIMPLEGAMESOUND_H
#define _SIMPLEGAMESOUND_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <GameSoundDefs.h>
#include <GameSound.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// BGameSound class -------------------------------------------------------------
class BSimpleGameSound : public BGameSound
{
public:
							BSimpleGameSound(const entry_ref * inFile,
											 BGameSoundDevice * device = NULL);
							BSimpleGameSound(const char * inFile,
											 BGameSoundDevice * device = NULL);
							BSimpleGameSound(const void * inData,
											 size_t inFrameCount,
											 const gs_audio_format * format,
											 BGameSoundDevice * device = NULL);

							BSimpleGameSound(const BSimpleGameSound & other);

	virtual					~BSimpleGameSound();

	virtual	BGameSound *	Clone() const;

	virtual	status_t Perform(int32 selector, void * data);

			status_t		SetIsLooping(bool looping);
			bool			IsLooping() const;

private:
			status_t		Init(const entry_ref * inFile);
			status_t		Init(const void* inData,
									int64 inFrameCount,
									const gs_audio_format* format);
				
	/* leave these declarations private unless you plan on actually implementing and using them. */
	BSimpleGameSound();
	BSimpleGameSound& operator=(const BSimpleGameSound&);

	/* fbc data and virtuals */

	uint32 _reserved_BSimpleGameSound_[12];

virtual	status_t _Reserved_BSimpleGameSound_0(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BSimpleGameSound_23(int32 arg, ...);

};

#endif	//	_SIMPLE_GAME_SOUND_H
