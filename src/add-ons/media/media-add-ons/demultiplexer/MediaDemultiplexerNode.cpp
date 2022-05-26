// MediaDemultiplexerNode.cpp
//
// Andrew Bachmann, 2002
//
// The MediaDemultiplexerNode class
// takes a multistream input and supplies
// the individual constituent streams as
// the output.

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <MediaEventLooper.h>
#include <Errors.h>
#include <BufferGroup.h>
#include <TimeSource.h>
#include <Buffer.h>
#include <limits.h>

#include "MediaDemultiplexerNode.h"
#include "misc.h"

#include <stdio.h>
#include <string.h>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MediaDemultiplexerNode::~MediaDemultiplexerNode(void)
{
	fprintf(stderr,"MediaDemultiplexerNode::~MediaDemultiplexerNode\n");
	// Stop the BMediaEventLooper thread
	Quit();
}

MediaDemultiplexerNode::MediaDemultiplexerNode(
				const flavor_info * info = 0,
				BMessage * config = 0,
				BMediaAddOn * addOn = 0)
	: BMediaNode("MediaDemultiplexerNode"),
	  BMediaEventLooper(),
  	  BBufferConsumer(B_MEDIA_MULTISTREAM),
  	  BBufferProducer(B_MEDIA_UNKNOWN_TYPE) // no B_MEDIA_ANY
{
	fprintf(stderr,"MediaDemultiplexerNode::MediaDemultiplexerNode\n");
	// keep our creator around for AddOn calls later
	fAddOn = addOn;
	// null out our latency estimates
	fDownstreamLatency = 0;
	fInternalLatency = 0;	
	// don't overwrite available space, and be sure to terminate
	strncpy(input.name,"Demultiplexer Input",B_MEDIA_NAME_LENGTH-1);
	input.name[B_MEDIA_NAME_LENGTH-1] = '\0';
	// initialize the input
	input.node = media_node::null;               // until registration
	input.source = media_source::null; 
	input.destination = media_destination::null; // until registration
	GetInputFormat(&input.format);
	
	outputs.empty();
	// outputs initialized after we connect,
	// find a suitable extractor,
	// and it tells us the ouputs
	
	fInitCheckStatus = B_OK;
}

status_t MediaDemultiplexerNode::InitCheck(void) const
{
	fprintf(stderr,"MediaDemultiplexerNode::InitCheck\n");
	return fInitCheckStatus;
}

status_t MediaDemultiplexerNode::GetConfigurationFor(
				BMessage * into_message)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetConfigurationFor\n");
	return B_OK;
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * MediaDemultiplexerNode::AddOn(
				int32 * internal_id) const
{
	fprintf(stderr,"MediaDemultiplexerNode::AddOn\n");
	// BeBook says this only gets called if we were in an add-on.
	if (fAddOn != 0) {
		// If we get a null pointer then we just won't write.
		if (internal_id != 0) {
			internal_id = 0;
		}
	}
	return fAddOn;
}

void MediaDemultiplexerNode::Start(
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaDemultiplexerNode::Start(pt=%lld)\n",performance_time);
	BMediaEventLooper::Start(performance_time);
}

void MediaDemultiplexerNode::Stop(
				bigtime_t performance_time,
				bool immediate)
{
	if (immediate) {
		fprintf(stderr,"MediaDemultiplexerNode::Stop(pt=%lld,<immediate>)\n",performance_time);
	} else {
		fprintf(stderr,"MediaDemultiplexerNode::Stop(pt=%lld,<scheduled>)\n",performance_time);
	}
	BMediaEventLooper::Stop(performance_time,immediate);
}

void MediaDemultiplexerNode::Seek(
				bigtime_t media_time,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaDemultiplexerNode::Seek(mt=%lld,pt=%lld)\n",media_time,performance_time);
	BMediaEventLooper::Seek(media_time,performance_time);
}

void MediaDemultiplexerNode::SetRunMode(
				run_mode mode)
{
	fprintf(stderr,"MediaDemultiplexerNode::SetRunMode(%i)\n",mode);
	BMediaEventLooper::SetRunMode(mode);
}

void MediaDemultiplexerNode::TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time)
{
	fprintf(stderr,"MediaDemultiplexerNode::TimeWarp(rt=%lld,pt=%lld)\n",at_real_time,to_performance_time);
	BMediaEventLooper::TimeWarp(at_real_time,to_performance_time);
}

