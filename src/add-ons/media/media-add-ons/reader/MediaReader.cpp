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
#include "../AbstractFileInterfaceNode.h"
#include "MediaReader.h"
#include "misc.h"
#include "debug.h"

#include <Buffer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <Entry.h>
#include <Errors.h>
#include <File.h>
#include <FileInterface.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <TimeSource.h>


#include <limits.h>
#include <stdio.h>
#include <string.h>


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
				size_t defaultChunkSize,
				float defaultBitRate,
				const flavor_info * info,
				BMessage * config,
				BMediaAddOn * addOn)
	: BMediaNode("MediaReader"),
	  BBufferProducer(B_MEDIA_MULTISTREAM),
	  AbstractFileInterfaceNode(defaultChunkSize, defaultBitRate, info, config, addOn)
{
	CALLED();

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
	GetFormat(&output.format);
}


// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //
void MediaReader::Preroll(void)
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}


status_t MediaReader::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	CALLED();

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
	CALLED();
	
	// now we can do this
	output.node = Node();
	output.source.id = 0;
	output.source.port = output.node.port; // same as ControlPort();

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
	CALLED();

	status_t status = AbstractFileInterfaceNode::SetRef(file,B_READ_ONLY,create,out_time);
	if (status != B_OK) {
		PRINT("AbstractFileInterfaceNode::SetRef returned an error\n");
		return status;
	}

	if (output.destination == media_destination::null) {
		// reset the format, and set the requirements imposed by this file
		GetFormat(&output.format);
		AddRequirements(&output.format);
		return B_OK;
	}

	// if we are connected we may have to re-negotiate the connection
	media_format format;
	GetFormat(&format);
	AddRequirements(&format);
	if (format_is_acceptible(format,output.format)) {
		fprintf(stderr,"  compatible format = no re-negotiation necessary\n");
		return B_OK;
	}
	// first try the easy way : SORRY DEPRECATED into private :-(
	// this code from MediaWriter would be different for MediaReader even if it worked...
	//	int32 change_tag = NewChangeTag();
	//	status = this->BBufferConsumer::RequestFormatChange(output.source,output.destination,&format,&change_tag);
	//	if (status == B_OK) {
	//		fprintf(stderr,"  format change successful\n");
	//		return B_OK;
	//	}

	// okay, the hard way requires we get the MediaRoster
	BMediaRoster * roster = BMediaRoster::Roster(&status);
	if (roster == 0)
		return B_MEDIA_SYSTEM_FAILURE;

	if (status != B_OK)
		return status;

	// before disconnect one should always stop the nodes (bebook says)
	// requires run_state cast since the return type on RunState() is
	// wrong [int32]
	run_state destinationRunState = run_state(RunState());
	if (destinationRunState == BMediaEventLooper::B_STARTED)
		Stop(0,true); // stop us right now

	// should also stop the destination if it is running, but how?
	/*	BMediaNode destinationNode = ??
	run_state destinationRunState = destinationNode->??;
	status = destinationNode->StopNode(??,0,true);
	if (status != B_OK) {
		return status;
	}  */
	// we should disconnect right now
	media_destination outputDestination = output.destination;
	status = roster->Disconnect(output.source.id,output.source,
							    output.destination.id,output.destination);
	if (status != B_OK)
		return status;

	// if that went okay, we'll try reconnecting	
	media_output connectOutput;
	media_input connectInput;
	status = roster->Connect(output.source,outputDestination,
							 &format,&connectOutput,&connectInput);
	if (status != B_OK)
		return status;

	// now restart if necessary
	if (destinationRunState == BMediaEventLooper::B_STARTED) {
		Start(0);
	}							 
	return status;
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
	CALLED();

	if ((type != B_MEDIA_MULTISTREAM) && (type != B_MEDIA_UNKNOWN_TYPE)) {
		PRINT("\t<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	GetFormat(format);
	AddRequirements(format);
	return B_OK;
}


// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
status_t MediaReader::FormatProposal(
				const media_source & output_source,
				media_format * format)
{
	CALLED();

	if (output.source != output_source) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
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
	media_format myFormat;
	GetFormat(&myFormat);
	if (!format_is_acceptible(*format,myFormat)) {
		PRINT("\t<- B_MEDIA_BAD_FORMAT\n");
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
	CALLED();

	if (output.source != source) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}	
	status_t status = FormatProposal(source,io_format);
	if (status != B_OK) {
		PRINT("\terror returned by FormatProposal\n");
		GetFormat(io_format);
		return status;
	}

	return ResolveWildcards(io_format);
}


status_t MediaReader::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	CALLED();

	if (*cookie != 0) {
		PRINT("\t<- B_ERROR (no more outputs)\n");
		return B_ERROR;
	}

	// so next time they won't get the same output again
	*cookie = 1;
	*out_output = output;
	return B_OK;
}


status_t MediaReader::DisposeOutputCookie(
				int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
	return B_OK;
}


