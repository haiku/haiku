/*****************************************************************************/
// SimpleGameSound.h
//
// This is "a class for sound effects that are short, and consist of 
// non-changing samples in memory."
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

#ifndef GAME_KIT_SIMPLE_GAME_SOUND
#define GAME_KIT_SIMPLE_GAME_SOUND

#include <GameSound.h>

class BSimpleGameSound : public BGameSound
{
public:
	BSimpleGameSound( const entry_ref* pInFile, 
		BGameSoundDevice* pDevice = NULL );
	BSimpleGameSound( const char* pInFile,
		BGameSoundDevice* pDevice = NULL );
	BSimpleGameSound( const void* pInData,
		size_t inFrameCount, const gs_audio_format* pFormat,
		BGameSoundDevice* pDevice = NULL );

	BSimpleGameSound( const BSimpleGameSound& other );

	virtual ~BSimpleGameSound();

	virtual	BGameSound* Clone() const;

	virtual	status_t Perform( int32 selector, void* pData );

	status_t SetIsLooping( bool looping );
	bool IsLooping() const;

private:

	// Leave these declarations private unless you plan on actually 
	// implementing and using them.
	BSimpleGameSound();
	BSimpleGameSound& operator=( const BSimpleGameSound& );
};

#endif