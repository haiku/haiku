/*****************************************************************************/
// GameSound.h
//
// This is an abstract base class for "all sounds being played using the
// gamesound kit. Use one of the concrete subclasses for actually playing
// sound."
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

#ifndef GAME_KIT_GAME_SOUND
#define GAME_KIT_GAME_SOUND

#include <GameSoundDefs.h>
#include <new>

struct gs_audio_format;
class BGameSoundDevice;


class BGameSound
{
public:
	BGameSound( BGameSoundDevice* pDevice = NULL );
	virtual ~BGameSound();

	virtual	BGameSound* Clone() const = 0;
	status_t InitCheck() const;

	BGameSoundDevice* Device() const;
	gs_id ID() const;
	
	// Only valid after Init().
	const gs_audio_format& Format() const;

	virtual	status_t StartPlaying();
	virtual	bool IsPlaying();
	virtual	status_t StopPlaying();
	
	// Ramp duration is in seconds.
	status_t SetGain( float gain, bigtime_t duration = 0 );
	status_t SetPan( float pan, bigtime_t duration = 0 );
	float Gain();
	float Pan();

	virtual	status_t SetAttributes( gs_attribute* pInAttributes,
		size_t inAttributeCount );
	virtual	status_t GetAttributes( gs_attribute* pOutAttributes,
		size_t outAttributeCount );

	virtual	status_t Perform( int32 selector, void * data );

	void* operator new( size_t size );
	void* operator new( size_t size, const nothrow_t& ) throw();
	void  operator delete( void* ptr );
	void  operator delete( void* ptr, const nothrow_t& ) throw();

	// These were implemented in BPrivGameSound, presumably
	// for reasons of privacy. If that's the only reason, then
	// we can put them here, because you've got the source, my man.
	// Otherwise, we'll put 'em where they have to be.
	static status_t SetMemoryPoolSize( size_t in_poolSize );
	static status_t LockMemoryPool( bool in_lockInCore );
	static int32 SetMaxSoundCount( int32 in_maxCount );

protected:
	status_t SetInitError( status_t in_initError );
	status_t Init( gs_id handle );

	BGameSound( const BGameSound& other );
	BGameSound& operator=( const BGameSound & other );
	
private:
	BGameSound();
	
	// Our implementation details go here.
};

#endif // GAME_KIT_GAME_SOUND