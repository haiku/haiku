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

#include "MediaWriter.h"

#include <stdio.h>
#include <string.h>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaWriter::~MediaWriter(void)
{
	fprintf(stderr,"MediaWriter::~MediaWriter\n");
	// Stop the BMediaEventLooper thread
	Quit();
	if (outputFile != 0) {
		delete outputFile;
	}
	if (bufferGroup != 0) {
		BBufferGroup * group = bufferGroup;
		bufferGroup = 0;
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
	  BBufferConsumer(B_MEDIA_MULTISTREAM),
	  BFileInterface(),
	  BControllable(),
	  BMediaEventLooper()
{
	fprintf(stderr,"MediaWriter::MediaWriter\n");
	// keep our creator around for AddOn calls later
	mediaWriterAddOn = addOn;
	// null some fields
	outputFile = 0;
	bufferGroup = 0;
	output_mime_type[0] = '\0';
	// don't overwrite available space, and be sure to terminate
	strncpy(input.name,"MediaWriter Input",B_MEDIA_NAME_LENGTH-1);
	input.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the input
	input.node = media_node::null;               // until registration
	input.source = media_source::null; 
	input.destination = media_destination::null; // until registration
	// initialize the parameters
	if (defaultChunkSize <= 0) {
		initCheckStatus = B_BAD_VALUE;
		return;
	}
	defaultChunkSizeParam = defaultChunkSize;
	defaultChunkSizeParamChangeTime = 0;
	if (defaultBitRate <= 0) {
		initCheckStatus = B_BAD_VALUE;
		return;
	}
	defaultBitRateParam = defaultBitRate;
	defaultBitRateParamChangeTime = 0;
	// From the chunk size and bit rate we compute the buffer period.
	defaultBufferPeriodParam = int32(8000000/1024*defaultChunkSizeParam/defaultBitRateParam);
	if (defaultBufferPeriodParam <= 0) {
		initCheckStatus = B_BAD_VALUE;
		return;
	}
	defaultBufferPeriodParamChangeTime = 0;
	
	// this is also our preferred format
	initCheckStatus = ResetFormat(&input.format);
}

status_t MediaWriter::InitCheck(void) const
{
	fprintf(stderr,"MediaWriter::InitCheck\n");
	return initCheckStatus;
}

status_t MediaWriter::GetConfigurationFor(
				BMessage * into_message)
{
	fprintf(stderr,"MediaWriter::GetConfigurationFor\n");
	return B_OK;
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * MediaWriter::AddOn(
				int32 * internal_id) const
{
	fprintf(stderr,"MediaWriter::AddOn\n");
	// BeBook says this only gets called if we were in an add-on.
	if (mediaWriterAddOn != 0) {
		// If we get a null pointer then we just won't write.
		if (internal_id != 0) {
			internal_id = 0;
		}
	}
	return mediaWriterAddOn;
}

void MediaWriter::Start(
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaWriter::Start(pt=%i)\n",performance_time);
	BMediaEventLooper::Start(performance_time);
}

void MediaWriter::Stop(
				bigtime_t performance_time,
				bool immediate)
{
	fprintf(stderr,"MediaWriter::Stop(pt=%i,%s)\n",performance_time,(immediate?"now":"then"));
	BMediaEventLooper::Stop(performance_time,immediate);
}

void MediaWriter::Seek(
				bigtime_t media_time,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaWriter::Seek(mt=%i,pt=%i)\n",media_time,performance_time);
	BMediaEventLooper::Seek(media_time,performance_time);
}

void MediaWriter::SetRunMode(
				run_mode mode)
{
	fprintf(stderr,"MediaWriter::SetRunMode(%i)\n",mode);
	BMediaEventLooper::SetRunMode(mode);
}

void MediaWriter::TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time)
{
	fprintf(stderr,"MediaWriter::TimeWarp(rt=%i,pt=%i)\n",at_real_time,to_performance_time);
	BMediaEventLooper::TimeWarp(at_real_time,to_performance_time);
}

void MediaWriter::Preroll(void)
{
	fprintf(stderr,"MediaWriter::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

void MediaWriter::SetTimeSource(
				BTimeSource * time_source)
{
	fprintf(stderr,"MediaWriter::SetTimeSource\n");
	BMediaNode::SetTimeSource(time_source);
}

status_t MediaWriter::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	fprintf(stderr,"MediaWriter::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// none for now
		// maybe seeks later
		default:
			// XXX:FileInterface before BBufferConsumer or vice versa?
			status = BFileInterface::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			status = BBufferConsumer::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			status = BMediaNode::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			BMediaNode::HandleBadMessage(message,data,size);
			status = B_ERROR;
			break;
	}
	return status;
}

status_t MediaWriter::RequestCompleted(
				const media_request_info & info)
{
	fprintf(stderr,"MediaWriter::RequestCompleted\n");
	return BMediaNode::RequestCompleted(info);
}

status_t MediaWriter::DeleteHook(
				BMediaNode * node)
{
	fprintf(stderr,"MediaWriter::DeleteHook\n");
	return BMediaEventLooper::DeleteHook(node);
}

void MediaWriter::NodeRegistered(void)
{
	fprintf(stderr,"MediaWriter::NodeRegistered\n");
	// start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
	
	// now we can do this
	input.node = Node();
	input.destination.id = 0;
	input.destination.port = input.node.port;
	
	// and set up our parameter web
	SetParameterWeb(MakeParameterWeb());
}

status_t MediaWriter::GetNodeAttributes(
				media_node_attribute * outAttributes,
				size_t inMaxCount)
{
	fprintf(stderr,"MediaWriter::GetNodeAttributes\n");
	return BMediaNode::GetNodeAttributes(outAttributes,inMaxCount);
}

status_t MediaWriter::AddTimer(
					bigtime_t at_performance_time,
					int32 cookie)
{
	fprintf(stderr,"MediaWriter::AddTimer\n");
	return BMediaEventLooper::AddTimer(at_performance_time,cookie);
}

// protected:

BParameterWeb * MediaWriter::MakeParameterWeb(void)
{
	fprintf(stderr,"MediaWriter::MakeParameterWeb\n");
	
	BParameterWeb * web = new BParameterWeb();
	BParameterGroup * mainGroup = web->MakeGroup("MediaWriter Parameters");

	// these three are related:
	// DEFAULT_CHUNK_SIZE_PARAM = DEFAULT_BIT_RATE_PARAM / 1024 * DEFAULT_BUFFER_PERIOD_PARAM * 1000
	BParameterGroup * chunkSizeGroup = mainGroup->MakeGroup("Chunk Size Group");
	BContinuousParameter * chunkSizeParameter
	   = chunkSizeGroup->MakeContinuousParameter(
	     DEFAULT_CHUNK_SIZE_PARAM, B_MEDIA_MULTISTREAM,
		 "Chunk Size", B_GAIN, "bytes", 1024, 32*1024, 512);
	chunkSizeParameter->SetResponse(BContinuousParameter::B_LINEAR,1,0);
	chunkSizeParameter->SetValue(&defaultChunkSizeParam,sizeof(defaultChunkSizeParam),0);
	
	BParameterGroup * bitRateGroup = mainGroup->MakeGroup("Bit Rate Group");
	BContinuousParameter * bitRateParameter
	   = bitRateGroup->MakeContinuousParameter(
	     DEFAULT_BIT_RATE_PARAM, B_MEDIA_MULTISTREAM,
	     "Bit Rate", B_GAIN, "kbits/sec", 1, 320000, 1);
	bitRateParameter->SetResponse(BContinuousParameter::B_LINEAR,.001,0);
	bitRateParameter->SetValue(&defaultBitRateParam,sizeof(defaultBitRateParam),0);
	
	BParameterGroup * bufferPeriodGroup = mainGroup->MakeGroup("Buffer Period Group");
	BContinuousParameter * bufferPeriodParameter
	   = bufferPeriodGroup->MakeContinuousParameter(
	     DEFAULT_BUFFER_PERIOD_PARAM, B_MEDIA_MULTISTREAM,
	     "Buffer Period", B_GAIN, "ms", 1, 10000, 1);
	bufferPeriodParameter->SetResponse(BContinuousParameter::B_LINEAR,1,0);
	bufferPeriodParameter->SetValue(&defaultBufferPeriodParam,sizeof(defaultBufferPeriodParam),0);
	
	return web;
}

// -------------------------------------------------------- //
// implementation of BFileInterface
// -------------------------------------------------------- //

status_t MediaWriter::GetNextFileFormat(
				int32 * cookie,
				media_file_format * out_format)
{
	fprintf(stderr,"MediaWriter::GetNextFileFormat\n");
	// let's not crash even if they are stupid
	if (out_format == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (cookie != 0) {
		// it's valid but they already got our 1 file format
		if (*cookie != 0) {
			fprintf(stderr,"<- B_ERROR\n");
			return B_ERROR;
		}
		// so next time they won't get the same format again
		*cookie = 1;
	}
	*out_format = GetFileFormat();
	return B_OK;
}

void MediaWriter::DisposeFileFormatCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaWriter::DisposeFileFormatCookie\n");
	// nothing to do since our cookies are just integers
}

status_t MediaWriter::GetDuration(
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaWriter::GetDuration\n");
	if (out_time == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (outputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}	
	return outputFile->GetSize(out_time);
}

status_t MediaWriter::SniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality)
{
	fprintf(stderr,"MediaWriter::SniffRef\n");
	return StaticSniffRef(file,out_mime_type,out_quality);
}

status_t MediaWriter::SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaWriter::SetRef\n");
	if (out_time == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes today thanks
	}
	status_t status;
	output_ref = file;
	if (outputFile == 0) {
		outputFile = new BFile(&output_ref,(B_WRITE_ONLY|(create?B_CREATE_FILE:0)));
		status = outputFile->InitCheck();
	} else {
		status = outputFile->SetTo(&output_ref,(B_WRITE_ONLY|(create?B_CREATE_FILE:0)));
	}
	if (status != B_OK) {
		fprintf(stderr,"<- failed BFile initialization\n");
		return status;
	}
	// cache the output mime type for later
	outputFile->WriteAttr("BEOS:TYPE",0,0,output_mime_type,B_MIME_TYPE_LENGTH);
	// respecialize our preferred format based on this file type
	status = ResetFormat(&input.format);
	if (status != B_OK) {
		fprintf(stderr,"<- ResetFormat failed\n");
		return status;
	}
	// compute the duration and return any error
	return GetDuration(out_time);
}

status_t MediaWriter::GetRef(
				entry_ref * out_ref,
				char * out_mime_type)
{
	fprintf(stderr,"MediaWriter::GetRef\n");
	if ((out_ref == 0) || (out_mime_type == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes today thanks
	}
	if (outputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT; // the output_ref isn't valid yet either
	}
	*out_ref = output_ref;
	// they hopefully allocated enough space (no way to check :-/ )
	strcpy(out_mime_type,output_mime_type);
	return B_OK;
}

// provided for BMediaWriterAddOn

status_t MediaWriter::StaticSniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality)
{
	fprintf(stderr,"MediaWriter::StaticSniffRef\n");
	if ((out_mime_type == 0) || (out_quality == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // we refuse to crash because you were stupid
	}	
	BNode node(&file);
	status_t initCheck = node.InitCheck();
	if (initCheck != B_OK) {
		fprintf(stderr,"<- failed BNode::InitCheck()\n");
		return initCheck;
	}
	// they hopefully allocated enough room
	node.WriteAttr("BEOS:TYPE",0,0,out_mime_type,B_MIME_TYPE_LENGTH);
	*out_quality = 1.0; // we handle all files perfectly!  we are so amazing!
	return B_OK;
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

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
	// XXX: broken???
//	if (!format_is_compatible(input.format,*format)) {
//		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
//		return B_MEDIA_BAD_FORMAT;
//	}
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
	return;
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
	if (bufferGroup != 0) {
		BBufferGroup * group = bufferGroup;
		bufferGroup = 0;
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
	if ((input.source == producer) && (input.destination == where)) {
		input.source = media_source::null;
		ResetFormat(&input.format);
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
// implementation for BControllable
// -------------------------------------------------------- //

const int32 MediaWriter::DEFAULT_CHUNK_SIZE_PARAM = 1;
const int32 MediaWriter::DEFAULT_BIT_RATE_PARAM = 2;
const int32 MediaWriter::DEFAULT_BUFFER_PERIOD_PARAM = 3;

status_t MediaWriter::GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize)
{
	fprintf(stderr,"MediaWriter::GetParameterValue\n");
	if ((last_change == 0) || (value == 0) || (ioSize == 0)) {
		return B_BAD_VALUE; // no crashing
	}	
	switch (id) {
		case DEFAULT_CHUNK_SIZE_PARAM:
			if (*ioSize < sizeof(size_t)) {
				return B_ERROR; // not enough room
			}
			*last_change = defaultChunkSizeParamChangeTime;
			*((size_t*)value) = defaultChunkSizeParam;
			*ioSize = sizeof(size_t);
			break;
			
		case DEFAULT_BIT_RATE_PARAM:
			if (*ioSize < sizeof(float)) {
				return B_ERROR; // not enough room
			}
			*last_change = defaultBitRateParamChangeTime;
			*((float*)value) = defaultBitRateParam;
			*ioSize = sizeof(float);
			break;
		
		case DEFAULT_BUFFER_PERIOD_PARAM:
			if (*ioSize < sizeof(bigtime_t)) {
				return B_ERROR; // not enough room
			}
			*last_change = defaultBufferPeriodParamChangeTime;
			*((bigtime_t*)value) = defaultBufferPeriodParam;
			*ioSize = sizeof(bigtime_t);
			break;
				
		default:
			fprintf(stderr,"MediaWriter::GetParameterValue unknown id (%i)\n",id);
			return B_ERROR;
	}
	return B_OK;			
}
				
void MediaWriter::SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size)
{
	fprintf(stderr,"MediaWriter::SetParameterValue\n");
	switch (id) {
		case DEFAULT_CHUNK_SIZE_PARAM:
		case DEFAULT_BIT_RATE_PARAM:
		case DEFAULT_BUFFER_PERIOD_PARAM:
			{
				media_timed_event event(when, BTimedEventQueue::B_PARAMETER,
										NULL, BTimedEventQueue::B_NO_CLEANUP,
										size, id, (char*) value, size);
				EventQueue()->AddEvent(event);
			}
			break;
			
		default:
			fprintf(stderr,"MediaWriter::SetParameterValue unknown id (%i)\n",id);
			break;
	}
}			

// the default implementation should call the add-on main()
status_t MediaWriter::StartControlPanel(
				BMessenger * out_messenger)
{
	BControllable::StartControlPanel(out_messenger);
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void MediaWriter::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaWriter::HandleEvent(");
	switch (event->type) {
		case BTimedEventQueue::B_START:
			fprintf(stderr,"B_START)\n");
			if (RunState() != B_STARTED) {
				media_timed_event firstBufferEvent(event->event_time, BTimedEventQueue::B_HANDLE_BUFFER);
				//this->HandleEvent(&firstBufferEvent, 0, false);
				EventQueue()->AddEvent(firstBufferEvent);
			}
			break;
		case BTimedEventQueue::B_SEEK:
			// XXX: argghh.. seek events seem broken when received from
			//      the usual BMediaEventLooper::Seek() dispatcher :-(
			fprintf(stderr,"B_SEEK(t=%i,d=%i,bd=%ld))\n",event->event_time,event->data,event->bigdata);
			if (outputFile != 0) {
				outputFile->Seek(event->bigdata,SEEK_SET);
			}
			break;			
		case BTimedEventQueue::B_STOP:
			fprintf(stderr,"B_STOP)\n");
			// flush the queue so downstreamers don't get any more
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			fprintf(stderr,"B_HANDLE_BUFFER)\n");
			if ((RunState() != BMediaEventLooper::B_STARTED)
			 	   || (input.source == media_source::null)) {
				break; // confusing to receive a buffer without an input :-|
			}
			HandleBuffer(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			fprintf(stderr,"B_DATA_STATUS)\n");
//			SendDataStatus(event->data,output.destination,event->event_time);
			break;
		case BTimedEventQueue::B_PARAMETER:
			fprintf(stderr,"B_PARAMETER)\n");
			HandleParameter(event,lateness,realTimeEvent);
			break;
		default:
			fprintf(stderr,"%i)\n",event->type);
			break;
	}
}

/* override to clean up custom events you have added to your queue */
void MediaWriter::CleanUpEvent(
				const media_timed_event *event)
{
	return BMediaEventLooper::CleanUpEvent(event);
}
		
/* called from Offline mode to determine the current time of the node */
/* update your internal information whenever it changes */
bigtime_t MediaWriter::OfflineTime()
{
	fprintf(stderr,"MediaWriter::OfflineTime\n");
	if (outputFile == 0) {
		return 0;
	} else {
		return outputFile->Position();
	}
}

/* override only if you know what you are doing! */
/* otherwise much badness could occur */
/* the actual control loop function: */
/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
void MediaWriter::ControlLoop() {
	BMediaEventLooper::ControlLoop();
}

// protected:

status_t MediaWriter::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaWriter::HandleBuffer\n");
	status_t status = B_OK;
	return status;
}

status_t MediaWriter::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaWriter::HandleParameter\n");
	status_t status = B_OK;
	
	bool chunkSizeUpdated = false, bitRateUpdated = false, bufferPeriodUpdated = false;
	
	size_t dataSize = size_t(event->data);
	int32 param = int32(event->bigdata);
	
	switch (param) {
		case DEFAULT_CHUNK_SIZE_PARAM:
			if (dataSize < sizeof(size_t)) {
				fprintf(stderr,"<- B_BAD_VALUE\n",param);
				status = B_BAD_VALUE;
			} else {
				size_t newDefaultChunkSize = *((size_t*)event->user_data);
				// ignore non positive chunk sizes
				// XXX: we may decide later that a 0 chunk size means ship the whole file in one chunk (!)
				if ((newDefaultChunkSize > 0) && (newDefaultChunkSize != defaultChunkSizeParam)) {
					defaultChunkSizeParam = newDefaultChunkSize;
					defaultChunkSizeParamChangeTime = TimeSource()->Now();
					chunkSizeUpdated = true;
					if (leastRecentlyUpdatedParameter == DEFAULT_CHUNK_SIZE_PARAM) {
						// Okay we were the least recently updated parameter,
						// but we just got an update so we are no longer that.
						// Let's figure out who the new least recently updated
						// parameter is.  We are going to prefer to compute the
						// bit rate since you usually don't want to muck with
						// the buffer period.  However, if you just set the bitrate
						// then we are stuck with making the buffer period the new
						// parameter to be computed.
						if (lastUpdatedParameter == DEFAULT_BIT_RATE_PARAM) {
							leastRecentlyUpdatedParameter = DEFAULT_BUFFER_PERIOD_PARAM;
						} else {
							leastRecentlyUpdatedParameter = DEFAULT_BIT_RATE_PARAM;
						}
					}
					// we just got an update, so we are the new lastUpdatedParameter
					lastUpdatedParameter = DEFAULT_CHUNK_SIZE_PARAM;
					// now we have to compute the new value for the leastRecentlyUpdatedParameter
					// we use the chunk size change time to preserve "simultaneity" information
					if (leastRecentlyUpdatedParameter == DEFAULT_BUFFER_PERIOD_PARAM) {
						defaultBufferPeriodParam = MAX(1,int32(8000000/1024*defaultChunkSizeParam/defaultBitRateParam));
						defaultBufferPeriodParamChangeTime = defaultChunkSizeParamChangeTime;
						bufferPeriodUpdated = true;
					} else { // must have been bit rate
						defaultBitRateParam = MAX(0.001,8000000/1024*defaultChunkSizeParam/defaultBufferPeriodParam);
						defaultBitRateParamChangeTime = defaultChunkSizeParamChangeTime;
						bitRateUpdated = true;
					}
				}
			}
			break;		
		case DEFAULT_BIT_RATE_PARAM:
			if (dataSize < sizeof(size_t)) {
				fprintf(stderr,"<- B_BAD_VALUE\n",param);
				status = B_BAD_VALUE;
			} else {
				float newDefaultBitRate = *((size_t*)event->user_data);
				// ignore non positive bitrates
				if ((newDefaultBitRate > 0) && (newDefaultBitRate != defaultBitRateParam)) {
					defaultBitRateParam = newDefaultBitRate;
					defaultBitRateParamChangeTime = TimeSource()->Now();
					bitRateUpdated = true;
					if (leastRecentlyUpdatedParameter == DEFAULT_BIT_RATE_PARAM) {
						// Okay we were the least recently updated parameter,
						// but we just got an update so we are no longer that.
						// Let's figure out who the new least recently updated
						// parameter is.  We are going to prefer to compute the
						// chunk size since you usually don't want to muck with
						// the buffer period.  However, if you just set the chunk size
						// then we are stuck with making the buffer period the new
						// parameter to be computed.
						if (lastUpdatedParameter == DEFAULT_CHUNK_SIZE_PARAM) {
							leastRecentlyUpdatedParameter = DEFAULT_BUFFER_PERIOD_PARAM;
						} else {
							leastRecentlyUpdatedParameter = DEFAULT_CHUNK_SIZE_PARAM;
						}
					}
					// we just got an update, so we are the new lastUpdatedParameter
					lastUpdatedParameter = DEFAULT_BIT_RATE_PARAM;
					// now we have to compute the new value for the leastRecentlyUpdatedParameter
					// we use the bit rate change time to preserve "simultaneity" information
					if (leastRecentlyUpdatedParameter == DEFAULT_BUFFER_PERIOD_PARAM) {
						defaultBufferPeriodParam = MAX(1,int32(8000000/1024*defaultChunkSizeParam/defaultBitRateParam));
						defaultBufferPeriodParamChangeTime = defaultBitRateParamChangeTime;
						bufferPeriodUpdated = true;
					} else { // must have been chunk size
						defaultChunkSizeParam = MAX(1,int32(1024/8000000*defaultBitRateParam*defaultBufferPeriodParam));
						defaultChunkSizeParamChangeTime = defaultBitRateParamChangeTime;
						chunkSizeUpdated = true;
					}
				}
			}
			break;		
		case DEFAULT_BUFFER_PERIOD_PARAM:
			if (dataSize < sizeof(bigtime_t)) {
				fprintf(stderr,"<- B_BAD_VALUE\n",param);
				status = B_BAD_VALUE;
			} else {
				bigtime_t newBufferPeriod = *((bigtime_t*)event->user_data);
				// ignore non positive buffer period
				if ((newBufferPeriod > 0) && (newBufferPeriod != defaultBufferPeriodParam)) {
					defaultBufferPeriodParam = newBufferPeriod;
					defaultBufferPeriodParamChangeTime = TimeSource()->Now();
					bufferPeriodUpdated = true;
					if (lastUpdatedParameter == DEFAULT_BUFFER_PERIOD_PARAM) {
						// prefer to update bit rate, unless you just set it
						if (lastUpdatedParameter == DEFAULT_BIT_RATE_PARAM) {
							leastRecentlyUpdatedParameter = DEFAULT_CHUNK_SIZE_PARAM;
						} else {
							leastRecentlyUpdatedParameter = DEFAULT_BIT_RATE_PARAM;
						}
					}
					// we just got an update, so we are the new lastUpdatedParameter
					lastUpdatedParameter = DEFAULT_BUFFER_PERIOD_PARAM;
					// now we have to compute the new value for the leastRecentlyUpdatedParameter
					// we use the buffer period change time to preserve "simultaneity" information
					if (leastRecentlyUpdatedParameter == DEFAULT_BIT_RATE_PARAM) {
						defaultBitRateParam = MAX(0.001,8000000/1024*defaultChunkSizeParam/defaultBufferPeriodParam);
						defaultBitRateParamChangeTime = defaultBufferPeriodParamChangeTime;
						bitRateUpdated = true;
					} else { // must have been chunk size
						defaultChunkSizeParam = MAX(1,int32(1024/8000000*defaultBitRateParam*defaultBufferPeriodParam));
						defaultChunkSizeParamChangeTime = defaultBufferPeriodParamChangeTime;
						chunkSizeUpdated = true;
					}					
				}
			}
			break;	
		default:
			fprintf(stderr,"MediaWriter::HandleParameter called with unknown param id (%i)\n",param);
			status = B_ERROR;
	}
	// send updates out for all the parameters that changed
	// in every case this should be two updates. (if I have not made an error :-) )
	if (chunkSizeUpdated) {
		BroadcastNewParameterValue(defaultChunkSizeParamChangeTime,
								   DEFAULT_CHUNK_SIZE_PARAM,
								   &defaultChunkSizeParam,
								   sizeof(defaultChunkSizeParam));
	}
	if (bitRateUpdated) {
		BroadcastNewParameterValue(defaultBitRateParamChangeTime,
								   DEFAULT_BIT_RATE_PARAM,
								   &defaultBitRateParam,
								   sizeof(defaultBitRateParam));
	}
	if (bufferPeriodUpdated) {
		BroadcastNewParameterValue(defaultBufferPeriodParamChangeTime,
								   DEFAULT_BUFFER_PERIOD_PARAM,
								   &defaultBufferPeriodParam,
								   sizeof(defaultBufferPeriodParam));
	}
	return status;
}

// -------------------------------------------------------- //
// MediaWriter specific functions
// -------------------------------------------------------- //

status_t MediaWriter::GetFlavor(
				int32 id,
				const flavor_info ** out_info)
{
	fprintf(stderr,"MediaWriter::GetFlavor\n");
	static flavor_info flavorInfo;
	flavorInfo.name = "MediaWriter";
	flavorInfo.info =
	     "A MediaWriter node consumes a multistream and writes a file.";
	flavorInfo.kinds = B_FILE_INTERFACE | B_BUFFER_CONSUMER | B_CONTROLLABLE;
	flavorInfo.flavor_flags = B_FLAVOR_IS_LOCAL;
	flavorInfo.possible_count = INT_MAX;
	
	flavorInfo.in_format_count = 1; // 1 input
	flavorInfo.in_formats = &GetFormat();

	flavorInfo.out_format_count = 0; // no outputs
	flavorInfo.out_formats = 0;

	flavorInfo.internal_id = id;
		
	*out_info = &flavorInfo;
	return B_OK;
}

media_format & MediaWriter::GetFormat()
{
	fprintf(stderr,"MediaWriter::GetFormat\n");
	static media_format format;
	format.type = B_MEDIA_MULTISTREAM;
	format.require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
	format.u.multistream = media_multistream_format::wildcard;
	return format;
}

media_file_format & MediaWriter::GetFileFormat()
{
	fprintf(stderr,"MediaWriter::GetFileFormat\n");
	static media_file_format file_format;
	file_format.capabilities =
			    media_file_format::B_WRITABLE
			  | media_file_format::B_PERFECTLY_SEEKABLE
			  | media_file_format::B_IMPERFECTLY_SEEKABLE
			  | media_file_format::B_KNOWS_ANYTHING;
	/* I don't know what to initialize this to. (or if I should) */
	// format.id =
	file_format.family = B_ANY_FORMAT_FAMILY;
	file_format.version = 100;
	strcpy(file_format.mime_type,"");
	strcpy(file_format.pretty_name,"any media file format");
	strcpy(file_format.short_name,"any");
	strcpy(file_format.file_extension,"");
	return file_format;
}

status_t MediaWriter::ResetFormat(media_format * format)
{
	fprintf(stderr,"MediaWriter::ResetFormat\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*format = GetFormat();
	return ResolveWildcards(&format->u.multistream);
}

// Here we make some guesses based on the file's mime type
// It's not our job to fill all the fields but this may help
// find the right node that does have that job.
status_t MediaWriter::ResolveWildcards(
				media_multistream_format * multistream_format)
{
	fprintf(stderr,"MediaWriter::ResolveWildcards\n");
	if (strcmp("video/x-msvideo",output_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_AVI;
		}
	} else
	if (strcmp("video/mpeg",output_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_MPEG1;
		}
	} else
	if (strcmp("video/quicktime",output_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_QUICKTIME;
		}
	} else
	if (strcmp("audio/x-mpeg",output_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_MPEG1;
		}
	}
	// the thing that we connect to should really supply these,
	// but just in case we have some defaults handy!
	if (multistream_format->max_bit_rate == media_multistream_format::wildcard.max_bit_rate) {
		multistream_format->max_bit_rate = defaultBitRateParam;
	}
	if (multistream_format->max_chunk_size == media_multistream_format::wildcard.max_chunk_size) {
		multistream_format->max_chunk_size = defaultChunkSizeParam;
	}
	// we also default the averages to the maxes
	if (multistream_format->avg_bit_rate == media_multistream_format::wildcard.avg_bit_rate) {
		multistream_format->avg_bit_rate = multistream_format->max_bit_rate;
	}
	if (multistream_format->avg_chunk_size == media_multistream_format::wildcard.avg_chunk_size) {
		multistream_format->avg_chunk_size = multistream_format->max_chunk_size;
	}
	return B_OK;
}

status_t MediaWriter::GetFilledBuffer(
				BBuffer ** outBuffer)
{
	fprintf(stderr,"MediaWriter::GetFilledBuffer\n");
//	BBuffer * buffer = bufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,-1);
//	if (buffer == 0) {
		// XXX: add a new buffer and get it
//		fprintf(stderr,"MediaWriter::GetFilledBuffer needs a new buffer.\n");
//		return B_ERROR; // don't send the buffer
//	}
//	status_t status = FillFileBuffer(buffer);
//	*outBuffer = buffer;
//	return status;
	return B_OK;
}		

status_t MediaWriter::FillFileBuffer(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaWriter::FillFileBuffer\n");
	if (outputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}	
	off_t position = outputFile->Position();
	ssize_t bytesWrite = outputFile->Write(buffer->Data(),buffer->SizeAvailable());
	if (bytesWrite < 0) {
		fprintf(stderr,"<- B_FILE_ERROR\n");
		return B_FILE_ERROR; // some sort of file related error
	}
	buffer->SetSizeUsed(bytesWrite);
	media_header * header = buffer->Header();
	header->type = B_MEDIA_MULTISTREAM;
	header->size_used = bytesWrite;
	header->file_pos = position;
	header->orig_size = bytesWrite;
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
