// MediaReader.cpp
//
// Andrew Bachmann, 2002
//
// A MediaReader is a node that
// implements FileInterface and BBufferProducer.
// It reads any file and produces one output,
// which is a multistream.
//
// see also MediaReaderAddOn.cpp

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <BufferProducer.h>
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
#include "MediaReader.h"

#include <stdio.h>
#include <string.h>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaReader::~MediaReader(void)
{
	fprintf(stderr,"MediaReader::~MediaReader\n");
	if (fBufferGroup != 0) {
		BBufferGroup * group = fBufferGroup;
		fBufferGroup = 0;
		delete group;
	}	
}

MediaReader::MediaReader(
				size_t defaultChunkSize = 8192,
				float defaultBitRate = 800000,
				const flavor_info * info = 0,
				BMessage * config = 0,
				BMediaAddOn * addOn = 0)
	: BMediaNode("MediaReader"),
	  AbstractFileInterfaceNode(defaultChunkSize,defaultBitRate,info,config,addOn),
	  BBufferProducer(B_MEDIA_MULTISTREAM)
{
	fprintf(stderr,"MediaReader::MediaReader\n");
	// null some fields
	fBufferGroup = 0;	
	// start enabled
	fOutputEnabled = true;
	// don't overwrite available space, and be sure to terminate
	strncpy(output.name,"MediaReader Output",B_MEDIA_NAME_LENGTH-1);
	output.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the output
	output.node = media_node::null;     // until registration
	output.source = media_source::null; // until registration
	output.destination = media_destination::null;
	output.format = *GetFormat();
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

void MediaReader::Preroll(void)
{
	fprintf(stderr,"MediaReader::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t MediaReader::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	fprintf(stderr,"MediaReader::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// no special messages for now
		default:
			status = BBufferProducer::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			status = AbstractFileInterfaceNode::HandleMessage(message,data,size);
			break;
	}
	return status;
}

void MediaReader::NodeRegistered(void)
{
	fprintf(stderr,"MediaReader::NodeRegistered\n");
	
	// now we can do this
	output.node = Node();
	output.source.id = 0;
	output.source.port = output.node.port;

	// creates the parameter web and starts the looper thread
	AbstractFileInterfaceNode::NodeRegistered();
}

// -------------------------------------------------------- //
// implementation of BFileInterface
// -------------------------------------------------------- //

status_t MediaReader::SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaReader::SetRef\n");
	status_t status = AbstractFileInterfaceNode::SetRef(file,B_READ_ONLY,create,out_time);
	if (status != B_OK) {
		fprintf(stderr,"AbstractFileInterfaceNode::SetRef returned an error\n");
		return status;
	}
	// reset the format, and set the requirements imposed by this file
	output.format = *GetFormat();
	AddRequirements(&output.format);
	// if we are connected we have to re-negotiate the connection
	if (output.destination != media_destination::null) {
		fprintf(stderr,"  error connection re-negotiation not implemented");
		// XXX: implement re-negotiation
	}
	return B_OK;	
}

// -------------------------------------------------------- //
// implemention of BBufferProducer
// -------------------------------------------------------- //

// They are asking us to make the first offering.
// So, we get a fresh format and then add requirements based
// on the current file. (if any)
status_t MediaReader::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	fprintf(stderr,"MediaReader::GetRef\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if ((type != B_MEDIA_MULTISTREAM) && (type != B_MEDIA_UNKNOWN_TYPE)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	*format = *GetFormat();
	AddRequirements(format);
	return B_OK;
}

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

// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
status_t MediaReader::FormatProposal(
				const media_source & output_source,
				media_format * format)
{
	fprintf(stderr,"MediaReader::FormatProposal\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if (output.source != output_source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE; // we only have one output so that better be it
	}
/*	media_format * myFormat = GetFormat();
	fprintf(stderr,"proposed format: ");
	print_media_format(format);
	fprintf(stderr,"\n");
	fprintf(stderr,"my format: ");
	print_media_format(myFormat);
	fprintf(stderr,"\n"); */
	// Be's format_is_compatible doesn't work.
//	if (!format_is_compatible(*format,*myFormat)) {
	if (!format_is_acceptible(*format,*GetFormat())) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	AddRequirements(format);
	return B_OK;
}

// Presumably we have already agreed with them that this format is
// okay.  But just in case, we check the offer. (and complain if it
// is invalid)  Then as the last thing we do, we get rid of any
// remaining wilcards.
status_t MediaReader::FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaReader::FormatChangeRequested\n");
	if (io_format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if (output.source != source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}	
	status_t status = FormatProposal(source,io_format);
	if (status != B_OK) {
		fprintf(stderr,"  error returned by FormatProposal\n");
		*io_format = *GetFormat();
		return status;
	}
	return ResolveWildcards(io_format);
}

