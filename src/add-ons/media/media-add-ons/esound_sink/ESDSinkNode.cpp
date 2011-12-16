/*
 * ESounD media addon for BeOS
 *
 * Copyright (c) 2006 François Revol (revol@free.fr)
 * 
 * Based on Multi Audio addon for Haiku,
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
//#define DEBUG 4
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
#include <MediaDefs.h>
#include <Message.h>

#include "ESDSinkNode.h"
#include "ESDEndpoint.h"
#ifdef DEBUG
  #define PRINTING
#endif
#include "debug.h"
#include <Debug.h>

#include <stdio.h>
#include <string.h>


// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ESDSinkNode::~ESDSinkNode(void)
{
	CALLED();
	fAddOn->GetConfigurationFor(this, NULL);
		
	BMediaEventLooper::Quit();
		
	fWeb = NULL;
	delete fDevice;
}

ESDSinkNode::ESDSinkNode(BMediaAddOn *addon, char* name, BMessage * config)
	: BMediaNode(name),
	  BBufferConsumer(B_MEDIA_RAW_AUDIO),
#if ENABLE_INPUT
	  BBufferProducer(B_MEDIA_RAW_AUDIO),
#endif
#ifdef ENABLE_TS
	  BTimeSource(),
#endif
	  BMediaEventLooper(),
	  fThread(-1),
	  fDevice(NULL),
	  fTimeSourceStarted(false),
	  fWeb(NULL),
	  fConfig(*config)
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	
	fAddOn = addon;
	fId = 0;
	
	AddNodeKind( B_PHYSICAL_OUTPUT );
#if ENABLE_INPUT
	AddNodeKind( B_PHYSICAL_INPUT );
#endif
		
	// initialize our preferred format object
	memset(&fPreferredFormat, 0, sizeof(fPreferredFormat)); // set everything to wildcard first
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
#if ESD_FMT == 8
	fPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
#else
	fPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
#endif
	fPreferredFormat.u.raw_audio.valid_bits = 0;
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.frame_rate = ESD_DEFAULT_RATE;
	fPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	
	// we'll use the consumer's preferred buffer size, if any
	fPreferredFormat.u.raw_audio.buffer_size = ESD_MAX_BUF / 4
/*						* (fPreferredFormat.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* fPreferredFormat.u.raw_audio.channel_count*/;
	
	if(config) {
		//PRINT_OBJECT(*config);
		config->FindString("hostname", &fHostname);
	}
	if (fHostname.Length() < 1)
		fHostname = "172.20.109.151";//"192.168.0.2";
	fPort = ESD_DEFAULT_PORT;
	fEnabled = false;
	
	fDevice = new ESDEndpoint();
	/*
	if (fDevice) {
		if (fDevice->Connect(fHostname.String()) >= 0) {
			fDevice->SetCommand();
			fDevice->SetFormat(ESD_FMT, 2);
			//fDevice->GetServerInfo();
			fInitCheckStatus = fDevice->SendDefaultCommand();
		}
	}
	*/
	if (!fDevice)
		return;
	fInitCheckStatus = B_OK;
}

status_t ESDSinkNode::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * ESDSinkNode::AddOn(
				int32 * internal_id) const
{
	CALLED();
	// BeBook says this only gets called if we were in an add-on.
	if (fAddOn != 0) {
		// If we get a null pointer then we just won't write.
		if (internal_id != 0) {
			*internal_id = fId;
		}
	}
	return fAddOn;
}

void ESDSinkNode::Preroll(void)
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t ESDSinkNode::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	CALLED();
	return B_ERROR;
}