void MediaDemultiplexerNode::Preroll(void)
{
	fprintf(stderr,"MediaDemultiplexerNode::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

void MediaDemultiplexerNode::SetTimeSource(
				BTimeSource * time_source)
{
	fprintf(stderr,"MediaDemultiplexerNode::SetTimeSource\n");
	BMediaNode::SetTimeSource(time_source);
}

status_t MediaDemultiplexerNode::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// no special messages for now
		default:
			status = BBufferConsumer::HandleMessage(message,data,size);
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

status_t MediaDemultiplexerNode::RequestCompleted(
				const media_request_info & info)
{
	fprintf(stderr,"MediaDemultiplexerNode::RequestCompleted\n");
	return BMediaNode::RequestCompleted(info);
}

status_t MediaDemultiplexerNode::DeleteHook(
				BMediaNode * node)
{
	fprintf(stderr,"MediaDemultiplexerNode::DeleteHook\n");
	return BMediaEventLooper::DeleteHook(node);
}

void MediaDemultiplexerNode::NodeRegistered(void)
{
	fprintf(stderr,"MediaDemultiplexerNode::NodeRegistered\n");

	// now we can do this
	input.node = Node();
	input.destination.id = 0;
	input.destination.port = input.node.port; // same as ControlPort()

	// outputs initialized after we connect,
	// find a suitable extractor,
	// and it tells us the ouputs
	
	// start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
}

status_t MediaDemultiplexerNode::GetNodeAttributes(
				media_node_attribute * outAttributes,
				size_t inMaxCount)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetNodeAttributes\n");
	return BMediaNode::GetNodeAttributes(outAttributes,inMaxCount);
}

status_t MediaDemultiplexerNode::AddTimer(
					bigtime_t at_performance_time,
					int32 cookie)
{
	fprintf(stderr,"MediaDemultiplexerNode::AddTimer\n");
	return BMediaEventLooper::AddTimer(at_performance_time,cookie);
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t MediaDemultiplexerNode::AcceptFormat(
				const media_destination & dest,
				media_format * format)
{
	fprintf(stderr,"MediaDemultiplexerNode::AcceptFormat\n");
	if (input.destination != dest) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION");
		return B_MEDIA_BAD_DESTINATION; // we only have one input so that better be it
	}
	media_format myFormat;
	GetInputFormat(&myFormat);
	// Be's format_is_compatible doesn't work,
	// so use our format_is_acceptible instead
	if (!format_is_acceptible(*format,myFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	AddRequirements(format);
	return B_OK;	
}

status_t MediaDemultiplexerNode::GetNextInput(
				int32 * cookie,
				media_input * out_input)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetNextInput\n");
	if (*cookie != 0) {
		fprintf(stderr,"<- B_ERROR (no more inputs)\n");
		return B_ERROR;
	}

	// so next time they won't get the same input again
	*cookie = 1;
	*out_input = input;
	return B_OK;
}

void MediaDemultiplexerNode::DisposeInputCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaDemultiplexerNode::DisposeInputCookie\n");
	// nothing to do since our cookies are just integers
	return; // B_OK;
}

void MediaDemultiplexerNode::BufferReceived(
				BBuffer * buffer)
{
	fprintf(stderr,"MediaDemultiplexerNode::BufferReceived\n");
	switch (buffer->Header()->type) {
//		case B_MEDIA_PARAMETERS:
//			{
//			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
//			if (status != B_OK) {
//				fprintf(stderr,"ApplyParameterData in MediaDemultiplexerNode::BufferReceived failed\n");
//			}			
//			buffer->Recycle();
//			}
//			break;
		case B_MEDIA_MULTISTREAM:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in MediaDemultiplexerNode::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in MediaDemultiplexerNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in MediaDemultiplexerNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void MediaDemultiplexerNode::ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	fprintf(stderr,"MediaDemultiplexerNode::ProducerDataStatus\n");
	if (input.destination != for_whom) {
		fprintf(stderr,"invalid destination received in MediaDemultiplexerNode::ProducerDataStatus\n");
		return;
	}
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&input, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t MediaDemultiplexerNode::GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetLatencyFor\n");
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

status_t MediaDemultiplexerNode::Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input)
{
	fprintf(stderr,"MediaDemultiplexerNode::Connected\n");
	if (input.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	// find an appropriate extractor to handle this type
	fprintf(stderr,"  XXX: no extractors yet\n");

	// initialize the outputs here
	// provide all the types that the extractor claims
	outputs.empty();

	// compute the latency or just guess
	fInternalLatency = 500; // just guess
	fprintf(stderr,"  internal latency guessed = %lld\n",fInternalLatency);
	
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	input.source = producer;
	input.format = with_format;
	*out_input = input;
	return B_OK;
}

void MediaDemultiplexerNode::Disconnected(
				const media_source & producer,
				const media_destination & where)
{
	fprintf(stderr,"MediaDemultiplexerNode::Disconnected\n");
	if (input.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (input.source != producer) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	input.source = media_source::null;
	GetInputFormat(&input.format);
	
	outputs.empty();
}

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
status_t MediaDemultiplexerNode::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format)
{
	fprintf(stderr,"MediaDemultiplexerNode::FormatChanged\n");
	if (input.source != producer) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (input.destination != consumer) {
		return B_MEDIA_BAD_DESTINATION;
	}
	// XXX: implement
	fprintf(stderr,"  This is because we asked to have the format changed.\n"
	               "  Therefore we must switch to the other extractor that\n"
	               "  we presumably have ready.");
	input.format = format;
	return B_OK;
}

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
status_t MediaDemultiplexerNode::SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags)
{
	fprintf(stderr,"MediaDemultiplexerNode::SeekTagRequested\n");
	// XXX: implement this
	return BBufferConsumer::SeekTagRequested(destination,in_target_time,in_flags,
											out_seek_tag,out_tagged_time,out_flags);
}

// -------------------------------------------------------- //
// implemention of BBufferProducer
// -------------------------------------------------------- //

// They are asking us to make the first offering.
// So, we get a fresh format and then add requirements
status_t MediaDemultiplexerNode::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	fprintf(stderr,"MediaDemultiplexerNode::FormatSuggestionRequested\n");
	// XXX: how do I pick which stream to supply here?....
	//      answer?: get the first compatible stream that is available
	fprintf(stderr,"  format suggestion requested not implemented\n");
//	if ((type != B_MEDIA_MULTISTREAM) && (type != B_MEDIA_UNKNOWN_TYPE)) {
//		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
//		return B_MEDIA_BAD_FORMAT;
//	}
	GetOutputFormat(format);
//	AddRequirements(format);

	return B_OK;
}

// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
status_t MediaDemultiplexerNode::FormatProposal(
				const media_source & output_source,
				media_format * format)
{
	fprintf(stderr,"MediaDemultiplexerNode::FormatProposal\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == output_source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}
	return itr->FormatProposal(format);
}

// Presumably we have already agreed with them that this format is
// okay.  But just in case, we check the offer. (and complain if it
// is invalid)  Then as the last thing we do, we get rid of any
// remaining wilcards.
status_t MediaDemultiplexerNode::FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaDemultiplexerNode::FormatChangeRequested\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}
	return itr->FormatChangeRequested(destination,io_format);
}

