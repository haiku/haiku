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
//	File Name:		FileGameSound.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BFileGameSound is a class that streams data out of a file.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <scheduler.h>

// Project Includes ------------------------------------------------------------
#include <GameSoundDevice.h>
#include <GSUtility.h>

// Local Includes --------------------------------------------------------------
#include <FileGameSound.h>

// Local Defines ---------------------------------------------------------------
const int32 kPages = 20;
struct _gs_media_tracker
{
	BMediaFile *	file;
	BMediaTrack * 	stream;
	int64			frames;
	size_t			position;
};

// Locale utility functions -----------------------------------------------
bool FillBuffer(_gs_ramp * ramp, uint8 * data, uint8 * buffer, size_t * bytes)
{
	int32 samples = *bytes / sizeof(int16);
	
	for(int32 byte = 0; byte < samples; byte++)
	{
		float gain = *ramp->value;
		data[byte] = uint8(float(buffer[byte]) * gain);
				
		if (ChangeRamp(ramp))
		{
			*bytes = byte * sizeof(int16);
			return true;
		}
	}
	
	return false;
}

bool FillBuffer(_gs_ramp * ramp, int16 * data, int16 * buffer, size_t * bytes)
{
	int32 samples = *bytes / sizeof(int16);
	
	for(int32 byte = 0; byte < samples; byte++)
	{
		float gain = *ramp->value;
		data[byte] = int16(float(buffer[byte]) * gain);
				
		if (ChangeRamp(ramp))
		{
			*bytes = byte * sizeof(int16);
			return true;
		}
	}
	
	return false;
}

bool FillBuffer(_gs_ramp * ramp, int32 * data, int32 * buffer, size_t * bytes)
{
	size_t byte = 0;
	bool bytesAreReady = (*bytes > 0);

	while(bytesAreReady)
	{
		float gain = *ramp->value;
		data[byte] = int32(float(buffer[byte]) * gain);
		
		if (ChangeRamp(ramp))
		{
			*bytes = byte;
			return true;
		}
		
		byte++;
		bytesAreReady = (byte >= *bytes);
	}
	
	return false;
}

bool FillBuffer(_gs_ramp * ramp, float * data, float * buffer, size_t * bytes)
{
	size_t byte = 0;
	bool bytesAreReady = (*bytes > 0);

	while(bytesAreReady)
	{
		float gain = *ramp->value;
		data[byte] = buffer[byte] * gain;
		
		if (ChangeRamp(ramp))
		{
			*bytes = byte;
			return true;
		}
		
		byte++;
		bytesAreReady = (byte >= *bytes);
	}
	
	return false;
}
	 
// BFileGameSound -------------------------------------------------------
BFileGameSound::BFileGameSound(const entry_ref *file,
							   bool looping,
							   BGameSoundDevice *device)
 	:	BStreamingGameSound(device),
		fAudioStream(NULL),
 		fStopping(false),
 		fLooping(looping),
 		fBuffer(NULL),
 		fPlayPosition(0),
 		fPausing(NULL),
 		fPaused(false),
 		fPauseGain(1.0)
{
	if (InitCheck() == B_OK) SetInitError(Init(file));							
}


BFileGameSound::BFileGameSound(const char *file,
							   bool looping,
							   BGameSoundDevice *device)
 	:	BStreamingGameSound(device),
		fAudioStream(NULL),
 		fStopping(false),
 		fLooping(looping), 
		fBuffer(NULL),
 		fPlayPosition(0),
 		fPausing(NULL),
 		fPaused(false),
 		fPauseGain(1.0)
{
	if (InitCheck() == B_OK)
	{
		entry_ref node;
	
		if (get_ref_for_path(file, &node) != B_OK)
			SetInitError(B_ENTRY_NOT_FOUND);
		else	
			SetInitError(Init(&node));
	}
}


BFileGameSound::~BFileGameSound()
{
	if (fReadThread >= 0) kill_thread(fReadThread);
	
	if (fAudioStream)
	{	
		if (fAudioStream->stream) fAudioStream->file->ReleaseTrack(fAudioStream->stream);
	
		fAudioStream->file->CloseFile();
		
		//delete fAudioStream->stream;
		delete fAudioStream->file;
	}
	
	delete [] fBuffer;	
	delete fAudioStream;
}