void ESDSinkNode::NodeRegistered(void)
{
	CALLED();
	
	if (fInitCheckStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}
	
	SetPriority(B_REAL_TIME_PRIORITY);
	
	Run();
	
//	media_input *input = new media_input;

	fInput.format = fPreferredFormat;
	fInput.destination.port = ControlPort();
	fInput.destination.id = 0;
	fInput.node = Node();
	sprintf(fInput.name, "output %ld", fInput.destination.id);
		
	fOutput.format = fPreferredFormat;
	fOutput.destination = media_destination::null;
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.node = Node();
	sprintf(fOutput.name, "input %ld", fOutput.source.id);
		
	// Set up our parameter web
	fWeb = MakeParameterWeb();
	SetParameterWeb(fWeb);
	
	/* apply configuration */
#ifdef PRINTING
	bigtime_t start = system_time();
#endif
		
	int32 index = 0;
	int32 parameterID = 0;
	const void *data;
	ssize_t size;
	while(fConfig.FindInt32("parameterID", index, &parameterID) == B_OK) {
		if(fConfig.FindData("parameterData", B_RAW_TYPE, index, &data, &size) == B_OK)
			SetParameterValue(parameterID, TimeSource()->Now(), data, size);
		index++;
	}
	
#ifdef PRINTING
	PRINT(("apply configuration in : %lld\n", system_time() - start));
#endif
}

status_t ESDSinkNode::RequestCompleted(const media_request_info &info)
{
	CALLED();
	return B_OK;
}