status_t MediaDemultiplexerNode::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetNextOutput\n");
	// they want a clean start
	if (*cookie == 0) {
		*cookie = (int32)outputs.begin();
	}
	vector<MediaOutputInfo>::iterator itr
		= (vector<MediaOutputInfo>::iterator)(*cookie);
	// XXX: check here if the vector has been modified.
	//      if the iterator is invalid, return an error code??
	
	// they already got our 1 output
	if (itr == outputs.end()) {
		fprintf(stderr,"<- B_ERROR (no more outputs)\n");
		return B_ERROR;
	}
	// return this output
	*out_output = itr->output;
	// so next time they won't get the same output again
	*cookie = (int32)++itr;
	return B_OK;
}

status_t MediaDemultiplexerNode::DisposeOutputCookie(
				int32 cookie)
{
	fprintf(stderr,"MediaDemultiplexerNode::DisposeOutputCookie\n");
	// nothing to do since our cookies are part of the vector iterator
	return B_OK;
}

status_t MediaDemultiplexerNode::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	fprintf(stderr,"MediaDemultiplexerNode::SetBufferGroup\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == for_source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}
	return itr->SetBufferGroup(group);
}

	/* Format of clipping is (as int16-s): <from line> <npairs> <startclip> <endclip>. */
	/* Repeat for each line where the clipping is different from the previous line. */
	/* If <npairs> is negative, use the data from line -<npairs> (there are 0 pairs after */
	/* a negative <npairs>. Yes, we only support 32k*32k frame buffers for clipping. */
	/* Any non-0 field of 'display' means that that field changed, and if you don't support */
	/* that change, you should return an error and ignore the request. Note that the buffer */
	/* offset values do not have wildcards; 0 (or -1, or whatever) are real values and must */
	/* be adhered to. */
