// A MediaFileProducer is a node that
// implements FileInterface and BBufferProducer.
// it produces one output, a multistream

#if !defined(_MEDIA_FILE_PRODUCER_H)
#define _MEDIA_FILE_PRODUCER_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <FileInterface.h>
#include <BufferProducer.h>

class MediaFileProducer :
    public BFileInterface,
    public BBufferProducer
{
protected:
MediaFileProducer();
virtual ~MediaFileProducer();

/*************************/
/* begin from BMediaNode */
public:
	/* this port is what a media node listens to for commands */
virtual port_id ControlPort() const;

virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const = 0;	/* Who instantiated you -- or NULL for app class */

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
virtual	void Preroll();
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

virtual		void NodeRegistered();	/* reserved 2 */

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

/*****************************/
/* begin from BFileInterface */
protected:
//included from BMediaNode
//virtual	status_t HandleMessage(
//                int32 message,
//				const void * data,
//				size_t size);

virtual	status_t GetNextFileFormat(
				int32 * cookie,
				media_file_format * out_format) = 0;
virtual	void DisposeFileFormatCookie(
				int32 cookie) = 0;

virtual	status_t GetDuration(
				bigtime_t * out_time) = 0;
virtual	status_t SniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality) = 0;
virtual	status_t SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time) = 0;
virtual	status_t GetRef(
				entry_ref * out_ref,
				char * out_mime_type) = 0;
/* end from BFileInterface */
/***************************/

/******************************/
/* begin from BBufferProducer */
protected:
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

/* end from BBufferProducer */
/****************************/

private:

		MediaFileProducer(	/* private unimplemented */
				const MediaFileProducer & clone);
		MediaFileProducer & operator=(
				const MediaFileProducer & clone);				

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaFileProducer_0(void *);
virtual		status_t _Reserved_MediaFileProducer_1(void *);
virtual		status_t _Reserved_MediaFileProducer_2(void *);
virtual		status_t _Reserved_MediaFileProducer_3(void *);
virtual		status_t _Reserved_MediaFileProducer_4(void *);
virtual		status_t _Reserved_MediaFileProducer_5(void *);
virtual		status_t _Reserved_MediaFileProducer_6(void *);
virtual		status_t _Reserved_MediaFileProducer_7(void *);
virtual		status_t _Reserved_MediaFileProducer_8(void *);
virtual		status_t _Reserved_MediaFileProducer_9(void *);
virtual		status_t _Reserved_MediaFileProducer_10(void *);
virtual		status_t _Reserved_MediaFileProducer_11(void *);
virtual		status_t _Reserved_MediaFileProducer_12(void *);
virtual		status_t _Reserved_MediaFileProducer_13(void *);
virtual		status_t _Reserved_MediaFileProducer_14(void *);
virtual		status_t _Reserved_MediaFileProducer_15(void *);

		uint32 _reserved_media_file_node_[16];

};

#endif /* _MEDIA_FILE_PRODUCER_H */
