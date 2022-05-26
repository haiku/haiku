//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		GameProducer.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	A MediaKit producer node which mixes sound from the GameKit
//					and sends them to the audio mixer
//------------------------------------------------------------------------------
#ifndef _GAME_PRODUCER_H
#define _GAME_PRODUCER_H


#include <media/BufferProducer.h>
#include <media/MediaEventLooper.h>
#include <GameSoundDefs.h>


class GameSoundBuffer;


class GameProducer : public BBufferProducer, public BMediaEventLooper {
public:
								GameProducer(GameSoundBuffer* object,
									const gs_audio_format * format);
								~GameProducer();

	// BMediaNode methods
			BMediaAddOn*		AddOn(int32* internal_id) const;

	// BBufferProducer methods
			status_t			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format);

			status_t			FormatProposal(const media_source& output,
									media_format* format);

			status_t	 		FormatChangeRequested(const media_source& source,
									const media_destination& destination,
									media_format* io_format,
									int32* _deprecated_);

			status_t			GetNextOutput(int32* cookie,
									media_output* _output);

			status_t			DisposeOutputCookie(int32 cookie);

			status_t			SetBufferGroup(const media_source& forSource,
									BBufferGroup* group);


			status_t			GetLatency(bigtime_t* _latency);

			status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format,
									media_source* _source,
									char* out_name);

			void				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* ioName);

			void				Disconnect(const media_source& what,
									const media_destination& where);

			void				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceDuration);

			void				EnableOutput(const media_source & what,
									bool enabled, int32* _deprecated_);

			status_t			SetPlayRate(int32 numerator, int32 denominator);

			status_t			HandleMessage(int32 message, const void* data,
									size_t size);

			void				AdditionalBufferRequested(const media_source& source,
									media_buffer_id prev_buffer,
									bigtime_t prev_time,
									const media_seek_tag* prev_tag);

			void 				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t new_latency,
									uint32 flags);

	// BMediaEventLooper methods
			void 				NodeRegistered();
			void 				SetRunMode(run_mode mode);
			void 				HandleEvent(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

	// GameProducer
			status_t			StartPlaying(GameSoundBuffer* sound);
			status_t			StopPlaying(GameSoundBuffer* sound);
			bool				IsPlaying(GameSoundBuffer* sound) const;

			int32				SoundCount() const;

private:
			BBuffer* 			FillNextBuffer(bigtime_t event_time);

			BBufferGroup*	 	fBufferGroup;
			bigtime_t 			fLatency;
			bigtime_t			fInternalLatency;
			media_output	 	fOutput;
			bool 				fOutputEnabled;
			media_format 		fPreferredFormat;

			bigtime_t			fStartTime;
			size_t				fFrameSize;
			int64				fFramesSent;
			GameSoundBuffer*	fObject;
			size_t				fBufferSize;
};


#endif	// _GAME_PRODUCER_H