void ESDSinkNode::SetTimeSource(BTimeSource *timeSource)
{
	CALLED();
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t ESDSinkNode::AcceptFormat(
				const media_destination & dest,
				media_format * format)
{
	status_t err;
	CALLED();
	
	if(fInput.destination != dest) {
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
	
	if ( format->type != B_MEDIA_RAW_AUDIO ) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}	
	
	/*if(format->u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
		&& channel->fPreferredFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT)
		format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	else*/ 
	format->u.raw_audio.format = fPreferredFormat.u.raw_audio.format;
	format->u.raw_audio.valid_bits = fPreferredFormat.u.raw_audio.valid_bits;
	
	format->u.raw_audio.frame_rate    = fPreferredFormat.u.raw_audio.frame_rate;
	format->u.raw_audio.channel_count = fPreferredFormat.u.raw_audio.channel_count;
	format->u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	format->u.raw_audio.buffer_size   = ESD_MAX_BUF / 4
/*						* (format->u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* format->u.raw_audio.channel_count*/;
	
	
	/*media_format myFormat;
	GetFormat(&myFormat);
	if (!format_is_acceptible(*format,myFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}*/
	//AddRequirements(format);
	
	// start connecting here
	err = fDevice->Connect(fHostname.String(), fPort);
	
	return B_OK;	
}

status_t ESDSinkNode::GetNextInput(
				int32 * cookie,
				media_input * out_input)
{
	CALLED();
	
	if ((*cookie < 1) && (*cookie >= 0)) {
		*out_input = fInput;
		*cookie += 1;
		PRINT(("input.format : %lu\n", fInput.format.u.raw_audio.format));
		return B_OK;
	} else
		return B_BAD_INDEX;
}

void ESDSinkNode::DisposeInputCookie(
				int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
}

void ESDSinkNode::BufferReceived(
				BBuffer * buffer)
{
	CALLED();
	switch (buffer->Header()->type) {
		/*case B_MEDIA_PARAMETERS:
			{
			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
			if (status != B_OK) {
				fprintf(stderr,"ApplyParameterData in ESDSinkNode::BufferReceived failed\n");
			}			
			buffer->Recycle();
			}
			break;*/
		case B_MEDIA_RAW_AUDIO:
#if 0
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in ESDSinkNode::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in ESDSinkNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
#endif
			if (fDevice->CanSend()) {
				
				fDevice->Write(buffer->Data(), buffer->SizeUsed());
				
			}
			buffer->Recycle();
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in ESDSinkNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void ESDSinkNode::ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	CALLED();
	
	if(fInput.destination != for_whom) {
		fprintf(stderr,"invalid destination received in ESDSinkNode::ProducerDataStatus\n");
		return;
	}
	
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&fInput, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t ESDSinkNode::GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource)
{
	CALLED();
	if ((out_latency == 0) || (out_timesource == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	if(fInput.destination != for_whom) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	bigtime_t intl = EventLatency();
	bigtime_t netl = 0LL;
	if (fDevice)
		netl = fDevice->GetLatency();
	// I don't want to swap
	if (netl > 500000)
		netl = 500000;
	*out_latency = intl + netl;
	fprintf(stderr, "int latency %Ld, net latency %Ld, total latency %Ld\n", intl, netl, *out_latency);
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

status_t ESDSinkNode::Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input)
{
	status_t err;
	CALLED();
	
	if(fInput.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	// 
	if (fDevice) {
		err = fDevice->WaitForConnect();
		if (err < B_OK)
			return err;
		fDevice->SetCommand();
		//fDevice->GetServerInfo();
		fDevice->SetFormat(ESD_FMT, 2);
		err = fDevice->SendDefaultCommand();
		if (err < B_OK)
			return err;
	}
	// use one buffer length latency
	fInternalLatency = with_format.u.raw_audio.buffer_size * 10000 / 2
			/ ( (with_format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
				* with_format.u.raw_audio.channel_count) 
			/ ((int32)(with_format.u.raw_audio.frame_rate / 100));
			
	PRINT(("  internal latency = %lld\n",fInternalLatency));
	
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	fInput.source = producer;
	fInput.format = with_format;
	*out_input = fInput;
		
	return B_OK;
}

void ESDSinkNode::Disconnected(
				const media_source & producer,
				const media_destination & where)
{
	CALLED();

	if(fInput.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (fInput.source != producer) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
		
	fInput.source = media_source::null;
	fInput.format = fPreferredFormat;
	//GetFormat(&channel->fInput.format);
	if (fDevice)
		fDevice->Disconnect();
}

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
status_t ESDSinkNode::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format)
{
	CALLED();

	if(fInput.destination != consumer) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (fInput.source != producer) {
		return B_MEDIA_BAD_SOURCE;
	}
		
	return B_ERROR;
}

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
status_t ESDSinkNode::SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags)
{
	CALLED();
	return BBufferConsumer::SeekTagRequested(destination,in_target_time,in_flags,
											out_seek_tag,out_tagged_time,out_flags);
}

// -------------------------------------------------------- //
// implementation for BBufferProducer
// -------------------------------------------------------- //
#if 0
status_t 
ESDSinkNode::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
{
	// FormatSuggestionRequested() is not necessarily part of the format negotiation
	// process; it's simply an interrogation -- the caller wants to see what the node's
	// preferred data format is, given a suggestion by the caller.
	CALLED();

	if (!format)
	{
		fprintf(stderr, "\tERROR - NULL format pointer passed in!\n");
		return B_BAD_VALUE;
	}

	// this is the format we'll be returning (our preferred format)
	*format = fPreferredFormat;

	// a wildcard type is okay; we can specialize it
	if (type == B_MEDIA_UNKNOWN_TYPE) type = B_MEDIA_RAW_AUDIO;

	// we only support raw audio
	if (type != B_MEDIA_RAW_AUDIO) return B_MEDIA_BAD_FORMAT;
	else return B_OK;
}

status_t 
ESDSinkNode::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect() process.  We hand
	// out a suggested format, with wildcards for any variations we support.
	CALLED();
	node_output *channel = FindOutput(output);
	
	// is this a proposal for our select output?
	if (channel == NULL)
	{
		fprintf(stderr, "ESDSinkNode::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// we only support floating-point raw audio, so we always return that, but we
	// supply an error code depending on whether we found the proposal acceptable.
	media_type requestedType = format->type;
	*format = channel->fPreferredFormat;
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO))
	{
		fprintf(stderr, "ESDSinkNode::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else return B_OK;		// raw audio or wildcard type, either is okay by us
}

status_t 
ESDSinkNode::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}

status_t 
ESDSinkNode::GetNextOutput(int32* cookie, media_output* out_output)
{
	CALLED();

	if ((*cookie < fOutputs.CountItems()) && (*cookie >= 0)) {
		node_output *channel = (node_output *)fOutputs.ItemAt(*cookie);
		*out_output = channel->fOutput;
		*cookie += 1;
		return B_OK;
	} else
		return B_BAD_INDEX;
}

status_t 
ESDSinkNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}

status_t 
ESDSinkNode::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	CALLED();

	node_output *channel = FindOutput(for_source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "ESDSinkNode::SetBufferGroup returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Are we being passed the buffer group we're already using?
	if (newGroup == channel->fBufferGroup) return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point we delete
	// the one we are using and use the specified one instead.  If the specified group is
	// NULL, we need to recreate one ourselves, and use *that*.  Note that if we're
	// caching a BBuffer that we requested earlier, we have to Recycle() that buffer
	// *before* deleting the buffer group, otherwise we'll deadlock waiting for that
	// buffer to be recycled!
	delete channel->fBufferGroup;		// waits for all buffers to recycle
	if (newGroup != NULL)
	{
		// we were given a valid group; just use that one from now on
		channel->fBufferGroup = newGroup;
	}
	else
	{
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		size_t size = channel->fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(fLatency / BufferDuration() + 1 + 1);
		channel->fBufferGroup = new BBufferGroup(size, count);
	}

	return B_OK;
}

status_t 
ESDSinkNode::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!
	CALLED();

	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "ESDSinkNode::PrepareToConnect returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (channel->fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO)
	{
		fprintf(stderr, "\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}
	
	 // !!! validate all other fields except for buffer_size here, because the consumer might have
	// supplied different values from AcceptFormat()?

	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
	{
		format->u.raw_audio.buffer_size = 2048;		// pick something comfortable to suggest
		fprintf(stderr, "\tno buffer size provided, suggesting %lu\n", format->u.raw_audio.buffer_size);
	}
	else
	{
		fprintf(stderr, "\tconsumer suggested buffer_size %lu\n", format->u.raw_audio.buffer_size);
	}

	// Now reserve the connection, and return information about it
	channel->fOutput.destination = where;
	channel->fOutput.format = *format;
	*out_source = channel->fOutput.source;
	strncpy(out_name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);
	return B_OK;
}

void 
ESDSinkNode::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	CALLED();
	
	node_output *channel = FindOutput(source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "ESDSinkNode::Connect returning (cause : B_MEDIA_BAD_SOURCE)\n");
		return;
	}
	
	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	if (error)
	{
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and format
	// that we agreed on, and report our connection name again.
	channel->fOutput.destination = destination;
	channel->fOutput.format = format;
	strncpy(io_name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = channel->fOutput.format.u.raw_audio.buffer_size * 10000
			/ ( (channel->fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
				* channel->fOutput.format.u.raw_audio.channel_count) 
			/ ((int32)(channel->fOutput.format.u.raw_audio.frame_rate / 100));
	
	SetBufferDuration(duration);
	
	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(channel->fOutput.destination, &fLatency, &id);
	PRINT(("\tdownstream latency = %Ld\n", fLatency));

	fInternalLatency = BufferDuration();
	PRINT(("\tbuffer-filling took %Ld usec on this machine\n", fInternalLatency));
	//SetEventLatency(fLatency + fInternalLatency);

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!channel->fBufferGroup) 
		AllocateBuffers(*channel);
		
	// we are sure the thread is started
	StartThread();
}

void 
ESDSinkNode::Disconnect(const media_source& what, const media_destination& where)
{
	CALLED();
	
	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "ESDSinkNode::Disconnect() returning (cause : B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	// Make sure that our connection is the one being disconnected
	if ((where == channel->fOutput.destination) && (what == channel->fOutput.source))
	{
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		delete channel->fBufferGroup;
		channel->fBufferGroup = NULL;
	}
	else
	{
		fprintf(stderr, "\tDisconnect() called with wrong source/destination (%ld/%ld), ours is (%ld/%ld)\n",
			what.id, where.id, channel->fOutput.source.id, channel->fOutput.destination.id);
	}
}

void 
ESDSinkNode::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
{
	CALLED();
	
	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		return;
	}

	// If we're late, we need to catch up.  Respond in a manner appropriate to our
	// current run mode.
	if (RunMode() == B_RECORDING)
	{
		// A hardware capture node can't adjust; it simply emits buffers at
		// appropriate points.  We (partially) simulate this by not adjusting
		// our behavior upon receiving late notices -- after all, the hardware
		// can't choose to capture "sooner"....
	}
	else if (RunMode() == B_INCREASE_LATENCY)
	{
		// We're late, and our run mode dictates that we try to produce buffers
		// earlier in order to catch up.  This argues that the downstream nodes are
		// not properly reporting their latency, but there's not much we can do about
		// that at the moment, so we try to start producing buffers earlier to
		// compensate.
		fInternalLatency += how_much;
		SetEventLatency(fLatency + fInternalLatency);

		fprintf(stderr, "\tincreasing latency to %Ld\n", fLatency + fInternalLatency);
	}
	else
	{
		// The other run modes dictate various strategies for sacrificing data quality
		// in the interests of timely data delivery.  The way *we* do this is to skip
		// a buffer, which catches us up in time by one buffer duration.
		/*size_t nSamples = fOutput.format.u.raw_audio.buffer_size / sizeof(float);
		mSamplesSent += nSamples;*/

		fprintf(stderr, "\tskipping a buffer to try to catch up\n");
	}
}

void 
ESDSinkNode::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
{
	CALLED();

	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	node_output *channel = FindOutput(what);
	
	if (channel != NULL)
	{
		channel->fOutputEnabled = enabled;
	}
}

void 
ESDSinkNode::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}
#endif

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void ESDSinkNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent)
{
	CALLED();
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
			fprintf(stderr,"  unknown event type: %li\n",event->type);
			break;
	}
}

// protected:

// how should we handle late buffers?  drop them?
// notify the producer?
status_t ESDSinkNode::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent)
{
	CALLED();
	BBuffer * buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	if(fInput.destination.id != buffer->Header()->destination) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	media_header* hdr = buffer->Header();
	bigtime_t now = TimeSource()->Now();
	bigtime_t perf_time = hdr->start_time;
	
	// the how_early calculate here doesn't include scheduling latency because
	// we've already been scheduled to handle the buffer
	bigtime_t how_early = perf_time - EventLatency() - now;
	
	// if the buffer is late, we ignore it and report the fact to the producer
	// who sent it to us
	if ((RunMode() != B_OFFLINE) &&				// lateness doesn't matter in offline mode...
		(RunMode() != B_RECORDING) &&		// ...or in recording mode
		(how_early < 0LL))
	{
		//mLateBuffers++;
		NotifyLateProducer(fInput.source, -how_early, perf_time);
		fprintf(stderr,"	<- LATE BUFFER : %lli\n", how_early);
		buffer->Recycle();
	} else {
		if (fDevice->CanSend())
			fDevice->Write(buffer->Data(), buffer->SizeUsed());
	}
	return B_OK;
}

