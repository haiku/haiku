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
//	File Name:		GameSoundBuffer.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	Interface to a single sound, managed by the GameSoundDevice.
//------------------------------------------------------------------------------

#ifndef _GAMESOUNDBUFFER_H
#define _GAMESOUNDBUFFER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <GameSoundDefs.h>
#include <MediaDefs.h>
#include <MediaNode.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
const int32 kPausedAttribute = B_GS_FIRST_PRIVATE_ATTRIBUTE;

class GameProducer;
struct _gs_ramp;
struct Connection
{
	media_node producer, consumer;
	media_source source;
	media_destination destination;
	media_format format;
	media_node timeSource;
};

// GameSoundBuffer -------------------------------------------------------------
class GameSoundBuffer
{
public:

									GameSoundBuffer(const gs_audio_format* format);
	virtual							~GameSoundBuffer();
			
	virtual status_t				Connect(media_node * consumer);
			status_t				StartPlaying();
			status_t				StopPlaying();
			bool					IsPlaying();
	
			void					Play(void * data, int64 frames);
			void					UpdateMods();
	virtual void					Reset();
	
	virtual void * 					Data() { return NULL; }
			const gs_audio_format &	Format() const;
			
			bool					IsLooping() const;
			void					SetLooping(bool loop);
			float					Gain() const;
			status_t				SetGain(float gain, bigtime_t duration);
			float					Pan() const;
			status_t				SetPan(float pan, bigtime_t duration);	
		
	virtual	status_t				GetAttributes(gs_attribute * attributes,
												  size_t attributeCount);
	virtual status_t				SetAttributes(gs_attribute * attributes,
												  size_t attributeCount);
protected:

	virtual void					FillBuffer(void * data, int64 frames) = 0;
			
			gs_audio_format			fFormat;
			bool					fLooping;
			
			size_t					fFrameSize;
			
private:
			
			Connection *			fConnection;
			GameProducer *			fNode;
			bool					fIsConnected;
			bool					fIsPlaying;
			
			float					fGain;
			float					fPan, fPanLeft, fPanRight;
			_gs_ramp*				fGainRamp;
			_gs_ramp*				fPanRamp;				
};


class SimpleSoundBuffer : public GameSoundBuffer
{
public:
								SimpleSoundBuffer(const gs_audio_format* format,
													const void * data,
													int64 frames = 0);
	virtual						~SimpleSoundBuffer();
	
	virtual void *				Data() { return fBuffer; }
	virtual void				Reset();
	
protected:

	virtual	void				FillBuffer(void * data, int64 frames);
		
private:
			char *				fBuffer;
			size_t				fBufferSize;
			size_t				fPosition;
};

class StreamingSoundBuffer : public GameSoundBuffer
{
public:
								StreamingSoundBuffer(const gs_audio_format * format,
													 const void * streamHook);
	virtual						~StreamingSoundBuffer();
	
protected:
	
	virtual void				FillBuffer(void * data, int64 frames);
	
private:
	
			void *				fStreamHook;
};

#endif

