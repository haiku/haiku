// MediaDemultiplexerNode.h
//
// Andrew Bachmann, 2002
//
// The MediaDemultiplexerNode class
// takes a multistream input and supplies
// the individual constituent streams as
// the output.

#if !defined(_MEDIA_DEMULTIPLEXER_NODE_H)
#define _MEDIA_DEMULTIPLEXER_NODE_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <MediaEventLooper.h>
#include <BufferGroup.h>
#include <vector>
#include "MediaOutputInfo.h"

class MediaDemultiplexerNode :
	public BBufferConsumer,
	public BBufferProducer,
    public BMediaEventLooper
{
protected:
virtual ~MediaDemultiplexerNode(void);

public:

explicit MediaDemultiplexerNode(
				const flavor_info * info = 0,
				BMessage * config = 0,
				BMediaAddOn * addOn = 0);

virtual status_t InitCheck(void) const;

// see BMediaAddOn::GetConfigurationFor
virtual	status_t GetConfigurationFor(
				BMessage * into_message);

/*************************/
/* begin from BMediaNode */
public:
//	/* this port is what a media node listens to for commands */
// virtual port_id ControlPort(void) const;

virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;	/* Who instantiated you -- or NULL for app class */

protected:
		/* These don't return errors; instead, they use the global error condition reporter. */
		/* A node is required to have a queue of at least one pending command (plus TimeWarp) */
		/* and is recommended to allow for at least one pending command of each type. */
		/* Allowing an arbitrary number of outstanding commands might be nice, but apps */
		/* cannot depend on that happening. */
virtual	void Start(
				bigtime_t performance_time);
virtual	void Stop(
				bigtime_t performance_time,
				bool immediate);
virtual	void Seek(
				bigtime_t media_time,
				bigtime_t performance_time);
virtual	void SetRunMode(
				run_mode mode);
virtual	void TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time);
virtual	void Preroll(void);
virtual	void SetTimeSource(
				BTimeSource * time_source);

public:
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
				
protected:
		/* Called when requests have completed, or failed. */
virtual	status_t RequestCompleted(	/* reserved 0 */
				const media_request_info & info);

protected:
virtual		status_t DeleteHook(BMediaNode * node);		/* reserved 1 */

virtual		void NodeRegistered(void);	/* reserved 2 */

public:

		/* fill out your attributes in the provided array, returning however many you have. */
virtual		status_t GetNodeAttributes(	/* reserved 3 */
					media_node_attribute * outAttributes,
					size_t inMaxCount);

virtual		status_t AddTimer(
					bigtime_t at_performance_time,
					int32 cookie);

/* end from BMediaNode */
/***********************/

/******************************/
/* begin from BBufferConsumer */

//included from BMediaAddOn
//virtual	status_t HandleMessage(
//				int32 message,
//				const void * data,
//				size_t size);

	/* Someone, probably the producer, is asking you about this format. Give */
	/* your honest opinion, possibly modifying *format. Do not ask upstream */
	/* producer about the format, since he's synchronously waiting for your */
	/* reply. */
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput(
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	void ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format);

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
virtual	status_t SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags);

/* end from BBufferConsumer */
/****************************/

/******************************/
/* begin from BBufferProducer */
protected:
	/* functionality of BBufferProducer */
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format);
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format);
	/* If the format isn't good, put a good format into *io_format and return error */
	/* If format has wildcard, specialize to what you can do (and change). */
	/* If you can change the format, return OK. */
	/* The request comes from your destination sychronously, so you cannot ask it */
	/* whether it likes it -- you should assume it will since it asked. */
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_);
virtual	status_t GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output);
virtual	status_t DisposeOutputCookie(
				int32 cookie);
	/* In this function, you should either pass on the group to your upstream guy, */
	/* or delete your current group and hang on to this group. Deleting the previous */
	/* group (unless you passed it on with the reclaim flag set to false) is very */
	/* important, else you will 1) leak memory and 2) block someone who may want */
	/* to reclaim the buffers living in that group. */
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
	/* Format of clipping is (as int16-s): <from line> <npairs> <startclip> <endclip>. */
	/* Repeat for each line where the clipping is different from the previous line. */
	/* If <npairs> is negative, use the data from line -<npairs> (there are 0 pairs after */
	/* a negative <npairs>. Yes, we only support 32k*32k frame buffers for clipping. */
	/* Any non-0 field of 'display' means that that field changed, and if you don't support */
	/* that change, you should return an error and ignore the request. Note that the buffer */
	/* offset values do not have wildcards; 0 (or -1, or whatever) are real values and must */
	/* be adhered to. */
virtual	status_t VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_);
	/* Iterates over all outputs and maxes the latency found */
virtual	status_t GetLatency(
				bigtime_t * out_latency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name);
virtual	void Disconnect(
				const media_source & what,
				const media_destination & where);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_);
virtual	status_t SetPlayRate(
				int32 numer,
				int32 denom);

//included from BMediaNode
//virtual	status_t HandleMessage(	/* call this from the thread that listens to the port */
//				int32 message,
//				const void * data,
//				size_t size);

virtual	void AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag);	//	may be NULL

virtual	void LatencyChanged(					//	used to be Reserved 1
				const media_source & source,
				const media_destination & destination,
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

		/* override to clean up custom events you have added to your queue */
		virtual void		CleanUpEvent(const media_timed_event *event);
		
		/* called from Offline mode to determine the current time of the node */
		/* update your internal information whenever it changes */
		virtual	bigtime_t	OfflineTime();

		/* override only if you know what you are doing! */
		/* otherwise much badness could occur */
		/* the actual control loop function: */
		/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
		virtual void		ControlLoop();

/* end from BMediaEventLooper */
/******************************/

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
virtual status_t HandleBuffer(
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

protected:

void CreateBufferGroup(MediaOutputInfo * output_info);
void ComputeInternalLatency();

public:

static void GetFlavor(flavor_info * outInfo, int32 id);

private:

static void GetInputFormat(media_format * outFormat);
static void GetOutputFormat(media_format * outFormat);

protected:

virtual status_t AddRequirements(media_format * format);

private:

		MediaDemultiplexerNode(	/* private unimplemented */
				const MediaDemultiplexerNode & clone);
		MediaDemultiplexerNode & operator=(
				const MediaDemultiplexerNode & clone);				

		status_t fInitCheckStatus;

		BMediaAddOn * fAddOn;

		media_input input;
		vector<MediaOutputInfo> outputs;

		bigtime_t fDownstreamLatency;
		bigtime_t fInternalLatency;

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaDemultiplexerNode_0(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_1(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_2(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_3(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_4(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_5(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_6(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_7(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_8(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_9(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_10(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_11(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_12(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_13(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_14(void *);
virtual		status_t _Reserved_MediaDemultiplexerNode_15(void *);

		uint32 _reserved_media_demultiplexer_node_[16];

};

#endif /* _MEDIA_DEMULTIPLEXER_NODE_H */
