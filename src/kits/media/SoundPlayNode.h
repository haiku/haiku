#ifndef _SOUND_PLAY_NODE_
#define _SOUND_PLAY_NODE_

#include <BufferProducer.h>
#include <MediaEventLooper.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include "SoundPlayer.h"

/***********************************************************************
 * AUTHOR: Marcus Overhagen, Jérôme Duval
 *   FILE: SoundPlayNode.h
 *  DESCR: This is the BBufferProducer, used internally by BSoundPlayer
 *         This belongs into a private namespace, but isn't for 
 *         compatibility reasons.
 ***********************************************************************/

class _SoundPlayNode 
	: public BBufferProducer, public BMediaEventLooper
{
public:
	_SoundPlayNode(const char *name, BSoundPlayer *player);
	~_SoundPlayNode();
	
	bool IsPlaying();
	bigtime_t CurrentTime();
	
/*************************/
/* begin from BMediaNode */
public:
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;	/* Who instantiated you -- or NULL for app class */

protected:
		/* These don't return errors; instead, they use the global error condition reporter. */
		/* A node is required to have a queue of at least one pending command (plus TimeWarp) */
		/* and is recommended to allow for at least one pending command of each type. */
		/* Allowing an arbitrary number of outstanding commands might be nice, but apps */
		/* cannot depend on that happening. */
virtual	void Preroll(void);

public:
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
				
protected:
virtual		void NodeRegistered(void);	/* reserved 2 */
virtual		status_t RequestCompleted(const media_request_info &info);
virtual		void SetTimeSource(BTimeSource *timeSource);
virtual 	void SetRunMode(run_mode mode);

/* end from BMediaNode */
/***********************/

/******************************/
/* begin from BBufferProducer */

		virtual status_t 	FormatSuggestionRequested(	media_type type,
														int32 quality,
														media_format* format);
		
		virtual status_t 	FormatProposal(				const media_source& output,
														media_format* format);
		
		virtual status_t 	FormatChangeRequested(		const media_source& source,
														const media_destination& destination,
														media_format* io_format,
														int32* _deprecated_);
		virtual status_t 	GetNextOutput(				int32* cookie,
														media_output* out_output);
		virtual status_t 	DisposeOutputCookie(		int32 cookie);
		
		virtual	status_t 	SetBufferGroup(				const media_source& for_source,
														BBufferGroup* group);
		virtual	status_t 	GetLatency(					bigtime_t * out_latency);
		
		virtual status_t 	PrepareToConnect(			const media_source& what,
														const media_destination& where,
														media_format* format,
														media_source* out_source,
														char* out_name);
		
		virtual void 		Connect(					status_t error, 
														const media_source& source,
														const media_destination& destination,
														const media_format& format,
														char* io_name);
		
		virtual void 		Disconnect(					const media_source& what,
														const media_destination& where);
		
		virtual void 		LateNoticeReceived(			const media_source& what,
														bigtime_t how_much,
														bigtime_t performance_time);
		
		virtual void 		EnableOutput(				const media_source & what,
														bool enabled,
														int32* _deprecated_);
		virtual void 		AdditionalBufferRequested(	const media_source& source,
														media_buffer_id prev_buffer,
														bigtime_t prev_time,
														const media_seek_tag* prev_tag);
		virtual void 		LatencyChanged(				const media_source& source,
														const media_destination& destination,
														bigtime_t new_latency,
														uint32 flags);
/* end from BBufferProducer */
/****************************/

/********************************/
/* start from BMediaEventLooper */

	protected:
		/* you must override to handle your events! */
		/* you should not call HandleEvent directly */
		virtual void		HandleEvent(	const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent = false);

/* end from BMediaEventLooper */
/******************************/

public:
	media_multi_audio_format Format() const;

protected:

virtual status_t HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t SendNewBuffer(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleParameter(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
	
	void 				AllocateBuffers();
	BBuffer*			FillNextBuffer(bigtime_t event_time);
				
private:
	BSoundPlayer 		*mPlayer;
	
	status_t 			mInitCheckStatus;
	bool 				mOutputEnabled;
	media_output		mOutput;
	BBufferGroup		*mBufferGroup;
	media_format		mFormat;
	bigtime_t 			mLatency;
	bigtime_t 			mInternalLatency;
	bigtime_t 			mStartTime;
	uint64 				mFramesSent;
	int32				mTooEarlyCount;
};

#endif
