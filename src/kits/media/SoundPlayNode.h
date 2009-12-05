/*
 * Copyright 2002-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 *		Jérôme Duval
 */
#ifndef _SOUND_PLAY_NODE_H
#define _SOUND_PLAY_NODE_H


#include <Buffer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <MediaEventLooper.h>
#include <SoundPlayer.h>


namespace BPrivate {


class SoundPlayNode : public BBufferProducer, public BMediaEventLooper {
public:
								SoundPlayNode(const char* name,
									BSoundPlayer* player);
	virtual						~SoundPlayNode();

			bool				IsPlaying();
			bigtime_t			CurrentTime();

	// BMediaNode methods

	virtual	BMediaAddOn*		AddOn(int32* _internalID) const;

protected:
	virtual	void				Preroll();

public:
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

protected:
	virtual	void				NodeRegistered();
	virtual	status_t			RequestCompleted(
									const media_request_info& info);
	virtual	void				SetTimeSource(BTimeSource* timeSource);
	virtual void				SetRunMode(run_mode mode);

	// BBufferProducer methods

	virtual status_t 			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format);

	virtual status_t		 	FormatProposal(const media_source& output,
									media_format* format);

	virtual status_t 			FormatChangeRequested(
									const media_source& source,
									const media_destination& destination,
									media_format* format, int32* _deprecated_);
	virtual status_t 			GetNextOutput(int32* cookie,
									media_output* _output);
	virtual status_t		 	DisposeOutputCookie(int32 cookie);

	virtual	status_t 			SetBufferGroup(const media_source& forSource,
									BBufferGroup* group);
	virtual	status_t 			GetLatency(bigtime_t* _latency);

	virtual status_t 			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format, media_source* _source,
									char* _name);

	virtual void 				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* name);

	virtual void 				Disconnect(const media_source& what,
									const media_destination& where);

	virtual void 				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceTime);

	virtual void 				EnableOutput(const media_source& what,
									bool enabled, int32* _deprecated_);
	virtual void 				AdditionalBufferRequested(
									const media_source& source,
									media_buffer_id previousBuffer,
									bigtime_t previousTime,
									const media_seek_tag* previousTag);
	virtual void 				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t newLatency, uint32 flags);

	// BMediaEventLooper methods

protected:
	virtual void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
public:
			media_multi_audio_format Format() const;

protected:
	virtual status_t			HandleStart(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			HandleSeek(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			HandleWarp(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			HandleStop(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			SendNewBuffer(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			HandleDataStatus(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual status_t			HandleParameter(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

			status_t 			AllocateBuffers();
			BBuffer*			FillNextBuffer(bigtime_t eventTime);

private:
			BSoundPlayer*		fPlayer;

			status_t 			fInitStatus;
			bool 				fOutputEnabled;
			media_output		fOutput;
			BBufferGroup*		fBufferGroup;
			bigtime_t 			fLatency;
			bigtime_t 			fInternalLatency;
			bigtime_t 			fStartTime;
			uint64 				fFramesSent;
			int32				fTooEarlyCount;
};


}	// namespace BPrivate


#endif	// _SOUND_PLAY_NODE_H
