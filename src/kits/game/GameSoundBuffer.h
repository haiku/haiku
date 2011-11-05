/*
 * Copyright 2001-2002, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Author:	Christopher ML Zumwalt May (zummy@users.sf.net)
 */

#ifndef _GAMESOUNDBUFFER_H
#define _GAMESOUNDBUFFER_H


#include <GameSoundDefs.h>
#include <MediaDefs.h>
#include <MediaNode.h>

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


class GameSoundBuffer {
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
			
			Connection *			fConnection;
			GameProducer *			fNode;

private:
			
			bool					fIsConnected;
			bool					fIsPlaying;
			
			float					fGain;
			float					fPan, fPanLeft, fPanRight;
			_gs_ramp*				fGainRamp;
			_gs_ramp*				fPanRamp;				
};


class SimpleSoundBuffer : public GameSoundBuffer {
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


class StreamingSoundBuffer : public GameSoundBuffer {
public:
								StreamingSoundBuffer(const gs_audio_format * format,
													 const void * streamHook,
													 size_t inBufferFrameCount,
													 size_t inBufferCount);
	virtual						~StreamingSoundBuffer();
	
protected:
	
	virtual void				FillBuffer(void * data, int64 frames);
	
private:
	
			void *				fStreamHook;
};

#endif