BGameSound *
BFileGameSound::Clone() const
{
	return NULL;
}


status_t
BFileGameSound::StartPlaying()
{
	// restart playback if needed
	if (IsPlaying()) StopPlaying();
	
	fPort = create_port(kPages, "audioque");
	
	// create the thread that will read the file
	fReadThread = spawn_thread(ReadThread, "audiostream", B_NORMAL_PRIORITY, this);
	if (fReadThread < B_OK) return B_NO_MORE_THREADS;
		
	status_t error = resume_thread(fReadThread);
	if (error != B_OK) return error;
	
	// start playing the file
	return BStreamingGameSound::StartPlaying();
}


status_t
BFileGameSound::StopPlaying()
{
	status_t error = BStreamingGameSound::StopPlaying();
	
	// start reading next time from the start of the file
	int64 frame = 0;
	fAudioStream->stream->SeekToFrame(&frame);
	
	fStopping = false;
	fAudioStream->position = 0;
	fPlayPosition = 0;
	
	// we don't need to read any more
	kill_thread(fReadThread);
	delete_port(fPort);
		
	return error;
}


status_t
BFileGameSound::Preload()
{
	if (!IsPlaying())
	{
		for(int32 i = 0; i < kPages / 2; i++) Load();
	}
	
	return B_OK;
}


void
BFileGameSound::FillBuffer(void *inBuffer,
						   size_t inByteCount)
{
	int32 cookie;
	size_t bytes = inByteCount;
	
	if (!fPaused || fPausing)
	{
		// time to read a new buffer
		if (fPlayPosition == 0) read_port_etc(fPort, &cookie, fBuffer, fBufferSize, B_TIMEOUT, 0);
	
		if (fPausing)
		{
			Lock();
		
			bool rampDone = false;
			
			// Fill the requsted buffer, stopping if the paused flag is set
			switch(Format().format)
			{
				case gs_audio_format::B_GS_U8:
					rampDone = ::FillBuffer(fPausing, (uint8*)inBuffer, (uint8*)&fBuffer[fPlayPosition], &bytes);
					break;
			
				case gs_audio_format::B_GS_S16:
					rampDone = ::FillBuffer(fPausing, (int16*)inBuffer, (int16*)&fBuffer[fPlayPosition], &bytes);
					break;
			
				case gs_audio_format::B_GS_S32:
					rampDone = ::FillBuffer(fPausing, (int32*)inBuffer, (int32*)&fBuffer[fPlayPosition], &bytes);
					break;
			
				case gs_audio_format::B_GS_F:
					rampDone = ::FillBuffer(fPausing, (float*)inBuffer, (float*)&fBuffer[fPlayPosition], &bytes);
					break;
			}
			
			// We finished ramping
			if (rampDone)
			{
				if (bytes < inByteCount && !fPausing)
				{
					// Since are resumming play back, we need to copy any remaining samples
					char * buffer = (char*)inBuffer;
					memcpy(&buffer[bytes], &fBuffer[fPlayPosition + bytes], inByteCount - bytes);
				}
				
				delete fPausing;
				fPausing = NULL;
			}
							
			Unlock();
		}
		else
		{
			size_t byte = 0;
			char * buffer = (char*)inBuffer;
			
			// We need to be able to stop asap when the pause flag is flipped.
			while(byte < bytes && (!fPaused || fPausing))
			{
				buffer[byte] = fBuffer[fPlayPosition + byte]; 
				byte++;
			}
			
			bytes = byte;
		}
	}
	
	fPlayPosition += bytes;
	if (fPlayPosition >= fBufferSize)
	{
		// We have finished reading the buffer. Setup for the next buffer.
		fPlayPosition = 0;
		memset(fBuffer, 0, fBufferSize);
	}				  		
}


status_t
BFileGameSound::Perform(int32 selector,
						void *data)
{
	return B_ERROR;
}