status_t MediaDemultiplexerNode::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	return BBufferProducer::VideoClippingChanged(for_source,num_shorts,clip_data,display,_deprecated_);
}

status_t MediaDemultiplexerNode::GetLatency(
				bigtime_t * out_latency)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetLatency\n");
	if (out_latency == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t MediaDemultiplexerNode::PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	fprintf(stderr,"MediaDemultiplexerNode::PrepareToConnect\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == what) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}
	return itr->PrepareToConnect(where,format,out_source,out_name);
}

void MediaDemultiplexerNode::Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name)
{
	fprintf(stderr,"MediaDemultiplexerNode::Connect\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	if (error != B_OK) {
		fprintf(stderr,"<- error already\n");
		itr->output.destination = media_destination::null;
		itr->output.format = itr->generalFormat;
		return;
	}

	// calculate the downstream latency
	// must happen before itr->Connect
	bigtime_t downstreamLatency;
	media_node_id id;
	FindLatencyFor(itr->output.destination, &downstreamLatency, &id);

	// record the agreed upon values
	status_t status;
	status = itr->Connect(destination,format,io_name,downstreamLatency);
	if (status != B_OK) {
		fprintf(stderr,"  itr->Connect returned an error\n");
		return;
	}

	// compute the internal latency
	// must happen after itr->Connect
	if (fInternalLatency == 0) {
		fInternalLatency = 100; // temporary until we finish computing it
		ComputeInternalLatency();
	}

	// If the downstream latency for this output is larger
	// than our current downstream latency, we have to increase
	// our current downstream latency to be the larger value.
	if (downstreamLatency > fDownstreamLatency) {
		SetEventLatency(fDownstreamLatency + fInternalLatency);
	}

	// XXX: what do I set the buffer duration to?
	//      it depends on which output is sending!!
	// SetBufferDuration(bufferPeriod);
	
	// XXX: do anything else?
	return;
}

void MediaDemultiplexerNode::ComputeInternalLatency() {
	fprintf(stderr,"MediaDemultiplexerNode::ComputeInternalLatency\n");
//	if (GetCurrentFile() != 0) {
//		bigtime_t start, end;
//		uint8 * data = new uint8[output.format.u.multistream.max_chunk_size]; // <- buffer group buffer size
//		BBuffer * buffer = 0;
//		ssize_t bytesRead = 0;
//		{ // timed section
//			start = TimeSource()->RealTime();
//			// first we try to use a real BBuffer
//			buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,fBufferPeriod);
//			if (buffer != 0) {
//				FillFileBuffer(buffer);
//			} else {
//				// didn't get a real BBuffer, try simulation by just a read from the disk
//				bytesRead = GetCurrentFile()->Read(data,output.format.u.multistream.max_chunk_size);
//			}
//			end = TimeSource()->RealTime();
//		}
//		bytesRead = buffer->SizeUsed();
//		delete data;
//		if (buffer != 0) {
//			buffer->Recycle();
//		}
//		GetCurrentFile()->Seek(-bytesRead,SEEK_CUR); // put it back where we found it
//	
//		fInternalLatency = end - start;
//		
//		fprintf(stderr,"  internal latency from disk read = %lld\n",fInternalLatency);
//	} else {
		fInternalLatency = 100; // just guess
		fprintf(stderr,"  internal latency guessed = %lld\n",fInternalLatency);
//	}
}

void MediaDemultiplexerNode::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	fprintf(stderr,"MediaDemultiplexerNode::Disconnect\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == what) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	if (itr->output.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	// if this output has an equal (or higher!) latency than
	// our current believed downstream latency, we may have to
	// update our downstream latency.
	bool updateDownstreamLatency = (itr->downstreamLatency >= fDownstreamLatency);
	// disconnect this output
	itr->Disconnect();
	// update the downstream latency if necessary
	if (updateDownstreamLatency) {
		bigtime_t newDownstreamLatency = 0;
		for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
			if (itr->downstreamLatency > newDownstreamLatency) {
				newDownstreamLatency = itr->downstreamLatency;
			}
		}
		fDownstreamLatency = newDownstreamLatency;
	}
}