status_t MediaReader::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	CALLED();

	if (output.source != for_source) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE; // we only have one output so that better be it
	}
	if (fBufferGroup != 0) {
		if (fBufferGroup == group)
			return B_OK; // time saver
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
		PRINT("\tdownstream latency = %lld, buffer period = %lld, buffer count = %ld\n",
				fDownstreamLatency, fBufferPeriod, count);

		// allocate the buffers
		fBufferGroup = new BBufferGroup(output.format.u.multistream.max_chunk_size,count);
		if (fBufferGroup == 0) {
			PRINT("\t<- B_NO_MEMORY\n");
			return B_NO_MEMORY;
		}
		status_t status = fBufferGroup->InitCheck();
		if (status != B_OK) {
			PRINT("\t<- fBufferGroup initialization failed\n");
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
	CALLED();

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
	CALLED();

	if (output.source != what) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}	
	if (output.destination != media_destination::null) {
		PRINT("\t<- B_MEDIA_ALREADY_CONNECTED\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	status_t status = FormatChangeRequested(output.source,where,format,0);
	if (status != B_OK) {
		PRINT("\t<- MediaReader::FormatChangeRequested failed\n");
		return status;
	}

	// last check for wildcards and general validity
	if (format->type != B_MEDIA_MULTISTREAM) {
		PRINT("\t<- B_MEDIA_BAD_FORMAT\n");
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
	CALLED();

	if (error != B_OK) {
		PRINT("\t<- error already\n");
		output.destination = media_destination::null;
		GetFormat(&output.format);
		return;
	}
	if (output.source != source) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		output.destination = media_destination::null;
		GetFormat(&output.format);
		return;
	}	
	
	// record the agreed upon values
	output.destination = destination;
	output.format = format;
	strncpy(io_name,output.name,B_MEDIA_NAME_LENGTH-1);
	io_name[B_MEDIA_NAME_LENGTH-1] = '\0';

	// determine our downstream latency
	media_node_id id;
	FindLatencyFor(output.destination, &fDownstreamLatency, &id);

	// compute the buffer period (must be done before setbuffergroup)
	fBufferPeriod = bigtime_t(1000 * 8000000 / 1024
	                     * output.format.u.multistream.max_chunk_size
			             / output.format.u.multistream.max_bit_rate);

	PRINT("\tmax chunk size = %ld, max bit rate = %f, buffer period = %lld\n",
			output.format.u.multistream.max_chunk_size,
			output.format.u.multistream.max_bit_rate,fBufferPeriod);

	// setup the buffers if they aren't setup yet
	if (fBufferGroup == 0) {
		status_t status = SetBufferGroup(output.source,0);
		if (status != B_OK) {
			PRINT("\t<- SetBufferGroup failed\n");
			output.destination = media_destination::null;
			GetFormat(&output.format);
			return;
		}
	}

	SetBufferDuration(fBufferPeriod);

	if (GetCurrentFile() != 0) {
		bigtime_t start, end;
		// buffer group buffer size
		uint8 * data = new uint8[output.format.u.multistream.max_chunk_size];
		BBuffer * buffer = 0;
		ssize_t bytesRead = 0;
		{ // timed section
			start = TimeSource()->RealTime();
			// first we try to use a real BBuffer
			buffer = fBufferGroup->RequestBuffer(
					output.format.u.multistream.max_chunk_size,fBufferPeriod);
			if (buffer != 0) {
				FillFileBuffer(buffer);
			} else {
				// didn't get a real BBuffer, try simulation by just a read from the disk
				bytesRead = GetCurrentFile()->Read(
						data, output.format.u.multistream.max_chunk_size);
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
		
		PRINT("\tinternal latency from disk read = %lld\n", fInternalLatency);
	} else {
		fInternalLatency = 100; // just guess
		PRINT("\tinternal latency guessed = %lld\n", fInternalLatency);
	}
	
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	
	// XXX: do anything else?
}


void MediaReader::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	CALLED();

	if (output.destination != where) {
		PRINT("\t<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (output.source != what) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	output.destination = media_destination::null;
	GetFormat(&output.format);
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
	CALLED();

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
				// XXX : shorten our buffer period
				//       We could opt to just not wait but we should
				//       probably gradually shorten the period so we
				//       don't starve others.  Also, we need to make
				//       sure we are catching up!  We may have some sort
				//       of time goal for how long it takes us to
				//       catch up, as well.
				break;
			case B_DROP_DATA:
				// Okay you asked for it, we'll skip ahead in the file!
				// We'll drop 1 buffer's worth
				if (GetCurrentFile() == 0) {
					PRINT("MediaReader::LateNoticeReceived called without"
						  "an GetCurrentFile() (!)\n");
				} else {
					GetCurrentFile()->Seek(output.format.u.multistream.max_chunk_size,SEEK_CUR);
				}
				break;
			default:
				// huh?? there aren't any more run modes.
				PRINT("MediaReader::LateNoticeReceived with unexpected run mode.\n");
				break;
		}
	}
}


void MediaReader::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
	CALLED();

	if (output.source != what) {
		PRINT("\t<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	fOutputEnabled = enabled;
}


status_t MediaReader::SetPlayRate(
				int32 numer,
				int32 denom)
{
	return BBufferProducer::SetPlayRate(numer,denom); // XXX: do something intelligent later
}


void MediaReader::AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	CALLED();

	if (output.source == source) {
		BBuffer * buffer;
		status_t status = GetFilledBuffer(&buffer);
		if (status != B_OK) {
			PRINT("MediaReader::AdditionalBufferRequested got an error from GetFilledBuffer.\n");
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
	CALLED();
	if ((output.source == source) && (output.destination == destination)) {
		fDownstreamLatency = new_latency;
		SetEventLatency(fDownstreamLatency + fInternalLatency);
	}
	// we may have to recompute the number of buffers that we are using
	// see SetBufferGroup
}


// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //
// protected:
status_t MediaReader::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent)
{
	CALLED();

	if (output.destination == media_destination::null)
		return B_MEDIA_NOT_CONNECTED;

	status_t status = B_OK;
	BBuffer * buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,fBufferPeriod);
	if (buffer != 0) {
	    status = FillFileBuffer(buffer);
	    if (status != B_OK) {
			PRINT("MediaReader::HandleEvent got an error from FillFileBuffer.\n");
			buffer->Recycle();
		} else {
			if (fOutputEnabled) {
				status = SendBuffer(buffer,output.destination);
				if (status != B_OK) {
					PRINT("MediaReader::HandleEvent got an error from SendBuffer.\n");
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
						bool realTimeEvent)
{
	CALLED();
	return SendDataStatus(event->data,output.destination,event->event_time);
}


// -------------------------------------------------------- //
// MediaReader specific functions
// -------------------------------------------------------- //
// static:
void MediaReader::GetFlavor(flavor_info * outInfo, int32 id)
{
	CALLED();

	if (outInfo == 0)
		return;

	AbstractFileInterfaceNode::GetFlavor(outInfo,id);
	outInfo->name = strdup("OpenBeOS Media Reader");
	outInfo->info = strdup("The OpenBeOS Media Reader reads a file and produces a multistream.");
	outInfo->kinds |= B_BUFFER_PRODUCER;
	outInfo->out_format_count = 1; // 1 output
	media_format * formats = new media_format[outInfo->out_format_count];
	GetFormat(&formats[0]);
	outInfo->out_formats = formats;
	return;
}


void MediaReader::GetFormat(media_format * outFormat)
{
	CALLED();

	AbstractFileInterfaceNode::GetFormat(outFormat);
	return;
}


void MediaReader::GetFileFormat(media_file_format * outFileFormat)
{
	CALLED();

	AbstractFileInterfaceNode::GetFileFormat(outFileFormat);
	outFileFormat->capabilities |= media_file_format::B_READABLE;
	return;
}


// protected:
status_t MediaReader::GetFilledBuffer(
				BBuffer ** outBuffer)
{
	CALLED();

	BBuffer * buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,-1);
	if (buffer == 0) {
		// XXX: add a new buffer and get it
		PRINT("MediaReader::GetFilledBuffer needs a new buffer.\n");
		return B_ERROR; // don't send the buffer
	}

	status_t status = FillFileBuffer(buffer);
	*outBuffer = buffer;
	return status;
}		


status_t MediaReader::FillFileBuffer(
				BBuffer * buffer)
{
	CALLED();

	if (GetCurrentFile() == 0) {
		PRINT("\t<- B_NO_INIT\n");
		return B_NO_INIT;
	}
	PRINT("\t%ld buffer bytes used, %ld buffer bytes available\n",
			buffer->SizeUsed(), buffer->SizeAvailable());
	off_t position = GetCurrentFile()->Position();
	ssize_t bytesRead = GetCurrentFile()->Read(buffer->Data(),buffer->SizeAvailable());
	if (bytesRead < 0) {
		PRINT("\t<- B_FILE_ERROR\n");
		return B_FILE_ERROR; // some sort of file related error
	}
	PRINT("\t%ld file bytes read at position %ld.\n",
			bytesRead, position);

	buffer->SetSizeUsed(bytesRead);
	media_header * header = buffer->Header();
	header->type = B_MEDIA_MULTISTREAM;
	header->size_used = bytesRead;
	header->file_pos = position;
	header->orig_size = bytesRead;
	header->time_source = TimeSource()->ID();
	header->start_time = TimeSource()->Now();
	// nothing more to say?
	return B_OK;
}


// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //
status_t MediaReader::_Reserved_MediaReader_0(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_1(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_2(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_3(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_4(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_5(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_6(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_7(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_8(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_9(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_10(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_11(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_12(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_13(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_14(void *) { return B_ERROR; }
status_t MediaReader::_Reserved_MediaReader_15(void *) { return B_ERROR; }
