/*******************************************************************************
/
/	File:			BBufferProducer.h
/
/   Description:  A BBufferProducer is any source of media buffers in the Media Kit
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_BUFFER_PRODUCER_H)
#define _BUFFER_PRODUCER_H

#include <MediaDefs.h>
#include <MediaNode.h>


class BBufferProducer :
	public virtual BMediaNode
{
protected:
		/* this has to be at the top to force a vtable */
virtual	~BBufferProducer();

public:

	/* Supported formats for low-level clipping data */
	enum {
		B_CLIP_SHORT_RUNS = 1
	};

	/* Handy conversion functions for dealing with clip information */
static	status_t ClipDataToRegion(
				int32 format,
				int32 size,
				const void * data, 
				BRegion * region);

		media_type ProducerType();

protected:
explicit	BBufferProducer(
				media_type producer_type /* = B_MEDIA_UNKNOWN_TYPE */);

		enum suggestion_quality {
			B_ANY_QUALITY = 0,
			B_LOW_QUALITY = 10,
			B_MEDIUM_QUALITY = 50,
			B_HIGH_QUALITY = 100
		};

	/* functionality of BBufferProducer */
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format) = 0;
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format) = 0;
	/* If the format isn't good, put a good format into *io_format and return error */
	/* If format has wildcard, specialize to what you can do (and change). */
	/* If you can change the format, return OK. */
	/* The request comes from your destination sychronously, so you cannot ask it */
	/* whether it likes it -- you should assume it will since it asked. */
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_) = 0;
virtual	status_t GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output) = 0;
virtual	status_t DisposeOutputCookie(
				int32 cookie) = 0;
	/* In this function, you should either pass on the group to your upstream guy, */
	/* or delete your current group and hang on to this group. Deleting the previous */
	/* group (unless you passed it on with the reclaim flag set to false) is very */
	/* important, else you will 1) leak memory and 2) block someone who may want */
	/* to reclaim the buffers living in that group. */
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group) = 0;
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
				bigtime_t * out_lantency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name) = 0;
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name) = 0;
virtual	void Disconnect(
				const media_source & what,
				const media_destination & where) = 0;
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time) = 0;
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_) = 0;
virtual	status_t SetPlayRate(
				int32 numer,
				int32 denom);

virtual	status_t HandleMessage(	/* call this from the thread that listens to the port */
				int32 message,
				const void * data,
				size_t size);

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

	/* Use this function in BBufferProducer to pass on the buffer. */
		status_t SendBuffer(
				BBuffer * buffer,
				const media_destination & destination);

		status_t SendDataStatus(
				int32 status,
				const media_destination & destination,
				bigtime_t at_time);

	/* Check in advance if a target is prepared to accept a format. You may */
	/* want to call this from Connect(), although that's not required. */
		status_t ProposeFormatChange(
				media_format * format,
				const media_destination & for_destination);
	/* Tell consumer to accept a proposed format change */
	/* YOU MUST NOT CALL BROADCAST_BUFFER WHILE THIS CALL IS OUTSTANDING! */
		status_t ChangeFormat(
				const media_source & for_source,
				const media_destination & for_destination,
				media_format * format);
	/* Check how much latency the down-stream graph introduces */
		status_t FindLatencyFor(
				const media_destination & for_destination,
				bigtime_t * out_latency,
				media_node_id * out_timesource);

	/* Find the tag of a previously seen buffer to expedite seeking */
		status_t FindSeekTag(
				const media_destination & for_destination,
				bigtime_t in_target_time,
				media_seek_tag * out_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags = 0,
				uint32 in_flags = 0);

	/* Set the initial latency, which is the maximum additional latency */
	/* that will be imposed while starting/syncing to a signal (such as */
	/* starting a TV capture card in the middle of a field). Most nodes */
	/* have this at 0 (the default); only TV input Nodes need it currently */
	/* because they slave to a low-resolution (59.94 Hz) clock that arrives */
	/* from the outside world. Call this from the constructor if you need it. */
		void SetInitialLatency(
				bigtime_t inInitialLatency,
				uint32 flags = 0);

private:

	friend class BMediaRoster;
	friend class BBufferConsumer;
	friend class BMediaNode;

		BBufferProducer();	/* private unimplemented */
		BBufferProducer(
				const BBufferProducer & clone);
		BBufferProducer & operator=(
				const BBufferProducer & clone);
		
		/* Mmmh, stuffing! */
			status_t _Reserved_BufferProducer_0(void *);	/* AdditionalBufferRequested() */
			status_t _Reserved_BufferProducer_1(void *);	/* LatencyChanged() */
virtual		status_t _Reserved_BufferProducer_2(void *);
virtual		status_t _Reserved_BufferProducer_3(void *);
virtual		status_t _Reserved_BufferProducer_4(void *);
virtual		status_t _Reserved_BufferProducer_5(void *);
virtual		status_t _Reserved_BufferProducer_6(void *);
virtual		status_t _Reserved_BufferProducer_7(void *);
virtual		status_t _Reserved_BufferProducer_8(void *);
virtual		status_t _Reserved_BufferProducer_9(void *);
virtual		status_t _Reserved_BufferProducer_10(void *);
virtual		status_t _Reserved_BufferProducer_11(void *);
virtual		status_t _Reserved_BufferProducer_12(void *);
virtual		status_t _Reserved_BufferProducer_13(void *);
virtual		status_t _Reserved_BufferProducer_14(void *);
virtual		status_t _Reserved_BufferProducer_15(void *);


		media_type	fProducerType;
		bigtime_t	fInitialLatency;
		uint32		fInitialFlags;

static	status_t clip_shorts_to_region(
				const int16 * data,
				int count,
				BRegion * output);
static	status_t clip_region_to_shorts(
				const BRegion * input,
				int16 * data,
				int max_count,
				int * out_count);

		uint32 _reserved_buffer_producer_[14];
};

#endif /* _BUFFER_PRODUCER_H */