status_t
BFileGameSound::SetPaused(bool isPaused,
						  bigtime_t rampTime)
{
	if (fPaused != isPaused)
	{
		Lock();
	
		// Clear any old ramping	
		delete fPausing;
		fPausing = NULL;
	
		if (rampTime > 100000) 
		{
			// Setup for ramping
			if (isPaused)
				fPausing = InitRamp(&fPauseGain, 0.0, Format().frame_rate, rampTime);
			else
				fPausing = InitRamp(&fPauseGain, 1.0, Format().frame_rate, rampTime);
		}
		
		fPaused = isPaused;
		Unlock();
	}
	
	return B_OK;
}


int32
BFileGameSound::IsPaused()
{
	if (fPausing) return B_PAUSE_IN_PROGRESS;
	
	if (fPaused) return B_PAUSED;
	
	return B_NOT_PAUSED;
}


status_t
BFileGameSound::Init(const entry_ref* file)
{
	fAudioStream = new _gs_media_tracker;
	memset(fAudioStream, 0, sizeof(_gs_media_tracker));
	
	fAudioStream->file = new BMediaFile(file);
	status_t error = fAudioStream->file->InitCheck();
	if (error != B_OK) return error;
	
	fAudioStream->stream = fAudioStream->file->TrackAt(0);
	
	// is this is an audio file?		
	media_format mformat;
	fAudioStream->stream->EncodedFormat(&mformat);
	if (!mformat.IsAudio()) return B_MEDIA_BAD_FORMAT;
	
	gs_audio_format dformat = Device()->Format();
	
	// request the format we want the sound
	memset(&mformat, 0, sizeof(media_format));
	mformat.type = B_MEDIA_RAW_AUDIO;
	fAudioStream->stream->DecodedFormat(&mformat);
	
	// translate the format into a "GameKit" friendly one
	gs_audio_format gsformat;
	media_to_gs_format(&gsformat, &mformat.u.raw_audio);	
	
	// Since the buffer sized read from the file is most likely differnt
	// then the buffer used by the audio mixer, we must allocate a buffer
	// large enough to hold the largest request.
	fBufferSize = gsformat.buffer_size;
	if (fBufferSize < dformat.buffer_size) fBufferSize = dformat.buffer_size;
	
	// create the buffer
	fBuffer = new char[fBufferSize * 2];
	memset(fBuffer, 0, fBufferSize * 2);
	
	fFrameSize = gsformat.channel_count * get_sample_size(gsformat.format);
	fAudioStream->frames = fAudioStream->stream->CountFrames();
	
	// Ask the device to attach our sound to it
	gs_id sound;
	error = Device()->CreateBuffer(&sound, this, &gsformat);
	if (error != B_OK) return error;
	
	return BGameSound::Init(sound);
}


int32
BFileGameSound::ReadThread(void* arg)
{
	BFileGameSound* obj = (BFileGameSound*)arg;
	
	while(true) obj->Load();			
	
	return 0;
}


bool
BFileGameSound::Load()
{
	int64 frames;
	char * buffer = &fBuffer[fBufferSize + fAudioStream->position];
	status_t err = fAudioStream->stream->ReadFrames(buffer, &frames);
	if (err < B_OK) {
		StopPlaying(); // XXX this is a hack, the whole class design is broken
	}
	
	int32 frame = fAudioStream->stream->CurrentFrame();
	
	fAudioStream->position += fFrameSize * frames;
	if (fAudioStream->position >= fBufferSize) 
	{
		// we have filled the enter buffer, time to send
		write_port(fPort, fBufferSize, &fBuffer[fBufferSize], fBufferSize);	
		fAudioStream->position = 0;
	}
	
	if (frame >= fAudioStream->frames) 
	{
		if (fLooping)
		{
			// since we are looping, we need to start reading from 
			// the begining of the file again. 
			int64 firstFrame = 0;
			fAudioStream->stream->SeekToFrame(&firstFrame);
			
			fStopping = true;
		}
		else fStopping = true;
	}
	
	return true;
}


bool
BFileGameSound::Read(void * buffer, size_t bytes)
{
	return false;		
}

	
/* unimplemented for protection of the user:
 *
 * BFileGameSound::BFileGameSound()
 * BFileGameSound::BFileGameSound(const BFileGameSound &)
 * BFileGameSound &BFileGameSound::operator=(const BFileGameSound &)
 */
 

status_t
BFileGameSound::_Reserved_BFileGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


