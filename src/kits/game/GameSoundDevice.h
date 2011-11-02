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
//	File Name:		GameSoundDevice.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	Manages the game producer. The class may change with out
//					notice and was only inteneded for use by the GameKit at 
//					this time. Use at your own risk.
//------------------------------------------------------------------------------

#ifndef _GAMESOUNDDEVICE_H
#define _GAMESOUNDDEVICE_H

#include <GameSoundDefs.h>


class BMediaNode;
class GameSoundBuffer;
struct Connection;

class BGameSoundDevice { 
public:
									BGameSoundDevice();
	virtual							~BGameSoundDevice();

	status_t						InitCheck() const;
	virtual const gs_audio_format &	Format() const;		
	virtual const gs_audio_format &	Format(gs_id sound) const;
		
	virtual	status_t				CreateBuffer(gs_id * sound,
										const gs_audio_format * format,
										const void * data,
										int64 frames);
	virtual status_t				CreateBuffer(gs_id * sound,
										const void * object,
										const gs_audio_format * format);
	virtual void					ReleaseBuffer(gs_id sound);
	
	virtual status_t				Buffer(gs_id sound,
										gs_audio_format * format,
										void *& data);
			
	virtual	bool					IsPlaying(gs_id sound);
	virtual	status_t				StartPlaying(gs_id sound);
	virtual status_t				StopPlaying(gs_id sound);
			
	virtual	status_t				GetAttributes(gs_id sound,
										gs_attribute * attributes,
										size_t attributeCount);
	virtual status_t				SetAttributes(gs_id sound,
										gs_attribute * attributes,
										size_t attributeCount);
											  	
protected:
			void					SetInitError(status_t error);
				
			gs_audio_format			fFormat;											
private:
			int32					AllocateSound();
			
			status_t				fInitError;
		
			bool					fIsConnected;
			
			int32					fSoundCount;
			GameSoundBuffer **		fSounds;
};

BGameSoundDevice* 	GetDefaultDevice();
void				ReleaseDevice();

#endif
