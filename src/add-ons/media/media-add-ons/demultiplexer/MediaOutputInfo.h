// MediaOutputInfo.h
//
// Andrew Bachmann, 2002
//
// A class to encapsulate and manipulate
// all the information for a particular
// output of a media node.

#if !defined(_MEDIA_OUTPUT_INFO_H)
#define _MEDIA_OUTPUT_INFO_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <BufferProducer.h>
#include <BufferGroup.h>

class MediaOutputInfo
{
public:	
	MediaOutputInfo(BBufferProducer * _node, char * name);
	~MediaOutputInfo();

virtual status_t SetBufferGroup(BBufferGroup * group);

virtual status_t FormatProposal(media_format * format);

virtual status_t FormatChangeRequested(
					const media_destination & destination,
					media_format * io_format);								   

virtual status_t PrepareToConnect(
					const media_destination & where,
					media_format * format,
					media_source * out_source,
					char * out_name);
	
virtual status_t Connect(
					const media_destination & destination,
					const media_format & format,
					char * io_name,
				 	bigtime_t _downstreamLatency);
					 
virtual status_t Disconnect();
	
virtual status_t EnableOutput(bool enabled);

virtual status_t AdditionalBufferRequested(
					media_buffer_id prev_buffer,
					bigtime_t prev_time,
					const media_seek_tag * prev_tag);
	
protected:

virtual status_t CreateBufferGroup();

public:
	
virtual uint32 ComputeBufferSize();
virtual bigtime_t ComputeBufferPeriod();
static uint32 ComputeBufferSize(const media_format & format);
static bigtime_t ComputeBufferPeriod(const media_format & format);

public:
	BBufferProducer * producer;

	media_output output;
	
	bool outputEnabled;
	
	BBufferGroup * bufferGroup;
	size_t bufferSize;
	
	bigtime_t downstreamLatency;
	
	bigtime_t bufferPeriod;

	// This format is the least restrictive we can
	// support in the general case.  (no restrictions
	// based on content)
	media_format generalFormat;

	// This format is the next least restrictive.  It
	// takes into account the content that we are using.
	// It should be the same as above with a few wildcards
	// removed.  Wildcards for things we are flexible on
	// may still be present.
	media_format wildcardedFormat;
	
	// This format provides default values for all fields.
	// These defaults are used to resolve all wildcards.
	media_format fullySpecifiedFormat;
	
	// do we need media_seek_tag in here?
};

#endif // _MEDIA_OUTPUT_INFO_H

