/*****************************************************************************/
// PushGameSound.h
//
// This is "a class for games that want to push data at the system, 
// rather than have a callback get called."
//
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef GAME_KIT_PUSH_GAME_SOUND
#define GAME_KIT_PUSH_GAME_SOUND

#include <StreamingGameSound.h>

class BPushGameSound : public BStreamingGameSound
{
public:
	BPushGameSound( size_t inBufferFrameCount,
		const gs_audio_format* pFormat, size_t inBufferCount = 2,
		BGameSoundDevice* pDevice = NULL );

	virtual ~BPushGameSound();

	enum lock_status 
	{
		lock_failed = -1,		//	not yet time to do more
		lock_ok = 0,			//	do more
		lock_ok_frames_dropped	//	you may have missed some buffers
	};

	virtual	lock_status LockNextPage( void** ppOut_pagePtr,
		size_t* pOut_pageSize );
	
	virtual	status_t UnlockPage( void* pIn_pagePtr );

	virtual	lock_status LockForCyclic( void** ppOut_basePtr,
		size_t* pOut_size );

	virtual	status_t UnlockCyclic();
	virtual	size_t	 CurrentPosition();

	virtual	BGameSound* Clone() const;

	virtual	status_t Perform( int32 selector, void* pData );

protected:

	// Will allocate handle and call Init().
	virtual	status_t SetParameters( size_t inBufferFrameCount,
		const gs_audio_format* pFormat, size_t inBufferCount );

	virtual	status_t SetStreamHook( 
		void (*hook)( void* pInCookie, void* pInBuffer, size_t inByteCount, BStreamingGameSound* pMe ),
		void* pCookie );
		
	// Default is to call stream hook, if any.
	virtual	void FillBuffer( void* pInBuffer, size_t inByteCount );

private:

	// Leave these declarations private unless you plan on actually 
	// implementing and using them.
	BPushGameSound();
	BPushGameSound( const BPushGameSound& );
	BPushGameSound& operator=( const BPushGameSound& );
};

#endif // GAME_KIT_PUSH_GAME_SOUND