status_t ESDSinkNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	PRINT(("ESDSinkNode::HandleDataStatus status:%li, lateness:%lli\n", event->data, lateness));
	switch(event->data) {
		case B_DATA_NOT_AVAILABLE:
			break;
		case B_DATA_AVAILABLE:
			break;
		case B_PRODUCER_STOPPED:
			break;
		default:
			break;
	}
	return B_OK;
}

status_t ESDSinkNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	if (RunState() != B_STARTED) {
	
	}
	return B_OK;
}

status_t ESDSinkNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	PRINT(("ESDSinkNode::HandleSeek(t=%lld,d=%li,bd=%lld)\n",event->event_time,event->data,event->bigdata));
	return B_OK;
}
						
status_t ESDSinkNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	return B_OK;
}

status_t ESDSinkNode::HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
	
	//StopThread();
	return B_OK;
}

status_t ESDSinkNode::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent)
{
	CALLED();
	return B_OK;
}

// -------------------------------------------------------- //
// implemention of BTimeSource
// -------------------------------------------------------- //
#ifdef ENABLE_TS

void
ESDSinkNode::SetRunMode(run_mode mode)
{
	CALLED();
	PRINT(("ESDSinkNode::SetRunMode mode:%i\n", mode));
	//BTimeSource::SetRunMode(mode);
}