status_t MediaReader::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	fprintf(stderr,"MediaReader::GetNextOutput\n");
	// let's not crash even if they are stupid
	if (out_output == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (cookie != 0) {
		// it's valid but they already got our 1 output
		if (*cookie != 0) {
			fprintf(stderr,"<- B_ERROR\n");
			return B_ERROR;
		}
		// so next time they won't get the same output again
		*cookie = 1;
	}
	*out_output = output;
	return B_OK;
}

status_t MediaReader::DisposeOutputCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaReader::DisposeOutputCookie\n");
	// nothing to do since our cookies are just integers
	return B_OK;
}

status_t MediaReader::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	fprintf(stderr,"MediaReader::SetBufferGroup\n");
	if (output.source != for_source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE; // we only have one output so that better be it
	}
	if (fBufferGroup != 0) {
		if (fBufferGroup == group) {
			return B_OK; // time saver
		}	
		delete fBufferGroup;
	}
	if (group != 0) {
		fBufferGroup = group;
	} else {
		// let's take advantage of this opportunity to recalculate
		// our downstream latency and ensure that it is up to date
		media_node_id id;
		FindLatencyFor(output.destination, &fDownstreamLatency, &id);
		// buffer period gets initialized in Connect() because
		// that is the first time we get the real values for
		// chunk size and bit rate, which are used to compute buffer period
		// note: you can still make a buffer group before connecting (why?)
		//       but we don't make it, you make it yourself and pass it here.
		//       not sure why anybody would want to do that since they need
		//       a connection anyway...
		if (fBufferPeriod <= 0) {
			fprintf(stderr,"<- B_NO_INIT");
			return B_NO_INIT;
		}
		int32 count = int32(fDownstreamLatency/fBufferPeriod)+2;
		// allocate the buffers
		fBufferGroup = new BBufferGroup(output.format.u.multistream.max_chunk_size,count);
		if (fBufferGroup == 0) {
			fprintf(stderr,"<- B_NO_MEMORY\n");
			return B_NO_MEMORY;
		}
		status_t status = fBufferGroup->InitCheck();
		if (status != B_OK) {
			fprintf(stderr,"<- fBufferGroup initialization failed\n");
			return status;
		}
	}
	return B_OK;
}

	/* Format of clipping is (as int16-s): <from line> <npairs> <startclip> <endclip>. */
	/* Repeat for each line where the clipping is different from the previous line. */
	/* If <npairs> is negative, use the data from line -<npairs> (there are 0 pairs after */
	/* a negative <npairs>. Yes, we only support 32k*32k frame buffers for clipping. */
	/* Any non-0 field of 'display' means that that field changed, and if you don't support */
	/* that change, you should return an error and ignore the request. Note that the buffer */
	/* offset values do not have wildcards; 0 (or -1, or whatever) are real values and must */
	/* be adhered to. */
status_t MediaReader::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	return BBufferProducer::VideoClippingChanged(for_source,num_shorts,clip_data,display,_deprecated_);
}

