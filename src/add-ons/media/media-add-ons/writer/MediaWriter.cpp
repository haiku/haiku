// MediaWriter.cpp
//
// Andrew Bachmann, 2002
//
// A MediaWriter is a node that
// implements FileInterface and BBufferConsumer.
// It consumes on input, which is a multistream,
// and writes the stream to a file.
//
// see also MediaWriterAddOn.cpp

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <BufferConsumer.h>
#include <FileInterface.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <File.h>
#include <Errors.h>
#include <Entry.h>
#include <BufferGroup.h>
#include <TimeSource.h>
#include <Buffer.h>
#include <ParameterWeb.h>
#include <limits.h>

#include "../AbstractFileInterfaceNode.h"
#include "MediaWriter.h"

#include <stdio.h>
#include <string.h>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaWriter::~MediaWriter(void)
{
	fprintf(stderr,"MediaWriter::~MediaWriter\n");
	if (fBufferGroup != 0) {
		BBufferGroup * group = fBufferGroup;
		fBufferGroup = 0;
		delete group;
	}	
}

MediaWriter::MediaWriter(
				size_t defaultChunkSize = 8192,
				float defaultBitRate = 800000,
				const flavor_info * info = 0,
				BMessage * config = 0,
				BMediaAddOn * addOn = 0)
	: BMediaNode("MediaWriter"),
	  AbstractFileInterfaceNode(defaultChunkSize,defaultBitRate,info,config,addOn),
	  BBufferConsumer(B_MEDIA_MULTISTREAM)
{
	fprintf(stderr,"MediaWriter::MediaWriter\n");
	// null some fields
	fBufferGroup = 0;
	// start enabled
	fInputEnabled = true;
	// don't overwrite available space, and be sure to terminate
	strncpy(input.name,"MediaWriter Input",B_MEDIA_NAME_LENGTH-1);
	input.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the input
	input.node = media_node::null;               // until registration
	input.source = media_source::null; 
	input.destination = media_destination::null; // until registration
	GetFormat(&input.format);
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

void MediaWriter::Preroll(void)
{
	fprintf(stderr,"MediaWriter::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t MediaWriter::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	fprintf(stderr,"MediaWriter::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// no special messages for now
		default:
			status = BBufferConsumer::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			status = AbstractFileInterfaceNode::HandleMessage(message,data,size);
			break;
	}
	return status;
}

void MediaWriter::NodeRegistered(void)
{
	fprintf(stderr,"MediaWriter::NodeRegistered\n");
	
	// now we can do this
	input.node = Node();
	input.destination.id = 0;
	input.destination.port = input.node.port;
	
	// creates the parameter web and starts the looper thread
	AbstractFileInterfaceNode::NodeRegistered();
}

// -------------------------------------------------------- //
// implementation of BFileInterface
// -------------------------------------------------------- //

status_t MediaWriter::SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaWriter::SetRef\n");
	status_t status = AbstractFileInterfaceNode::SetRef(file,B_WRITE_ONLY,create,out_time);
	if (status != B_OK) {
		fprintf(stderr,"AbstractFileInterfaceNode::SetRef returned an error\n");
		return status;
	}
	// reset the format, and set the requirements imposed by this file
	GetFormat(&input.format);
	AddRequirements(&input.format);
	// if we are connected we have to re-negotiate the connection
	if (input.source != media_source::null) {
		fprintf(stderr,"  error connection re-negotiation not implemented");
		// XXX: implement re-negotiation
	}
	return B_OK;	
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

/*
void print_multistream_format(media_multistream_format * format) {
	fprintf(stderr,"[");
	switch (format->format) {
	case media_multistream_format::B_ANY:			fprintf(stderr,"ANY"); break;
	case media_multistream_format::B_VID:			fprintf(stderr,"VID"); break;
	case media_multistream_format::B_AVI:			fprintf(stderr,"AVI"); break;
	case media_multistream_format::B_MPEG1:		fprintf(stderr,"MPEG1"); break;
	case media_multistream_format::B_MPEG2:		fprintf(stderr,"MPEG2"); break;
	case media_multistream_format::B_QUICKTIME:	fprintf(stderr,"QUICKTIME"); break;
	default:			fprintf(stderr,"????"); break;
	}
	fprintf(stderr," avg_bit_rate(%f) max_bit_rate(%f)",
			format->avg_bit_rate,format->max_bit_rate);
	fprintf(stderr," avg_chunk_size(%i) max_chunk_size(%i)",
			format->avg_chunk_size,format->max_chunk_size);
}	
	
void print_media_format(media_format * format) {
	fprintf(stderr,"{");
	switch (format->type) {
	case B_MEDIA_NO_TYPE:		fprintf(stderr,"NO_TYPE"); break;		
	case B_MEDIA_UNKNOWN_TYPE:	fprintf(stderr,"UNKNOWN_TYPE"); break;
	case B_MEDIA_RAW_AUDIO:		fprintf(stderr,"RAW_AUDIO"); break;
	case B_MEDIA_RAW_VIDEO:		fprintf(stderr,"RAW_VIDEO"); break;
	case B_MEDIA_VBL:			fprintf(stderr,"VBL"); break;
	case B_MEDIA_TIMECODE:		fprintf(stderr,"TIMECODE"); break;
	case B_MEDIA_MIDI:			fprintf(stderr,"MIDI"); break;
	case B_MEDIA_TEXT:			fprintf(stderr,"TEXT"); break;
	case B_MEDIA_HTML:			fprintf(stderr,"HTML"); break;
	case B_MEDIA_MULTISTREAM:	fprintf(stderr,"MULTISTREAM"); break;
	case B_MEDIA_PARAMETERS:	fprintf(stderr,"PARAMETERS"); break;
	case B_MEDIA_ENCODED_AUDIO:	fprintf(stderr,"ENCODED_AUDIO"); break;
	case B_MEDIA_ENCODED_VIDEO:	fprintf(stderr,"ENCODED_VIDEO"); break;
	default:					fprintf(stderr,"????"); break;
	}
	fprintf(stderr,":");
	switch (format->type) {
	case B_MEDIA_RAW_AUDIO:		fprintf(stderr,"RAW_AUDIO"); break;
	case B_MEDIA_RAW_VIDEO:		fprintf(stderr,"RAW_VIDEO"); break;
	case B_MEDIA_MULTISTREAM:	print_multistream_format(&format->u.multistream); break;
	case B_MEDIA_ENCODED_AUDIO:	fprintf(stderr,"ENCODED_AUDIO"); break;
	case B_MEDIA_ENCODED_VIDEO:	fprintf(stderr,"ENCODED_VIDEO"); break;
	default:					fprintf(stderr,"????"); break;
	}
	fprintf(stderr,"}");
}
*/

bool multistream_format_is_acceptible(
						const media_multistream_format & producer_format,
						const media_multistream_format & consumer_format)
{
	// first check the format, if necessary
	if (consumer_format.format != media_multistream_format::B_ANY) {
		if (consumer_format.format != producer_format.format) {
			return false;
		}
	}
	// then check the average bit rate
	if (consumer_format.avg_bit_rate != media_multistream_format::wildcard.avg_bit_rate) {
		if (consumer_format.avg_bit_rate != producer_format.avg_bit_rate) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the maximum bit rate
	if (consumer_format.max_bit_rate != media_multistream_format::wildcard.max_bit_rate) {
		if (consumer_format.max_bit_rate != producer_format.max_bit_rate) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the average chunk size
	if (consumer_format.avg_chunk_size != media_multistream_format::wildcard.avg_chunk_size) {
		if (consumer_format.avg_chunk_size != producer_format.avg_chunk_size) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// then check the maximum bit rate
	if (consumer_format.max_chunk_size != media_multistream_format::wildcard.max_chunk_size) {
		if (consumer_format.max_chunk_size != producer_format.max_chunk_size) {
			// do they have to match exactly?  I don't know.  assume yes.
			return false;
		}
	}
	// should also check format specific fields, and others?
	return true;
}						

bool format_is_acceptible(
						const media_format & producer_format,
						const media_format & consumer_format)
{
	// first check the type, if necessary
	if (consumer_format.type != B_MEDIA_UNKNOWN_TYPE) {
		if (consumer_format.type != producer_format.type) {
			return false;
		}
		switch (consumer_format.type) {
			case B_MEDIA_MULTISTREAM:
				if (!multistream_format_is_acceptible(producer_format.u.multistream,
													  consumer_format.u.multistream)) {
					return false;
				}
				break;
			default:
				fprintf(stderr,"format_is_acceptible : unimplemented type.\n");
				return format_is_compatible(producer_format,consumer_format);
				break;
		}
	}
	// should also check non-type fields?
	return true;
}

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t MediaWriter::AcceptFormat(
				const media_destination & dest,
				media_format * format)
{
	fprintf(stderr,"MediaWriter::AcceptFormat\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if (input.destination != dest) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION");
		return B_MEDIA_BAD_DESTINATION; // we only have one input so that better be it
	}
/*	media_format * myFormat = GetFormat();
	fprintf(stderr,"proposed format: ");
	print_media_format(format);
	fprintf(stderr,"\n");
	fprintf(stderr,"my format: ");
	print_media_format(myFormat);
	fprintf(stderr,"\n");*/
	// Be's format_is_compatible doesn't work.
//	if (!format_is_compatible(*format,*myFormat)) {
	media_format myFormat;
	GetFormat(&myFormat);
	if (!format_is_acceptible(*format,myFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	AddRequirements(format);
	return B_OK;	
}

status_t MediaWriter::GetNextInput(
				int32 * cookie,
				media_input * out_input)
{
	fprintf(stderr,"MediaWriter::GetNextInput\n");
	// let's not crash even if they are stupid
	if (out_input == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (cookie != 0) {
		// it's valid but they already got our 1 input
		if (*cookie != 0) {
			fprintf(stderr,"<- B_ERROR\n");
			return B_ERROR;
		}
		// so next time they won't get the same input again
		*cookie = 1;
	}
	*out_input = input;
	return B_OK;
}

void MediaWriter::DisposeInputCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaWriter::DisposeInputCookie\n");
	// nothing to do since our cookies are just integers
	return; // B_OK;
}

void MediaWriter::BufferReceived(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaWriter::BufferReceived\n");
	switch (buffer->Header()->type) {
		case B_MEDIA_PARAMETERS:
			{
			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
			if (status != B_OK) {
				fprintf(stderr,"ApplyParameterData in MediaWriter::BufferReceived failed\n");
			}			
			buffer->Recycle();
			}
			break;
		case B_MEDIA_MULTISTREAM:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in MediaWriter::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in MediaWriter::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in MediaWriter::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void MediaWriter::ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	fprintf(stderr,"MediaWriter::ProducerDataStatus\n");
	if (input.destination != for_whom) {
		fprintf(stderr,"invalid destination received in MediaWriter::ProducerDataStatus\n");
		return;
	}
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&input, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t MediaWriter::GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource)
{
	fprintf(stderr,"MediaWriter::GetLatencyFor\n");
	if ((out_latency == 0) || (out_timesource == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (input.destination != for_whom) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

status_t MediaWriter::Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input)
{
	fprintf(stderr,"MediaWriter::Connected\n");
	if (out_input == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if (input.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	// clear any stale buffer groups
	if (fBufferGroup != 0) {
		BBufferGroup * group = fBufferGroup;
		fBufferGroup = 0;
		delete group;
	}

	// XXX: set the latency here?
	// SetEventLatency(latency);

	// record the agreed upon values
	input.source = producer;
	input.format = with_format;
	*out_input = input;
	return B_OK;
}

void MediaWriter::Disconnected(
				const media_source & producer,
				const media_destination & where)
{
	fprintf(stderr,"MediaWriter::Disconnected\n");
	if (input.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (input.source != producer) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	input.source = media_source::null;
	GetFormat(&input.format);
	if (fBufferGroup != 0) {
		BBufferGroup * group = fBufferGroup;
		fBufferGroup = 0;
		delete group;
	}
}

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
status_t MediaWriter::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format)
{
	fprintf(stderr,"MediaWriter::FormatChanged\n");
	if (input.source != producer) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (input.destination != consumer) {
		return B_MEDIA_BAD_DESTINATION;
	}
	// Since we don't really care about the format of the data
	// we can just continue to treat things the same way.
	input.format = format;
	return B_OK;
}

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
status_t MediaWriter::SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags)
{
	fprintf(stderr,"MediaWriter::SeekTagRequested\n");
	BBufferConsumer::SeekTagRequested(destination,in_target_time,in_flags,
									out_seek_tag,out_tagged_time,out_flags);
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

// protected:

// how should we handle late buffers?  drop them?
// notify the producer?
status_t MediaWriter::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaWriter::HandleBuffer\n");
	status_t status = B_OK;
	BBuffer * buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	WriteFileBuffer(buffer);
	buffer->Recycle();
	return status;
}

status_t MediaWriter::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaWriter::HandleDataStatus");
	// we have no where to send a data status to.
	return B_OK;
}


// -------------------------------------------------------- //
// MediaWriter specific functions
// -------------------------------------------------------- //

// static:

void MediaWriter::GetFlavor(flavor_info * outInfo, int32 id)
{
	fprintf(stderr,"MediaWriter::GetFlavor\n");
	if (outInfo == 0) {
		return;
	}
	AbstractFileInterfaceNode::GetFlavor(outInfo,id);
	outInfo->name = "OpenBeOS Media Writer";
	outInfo->info = "The OpenBeOS Media Writer consumes a multistream and writes a file.";
	outInfo->kinds |= B_BUFFER_CONSUMER;
	outInfo->in_format_count = 1; // 1 input
	media_format * formats = new media_format[outInfo->in_format_count];
	GetFormat(&formats[0]);
	outInfo->in_formats = formats;
	return;
}

void MediaWriter::GetFormat(media_format * outFormat)
{
	fprintf(stderr,"MediaWriter::GetFormat\n");
	if (outFormat == 0) {
		return;
	}
	AbstractFileInterfaceNode::GetFormat(outFormat);
	return;
}

void MediaWriter::GetFileFormat(media_file_format * outFileFormat)
{
	fprintf(stderr,"MediaWriter::GetFileFormat\n");
	if (outFileFormat == 0) {
		return;
	}
	AbstractFileInterfaceNode::GetFileFormat(outFileFormat);
	outFileFormat->capabilities |= media_file_format::B_WRITABLE;
	return;
}

// protected:

status_t MediaWriter::WriteFileBuffer(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaWriter::WriteFileBuffer\n");
	if (GetCurrentFile() == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}
	ssize_t bytesWriten = GetCurrentFile()->Write(buffer->Data(),buffer->SizeUsed());
	if (bytesWriten < 0) {
		fprintf(stderr,"<- B_FILE_ERROR\n");
		return B_FILE_ERROR; // some sort of file related error
	}
	// nothing more to say?
	return B_OK;
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaWriter::_Reserved_MediaWriter_0(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_1(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_2(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_3(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_4(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_5(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_6(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_7(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_8(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_9(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_10(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_11(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_12(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_13(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_14(void *) {}
status_t MediaWriter::_Reserved_MediaWriter_15(void *) {}
