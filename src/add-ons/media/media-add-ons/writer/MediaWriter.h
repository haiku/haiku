// MediaWriter.h
//
// Andrew Bachmann, 2002
//
// A MediaWriter is a node that
// implements FileInterface and BBufferConsumer.
// It consumes on input, which is a multistream,
// and writes the stream to a file.  It has a rather
// unique interpretation of time.  Time is
// distance in the file.  So the duration is the
// file length. (in bytes) 

#if !defined(_MEDIA_WRITER_H)
#define _MEDIA_WRITER_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <FileInterface.h>
#include <BufferConsumer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <File.h>
#include <Entry.h>
#include <BufferGroup.h>

class MediaWriter :
    public BFileInterface,
    public BBufferConsumer,
    public BControllable,
    public BMediaEventLooper
{
protected:
virtual ~MediaWriter(void);

public:

explicit MediaWriter(
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

// provided for BMediaWriterAddOn
public:
static status_t StaticSniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality);

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

		MediaWriter(	/* private unimplemented */
				const MediaWriter & clone);
		MediaWriter & operator=(
				const MediaWriter & clone);				

		status_t initCheckStatus;

		BMediaAddOn * mediaWriterAddOn;
		media_input input;
		
		BFile * outputFile;
		entry_ref output_ref;
		char output_mime_type[B_MIME_TYPE_LENGTH+1];
		
		BBufferGroup * bufferGroup;
		
		bigtime_t downstreamLatency;
		bigtime_t internalLatency;
		
		bool intputEnabled;
		
		// this is computed from the real (negotiated) chunk size and bit rate,
		// not the defaults that are in the parameters
		bigtime_t bufferPeriod;

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaWriter_0(void *);
virtual		status_t _Reserved_MediaWriter_1(void *);
virtual		status_t _Reserved_MediaWriter_2(void *);
virtual		status_t _Reserved_MediaWriter_3(void *);
virtual		status_t _Reserved_MediaWriter_4(void *);
virtual		status_t _Reserved_MediaWriter_5(void *);
virtual		status_t _Reserved_MediaWriter_6(void *);
virtual		status_t _Reserved_MediaWriter_7(void *);
virtual		status_t _Reserved_MediaWriter_8(void *);
virtual		status_t _Reserved_MediaWriter_9(void *);
virtual		status_t _Reserved_MediaWriter_10(void *);
virtual		status_t _Reserved_MediaWriter_11(void *);
virtual		status_t _Reserved_MediaWriter_12(void *);
virtual		status_t _Reserved_MediaWriter_13(void *);
virtual		status_t _Reserved_MediaWriter_14(void *);
virtual		status_t _Reserved_MediaWriter_15(void *);

		uint32 _reserved_media_writer_[16];

};

#endif /* _MEDIA_WRITER_H */