status_t
ESDSinkNode::TimeSourceOp(const time_source_op_info &op, void *_reserved)
{
	CALLED();
	switch(op.op) {
		case B_TIMESOURCE_START:
			PRINT(("TimeSourceOp op B_TIMESOURCE_START\n"));
			if (RunState() != BMediaEventLooper::B_STARTED) {
				fTimeSourceStarted = true;
			
				media_timed_event startEvent(0, BTimedEventQueue::B_START);
				EventQueue()->AddEvent(startEvent);
			}	
			break;
		case B_TIMESOURCE_STOP:
			PRINT(("TimeSourceOp op B_TIMESOURCE_STOP\n"));
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			PRINT(("TimeSourceOp op B_TIMESOURCE_STOP_IMMEDIATELY\n"));
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_SEEK:
			PRINT(("TimeSourceOp op B_TIMESOURCE_SEEK\n"));
			BroadcastTimeWarp(op.real_time, op.performance_time);
			break;
		default:
			break;
	}
	return B_OK;
}
#endif

// -------------------------------------------------------- //
// implemention of BControllable
// -------------------------------------------------------- //

status_t 
ESDSinkNode::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* ioSize)
{
	CALLED();
	if (!fDevice)
		return B_ERROR;
	//PRINT(("id : %i\n", id));
	switch (id) {
		case PARAM_ENABLED:
			if (*ioSize < sizeof(bool))
				return B_NO_MEMORY;
			*(bool *)value = fEnabled;
			*ioSize = sizeof(bool);
			return B_OK;
		case PARAM_HOST:
		{
			BString s = fDevice->Host();
			*ioSize = MIN(*ioSize, s.Length());
			memcpy(value, s.String(), *ioSize);
			return B_OK;
		}
		case PARAM_PORT:
		{
			BString s;
			s << fDevice->Port();
			*ioSize = MIN(*ioSize, s.Length());
			memcpy(value, s.String(), *ioSize);
			return B_OK;
		}
		default:
			break;
	}
#if 0
	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}
