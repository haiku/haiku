// MediaFileProducer.cpp
//
// Andrew Bachmann, 2002
//
// A MediaFileProducer is a node that
// implements FileInterface and BBufferProducer.
// It reads any file and produces one output,
// which is a multistream.
//
// see also MediaFileProducerAddOn.cpp

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

#include "MediaFileProducer.h"

#include <stdio.h>
#include <string.h>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaFileProducer::~MediaFileProducer(void)
{
	fprintf(stderr,"MediaFileProducer::~MediaFileProducer\n");
	// Stop the BMediaEventLooper thread
	Quit();
	if (inputFile != 0) {
		delete inputFile;
	}
}

MediaFileProducer::MediaFileProducer(
				size_t defaultChunkSize = 8192,
				float defaultBitRate = 800000,
				const flavor_info * info = 0,
				BMessage * config = 0,
				BMediaAddOn * addOn = 0)
	: BMediaNode("MediaFileProducer"),
	  BBufferProducer(B_MEDIA_MULTISTREAM),
	  BFileInterface(),
	  BControllable(),
	  BMediaEventLooper()
{
	fprintf(stderr,"MediaFileProducer::MediaFileProducer\n");
	// keep our creator around for AddOn calls later
	mediaFileProducerAddOn = addOn;
	// null some fields
	inputFile = 0;
	bufferGroup = 0;
	input_mime_type[0] = '\0';
	// start enabled
	outputEnabled = true;
	// don't overwrite available space, and be sure to terminate
	strncpy(output.name,"MediaFileProducer Output",B_MEDIA_NAME_LENGTH-1);
	output.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the output
	output.node = media_node::null;     // until registration
	output.source = media_source::null; // until registration
	output.destination = media_destination::null;
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
	initCheckStatus = ResetFormat(&output.format);
}

status_t MediaFileProducer::InitCheck(void) const
{
	fprintf(stderr,"MediaFileProducer::InitCheck\n");
	return initCheckStatus;
}

status_t MediaFileProducer::GetConfigurationFor(
				BMessage * into_message)
{
	fprintf(stderr,"MediaFileProducer::GetConfigurationFor\n");
	return B_OK;
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * MediaFileProducer::AddOn(
				int32 * internal_id) const
{
	fprintf(stderr,"MediaFileProducer::AddOn\n");
	// BeBook says this only gets called if we were in an add-on.
	if (mediaFileProducerAddOn != 0) {
		// If we get a null pointer then we just won't write.
		if (internal_id != 0) {
			internal_id = 0;
		}
	}
	return mediaFileProducerAddOn;
}

void MediaFileProducer::Start(
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaFileProducer::Start(pt=%i)\n",performance_time);
	BMediaEventLooper::Start(performance_time);
}

void MediaFileProducer::Stop(
				bigtime_t performance_time,
				bool immediate)
{
	fprintf(stderr,"MediaFileProducer::Stop(pt=%i,%s)\n",performance_time,(immediate?"now":"then"));
	BMediaEventLooper::Stop(performance_time,immediate);
}

void MediaFileProducer::Seek(
				bigtime_t media_time,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaFileProducer::Seek(mt=%i,pt=%i)\n",media_time,performance_time);
	BMediaEventLooper::Seek(media_time,performance_time);
}

void MediaFileProducer::SetRunMode(
				run_mode mode)
{
	fprintf(stderr,"MediaFileProducer::SetRunMode(%i)\n",mode);
	BMediaEventLooper::SetRunMode(mode);
}

void MediaFileProducer::TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time)
{
	fprintf(stderr,"MediaFileProducer::TimeWarp(rt=%i,pt=%i)\n",at_real_time,to_performance_time);
	BMediaEventLooper::TimeWarp(at_real_time,to_performance_time);
}