status_t MediaReader::GetLatency(
				bigtime_t * out_latency)
{
	fprintf(stderr,"MediaReader::GetLatency\n");
	if (out_latency == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t MediaReader::PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	fprintf(stderr,"MediaReader::PrepareToConnect\n");
	if ((format == 0) || (out_source == 0) || (out_name == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes...
	}
	if (output.source != what) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}	
	if (output.destination != media_destination::null) {
		fprintf(stderr,"<- B_MEDIA_ALREADY_CONNECTED\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}	
	status_t status = FormatChangeRequested(output.source,where,format,0);
	if (status != B_OK) {
		fprintf(stderr,"<- MediaReader::FormatChangeRequested failed\n");
		return status;
	}
	// last check for wildcards and general validity
	if (format->type != B_MEDIA_MULTISTREAM) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	*out_source = output.source;
	output.destination = where;
	strncpy(out_name,output.name,B_MEDIA_NAME_LENGTH-1);
	out_name[B_MEDIA_NAME_LENGTH] = '\0';
	return ResolveWildcards(format);
}

void MediaReader::Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name)
{
	fprintf(stderr,"MediaReader::Connect\n");
	if (error != B_OK) {
		fprintf(stderr,"<- error already\n");
		output.destination = media_destination::null;
		output.format = *GetFormat();
		return;
	}
	if (output.source != source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		output.destination = media_destination::null;
		output.format = *GetFormat();
		return;
	}	
	
	// record the agreed upon values
	output.destination = destination;
	output.format = format;
	strncpy(io_name,output.name,B_MEDIA_NAME_LENGTH-1);
	io_name[B_MEDIA_NAME_LENGTH] = '\0';

	// determine our downstream latency
	media_node_id id;
	FindLatencyFor(output.destination, &fDownstreamLatency, &id);

	// compute the buffer period (must be done before setbuffergroup)
	fBufferPeriod = int32(8000000 / 1024
	                     * output.format.u.multistream.max_chunk_size
			             / output.format.u.multistream.max_bit_rate);

	// setup the buffers if they aren't setup yet
	if (fBufferGroup == 0) {
		status_t status = SetBufferGroup(output.source,0);
		if (status != B_OK) {
			fprintf(stderr,"<- SetBufferGroup failed\n");
			output.destination = media_destination::null;
			output.format = *GetFormat();
			return;
		}
	}
	SetBufferDuration(fBufferPeriod);
	
	if (GetCurrentFile() != 0) {
		bigtime_t start, end;
		uint8 * data = new uint8[output.format.u.multistream.max_chunk_size]; // <- buffer group buffer size
		BBuffer * buffer = 0;
		ssize_t bytesRead = 0;
		{ // timed section
			start = TimeSource()->RealTime();
			// first we try to use a real BBuffer
			buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,fBufferPeriod);
			if (buffer != 0) {
				FillFileBuffer(buffer);
			} else {
				// didn't get a real BBuffer, try simulation by just a read from the disk
				bytesRead = GetCurrentFile()->Read(data,output.format.u.multistream.max_chunk_size);
			}
			end = TimeSource()->RealTime();
		}
		bytesRead = buffer->SizeUsed();
		delete data;
		if (buffer != 0) {
			buffer->Recycle();
		}
		GetCurrentFile()->Seek(-bytesRead,SEEK_CUR); // put it back where we found it
	
		fInternalLatency = end - start;
		
		fprintf(stderr,"internal latency from disk read = %i\n",fInternalLatency);
	} else {
		fInternalLatency = 100; // just guess
		fprintf(stderr,"internal latency guessed = %i\n",fInternalLatency);
	}
	
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	
	// XXX: do anything else?
}

void MediaReader::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	fprintf(stderr,"MediaReader::Disconnect\n");
	if (output.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (output.source != what) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	output.destination = media_destination::null;
	output.format = *GetFormat();
	if (fBufferGroup != 0) {
		BBufferGroup * group = fBufferGroup;
		fBufferGroup = 0;
		delete group;
	}	
}

void MediaReader::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaReader::LateNoticeReceived\n");
	if (what == output.source) {
		switch (RunMode()) {
			case B_OFFLINE:
			    // nothing to do
				break;
			case B_RECORDING:
			    // nothing to do
				break;
			case B_INCREASE_LATENCY:
				fInternalLatency += how_much;
				SetEventLatency(fDownstreamLatency + fInternalLatency);
				break;
			case B_DECREASE_PRECISION:
				// What you want us to give you fewer bits or something?
				break;
			case B_DROP_DATA:
				// Okay you asked for it, we'll skip ahead in the file!
				// We'll drop 1 buffer's worth
				if (GetCurrentFile() == 0) {
					fprintf(stderr,"MediaReader::LateNoticeReceived called without an GetCurrentFile() (!)\n");
				} else {
					GetCurrentFile()->Seek(output.format.u.multistream.max_chunk_size,SEEK_CUR);
				}
				break;
			default:
				// huh?? there aren't any more run modes.
				fprintf(stderr,"MediaReader::LateNoticeReceived with unexpected run mode.\n");
				break;
		}
	}
}

void MediaReader::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaReader::EnableOutput\n");
	if (output.source != what) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	fOutputEnabled = enabled;
}

status_t MediaReader::SetPlayRate(
				int32 numer,
				int32 denom)
{
	BBufferProducer::SetPlayRate(numer,denom); // XXX: do something intelligent later
}

void MediaReader::AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	fprintf(stderr,"MediaReader::AdditionalBufferRequested\n");
	if (output.source == source) {
		BBuffer * buffer;
		status_t status = GetFilledBuffer(&buffer);
		if (status != B_OK) {
			fprintf(stderr,"MediaReader::AdditionalBufferRequested got an error from GetFilledBuffer.\n");
			return; // don't send the buffer
		}
		SendBuffer(buffer,output.destination);
	}
}

