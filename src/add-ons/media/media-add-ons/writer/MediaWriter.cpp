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
#include <MediaRoster.h>
#include <limits.h>

#include "../AbstractFileInterfaceNode.h"
#include "MediaWriter.h"
#include "misc.h"

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
	input.destination.port = input.node.port; // same as ControlPort()
	
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
	status_t status;
	status = AbstractFileInterfaceNode::SetRef(file,B_WRITE_ONLY,create,out_time);
	if (status != B_OK) {
		fprintf(stderr,"AbstractFileInterfaceNode::SetRef returned an error\n");
		return status;
	}
	if (input.source == media_source::null) {
		// reset the format, and set the requirements imposed by this file
		GetFormat(&input.format);
		AddRequirements(&input.format);
		return B_OK;
	}
	// if we are connected we may have to re-negotiate the connection
	media_format format;
	GetFormat(&format);
	AddRequirements(&format);
	if (format_is_acceptible(input.format,format)) {
		fprintf(stderr,"  compatible format = no re-negotiation necessary\n");
		return B_OK;
	}
	// first try the easy way : SORRY DEPRECATED into private :-(
//	int32 change_tag = NewChangeTag();
//	status = this->BBufferConsumer::RequestFormatChange(input.source,input.destination,&format,&change_tag);
//	if (status == B_OK) {
//		fprintf(stderr,"  format change successful\n");
//		return B_OK;
//	}
	// okay, the hard way requires we get the MediaRoster
	BMediaRoster * roster = BMediaRoster::Roster(&status);
	if (roster == 0) {
		return B_MEDIA_SYSTEM_FAILURE;
	}
	if (status != B_OK) {
		return status;
	}
	// before disconnect one should always stop the nodes (bebook says)
	// requires run_state cast since the return type on RunState() is
	// wrong [int32]
	run_state destinationRunState = run_state(RunState());
	if (destinationRunState == BMediaEventLooper::B_STARTED) {
		Stop(0,true); // stop us right now
	}
	// should also stop the source if it is running, but how?
/*	BMediaNode sourceNode = ??
	run_state sourceRunState = sourceNode->??;
	status = sourceNode->StopNode(??,0,true);
	if (status != B_OK) {
		return status;
	}  */
	// we should disconnect right now
	media_source inputSource = input.source;
	status = roster->Disconnect(input.source.id,input.source,
							    input.destination.id,input.destination);
	if (status != B_OK) {
		return status;
	}
	// if that went okay, we'll try reconnecting	
	media_output connectOutput;
	media_input connectInput;
	status = roster->Connect(inputSource,input.destination,
							 &format,&connectOutput,&connectInput);
	if (status != B_OK) {
		return status;
	}
	// now restart if necessary
	if (destinationRunState == BMediaEventLooper::B_STARTED) {
		Start(0);
	}							 
	return status;
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

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
			fprintf(stderr,"<- B_ERROR (no more inputs)\n");
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

	// compute the latency or just guess
	if (GetCurrentFile() != 0) {
		bigtime_t start, end;
		uint8 * data = new uint8[input.format.u.multistream.max_chunk_size]; // <- buffer group buffer size
		ssize_t bytesWritten = 0;
		{ // timed section
			start = TimeSource()->RealTime();
			bytesWritten = GetCurrentFile()->Write(data,input.format.u.multistream.max_chunk_size);
			end = TimeSource()->RealTime();
		}
		delete data;
		GetCurrentFile()->Seek(-bytesWritten,SEEK_CUR); // put it back where we found it
	
		fInternalLatency = end - start;
		
		fprintf(stderr,"  internal latency from disk write = %lld\n",fInternalLatency);
	} else {
		fInternalLatency = 500; // just guess
		fprintf(stderr,"  internal latency guessed = %lld\n",fInternalLatency);
	}
	
	SetEventLatency(fInternalLatency);

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
	return BBufferConsumer::SeekTagRequested(destination,in_target_time,in_flags,
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
	BBuffer * buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (buffer->Header()->destination != input.destination.id) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	WriteFileBuffer(buffer);
	buffer->Recycle();
	return B_OK;
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
	fprintf(stderr,"  writing %i bytes at %i\n",
			buffer->SizeUsed(),GetCurrentFile()->Position());
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
