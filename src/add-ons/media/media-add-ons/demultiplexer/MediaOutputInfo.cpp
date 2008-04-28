// MediaOutputInfo.cpp
//
// Andrew Bachmann, 2002
//
// A class to encapsulate and manipulate
// all the information for a particular
// output of a media node.

#include <MediaDefs.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <stdio.h>
#include <string.h>
#include "MediaOutputInfo.h"
#include "misc.h"

MediaOutputInfo::MediaOutputInfo(BBufferProducer * node, char * name) {
	producer = node;
	// null some fields
	bufferGroup = 0;
	bufferPeriod = 0;
	// start enabled
	outputEnabled = true;
	// don't overwrite available space, and be sure to terminate
	strncpy(output.name,name,B_MEDIA_NAME_LENGTH-1);
	output.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the output
	output.node = media_node::null;
	output.source = media_source::null;
	output.destination = media_destination::null;
}

MediaOutputInfo::~MediaOutputInfo() {
	if (bufferGroup != 0) {
		BBufferGroup * group = bufferGroup;
		bufferGroup = 0;
		delete group;
	}
}

status_t MediaOutputInfo::SetBufferGroup(BBufferGroup * group) {
	if (bufferGroup != 0) {
		if (bufferGroup == group) {
			return B_OK; // time saver
		}	
		delete bufferGroup;
	}
	bufferGroup = group;
}

// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
status_t MediaOutputInfo::FormatProposal(media_format * format)
{
	// Be's format_is_compatible doesn't work,
	// so use our format_is_acceptible instead
	if (!format_is_acceptible(*format,generalFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	// XXX: test because we don't trust them!
	format->SpecializeTo(&wildcardedFormat);
	return B_OK;
}

// Presumably we have already agreed with them that this format is
// okay.  But just in case, we check the offer. (and complain if it
// is invalid)  Then as the last thing we do, we get rid of any
// remaining wilcards.
status_t MediaOutputInfo::FormatChangeRequested(const media_destination & destination,
							   media_format * io_format)
{
	status_t status = FormatProposal(io_format);
	if (status != B_OK) {
		fprintf(stderr,"<- MediaOutputInfo::FormatProposal failed\n");
		*io_format = generalFormat;
		return status;
	}
	io_format->SpecializeTo(&fullySpecifiedFormat);
	return B_OK;
}								   

status_t MediaOutputInfo::PrepareToConnect(const media_destination & where,
						  media_format * format,
						  media_source * out_source,
						  char * out_name)
{
	if (output.destination != media_destination::null) {
		fprintf(stderr,"<- B_MEDIA_ALREADY_CONNECTED\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}
	status_t status = FormatChangeRequested(where,format);
	if (status != B_OK) {
		fprintf(stderr,"<- MediaOutputInfo::FormatChangeRequested failed\n");
		return status;
	}
	*out_source = output.source;
	output.destination = where;
	strncpy(out_name,output.name,B_MEDIA_NAME_LENGTH-1);
	out_name[B_MEDIA_NAME_LENGTH] = '\0';
	return B_OK;
}					  

status_t MediaOutputInfo::Connect(const media_destination & destination,
				 const media_format & format,
				 char * io_name,
				 bigtime_t _downstreamLatency)
{
	output.destination = destination;
	output.format = format;
	strncpy(io_name,output.name,B_MEDIA_NAME_LENGTH-1);
	io_name[B_MEDIA_NAME_LENGTH-1] = '\0';
	downstreamLatency = _downstreamLatency; // must be set before create buffer group
	
	status_t status = CreateBufferGroup(); // also initializes buffer period
	if (status != B_OK) {
		output.destination = media_destination::null;
		output.format = generalFormat;
		return status;
	}
	return B_OK;	
}

status_t MediaOutputInfo::Disconnect()
{
	output.destination = media_destination::null;
	output.format = generalFormat;
	if (bufferGroup != 0) {
		BBufferGroup * group = bufferGroup;
		bufferGroup = 0;
		delete group;
	}
}

status_t MediaOutputInfo::EnableOutput(bool enabled)
{
	outputEnabled = enabled;
	return B_OK;
}		

status_t MediaOutputInfo::AdditionalBufferRequested(
					media_buffer_id prev_buffer,
					bigtime_t prev_time,
					const media_seek_tag * prev_tag)
{
	// XXX: implement me
	return B_OK;
}

// protected:

status_t MediaOutputInfo::CreateBufferGroup() {
	bufferPeriod = ComputeBufferPeriod();
	
	if (bufferGroup == 0) {
		int32 count = int32(downstreamLatency/bufferPeriod)+2;
		fprintf(stderr,"  downstream latency = %lld, buffer period = %lld, buffer count = %i\n",
				downstreamLatency,bufferPeriod,count);
	
		// allocate the buffers
		bufferGroup = new BBufferGroup(ComputeBufferSize(),count);
		if (bufferGroup == 0) {
			fprintf(stderr,"<- B_NO_MEMORY\n");
			return B_NO_MEMORY;
		}
		status_t status = bufferGroup->InitCheck();
		if (status != B_OK) {
			fprintf(stderr,"<- BufferGroup initialization failed\n");
			BBufferGroup * group = bufferGroup;
			bufferGroup = 0;
			delete group;
			return status;
		}
	}
	return B_OK;
}

// public:

uint32 MediaOutputInfo::ComputeBufferSize() {
	return ComputeBufferSize(output.format);
}

// returns result in # of bytes
uint32 MediaOutputInfo::ComputeBufferSize(const media_format & format) {
	uint64 bufferSize = 1024; // default 1024 bytes
	switch (format.type) {
	case B_MEDIA_MULTISTREAM:
		bufferSize = format.u.multistream.max_chunk_size;
		break;
	case B_MEDIA_ENCODED_VIDEO:
		bufferSize = format.u.encoded_video.frame_size; 
		break;
	case B_MEDIA_RAW_VIDEO:
		if (format.u.raw_video.interlace == 0) {
			// okay, you have no fields, you need no space, right?
			bufferSize = 0;
		} else {
			// this is the size of a *field*, not a frame
			bufferSize = format.u.raw_video.display.bytes_per_row *
						 format.u.raw_video.display.line_count /
						 format.u.raw_video.interlace;
		}
		break;
	case B_MEDIA_ENCODED_AUDIO:
		bufferSize = format.u.encoded_audio.frame_size;
		break;
	case B_MEDIA_RAW_AUDIO:
		bufferSize = format.u.raw_audio.buffer_size;
		break;
	default:
		break;
	}
	if (bufferSize > INT_MAX) {
		bufferSize = INT_MAX;
	}
	return int32(bufferSize);
}

bigtime_t MediaOutputInfo::ComputeBufferPeriod() {
	return ComputeBufferPeriod(output.format);
}

// returns result in # of microseconds
bigtime_t MediaOutputInfo::ComputeBufferPeriod(const media_format & format) {
	bigtime_t bufferPeriod = 25*1000; // default 25 milliseconds
	switch (format.type) {
	case B_MEDIA_MULTISTREAM:
		// given a buffer size of 8192 bytes
		// and a bitrate of 1024 kilobits/millisecond (= 128 bytes/millisecond)
		// we need to produce a buffer every 64 milliseconds (= every 64000 microseconds)
		bufferPeriod = bigtime_t(1000.0 * 8.0 * ComputeBufferSize(format)
								 / format.u.multistream.max_bit_rate);
		break;
	case B_MEDIA_ENCODED_VIDEO:
		bufferPeriod = bigtime_t(1000.0 * 8.0 * ComputeBufferSize(format)
								 / format.u.encoded_video.max_bit_rate);
		break;
	case B_MEDIA_ENCODED_AUDIO:
		bufferPeriod = bigtime_t(1000.0 * 8.0 * ComputeBufferSize(format)
								 / format.u.encoded_audio.bit_rate);
		break;
	case B_MEDIA_RAW_VIDEO:
		// Given a field rate of 50.00 fields per second, (PAL)
		// we need to generate a field/buffer
		// every 1/50 of a second = 20000 microseconds.
		bufferPeriod = bigtime_t(1000000.0
								 / format.u.raw_video.field_rate);
		break;
	case B_MEDIA_RAW_AUDIO:
		// Given a sample size of 4 bytes [B_AUDIO_INT]
		// and a channel count of 2 and a buffer_size
		// of 256 bytes and a frame_rate of 44100 Hertz (1/sec)
		// 1 frame = 1 sample/channel.
		// comes to ??
		// this is a guess:
		bufferPeriod = bigtime_t(1000000.0 * ComputeBufferSize(format)
								 / (format.u.raw_audio.format
								    & media_raw_audio_format::B_AUDIO_SIZE_MASK)
								 / format.u.raw_audio.channel_count
							     / format.u.raw_audio.frame_rate);
		break;
	default:
		break;
	}
	return bufferPeriod;
}
	

