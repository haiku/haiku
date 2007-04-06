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
//	File Name:		FileGameSound.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BFileGameSound is a class that streams data out of a file.
//------------------------------------------------------------------------------

#ifndef _FILEGAMESOUND_H
#define _FILEGAMESOUND_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <StreamingGameSound.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
struct _gs_media_tracker;
struct _gs_ramp;

// FileGameSound -------------------------------------------------------------
class BFileGameSound : public BStreamingGameSound
{
public:
							BFileGameSound(const entry_ref * file,
											bool looping = true,
											BGameSoundDevice * device = NULL);
							BFileGameSound(const char * file,
											bool looping = true,
											BGameSoundDevice * device = NULL);

	virtual					~BFileGameSound();

	virtual	BGameSound *	Clone() const;

	virtual	status_t		StartPlaying();
	virtual	status_t		StopPlaying();
			status_t		Preload();		//	if you have stopped and want to start quickly again

	virtual	void			FillBuffer(void * inBuffer,
										size_t inByteCount);
	virtual	status_t 		Perform(int32 selector, void * data);
	virtual	status_t		SetPaused(bool isPaused,
										bigtime_t rampTime);
			enum {
				B_NOT_PAUSED,
				B_PAUSE_IN_PROGRESS,
				B_PAUSED
			};
			int32			IsPaused();

private:

			_gs_media_tracker *	fAudioStream;
			
			bool				fStopping;
			bool				fLooping;
			char				fReserved;
			char *				fBuffer;
			
			size_t				fFrameSize;
			size_t				fBufferSize;
			size_t				fPlayPosition;
			
			thread_id			fReadThread;
			port_id				fPort;
			
			_gs_ramp *			fPausing;
			bool				fPaused;
			float				fPauseGain;
			
			status_t			Init(const entry_ref* file);
			
			bool				Load();
			bool				Read(void * buffer, size_t bytes);
				
	/* leave these declarations private unless you plan on actually implementing and using them. */
	BFileGameSound();
	BFileGameSound(const BFileGameSound&);
	BFileGameSound& operator=(const BFileGameSound&);

	/* fbc data and virtuals */

	uint32 _reserved_BFileGameSound_[9];

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