#endif
	
return EINVAL;
}

void 
ESDSinkNode::SetParameterValue(int32 id, bigtime_t performance_time, const void* value, size_t size)
{
	CALLED();
	PRINT(("id : %li, performance_time : %lld, size : %li\n", id, performance_time, size));
	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}
	switch (id) {
		case PARAM_ENABLED:
			if (size != sizeof(bool))
				return;
			fEnabled = *(bool *)value;
			return;
		case PARAM_HOST:
		{
			fprintf(stderr, "set HOST: %s\n", (const char *)value);
			fHostname = (const char *)value;
#if 0
			if (fDevice && fDevice->Connected()) {
				if (fDevice->Connect(fHostname.String(), fPort) >= 0) {
					fDevice->SetCommand();
					fDevice->SetFormat(ESD_FMT, 2);
					//fDevice->GetServerInfo();
					fInitCheckStatus = fDevice->SendDefaultCommand();
				}
			}
#endif
			return;
		}
		case PARAM_PORT:
		{
			fprintf(stderr, "set PORT: %s\n", (const char *)value);
			fPort = atoi((const char *)value);
#if 0
			if (fDevice && fDevice->Connected()) {
				if (fDevice->Connect(fHostname.String(), fPort) >= 0) {
					fDevice->SetCommand();
					fDevice->SetFormat(ESD_FMT, 2);
					//fDevice->GetServerInfo();
					fInitCheckStatus = fDevice->SendDefaultCommand();
				}
			}
#endif
			return;
		}
		default:
			break;
	}
}

