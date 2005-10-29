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
//	File Name:		GameSound.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	Base class for play sounds using the game kit
//------------------------------------------------------------------------------

#ifndef _GAMESOUND_H
#define _GAMESOUND_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <GameSoundDefs.h>
#include <new>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
class BGameSoundDevice;

// BGameSound class -------------------------------------------------------------
class BGameSound
{
public:
									BGameSound(BGameSoundDevice * device = NULL);
	virtual							~BGameSound();

	virtual	BGameSound *			Clone() const = 0;
			status_t				InitCheck() const;
			
			// BGameSound attributes
			BGameSoundDevice *		Device() const;
			gs_id					ID() const;
			const gs_audio_format & Format() const;		//	only valid after Init()

			// Playing sounds
	virtual	status_t				StartPlaying();
	virtual	bool					IsPlaying();
	virtual	status_t				StopPlaying();

			
			// Modifing the playback
			status_t				SetGain(float gain,
											bigtime_t duration = 0);	//	ramp duration in seconds
			status_t				SetPan(float pan,
											bigtime_t duration = 0);	//	ramp duration in seconds
			float					Gain();
			float					Pan();

	virtual	status_t				SetAttributes(gs_attribute * inAttributes,
													size_t inAttributeCount);
	virtual	status_t				GetAttributes(gs_attribute * outAttributes,
													size_t inAttributeCount);

			void * 					operator new(size_t size);
			void *					operator new(size_t size, const nothrow_t &) throw();
			void					operator delete(void * ptr);
#if !__MWERKS__
			//	there's a bug in MWCC under R4.1 and earlier
			void					operator delete(void * ptr, 
													const nothrow_t &) throw();
#endif

			static	status_t		SetMemoryPoolSize(size_t in_poolSize);
			static	status_t		LockMemoryPool(bool in_lockInCore);
			static	int32			SetMaxSoundCount(int32 in_maxCount);
			
			virtual	status_t 		Perform(int32 selector, void * data);
protected:

			status_t				SetInitError(status_t in_initError);
			status_t				Init(gs_id handle);

									BGameSound(const BGameSound & other);
			BGameSound &			operator=(const BGameSound & other);
			
private:
			BGameSoundDevice*		fDevice;
			status_t				fInitError;
			
			gs_audio_format			fFormat;		
			gs_id					fSound;
	
	/* leave these declarations private unless you plan on actually implementing and using them. */
	BGameSound();

	/* fbc data and virtuals */

	uint32 _reserved_BGameSound_[16];

virtual	status_t _Reserved_BGameSound_0(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_1(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_2(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_3(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_4(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_5(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_6(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_7(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_8(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_9(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_10(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_11(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_12(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_13(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_14(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_15(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_16(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_17(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_18(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_19(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_20(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_21(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_22(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_23(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_24(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_25(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_26(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_27(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_28(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_29(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_30(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_31(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_32(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_33(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_34(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_35(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_36(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_37(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_38(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_39(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_40(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_41(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_42(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_43(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_44(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_45(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_46(int32 arg, ...);
virtual	status_t _Reserved_BGameSound_47(int32 arg, ...);

};


#endif	//	_GAME_SOUND_H
