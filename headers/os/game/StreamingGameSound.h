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
//	File Name:		StreamingGameSound.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BStreamingGameSound is a class for all kinds of streaming
//					(data not known beforehand) game sounds.
//------------------------------------------------------------------------------


#ifndef _STREAMINGGAMESOUND_H
#define _STREAMINGGAMESOUND_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <SupportDefs.h>
#include <GameSound.h>
#include <Locker.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BStreamingGameSound : public BGameSound
{
public:
							BStreamingGameSound(size_t inBufferFrameCount,
												const gs_audio_format * format,
												size_t inBufferCount = 2,
												BGameSoundDevice * device = NULL);

	virtual					~BStreamingGameSound();

	virtual	BGameSound *	Clone() const;

	virtual	status_t		SetStreamHook(void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
										  void * cookie);
	virtual	void			FillBuffer(void * inBuffer,
										size_t inByteCount);

	virtual	status_t		SetAttributes(gs_attribute * inAttributes,
										  size_t inAttributeCount);

	virtual	status_t 		Perform(int32 selector, void * data);

protected:

							BStreamingGameSound(BGameSoundDevice * device);
	
	virtual status_t		SetParameters(size_t inBufferFrameCount,
								 		  const gs_audio_format * format,
								 		  size_t inBufferCount);
		
			bool			Lock();
			void			Unlock();

private:

		void				(*fStreamHook)(void * cookie, void * buffer, size_t bytes, BStreamingGameSound * sound);
		void *				fStreamCookie;
		BLocker				fLock;

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