void MediaFileProducer::Preroll(void)
{
	fprintf(stderr,"MediaFileProducer::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

void MediaFileProducer::SetTimeSource(
				BTimeSource * time_source)
{
	fprintf(stderr,"MediaFileProducer::SetTimeSource\n");
	BMediaNode::SetTimeSource(time_source);
}

status_t MediaFileProducer::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	fprintf(stderr,"MediaFileProducer::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// none for now
		// maybe seeks later
		default:
			// XXX:FileInterface before BufferProducer or vice versa?
			status = BFileInterface::HandleMessage(message,data,size);
			if (status == B_OK) {
				break;
			}
			status = BBufferProducer::HandleMessage(message,data,size);
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

status_t MediaFileProducer::RequestCompleted(
				const media_request_info & info)
{
	fprintf(stderr,"MediaFileProducer::RequestCompleted\n");
	return BMediaNode::RequestCompleted(info);
}

status_t MediaFileProducer::DeleteHook(
				BMediaNode * node)
{
	fprintf(stderr,"MediaFileProducer::DeleteHook\n");
	return BMediaEventLooper::DeleteHook(node);
}

void MediaFileProducer::NodeRegistered(void)
{
	fprintf(stderr,"MediaFileProducer::NodeRegistered\n");
	// start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
	
	// now we can do this
	output.node = Node();
	output.source.id = 0;
	output.source.port = output.node.port;
	
	// and set up our parameter web
	SetParameterWeb(MakeParameterWeb());
}

status_t MediaFileProducer::GetNodeAttributes(
				media_node_attribute * outAttributes,
				size_t inMaxCount)
{
	fprintf(stderr,"MediaFileProducer::GetNodeAttributes\n");
	return BMediaNode::GetNodeAttributes(outAttributes,inMaxCount);
}

status_t MediaFileProducer::AddTimer(
					bigtime_t at_performance_time,
					int32 cookie)
{
	fprintf(stderr,"MediaFileProducer::AddTimer\n");
	return BMediaEventLooper::AddTimer(at_performance_time,cookie);
}

// protected:

BParameterWeb * MediaFileProducer::MakeParameterWeb(void)
{
	fprintf(stderr,"MediaFileProducer::MakeParameterWeb\n");
	
	BParameterWeb * web = new BParameterWeb();
	BParameterGroup * mainGroup = web->MakeGroup("MediaFileProducer Parameters");

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

status_t MediaFileProducer::GetNextFileFormat(
				int32 * cookie,
				media_file_format * out_format)
{
	fprintf(stderr,"MediaFileProducer::GetNextFileFormat\n");
	// let's not crash even if they are stupid
	if (cookie != 0) {
		// it's valid but they already got our 1 file format
		if (*cookie != 0) {
			fprintf(stderr,"<- B_ERROR\n");
			return B_ERROR;
		}
		// so next time they won't get the same format again
		*cookie = 1;
	}
	if (out_format == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*out_format = GetFileFormat();
	return B_OK;
}

void MediaFileProducer::DisposeFileFormatCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaFileProducer::DisposeFileFormatCookie\n");
	// nothing to do since our cookies are just integers
}

status_t MediaFileProducer::GetDuration(
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaFileProducer::GetDuration\n");
	if (out_time == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (inputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}	
	return inputFile->GetSize(out_time);
}

status_t MediaFileProducer::SniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality)
{
	fprintf(stderr,"MediaFileProducer::SniffRef\n");
	return StaticSniffRef(file,out_mime_type,out_quality);
}

status_t MediaFileProducer::SetRef(
				const entry_ref & file,
				bool create,
				bigtime_t * out_time)
{
	fprintf(stderr,"MediaFileProducer::SetRef\n");
	if (out_time == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes today thanks
	}
	status_t status;
	input_ref = file;
	if (inputFile == 0) {
		inputFile = new BFile(&input_ref,(B_READ_ONLY|(create?B_CREATE_FILE:0)));
		status = inputFile->InitCheck();
	} else {
		status = inputFile->SetTo(&input_ref,(B_READ_ONLY|(create?B_CREATE_FILE:0)));
	}
	if (status != B_OK) {
		fprintf(stderr,"<- failed BFile initialization\n");
		return status;
	}
	// cache the input mime type for later
	inputFile->ReadAttr("BEOS:TYPE",0,0,input_mime_type,B_MIME_TYPE_LENGTH);
	// respecialize our preferred format based on this file type
	status = ResetFormat(&output.format);
	if (status != B_OK) {
		fprintf(stderr,"<- ResetFormat failed\n");
		return status;
	}
	// compute the duration and return any error
	return GetDuration(out_time);
}

status_t MediaFileProducer::GetRef(
				entry_ref * out_ref,
				char * out_mime_type)
{
	fprintf(stderr,"MediaFileProducer::GetRef\n");
	if ((out_ref == 0) || (out_mime_type == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes today thanks
	}
	if (inputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT; // the input_ref isn't valid yet either
	}
	*out_ref = input_ref;
	// they hopefully allocated enough space (no way to check :-/ )
	strcpy(out_mime_type,input_mime_type);
	return B_OK;
}

// provided for BMediaFileProducerAddOn

status_t MediaFileProducer::StaticSniffRef(
				const entry_ref & file,
				char * out_mime_type,	/* 256 bytes */
				float * out_quality)
{
	fprintf(stderr,"MediaFileProducer::StaticSniffRef\n");
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
	node.ReadAttr("BEOS:TYPE",0,0,out_mime_type,B_MIME_TYPE_LENGTH);
	*out_quality = 1.0; // we handle all files perfectly!  we are so amazing!
	return B_OK;
}

// -------------------------------------------------------- //
// implemention of BBufferProducer
// -------------------------------------------------------- //

status_t MediaFileProducer::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	fprintf(stderr,"MediaFileProducer::GetRef\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if ((type != B_MEDIA_MULTISTREAM) && (type != B_MEDIA_UNKNOWN_TYPE)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	return ResetFormat(format);
}

status_t MediaFileProducer::FormatProposal(
				const media_source & output_source,
				media_format * format)
{
	fprintf(stderr,"MediaFileProducer::FormatProposal\n");
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	if (output.source != output_source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE; // we only have one output so that better be it
	}
	if (!format_is_compatible(*format,output.format)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	output.format.SpecializeTo(format);
	return B_OK;
}

status_t MediaFileProducer::FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaFileProducer::FormatChangeRequested\n");
	if (io_format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	status_t status = FormatProposal(source,io_format);
	if (status == B_MEDIA_BAD_FORMAT) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		ResetFormat(io_format);
	}
	return status;
}

status_t MediaFileProducer::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	fprintf(stderr,"MediaFileProducer::GetNextOutput\n");
	// let's not crash even if they are stupid
	if (cookie != 0) {
		// it's valid but they already got our 1 output
		if (*cookie != 0) {
			fprintf(stderr,"<- B_ERROR\n");
			return B_ERROR;
		}
		// so next time they won't get the same output again
		*cookie = 1;
	}
	if (out_output == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*out_output = output;
	return B_OK;
}

status_t MediaFileProducer::DisposeOutputCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaFileProducer::DisposeOutputCookie\n");
	// nothing to do since our cookies are just integers
	return B_OK;
}

status_t MediaFileProducer::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	fprintf(stderr,"MediaFileProducer::SetBufferGroup\n");
	if (output.source != for_source) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE; // we only have one output so that better be it
	}
	if (bufferGroup != 0) {
		if (bufferGroup == group) {
			return B_OK; // time saver
		}	
		delete bufferGroup;
	}
	if (group != 0) {
		bufferGroup = group;
	} else {
		// let's take advantage of this opportunity to recalculate
		// our downstream latency and ensure that it is up to date
		media_node_id id;
		FindLatencyFor(output.destination, &downstreamLatency, &id);
		// buffer period gets initialized in Connect() because
		// that is the first time we get the real values for
		// chunk size and bit rate, which are used to compute buffer period
		// note: you can still make a buffer group before connecting (why?)
		//       but we don't make it, you make it yourself and pass it here.
		//       not sure why anybody would want to do that since they need
		//       a connection anyway...
		if (bufferPeriod <= 0) {
			fprintf(stderr,"<- B_NO_INIT");
			return B_NO_INIT;
		}
		int32 count = int32(downstreamLatency/bufferPeriod)+2;
		// allocate the buffers
		bufferGroup = new BBufferGroup(output.format.u.multistream.max_chunk_size,count);
		if (bufferGroup == 0) {
			fprintf(stderr,"<- B_NO_MEMORY\n");
			return B_NO_MEMORY;
		}
		status_t status = bufferGroup->InitCheck();
		if (status != B_OK) {
			fprintf(stderr,"<- bufferGroup initialization failed\n");
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
status_t MediaFileProducer::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	return BBufferProducer::VideoClippingChanged(for_source,num_shorts,clip_data,display,_deprecated_);
}

status_t MediaFileProducer::GetLatency(
				bigtime_t * out_latency)
{
	fprintf(stderr,"MediaFileProducer::GetLatency\n");
	if (out_latency == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t MediaFileProducer::PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	fprintf(stderr,"MediaFileProducer::PrepareToConnect\n");
	if ((format == 0) || (out_source == 0) || (out_name == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashes...
	}
	if (output.destination != media_destination::null) {
		fprintf(stderr,"<- B_MEDIA_ALREADY_CONNECTED\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}	
	status_t status = FormatChangeRequested(output.source,where,format,0);
	if (status != B_OK) {
		fprintf(stderr,"<- MediaFileProducer::FormatChangeRequested failed\n");
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
	return ResolveWildcards(&format->u.multistream);
}

void MediaFileProducer::Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name)
{
	fprintf(stderr,"MediaFileProducer::Connect\n");
	if (error != B_OK) {
		fprintf(stderr,"<- error already\n");
		output.destination = media_destination::null;
		ResetFormat(&output.format);
		return;
	}
	
	// record the agreed upon values
	output.destination = destination;
	output.format = format;
	strncpy(io_name,output.name,B_MEDIA_NAME_LENGTH-1);
	io_name[B_MEDIA_NAME_LENGTH] = '\0';

	// determine our downstream latency
	media_node_id id;
	FindLatencyFor(output.destination, &downstreamLatency, &id);

	// compute the buffer period (must be done before setbuffergroup)
	bufferPeriod = int32(8000000 / 1024
	                     * output.format.u.multistream.max_chunk_size
			             / output.format.u.multistream.max_bit_rate);

	// setup the buffers if they aren't setup yet
	if (bufferGroup == 0) {
		status_t status = SetBufferGroup(output.source,0);
		if (status != B_OK) {
			fprintf(stderr,"<- SetBufferGroup failed\n");
			return;
		}
	}
	SetBufferDuration(bufferPeriod);
	
	if (inputFile != 0) {
		bigtime_t start, end;
		uint8 * data = new uint8[output.format.u.multistream.max_chunk_size]; // <- buffer group buffer size
		BBuffer * buffer = 0;
		ssize_t bytesRead = 0;
		{ // timed section
			start = TimeSource()->RealTime();
			// first we try to use a real BBuffer
			buffer = bufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,bufferPeriod);
			if (buffer != 0) {
				FillFileBuffer(buffer);
			} else {
				// didn't get a real BBuffer, try simulation by just a read from the disk
				bytesRead = inputFile->Read(data,output.format.u.multistream.max_chunk_size);
			}
			end = TimeSource()->RealTime();
		}
		bytesRead = buffer->SizeUsed();
		delete data;
		if (buffer != 0) {
			buffer->Recycle();
		}
		inputFile->Seek(-bytesRead,SEEK_CUR); // put it back where we found it
	
		internalLatency = end - start;
		
		fprintf(stderr,"internal latency from disk read = %i\n",internalLatency);
	} else {
		internalLatency = 100; // just guess
		fprintf(stderr,"internal latency guessed = %i\n",internalLatency);
	}
	
	SetEventLatency(downstreamLatency + internalLatency);
	
	// XXX: do anything else?
}

void MediaFileProducer::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	fprintf(stderr,"MediaFileProducer::Disconnect\n");
	if ((where == output.destination) && (what == output.source)) {
		output.destination = media_destination::null;
		ResetFormat(&output.format);
		BBufferGroup * group = bufferGroup;
		bufferGroup = 0;
		delete group;
	}
}

void MediaFileProducer::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaFileProducer::LateNoticeReceived\n");
	if (what == output.source) {
		switch (RunMode()) {
			case B_OFFLINE:
			    // nothing to do
				break;
			case B_RECORDING:
			    // nothing to do
				break;
			case B_INCREASE_LATENCY:
				internalLatency += how_much;
				SetEventLatency(downstreamLatency + internalLatency);
				break;
			case B_DECREASE_PRECISION:
				// What you want us to give you fewer bits or something?
				break;
			case B_DROP_DATA:
				// Okay you asked for it, we'll skip ahead in the file!
				// We'll drop 1 buffer's worth
				if (inputFile == 0) {
					fprintf(stderr,"MediaFileProducer::LateNoticeReceived called without an inputFile (!)\n");
				} else {
					inputFile->Seek(output.format.u.multistream.max_chunk_size,SEEK_CUR);
				}
				break;
			default:
				// huh?? there aren't any more run modes.
				fprintf(stderr,"MediaFileProducer::LateNoticeReceived with unexpected run mode.\n");
				break;
		}
	}
}

void MediaFileProducer::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaFileProducer::EnableOutput\n");
	if (what == output.source) {
		outputEnabled = enabled;
	}
}

status_t MediaFileProducer::SetPlayRate(
				int32 numer,
				int32 denom)
{
	BBufferProducer::SetPlayRate(numer,denom); // XXX: do something intelligent later
}

void MediaFileProducer::AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	fprintf(stderr,"MediaFileProducer::AdditionalBufferRequested\n");
	if (output.source == source) {
		BBuffer * buffer;
		status_t status = GetFilledBuffer(&buffer);
		if (status != B_OK) {
			fprintf(stderr,"MediaFileProducer::AdditionalBufferRequested got an error from GetFilledBuffer.\n");
			return; // don't send the buffer
		}
		SendBuffer(buffer,output.destination);
	}
}

void MediaFileProducer::LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags)
{
	fprintf(stderr,"MediaFileProducer::LatencyChanged\n");
	if ((output.source == source) && (output.destination == destination)) {
		downstreamLatency = new_latency;
		SetEventLatency(downstreamLatency + internalLatency);
	}
}

// -------------------------------------------------------- //
// implementation for BControllable
// -------------------------------------------------------- //

const int32 MediaFileProducer::DEFAULT_CHUNK_SIZE_PARAM = 1;
const int32 MediaFileProducer::DEFAULT_BIT_RATE_PARAM = 2;
const int32 MediaFileProducer::DEFAULT_BUFFER_PERIOD_PARAM = 3;

status_t MediaFileProducer::GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize)
{
	fprintf(stderr,"MediaFileProducer::GetParameterValue\n");
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
			fprintf(stderr,"MediaFileProducer::GetParameterValue unknown id (%i)\n",id);
			return B_ERROR;
	}
	return B_OK;			
}
				