void MediaReader::LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags)
{
	fprintf(stderr,"MediaReader::LatencyChanged\n");
	if ((output.source == source) && (output.destination == destination)) {
		fDownstreamLatency = new_latency;
		SetEventLatency(fDownstreamLatency + fInternalLatency);
	}
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

// protected:

status_t MediaReader::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaReader::HandleBuffer\n");
	if (output.destination == media_destination::null) {
		return B_MEDIA_NOT_CONNECTED;
	}
	status_t status = B_OK;
	BBuffer * buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,fBufferPeriod);
	if (buffer != 0) {
	    status = FillFileBuffer(buffer);
	    if (status != B_OK) {
			fprintf(stderr,"MediaReader::HandleEvent got an error from FillFileBuffer.\n");
			buffer->Recycle();
		} else {
			if (fOutputEnabled) {
				status = SendBuffer(buffer,output.destination);
				if (status != B_OK) {
					fprintf(stderr,"MediaReader::HandleEvent got an error from SendBuffer.\n");
					buffer->Recycle();
				}
			} else {
				buffer->Recycle();
			}
		}
	}
	bigtime_t nextEventTime = event->event_time+fBufferPeriod;
	media_timed_event nextBufferEvent(nextEventTime, BTimedEventQueue::B_HANDLE_BUFFER);
	EventQueue()->AddEvent(nextBufferEvent);
	return status;
}

status_t MediaReader::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaReader::HandleDataStatus");
	return SendDataStatus(event->data,output.destination,event->event_time);
}

// -------------------------------------------------------- //
// MediaReader specific functions
// -------------------------------------------------------- //

// static:

void MediaReader::GetFlavor(flavor_info * info, int32 id)
{
	fprintf(stderr,"MediaReader::GetFlavor\n");
	AbstractFileInterfaceNode::GetFlavor(info,id);
	info->name = "OpenBeOS Media Reader";
	info->info = "The OpenBeOS Media Reader reads a file and produces a multistream.";
	info->kinds |= B_BUFFER_PRODUCER;
	info->out_format_count = 1; // 1 output
	info->out_formats = GetFormat();
	return;
}

media_format * MediaReader::GetFormat()
{
	fprintf(stderr,"MediaReader::GetFormat\n");
	return AbstractFileInterfaceNode::GetFormat();
}

media_file_format * MediaReader::GetFileFormat()
{
	fprintf(stderr,"MediaReader::GetFileFormat\n");
	static bool initialized = false;
	static media_file_format * file_format;
	if (initialized == false) {
		file_format = AbstractFileInterfaceNode::GetFileFormat();
		file_format->capabilities |= media_file_format::B_READABLE;
		initialized = true;
	}
	return file_format;
}

// protected:

status_t MediaReader::GetFilledBuffer(
				BBuffer ** outBuffer)
{
	fprintf(stderr,"MediaReader::GetFilledBuffer\n");
	BBuffer * buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,-1);
	if (buffer == 0) {
		// XXX: add a new buffer and get it
		fprintf(stderr,"MediaReader::GetFilledBuffer needs a new buffer.\n");
		return B_ERROR; // don't send the buffer
	}
	status_t status = FillFileBuffer(buffer);
	*outBuffer = buffer;
	return status;
}		

status_t MediaReader::FillFileBuffer(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaReader::FillFileBuffer\n");
	if (GetCurrentFile() == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}	
	off_t position = GetCurrentFile()->Position();
	ssize_t bytesRead = GetCurrentFile()->Read(buffer->Data(),buffer->SizeAvailable());
	if (bytesRead < 0) {
		fprintf(stderr,"<- B_FILE_ERROR\n");
		return B_FILE_ERROR; // some sort of file related error
	}
	buffer->SetSizeUsed(bytesRead);
	media_header * header = buffer->Header();
	header->type = B_MEDIA_MULTISTREAM;
	header->size_used = bytesRead;
	header->file_pos = position;
	header->orig_size = bytesRead;
	header->time_source = TimeSource()->ID();
	// in our world performance time corresponds to bytes in the file
	// so the start time of this buffer of bytes is simply its position
	header->start_time = position;
	// nothing more to say?
	return B_OK;
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaReader::_Reserved_MediaReader_0(void *) {}
status_t MediaReader::_Reserved_MediaReader_1(void *) {}
status_t MediaReader::_Reserved_MediaReader_2(void *) {}
status_t MediaReader::_Reserved_MediaReader_3(void *) {}
status_t MediaReader::_Reserved_MediaReader_4(void *) {}
status_t MediaReader::_Reserved_MediaReader_5(void *) {}
status_t MediaReader::_Reserved_MediaReader_6(void *) {}
status_t MediaReader::_Reserved_MediaReader_7(void *) {}
status_t MediaReader::_Reserved_MediaReader_8(void *) {}
status_t MediaReader::_Reserved_MediaReader_9(void *) {}
status_t MediaReader::_Reserved_MediaReader_10(void *) {}
status_t MediaReader::_Reserved_MediaReader_11(void *) {}
status_t MediaReader::_Reserved_MediaReader_12(void *) {}
status_t MediaReader::_Reserved_MediaReader_13(void *) {}
status_t MediaReader::_Reserved_MediaReader_14(void *) {}
status_t MediaReader::_Reserved_MediaReader_15(void *) {}