BParameterWeb* 
ESDSinkNode::MakeParameterWeb()
{
	CALLED();
	BParameterWeb* web = new BParameterWeb;
	BParameterGroup *group = web->MakeGroup("Server");
	BParameter *p;
	// XXX: use B_MEDIA_UNKNOWN_TYPE or _NO_TYPE ?
	// keep in sync with enum { PARAM_* } !
	p = group->MakeDiscreteParameter(PARAM_ENABLED, B_MEDIA_RAW_AUDIO, "Enable", B_ENABLE);
#if defined(B_BEOS_VERSION_DANO) || defined(__HAIKU__)
	p = group->MakeTextParameter(PARAM_HOST, B_MEDIA_RAW_AUDIO, "Hostname", B_GENERIC, 128);
	p = group->MakeTextParameter(PARAM_PORT, B_MEDIA_RAW_AUDIO, "Port", B_GENERIC, 16);
#endif
	return web;
}

// -------------------------------------------------------- //
// ESDSinkNode specific functions
// -------------------------------------------------------- //

status_t
ESDSinkNode::GetConfigurationFor(BMessage * into_message)
{
	CALLED();
	
	BParameter *parameter = NULL;
	void *buffer;
	size_t size = 128;
	bigtime_t last_change;
	status_t err;
	
	if (!into_message)
		return B_BAD_VALUE;
	
	buffer = malloc(size);
	
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER
			&& parameter->Type() != BParameter::B_DISCRETE_PARAMETER)
			continue;
			
		PRINT(("getting parameter %li\n", parameter->ID()));
		size = 128;
		while((err = GetParameterValue(parameter->ID(), &last_change, buffer, &size))==B_NO_MEMORY) {
			size += 128;
			free(buffer);
			buffer = malloc(size);
		}
		
		if(err == B_OK && size > 0) {
			into_message->AddInt32("parameterID", parameter->ID());
			into_message->AddData("parameterData", B_RAW_TYPE, buffer, size, false);
		} else {
			PRINT(("parameter %li err : %s\n", parameter->ID(), strerror(err)));
		}
	}
	
	//PRINT_OBJECT(*into_message);
	
	return B_OK;
}

// static:

void ESDSinkNode::GetFlavor(flavor_info * outInfo, int32 id)
{
	CALLED();

	outInfo->flavor_flags = B_FLAVOR_IS_GLOBAL;
//	outInfo->possible_count = 0;	// any number
	outInfo->possible_count = 1;	// only 1
	outInfo->in_format_count = 0; // no inputs
	outInfo->in_formats = 0;
	outInfo->out_format_count = 0; // no outputs
	outInfo->out_formats = 0;
	outInfo->internal_id = id;
	
	outInfo->name = new char[256];
		strcpy(outInfo->name, "ESounD Out");
	outInfo->info = new char[256];
		strcpy(outInfo->info, "The ESounD Sink node outputs a network Enlightenment Sound Daemon.");
	outInfo->kinds = /*B_TIME_SOURCE | *//*B_CONTROLLABLE | */ 0;

#if ENABLE_INPUT
	outInfo->kinds |= B_BUFFER_PRODUCER | B_PHYSICAL_INPUT;
	outInfo->out_format_count = 1; // 1 output
	media_format * outformats = new media_format[outInfo->out_format_count];
	GetFormat(&outformats[0]);
	outInfo->out_formats = outformats;
#endif
	
	outInfo->kinds |= B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT;
	outInfo->in_format_count = 1; // 1 input
	media_format * informats = new media_format[outInfo->in_format_count];
	GetFormat(&informats[0]);
	outInfo->in_formats = informats;
}

void ESDSinkNode::GetFormat(media_format * outFormat)
{
	CALLED();

	outFormat->type = B_MEDIA_RAW_AUDIO;
	outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
	outFormat->u.raw_audio = media_raw_audio_format::wildcard;
}
