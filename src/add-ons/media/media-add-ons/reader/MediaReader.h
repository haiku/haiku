// MediaReader.h
//
// Andrew Bachmann, 2002
//
// A MediaReader is a node that
// implements FileInterface and BBufferProducer.
// It reads any file and produces one output,
// which is a multistream.  It has a rather
// unique interpretation of time.  Time is
// distance in the file.  So the duration is the
// file length. (in bytes) 

#if !defined(_MEDIA_READER_H)
#define _MEDIA_READER_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <FileInterface.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <File.h>
#include <Entry.h>
#include <BufferGroup.h>

class MediaReader :
    public BFileInterface,
    public BBufferProducer,
    public BControllable,
    public BMediaEventLooper
{
protected:
virtual ~MediaReader(void);

public:

explicit MediaReader(
				size_t defaultChunkSize = 8192, // chunk size = 8 KB
				float defaultBitRate = 800000,  // bit rate = 100.000 KB/sec = 5.85 MB/minute
				const flavor_info * info = 0,   // buffer period = 80 milliseconds
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

protected:
virtual BParameterWeb * MakeParameterWeb(void);

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
				media_file_format * out_format);
virtual	void DisposeFileFormatCookie(
				int32 cookie);

virtual	status_t GetDuration(
				bigtime_t * out_time);

virtual	status_t SniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality);

virtual	status_t SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time);
virtual	status_t GetRef(
				entry_ref * out_ref,
				char * out_mime_type);

/* end from BFileInterface */
/***************************/

// provided for BMediaReaderAddOn
public:
static status_t StaticSniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality);

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

/****************************/
/* begin from BControllable */

//included from BMediaNode
//virtual	status_t HandleMessage(
//                int32 message,
//				const void * data,
//				size_t size);
public:		
		/* These are alternate methods of accomplishing the same thing as */
		/* connecting to control information source/destinations would. */
virtual	status_t GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize);
virtual	void SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size);
virtual	status_t StartControlPanel(
				BMessenger * out_messenger);

/* end from BControllable */
/**************************/

public:
		// these three are related:
		// DEFAULT_CHUNK_SIZE = (DEFAULT_BIT_RATE * 1024) * (DEFAULT_BUFFER_PERIOD / 8000000)
		static const int32 DEFAULT_CHUNK_SIZE_PARAM;    // in bytes
		static const int32 DEFAULT_BIT_RATE_PARAM;      // in 1000*kilobits/sec
		static const int32 DEFAULT_BUFFER_PERIOD_PARAM; // milliseconds

private:
		size_t defaultChunkSizeParam;				
		bigtime_t defaultChunkSizeParamChangeTime;
		float defaultBitRateParam;				
		bigtime_t defaultBitRateParamChangeTime;
		bigtime_t defaultBufferPeriodParam;				
		bigtime_t defaultBufferPeriodParamChangeTime;

		// This is used to figure out which parameter to compute
		// when enforcing the above constraint relating the three params
		int32 lastUpdatedParameter;
		int32 leastRecentlyUpdatedParameter;

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

virtual status_t HandleBuffer(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);

virtual status_t HandleParameter(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);

public:

static status_t GetFlavor(int32 id, const flavor_info ** out_info);
static media_format & GetFormat();
static media_file_format & GetFileFormat();

protected:

virtual status_t ResetFormat(media_format * format);
virtual status_t ResolveWildcards(media_multistream_format * multistream_format);
virtual status_t GetFilledBuffer(BBuffer ** outBuffer);
virtual status_t FillFileBuffer(BBuffer * buffer);

private:

		MediaReader(	/* private unimplemented */
				const MediaReader & clone);
		MediaReader & operator=(
				const MediaReader & clone);				

		status_t initCheckStatus;

		BMediaAddOn * mediaReaderAddOn;
		media_output output;
		
		BFile * inputFile;
		entry_ref input_ref;
		char input_mime_type[B_MIME_TYPE_LENGTH+1];
		
		BBufferGroup * bufferGroup;
		
		bigtime_t downstreamLatency;
		bigtime_t internalLatency;
		
		bool outputEnabled;
		
		// this is computed from the real (negotiated) chunk size and bit rate,
		// not the defaults that are in the parameters
		bigtime_t bufferPeriod;

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaReader_0(void *);
virtual		status_t _Reserved_MediaReader_1(void *);
virtual		status_t _Reserved_MediaReader_2(void *);
virtual		status_t _Reserved_MediaReader_3(void *);
virtual		status_t _Reserved_MediaReader_4(void *);
virtual		status_t _Reserved_MediaReader_5(void *);
virtual		status_t _Reserved_MediaReader_6(void *);
virtual		status_t _Reserved_MediaReader_7(void *);
virtual		status_t _Reserved_MediaReader_8(void *);
virtual		status_t _Reserved_MediaReader_9(void *);
virtual		status_t _Reserved_MediaReader_10(void *);
virtual		status_t _Reserved_MediaReader_11(void *);
virtual		status_t _Reserved_MediaReader_12(void *);
virtual		status_t _Reserved_MediaReader_13(void *);
virtual		status_t _Reserved_MediaReader_14(void *);
virtual		status_t _Reserved_MediaReader_15(void *);

		uint32 _reserved_media_reader_[16];

};

#endif /* _MEDIA_READER_H */
