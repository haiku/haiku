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

#ifndef _PUSHGAMESOUND_H
#define _PUSHGAMESOUND_H

#include <StreamingGameSound.h>


class BPushGameSound : public BStreamingGameSound
{
public:
							BPushGameSound(size_t inBufferFrameCount,
											const gs_audio_format * format,
											size_t inBufferCount = 2,
											BGameSoundDevice * device = NULL);

	virtual					~BPushGameSound();

		enum lock_status {
			lock_failed = -1,		//	not yet time to do more
			lock_ok = 0,			//	do more
			lock_ok_frames_dropped	//	you may have missed some buffers
		};

	virtual	lock_status		LockNextPage(void ** out_pagePtr,
											size_t * out_pageSize);
	virtual	status_t		UnlockPage(void * in_pagePtr);

	virtual	lock_status		LockForCyclic(void ** out_basePtr,
											size_t * out_size);
	virtual	status_t		UnlockCyclic();
	virtual	size_t			CurrentPosition();

	virtual	BGameSound *	Clone() const;

	virtual	status_t 		Perform(int32 selector, void * data);

protected:
		
							BPushGameSound(BGameSoundDevice * device);

	virtual	status_t		SetParameters(size_t inBufferFrameCount,
												const gs_audio_format * format,
												size_t inBufferCount);

	virtual	status_t		SetStreamHook(void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
												void * cookie);
												
	virtual	void			FillBuffer(void * inBuffer,
											size_t inByteCount);


private:

			/* leave these declarations private unless you plan on actually implementing and using them. */
			BPushGameSound();
			BPushGameSound(const BPushGameSound&);
			BPushGameSound& operator=(const BPushGameSound&);

			bool			BytesReady(size_t * bytes);
			
			sem_id			fLock;
			BList *			fPageLocked;
			
			size_t			fLockPos;
			size_t			fPlayPos;
			
			char *			fBuffer;
			size_t			fPageSize;
			int32			fPageCount;
			size_t			fBufferSize;

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
