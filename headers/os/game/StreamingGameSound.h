/*****************************************************************************/
// StreamingGameSound.h
//
// This is "a class for all kinds of streaming (data not known beforehand)
// game sounds."
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

#ifndef GAME_KIT_STREAMING_GAME_SOUND
#define GAME_KIT_STREAMING_GAME_SOUND

#include <SupportDefs.h>
#include <GameSound.h>
#include <Locker.h>

class BStreamingGameSound : public BGameSound
{
public:
	BStreamingGameSound( size_t inBufferFrameCount,
		const gs_audio_format* pFormat, size_t inBufferCount = 2,
		BGameSoundDevice* pDevice = NULL );

	virtual ~BStreamingGameSound();

	virtual	BGameSound* Clone() const;

	virtual	status_t SetStreamHook( 
		void (*hook)( void* pInCookie, void* pInBuffer, size_t inByteCount, BStreamingGameSound* pMe ),
		void* pCookie );
		
	// The default is to call stream hook, if any.
	virtual	void FillBuffer( void * inBuffer, size_t inByteCount );

	virtual	status_t SetAttributes( gs_attribute* inAttributes,
		size_t inAttributeCount );

	virtual	status_t Perform( int32 selector, void* pData );

protected:
	BStreamingGameSound( BGameSoundDevice* pDevice );
	
	// Will allocate handle and call Init().
	virtual	status_t SetParameters( size_t inBufferFrameCount,
		const gs_audio_format* pFormat, size_t inBufferCount );

	bool Lock();
	void Unlock();

private:

	BLocker _m_lock;

	static void stream_callback( void* pCookie, gs_id handle, 
		void* pBuffer, int32 offset, int32 size, size_t bufSize );

	// Leave these declarations private unless you plan on actually 
	// implementing and using them.
	BStreamingGameSound();
	BStreamingGameSound( const BStreamingGameSound& );
	BStreamingGameSound& operator=( const BStreamingGameSound& );
};

#endif // GAME_KIT_STREAMING_GAME_SOUND