void MediaFileProducer::SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size)
{
	fprintf(stderr,"MediaFileProducer::SetParameterValue\n");
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
			fprintf(stderr,"MediaFileProducer::SetParameterValue unknown id (%i)\n",id);
			break;
	}
}			

// the default implementation should call the add-on main()
status_t MediaFileProducer::StartControlPanel(
				BMessenger * out_messenger)
{
	BControllable::StartControlPanel(out_messenger);
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void MediaFileProducer::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaFileProducer::HandleEvent(");
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
			if (inputFile != 0) {
				inputFile->Seek(event->bigdata,SEEK_SET);
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
			 	   || (output.destination == media_destination::null)) {
				break;
			}
			HandleBuffer(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			fprintf(stderr,"B_DATA_STATUS)\n");
			SendDataStatus(event->data,output.destination,event->event_time);
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
void MediaFileProducer::CleanUpEvent(
				const media_timed_event *event)
{
	return BMediaEventLooper::CleanUpEvent(event);
}
		
/* called from Offline mode to determine the current time of the node */
/* update your internal information whenever it changes */
bigtime_t MediaFileProducer::OfflineTime()
{
	fprintf(stderr,"MediaFileProducer::OfflineTime\n");
	if (inputFile == 0) {
		return 0;
	} else {
		return inputFile->Position();
	}
}

/* override only if you know what you are doing! */
/* otherwise much badness could occur */
/* the actual control loop function: */
/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
void MediaFileProducer::ControlLoop() {
	BMediaEventLooper::ControlLoop();
}

// protected:

status_t MediaFileProducer::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaFileProducer::HandleBuffer\n");
	status_t status = B_OK;
	BBuffer * buffer = bufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,bufferPeriod);
	if (buffer != 0) {
	    status = FillFileBuffer(buffer);
	    if (status != B_OK) {
			fprintf(stderr,"MediaFileProducer::HandleEvent got an error from FillFileBuffer.\n");
			buffer->Recycle();
		} else {
			if (outputEnabled) {
				status = SendBuffer(buffer,output.destination);
			}
			if (status != B_OK) {
				fprintf(stderr,"MediaFileProducer::HandleEvent got an error from SendBuffer.\n");
				buffer->Recycle();
			}
		}
	}
	bigtime_t nextEventTime = event->event_time+bufferPeriod;
	media_timed_event nextBufferEvent(nextEventTime, BTimedEventQueue::B_HANDLE_BUFFER);
	EventQueue()->AddEvent(nextBufferEvent);
	return status;
}

status_t MediaFileProducer::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaFileProducer::HandleParameter\n");
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
			fprintf(stderr,"MediaFileProducer::HandleParameter called with unknown param id (%i)\n",param);
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
// MediaFileProducer specific functions
// -------------------------------------------------------- //

status_t MediaFileProducer::GetFlavor(
				int32 id,
				const flavor_info ** out_info)
{
	fprintf(stderr,"MediaFileProducer::GetFlavor\n");
	static flavor_info flavorInfo;
	flavorInfo.name = "MediaFileProducer";
	flavorInfo.info =
	     "A MediaFileProducer node reads a file and produces a multistream.\n";
	flavorInfo.kinds = B_FILE_INTERFACE | B_BUFFER_PRODUCER | B_CONTROLLABLE;
	flavorInfo.flavor_flags = B_FLAVOR_IS_LOCAL;
	flavorInfo.possible_count = INT_MAX;
	
	flavorInfo.in_format_count = 0; // no inputs
	flavorInfo.in_formats = 0;

	flavorInfo.out_format_count = 1; // 1 output
	flavorInfo.out_formats = &GetFormat();

	flavorInfo.internal_id = id;
		
	*out_info = &flavorInfo;
	return B_OK;
}

media_format & MediaFileProducer::GetFormat()
{
	fprintf(stderr,"MediaFileProducer::GetFormat\n");
	static media_format format;
	format.type = B_MEDIA_MULTISTREAM;
	format.require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
	format.u.multistream = media_multistream_format::wildcard;
	return format;
}

media_file_format & MediaFileProducer::GetFileFormat()
{
	fprintf(stderr,"MediaFileProducer::GetFileFormat\n");
	static media_file_format file_format;
	file_format.capabilities =
			    media_file_format::B_READABLE
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

status_t MediaFileProducer::ResetFormat(media_format * format)
{
	fprintf(stderr,"MediaFileProducer::ResetFormat\n");
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
status_t MediaFileProducer::ResolveWildcards(
				media_multistream_format * multistream_format)
{
	fprintf(stderr,"MediaFileProducer::ResolveWildcards\n");
	if (strcmp("video/x-msvideo",input_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_AVI;
		}
	} else
	if (strcmp("video/mpeg",input_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_MPEG1;
		}
	} else
	if (strcmp("video/quicktime",input_mime_type) == 0) {
		if (multistream_format->format == media_multistream_format::wildcard.format) {
			multistream_format->format = media_multistream_format::B_QUICKTIME;
		}
	} else
	if (strcmp("audio/x-mpeg",input_mime_type) == 0) {
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

status_t MediaFileProducer::GetFilledBuffer(
				BBuffer ** outBuffer)
{
	fprintf(stderr,"MediaFileProducer::GetFilledBuffer\n");
	BBuffer * buffer = bufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,-1);
	if (buffer == 0) {
		// XXX: add a new buffer and get it
		fprintf(stderr,"MediaFileProducer::GetFilledBuffer needs a new buffer.\n");
		return B_ERROR; // don't send the buffer
	}
	status_t status = FillFileBuffer(buffer);
	*outBuffer = buffer;
	return status;
}		

status_t MediaFileProducer::FillFileBuffer(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaFileProducer::FillFileBuffer\n");
	if (inputFile == 0) {
		fprintf(stderr,"<- B_NO_INIT\n");
		return B_NO_INIT;
	}	
	off_t position = inputFile->Position();
	ssize_t bytesRead = inputFile->Read(buffer->Data(),buffer->SizeAvailable());
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

status_t MediaFileProducer::_Reserved_MediaFileProducer_0(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_1(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_2(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_3(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_4(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_5(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_6(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_7(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_8(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_9(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_10(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_11(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_12(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_13(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_14(void *) {}
status_t MediaFileProducer::_Reserved_MediaFileProducer_15(void *) {}
