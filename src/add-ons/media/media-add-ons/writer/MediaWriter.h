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

#include "../AbstractFileInterfaceNode.h"

class MediaWriter :
    public BBufferConsumer,
    public AbstractFileInterfaceNode
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

/*************************/
/* begin from BMediaNode */
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

/* end from BMediaNode */
/***********************/

/*****************************/
/* begin from BFileInterface */
protected:
virtual	status_t SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time);

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

/*****************/
/* BControllable */
/*****************/

/*********************/
/* BMediaEventLooper */
/*********************/

protected:

virtual status_t HandleBuffer(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);

public:

static void GetFlavor(flavor_info * outInfo, int32 id);
static void GetFormat(media_format * outFormat);
static void GetFileFormat(media_file_format * outFileFormat);

protected:

virtual status_t WriteFileBuffer(BBuffer * buffer);

private:

		MediaWriter(	/* private unimplemented */
				const MediaWriter & clone);
		MediaWriter & operator=(
				const MediaWriter & clone);				

		media_input input;
		
		BBufferGroup * fBufferGroup;
		bigtime_t fInternalLatency;
		// this is computed from the real (negotiated) chunk size and bit rate,
		// not the defaults that are in the parameters
		bigtime_t fBufferPeriod;

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