void MediaDemultiplexerNode::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
	fprintf(stderr,"MediaDemultiplexerNode::LateNoticeReceived\n");
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == what) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
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
			// XXX: try to catch up by producing buffers faster
			break;
		case B_DROP_DATA:
			// XXX: should we really drop buffers?  just for that output?
			break;
		default:
			// huh?? there aren't any more run modes.
			fprintf(stderr,"MediaDemultiplexerNode::LateNoticeReceived with unexpected run mode.\n");
			break;
	}
}

void MediaDemultiplexerNode::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
	fprintf(stderr,"MediaDemultiplexerNode::EnableOutput\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == what) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	status_t status = itr->EnableOutput(enabled);
	if (status != B_OK) {
		fprintf(stderr,"  error in itr->EnableOutput\n");
		return;
	}
	return;
}

status_t MediaDemultiplexerNode::SetPlayRate(
				int32 numer,
				int32 denom)
{
	BBufferProducer::SetPlayRate(numer,denom); // XXX: do something intelligent later
}

void MediaDemultiplexerNode::AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	fprintf(stderr,"MediaDemultiplexerNode::AdditionalBufferRequested\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	BBuffer * buffer;
	status_t status = itr->AdditionalBufferRequested(prev_buffer,prev_time,prev_tag);
	if (status != B_OK) {
		fprintf(stderr,"  itr->AdditionalBufferRequested returned an error.\n");
		return;
	}
	return;
}

void MediaDemultiplexerNode::LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags)
{
	fprintf(stderr,"MediaDemultiplexerNode::LatencyChanged\n");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		if (itr->output.source == source) {
			break;
		}
	}
	if (itr == outputs.end()) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	if (itr->output.destination != destination) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	fDownstreamLatency = new_latency;
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	// XXX: we may have to recompute the number of buffers that we are using
	// see SetBufferGroup
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void MediaDemultiplexerNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleEvent\n");
	switch (event->type) {
		case BTimedEventQueue::B_START:
			HandleStart(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_SEEK:
			HandleSeek(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_WARP:
			HandleWarp(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			if (RunState() == BMediaEventLooper::B_STARTED) {
				HandleBuffer(event,lateness,realTimeEvent);
			}
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			HandleDataStatus(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_PARAMETER:
			HandleParameter(event,lateness,realTimeEvent);
			break;
		default:
			fprintf(stderr,"  unknown event type: %i\n",event->type);
			break;
	}
}

/* override to clean up custom events you have added to your queue */
void MediaDemultiplexerNode::CleanUpEvent(
				const media_timed_event *event)
{
	BMediaEventLooper::CleanUpEvent(event);
}
		
/* called from Offline mode to determine the current time of the node */
/* update your internal information whenever it changes */
bigtime_t MediaDemultiplexerNode::OfflineTime()
{
	fprintf(stderr,"MediaDemultiplexerNode::OfflineTime\n");
	return BMediaEventLooper::OfflineTime();
// XXX: do something else?
}

/* override only if you know what you are doing! */
/* otherwise much badness could occur */
/* the actual control loop function: */
/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
void MediaDemultiplexerNode::ControlLoop() {
	BMediaEventLooper::ControlLoop();
}

// protected:

status_t MediaDemultiplexerNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleStart()\n");
	if (RunState() != B_STARTED) {
// XXX: Either use the following line or the lines that are not commented.
// There doesn't seem to be a practical difference that i can tell.
//		HandleBuffer(event,lateness,realTimeEvent);
		media_timed_event firstBufferEvent(event->event_time, BTimedEventQueue::B_HANDLE_BUFFER);
		HandleEvent(&firstBufferEvent, 0, false);
		EventQueue()->AddEvent(firstBufferEvent);
	}
	return B_OK;
}

status_t MediaDemultiplexerNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleSeek(t=%lld,d=%i,bd=%lld)\n",event->event_time,event->data,event->bigdata);
	return B_OK;
}
						
status_t MediaDemultiplexerNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleWarp\n");
	return B_OK;
}

status_t MediaDemultiplexerNode::HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleStop\n");
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
	return B_OK;
}

