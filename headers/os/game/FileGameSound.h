/*****************************************************************************/
// FileGameSound.h
//
// This is "a class that streams data out of a file."
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

#ifndef GAME_KIT_FILE_GAME_SOUND
#define GAME_KIT_FILE_GAME_SOUND

#include <StreamingGameSound.h>

class BMediaFile;
namespace BPrivate 
{
	class BTrackReader;
}

class BFileGameSound : public BStreamingGameSound
{
public:
	BFileGameSound( const entry_ref* pFile, bool looping = true,
		BGameSoundDevice* pDevice = NULL );
	BFileGameSound( const char* pFilename, bool looping = true,
		BGameSoundDevice* pDevice = NULL );

	virtual ~BFileGameSound();

	virtual	BGameSound* Clone() const;

	virtual	status_t StartPlaying();
	virtual	status_t StopPlaying();
	
	// Use this if you have stopped and want to start again quickly.
	status_t Preload();

	virtual	void FillBuffer( void* pInBuffer, size_t inByteCount );

	virtual	status_t Perform( int32 selector, void* pData );

	virtual	status_t SetPaused( bool isPaused, bigtime_t rampTime );

	enum
	{
		B_NOT_PAUSED,
		B_PAUSE_IN_PROGRESS,
		B_PAUSED
	};

	int32 IsPaused();

private:
	// Leave these declarations private unless you plan on actually 
	// implementing and using them.
	BFileGameSound();
	BFileGameSound( const BFileGameSound& );
	BFileGameSound& operator=( const BFileGameSound& );
};

#endif // GAME_KIT_FILE_GAME_SOUND