status_t MediaDemultiplexerNode::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleBuffer\n");
	BBuffer * buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	if (buffer->Header()->destination != input.destination.id) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (outputs.begin() == outputs.end()) {
		fprintf(stderr,"<- B_MEDIA_NOT_CONNECTED\n");
		return B_MEDIA_NOT_CONNECTED;
	}
	status_t status = B_OK;
	fprintf(stderr,"  XXX: HandleBuffer not yet implemented.\n");
	// we have to hand the buffer to the extractor
	// and then whenever we get a buffer for an output send it
	// to that particular output (assuming it exists and is enabled)
//	BBuffer * buffer = fBufferGroup->RequestBuffer(output.format.u.multistream.max_chunk_size,fBufferPeriod);
//	if (buffer != 0) {
//	    status = FillFileBuffer(buffer);
//	    if (status != B_OK) {
//			fprintf(stderr,"MediaDemultiplexerNode::HandleEvent got an error from FillFileBuffer.\n");
//			buffer->Recycle();
//		} else {
//			if (fOutputEnabled) {
//				status = SendBuffer(buffer,output.destination);
//				if (status != B_OK) {
//					fprintf(stderr,"MediaDemultiplexerNode::HandleEvent got an error from SendBuffer.\n");
//					buffer->Recycle();
//				}
//			} else {
				buffer->Recycle();
//			}
//		}
//	}
	bigtime_t nextEventTime = event->event_time+10000; // fBufferPeriod; // XXX : should multiply
	media_timed_event nextBufferEvent(nextEventTime, BTimedEventQueue::B_HANDLE_BUFFER);
	EventQueue()->AddEvent(nextBufferEvent);
	return status;
}

status_t MediaDemultiplexerNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleDataStatus");
	// find the information for this output
	vector<MediaOutputInfo>::iterator itr;
	for(itr = outputs.begin() ; (itr != outputs.end()) ; itr++) {
		SendDataStatus(event->data,itr->output.destination,event->event_time);
	}
	return B_OK;
}

status_t MediaDemultiplexerNode::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"MediaDemultiplexerNode::HandleParameter");
	return B_OK;
}

// -------------------------------------------------------- //
// MediaDemultiplexerNode specific functions
// -------------------------------------------------------- //

// public:

void MediaDemultiplexerNode::GetFlavor(flavor_info * outInfo, int32 id)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetFlavor\n");
	if (outInfo == 0) {
		return;
	}
	outInfo->name = "Haiku Demultiplexer";
	outInfo->info = "A MediaDemultiplexerNode node demultiplexes a multistream into its constituent streams.";
	outInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER;
	outInfo->flavor_flags = B_FLAVOR_IS_LOCAL;
	outInfo->possible_count = INT_MAX;
	outInfo->in_format_count = 1; // 1 input
	media_format * inFormats = new media_format[outInfo->in_format_count];
	GetInputFormat(&inFormats[0]);
	outInfo->in_formats = inFormats;
	outInfo->out_format_count = 1; // 1 output
	media_format * outFormats = new media_format[outInfo->out_format_count];
	GetOutputFormat(&outFormats[0]);
	outInfo->out_formats = outFormats;
	outInfo->internal_id = id;
	return;
}

void MediaDemultiplexerNode::GetInputFormat(media_format * outFormat)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetInputFormat\n");
	if (outFormat == 0) {
		return;
	}
	outFormat->type = B_MEDIA_MULTISTREAM;
	outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
	outFormat->u.multistream = media_multistream_format::wildcard;
}

void MediaDemultiplexerNode::GetOutputFormat(media_format * outFormat)
{
	fprintf(stderr,"MediaDemultiplexerNode::GetOutputFormat\n");
	if (outFormat == 0) {
		return;
	}
	outFormat->type = B_MEDIA_UNKNOWN_TYPE; // more like ANY_TYPE than unknown
	outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
}

// protected:

status_t MediaDemultiplexerNode::AddRequirements(media_format * format)
{
	fprintf(stderr,"MediaDemultiplexerNode::AddRequirements\n");
	return B_OK;
}

// -------------------------------------------------------- //
// stuffing
// -------------------------------------------------------- //

status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_0(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_1(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_2(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_3(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_4(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_5(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_6(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_7(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_8(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_9(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_10(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_11(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_12(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_13(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_14(void *) {}
status_t MediaDemultiplexerNode::_Reserved_MediaDemultiplexerNode_15(void *) {}
