/*
 * multiaudio replacement media addon for BeOS
 *
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

#include "MultiAudioNode.h"
#include "driver_io.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

const char * multi_string[] =
{
	"NAME IS ATTACHED",
	"Output", "Input", "Setup", "Tone Control", "Extended Setup", "Enhanced Setup", "Master",
	"Beep", "Phone", "Mic", "Line", "CD", "Video", "Aux", "Wave", "Gain", "Level", "Volume",
	"Mute", "Enable", "Stereo Mix", "Mono Mix", "Output Stereo Mix", "Output Mono Mix", "Output Bass",
	"Output Treble", "Output 3D Center", "Output 3D Depth" 
};

node_input::node_input(media_input &input, media_format format)
{
	CALLED();
	fInput = input;
	fPreferredFormat = format;
	fBufferCycle = 1;
	fBuffer = NULL;
}

node_input::~node_input()
{
	CALLED();
}

node_output::node_output(media_output &output, media_format format)
	: fBufferGroup(NULL),
	  fOutputEnabled(true)
{
	CALLED();
	fOutput = output;
	fPreferredFormat = format;
	fBufferCycle = 1;
}

node_output::~node_output()
{
	CALLED();
}


// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

MultiAudioNode::~MultiAudioNode(void)
{
	CALLED();
	fAddOn->GetConfigurationFor(this, NULL);
		
	StopThread();
	BMediaEventLooper::Quit();
		
	fWeb = NULL;	
}

MultiAudioNode::MultiAudioNode(BMediaAddOn *addon, char* name, MultiAudioDevice *device, 
	int32 internal_id, BMessage * config)
	: BMediaNode(name),
	  BBufferConsumer(B_MEDIA_RAW_AUDIO),
	  BBufferProducer(B_MEDIA_RAW_AUDIO),
	  BTimeSource(),
	  BMediaEventLooper(),
	  fThread(-1),
	  fDevice(device),
	  fTimeSourceStarted(false),
	  fWeb(NULL),
	  fConfig(*config)
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	
	if(!device)
		return;
		
	fAddOn = addon;
	fId = internal_id;
	
	AddNodeKind( B_PHYSICAL_OUTPUT );
	AddNodeKind( B_PHYSICAL_INPUT );
		
	// initialize our preferred format object
	memset(&fPreferredFormat, 0, sizeof(fPreferredFormat)); // set everything to wildcard first
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio.format = MultiAudioDevice::convert_multiaudio_format_to_media_format(fDevice->MFI.output.format);
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.frame_rate = MultiAudioDevice::convert_multiaudio_rate_to_media_rate(fDevice->MFI.input.rate);		// measured in Hertz
	fPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	
	// we'll use the consumer's preferred buffer size, if any
	fPreferredFormat.u.raw_audio.buffer_size = fDevice->MBL.return_record_buffer_size 
						* (fPreferredFormat.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* fPreferredFormat.u.raw_audio.channel_count;
	
	if(config) {
		PRINT_OBJECT(*config);
	}
		
	fInitCheckStatus = B_OK;
}

status_t MultiAudioNode::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * MultiAudioNode::AddOn(
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

void MultiAudioNode::Preroll(void)
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t MultiAudioNode::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	CALLED();
	return B_ERROR;
}

void MultiAudioNode::NodeRegistered(void)
{
	CALLED();
	
	if (fInitCheckStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}
	
	SetPriority(B_REAL_TIME_PRIORITY);
	
	Run();
	
	node_input *currentInput = NULL;
	int32 currentId = 0;
	
	for (int32 i = 0; i < fDevice->MD.output_channel_count; i++) {
	
		if(	(currentInput == NULL)
			|| (fDevice->MD.channels[i].designations & B_CHANNEL_MONO_BUS)
			|| (fDevice->MD.channels[currentId].designations & B_CHANNEL_STEREO_BUS
				&& ( fDevice->MD.channels[i].designations & B_CHANNEL_LEFT ||
					!(fDevice->MD.channels[i].designations & B_CHANNEL_STEREO_BUS))) 
			|| (fDevice->MD.channels[currentId].designations & B_CHANNEL_SURROUND_BUS
				&& ( fDevice->MD.channels[i].designations & B_CHANNEL_LEFT ||
					!(fDevice->MD.channels[i].designations & B_CHANNEL_SURROUND_BUS)))
			) {
			PRINT(("NodeRegistered() : creating an input for %i\n", i));
			PRINT(("%ld\t%d\t0x%lx\t0x%lx\n",fDevice->MD.channels[i].channel_id,
											fDevice->MD.channels[i].kind,
											fDevice->MD.channels[i].designations,
											fDevice->MD.channels[i].connectors));
			
			media_input *input = new media_input;

			input->format = fPreferredFormat;
			input->destination.port = ControlPort();
			input->destination.id = fInputs.CountItems();
			input->node = Node();
			sprintf(input->name, "output %ld", input->destination.id);
					
			currentInput = new node_input(*input, fPreferredFormat);
			currentInput->fPreferredFormat.u.raw_audio.channel_count = 1;
			currentInput->fInput.format = currentInput->fPreferredFormat;
			
			currentInput->fChannelId = fDevice->MD.channels[i].channel_id;
			fInputs.AddItem(currentInput);
			
			currentId = i;
		
		} else {
			PRINT(("NodeRegistered() : adding a channel\n"));
			currentInput->fPreferredFormat.u.raw_audio.channel_count++;
			currentInput->fInput.format = currentInput->fPreferredFormat;
		}
		currentInput->fInput.format.u.raw_audio.format = media_raw_audio_format::wildcard.format;
	}
	
	node_output *currentOutput = NULL;
	currentId = 0;
	
	for (int32 i = fDevice->MD.output_channel_count; 
		i < (fDevice->MD.output_channel_count + fDevice->MD.input_channel_count); i++) {
		
		if(	(currentOutput == NULL)
			|| (fDevice->MD.channels[i].designations & B_CHANNEL_MONO_BUS)
			|| (fDevice->MD.channels[currentId].designations & B_CHANNEL_STEREO_BUS
				&& ( fDevice->MD.channels[i].designations & B_CHANNEL_LEFT ||
					!(fDevice->MD.channels[i].designations & B_CHANNEL_STEREO_BUS))) 
			|| (fDevice->MD.channels[currentId].designations & B_CHANNEL_SURROUND_BUS
				&& ( fDevice->MD.channels[i].designations & B_CHANNEL_LEFT ||
					!(fDevice->MD.channels[i].designations & B_CHANNEL_SURROUND_BUS)))
			) {
			PRINT(("NodeRegistered() : creating an output for %i\n", i));
			PRINT(("%ld\t%d\t0x%lx\t0x%lx\n",fDevice->MD.channels[i].channel_id,
											fDevice->MD.channels[i].kind,
											fDevice->MD.channels[i].designations,
											fDevice->MD.channels[i].connectors));
			
			media_output *output = new media_output;

			output->format = fPreferredFormat;
			output->destination = media_destination::null;
			output->source.port = ControlPort();
			output->source.id = fOutputs.CountItems();
			output->node = Node();
			sprintf(output->name, "input %ld", output->source.id);
					
			currentOutput = new node_output(*output, fPreferredFormat);
			currentOutput->fPreferredFormat.u.raw_audio.channel_count = 1;
			currentOutput->fOutput.format = currentOutput->fPreferredFormat;
			currentOutput->fChannelId = fDevice->MD.channels[i].channel_id;
			fOutputs.AddItem(currentOutput);
			
			currentId = i;
		
		} else {
			PRINT(("NodeRegistered() : adding a channel\n"));
			currentOutput->fPreferredFormat.u.raw_audio.channel_count++;
			currentOutput->fOutput.format = currentOutput->fPreferredFormat;
		}
	}
		
	// Set up our parameter web
	fWeb = MakeParameterWeb();
	SetParameterWeb(fWeb);
	
	/* apply configuration */
	bigtime_t start = system_time();
		
	int32 index = 0;
	int32 parameterID = 0;
	const void *data;
	ssize_t size;
	while(fConfig.FindInt32("parameterID", index, &parameterID) == B_OK) {
		if(fConfig.FindData("parameterData", B_RAW_TYPE, index, &data, &size) == B_OK)
			SetParameterValue(parameterID, TimeSource()->Now(), data, size);
		index++;
	}
	
	PRINT(("apply configuration in : %ld\n", system_time() - start));
}

status_t MultiAudioNode::RequestCompleted(const media_request_info &info)
{
	CALLED();
	return B_OK;
}

void MultiAudioNode::SetTimeSource(BTimeSource *timeSource)
{
	CALLED();
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t MultiAudioNode::AcceptFormat(
				const media_destination & dest,
				media_format * format)
{
	CALLED();
	
	node_input *channel = FindInput(dest);
	
	if(channel==NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION");
		return B_MEDIA_BAD_DESTINATION; // we only have one input so that better be it
	}
	
	if (format == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
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
	
	channel->fFormat = channel->fPreferredFormat;
	
	/*if(format->u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
		&& channel->fPreferredFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT)
		format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	else*/
		format->u.raw_audio.format = channel->fPreferredFormat.u.raw_audio.format; //MultiAudioDevice::convert_multiaudio_format_to_media_format(fDevice->MFI.output.format);	//media_raw_audio_format::B_AUDIO_SHORT;
	
	format->u.raw_audio.frame_rate    = channel->fPreferredFormat.u.raw_audio.frame_rate;//MultiAudioDevice::convert_multiaudio_rate_to_media_rate(fDevice->MFI.output.rate); //48000.0;
	format->u.raw_audio.channel_count = channel->fPreferredFormat.u.raw_audio.channel_count;
	format->u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	format->u.raw_audio.buffer_size   = fDevice->MBL.return_playback_buffer_size 
						* (format->u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* format->u.raw_audio.channel_count;
	
	
	/*media_format myFormat;
	GetFormat(&myFormat);
	if (!format_is_acceptible(*format,myFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}*/
	//AddRequirements(format);
	return B_OK;	
}

status_t MultiAudioNode::GetNextInput(
				int32 * cookie,
				media_input * out_input)
{
	CALLED();
	// let's not crash even if they are stupid
	if (out_input == 0) {
		// no place to write!
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
		
	if ((*cookie < fInputs.CountItems()) && (*cookie >= 0)) {
		node_input *channel = (node_input *)fInputs.ItemAt(*cookie);
		*out_input = channel->fInput;
		*cookie += 1;
		PRINT(("input.format : %u\n", channel->fInput.format.u.raw_audio.format));
		return B_OK;
	} else
		return B_BAD_INDEX;
}

void MultiAudioNode::DisposeInputCookie(
				int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
}

void MultiAudioNode::BufferReceived(
				BBuffer * buffer)
{
	//CALLED();
	switch (buffer->Header()->type) {
		/*case B_MEDIA_PARAMETERS:
			{
			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
			if (status != B_OK) {
				fprintf(stderr,"ApplyParameterData in MultiAudioNode::BufferReceived failed\n");
			}			
			buffer->Recycle();
			}
			break;*/
		case B_MEDIA_RAW_AUDIO:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in MultiAudioNode::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in MultiAudioNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in MultiAudioNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void MultiAudioNode::ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	//CALLED();
	
	node_input *channel = FindInput(for_whom);
	
	if(channel==NULL) {
		fprintf(stderr,"invalid destination received in MultiAudioNode::ProducerDataStatus\n");
		return;
	}
	
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&channel->fInput, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t MultiAudioNode::GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource)
{
	CALLED();
	if ((out_latency == 0) || (out_timesource == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	node_input *channel = FindInput(for_whom);
	
	if(channel==NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

status_t MultiAudioNode::Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input)
{
	CALLED();
	if (out_input == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE; // no crashing
	}
	
	node_input *channel = FindInput(where);
	
	if(channel==NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	// use one buffer length latency
	fInternalLatency = with_format.u.raw_audio.buffer_size * 10000 / 2
			/ ( (with_format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
				* with_format.u.raw_audio.channel_count) 
			/ ((int32)(with_format.u.raw_audio.frame_rate / 100));
			
	PRINT(("  internal latency = %lld\n",fInternalLatency));
	
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	channel->fInput.source = producer;
	channel->fInput.format = with_format;
	*out_input = channel->fInput;
	
	// we are sure the thread is started
	StartThread();
	
	return B_OK;
}

void MultiAudioNode::Disconnected(
				const media_source & producer,
				const media_destination & where)
{
	CALLED();
	node_input *channel = FindInput(where);
	
	if(channel==NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (channel->fInput.source != producer) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
		
	channel->fInput.source = media_source::null;
	channel->fInput.format = channel->fPreferredFormat;
	FillWithZeros(*channel);
	//GetFormat(&channel->fInput.format);
}

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
status_t MultiAudioNode::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format)
{
	CALLED();
	node_input *channel = FindInput(consumer);
	
	if(channel==NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (channel->fInput.source != producer) {
		return B_MEDIA_BAD_SOURCE;
	}
		
	return B_ERROR;
}

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
status_t MultiAudioNode::SeekTagRequested(
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

status_t 
MultiAudioNode::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
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
MultiAudioNode::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect() process.  We hand
	// out a suggested format, with wildcards for any variations we support.
	CALLED();
	node_output *channel = FindOutput(output);
	
	// is this a proposal for our select output?
	if (channel == NULL)
	{
		fprintf(stderr, "MultiAudioNode::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// we only support floating-point raw audio, so we always return that, but we
	// supply an error code depending on whether we found the proposal acceptable.
	media_type requestedType = format->type;
	*format = channel->fPreferredFormat;
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO))
	{
		fprintf(stderr, "MultiAudioNode::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else return B_OK;		// raw audio or wildcard type, either is okay by us
}

status_t 
MultiAudioNode::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}

status_t 
MultiAudioNode::GetNextOutput(int32* cookie, media_output* out_output)
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
MultiAudioNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}

status_t 
MultiAudioNode::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	CALLED();

	node_output *channel = FindOutput(for_source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "MultiAudioNode::SetBufferGroup returning B_MEDIA_BAD_SOURCE\n");
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
MultiAudioNode::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
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
		fprintf(stderr, "MultiAudioNode::PrepareToConnect returning B_MEDIA_BAD_SOURCE\n");
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
	else if (format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT)
	{
		fprintf(stderr, "\tnon-short-audio format?!\n");
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
MultiAudioNode::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	CALLED();
	
	node_output *channel = FindOutput(source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "MultiAudioNode::Connect returning (cause : B_MEDIA_BAD_SOURCE)\n");
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
MultiAudioNode::Disconnect(const media_source& what, const media_destination& where)
{
	CALLED();
	
	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "MultiAudioNode::Disconnect() returning (cause : B_MEDIA_BAD_SOURCE)\n");
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
MultiAudioNode::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
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
MultiAudioNode::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
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
MultiAudioNode::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void MultiAudioNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	//CALLED();
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
status_t MultiAudioNode::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	//CALLED();
	BBuffer * buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == 0) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	node_input *channel = FindInput(buffer->Header()->destination);
	//PRINT(("buffer->Header()->destination : %i\n", buffer->Header()->destination));
	
	if(channel==NULL) {
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
		NotifyLateProducer(channel->fInput.source, -how_early, perf_time);
		fprintf(stderr,"	<- LATE BUFFER : %lli\n", how_early);
		buffer->Recycle();
	} else {
		//WriteBuffer(buffer, *channel);
		if(channel->fBuffer != NULL) {
			PRINT(("MultiAudioNode::HandleBuffer snoozing recycling channelId : %i, how_early:%lli\n", channel->fChannelId, how_early));
			//channel->fBuffer->Recycle();
			snooze(100);
			if(channel->fBuffer != NULL)
				buffer->Recycle();
			else
				channel->fBuffer = buffer;
		} else {
			//PRINT(("MultiAudioNode::HandleBuffer writing channelId : %i, how_early:%lli\n", channel->fChannelId, how_early));
			channel->fBuffer = buffer;
		}
	}
	return B_OK;
}

status_t MultiAudioNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	//CALLED();
	PRINT(("MultiAudioNode::HandleDataStatus status:%li, lateness:%li\n", event->data, lateness));
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

status_t MultiAudioNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	if (RunState() != B_STARTED) {
	
	}
	return B_OK;
}

status_t MultiAudioNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	PRINT(("MultiAudioNode::HandleSeek(t=%lld,d=%li,bd=%lld)\n",event->event_time,event->data,event->bigdata));
	return B_OK;
}
						
status_t MultiAudioNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	return B_OK;
}

status_t MultiAudioNode::HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
	
	//StopThread();
	return B_OK;
}

status_t MultiAudioNode::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	CALLED();
	return B_OK;
}

// -------------------------------------------------------- //
// implemention of BTimeSource
// -------------------------------------------------------- //

void
MultiAudioNode::SetRunMode(run_mode mode)
{
	CALLED();
	PRINT(("MultiAudioNode::SetRunMode mode:%i\n", mode));
	//BTimeSource::SetRunMode(mode);
}

status_t
MultiAudioNode::TimeSourceOp(const time_source_op_info &op, void *_reserved)
{
	CALLED();
	switch(op.op) {
		case B_TIMESOURCE_START:
			PRINT(("TimeSourceOp op B_TIMESOURCE_START\n"));
			if (RunState() != BMediaEventLooper::B_STARTED) {
				fTimeSourceStarted = true;
				StartThread();
			
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
				StopThread();
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			PRINT(("TimeSourceOp op B_TIMESOURCE_STOP_IMMEDIATELY\n"));
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				StopThread();
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

// -------------------------------------------------------- //
// implemention of BControllable
// -------------------------------------------------------- //

status_t 
MultiAudioNode::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* ioSize)
{
	CALLED();
	
	PRINT(("id : %i\n", id));
	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}

	if(!parameter) {
		// Hmmm, we were asked for a parameter that we don't actually
		// support.  Report an error back to the caller.
		PRINT(("\terror - asked for illegal parameter %ld\n", id));
		return B_ERROR;
	}
	
	multi_mix_value_info MMVI;
	multi_mix_value MMV[2];
	int rval;
	MMVI.values = MMV;
	id = id - 100;
	MMVI.item_count = 0;
	
	if (*ioSize < sizeof(float))
		return B_ERROR;
	
	if(parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
		MMVI.item_count = 1;
		MMV[0].id = id;
		
		if(parameter->CountChannels() == 2) {
			if (*ioSize < 2*sizeof(float))
				return B_ERROR;
			MMVI.item_count = 2;
			MMV[1].id = id + 1;
		}
		
	} else if(parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
		MMVI.item_count = 1;
		MMV[0].id = id;
	}
	
	if(MMVI.item_count > 0) {
		rval = fDevice->DoGetMix(&MMVI);
			
		if (B_OK != rval) {	
			fprintf(stderr, "Failed on DRIVER_GET_MIX\n");
		} else {
			
			if(parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
				((float*)value)[0] = MMV[0].gain;
				*ioSize = sizeof(float); 
				
				if(parameter->CountChannels() == 2) {
					((float*)value)[1] = MMV[1].gain;
					*ioSize = 2*sizeof(float); 
				}
				
				for(uint32 i=0; i < (*ioSize/sizeof(float)); i++) {
					PRINT(("B_CONTINUOUS_PARAMETER value[%i] : %f\n", i, ((float*)value)[i]));
				}
			} else if(parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
			
				BDiscreteParameter *dparameter = (BDiscreteParameter*) parameter;
				if(dparameter->CountItems()<=2) {
					((int32*)value)[0] = (MMV[0].enable) ? 1 : 0;
				} else {
					((int32*)value)[0] = MMV[0].mux;
				}
				*ioSize = sizeof(int32);
				
				for(uint32 i=0; i < (*ioSize/sizeof(int32)); i++) {
					PRINT(("B_DISCRETE_PARAMETER value[%i] : %i\n", i, ((int32*)value)[i]));
				}
			}
		
		}
	}
	return B_OK;
}

void 
MultiAudioNode::SetParameterValue(int32 id, bigtime_t performance_time, const void* value, size_t size)
{
	CALLED();
	PRINT(("id : %i, performance_time : %lld, size : %i\n", id, performance_time, size));
	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}
	if(parameter) {
		multi_mix_value_info MMVI;
		multi_mix_value MMV[2];
		int rval;
		MMVI.values = MMV;
		id = id - 100;
		MMVI.item_count = 0;
		
		if(parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
			for(uint32 i=0; i < (size/sizeof(float)); i++) {
				PRINT(("B_CONTINUOUS_PARAMETER value[%i] : %f\n", i, ((float*)value)[i]));
			}
			MMVI.item_count = 1;
			MMV[0].id = id;
			MMV[0].gain = ((float*)value)[0];
			
			if(parameter->CountChannels() == 2) {
				MMVI.item_count = 2;
				MMV[1].id = id + 1;
				MMV[1].gain = ((float*)value)[1];
			}
			
		} else if(parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
			for(uint32 i=0; i < (size/sizeof(int32)); i++) {
				PRINT(("B_DISCRETE_PARAMETER value[%i] : %i\n", i, ((int32*)value)[i]));
			}
			BDiscreteParameter *dparameter = (BDiscreteParameter*) parameter;
			
			if(dparameter->CountItems()<=2) {
				MMVI.item_count = 1;
				MMV[0].id = id;
				MMV[0].enable = (((int32*)value)[0] == 1) ? true : false;
			} else {
				MMVI.item_count = 1;
				MMV[0].id = id;
				MMV[0].mux = ((uint32*)value)[0];
			}
		}
		
		if(MMVI.item_count > 0) {
			rval = fDevice->DoSetMix(&MMVI);
				
			if (B_OK != rval)
			{	
				fprintf(stderr, "Failed on DRIVER_SET_MIX\n");
			}
		}
	}
	
}

BParameterWeb* 
MultiAudioNode::MakeParameterWeb()
{
	CALLED();
	BParameterWeb* web = new BParameterWeb;
	
	PRINT(("MMCI.control_count : %i\n", fDevice->MMCI.control_count));
	multi_mix_control		*MMC = fDevice->MMCI.controls;
	
	for(int i=0; i<fDevice->MMCI.control_count; i++) {
		if(MMC[i].flags & B_MULTI_MIX_GROUP && MMC[i].parent == 0) {
				PRINT(("NEW_GROUP\n"));
				int32 nb = 0;
				const char* childName;
				if(MMC[i].string != S_null)
					childName = multi_string[MMC[i].string];
				else
					childName = MMC[i].name;
				BParameterGroup *child = web->MakeGroup(childName);
				ProcessGroup(child, i, nb);
		}
	}
		
	return web;
}

void 
MultiAudioNode::ProcessGroup(BParameterGroup *group, int32 index, int32 &nbParameters)
{
	CALLED();
	multi_mix_control		*parent = &fDevice->MMCI.controls[index];
	multi_mix_control		*MMC = fDevice->MMCI.controls;
	for(int32 i=0; i<fDevice->MMCI.control_count; i++) {
		if(MMC[i].parent != parent->id)
			continue;
			
		const char* childName;
		if(MMC[i].string != S_null)
			childName = multi_string[MMC[i].string];
		else
			childName = MMC[i].name;
			
		if(MMC[i].flags & B_MULTI_MIX_GROUP) {
			PRINT(("NEW_GROUP\n"));
			int32 nb = 1;
			BParameterGroup *child = group->MakeGroup(childName);
			child->MakeNullParameter(MMC[i].id, B_MEDIA_RAW_AUDIO, childName, B_WEB_BUFFER_OUTPUT);
			ProcessGroup(child, i, nb);
		} else if(MMC[i].flags & B_MULTI_MIX_MUX) {
			PRINT(("NEW_MUX\n"));
			BDiscreteParameter *parameter = 
				group->MakeDiscreteParameter(100 + MMC[i].id, B_MEDIA_RAW_AUDIO, childName, B_INPUT_MUX);
			if(nbParameters>0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
			ProcessMux(parameter, i);
		} else if(MMC[i].flags & B_MULTI_MIX_GAIN) {
			PRINT(("NEW_GAIN\n"));
			group->MakeContinuousParameter(100 + MMC[i].id, B_MEDIA_RAW_AUDIO, "", B_MASTER_GAIN, 
				"dB", MMC[i].gain.min_gain, MMC[i].gain.max_gain, MMC[i].gain.granularity);
			
			if(i+1 <fDevice->MMCI.control_count && MMC[i+1].master == MMC[i].id && MMC[i+1].flags & B_MULTI_MIX_GAIN) {
				group->ParameterAt(nbParameters)->SetChannelCount(
					group->ParameterAt(nbParameters)->CountChannels() + 1);
				i++;
			}
			
			(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
			nbParameters++;
		} else if(MMC[i].flags & B_MULTI_MIX_ENABLE) {
			PRINT(("NEW_ENABLE\n"));
			if(MMC[i].string == S_MUTE)
				group->MakeDiscreteParameter(100 + MMC[i].id, B_MEDIA_RAW_AUDIO, childName, B_MUTE);
			else
				group->MakeDiscreteParameter(100 + MMC[i].id, B_MEDIA_RAW_AUDIO, childName, B_ENABLE);
			if(nbParameters>0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
		}
	}
}

void 
MultiAudioNode::ProcessMux(BDiscreteParameter *parameter, int32 index)
{
	CALLED();
	multi_mix_control		*parent = &fDevice->MMCI.controls[index];
	multi_mix_control		*MMC = fDevice->MMCI.controls;
	int32 	itemIndex = 0;
	for(int32 i=0; i<fDevice->MMCI.control_count; i++) {
		if(MMC[i].parent != parent->id)
			continue;
			
		const char* childName;
		if(MMC[i].string != S_null)
			childName = multi_string[MMC[i].string];
		else
			childName = MMC[i].name;	
			
		if(MMC[i].flags & B_MULTI_MIX_MUX_VALUE) {
			PRINT(("NEW_MUX_VALUE\n"));
			parameter->AddItem(itemIndex, childName);
			itemIndex++;
		}
	}
}

// -------------------------------------------------------- //
// MultiAudioNode specific functions
// -------------------------------------------------------- //

int32
MultiAudioNode::RunThread()
{
	CALLED();
	multi_buffer_info		MBI;//, oldMBI;
	MBI.info_size = sizeof(MBI);
	MBI._reserved_0 = 0;
	MBI._reserved_1 = 2;
	
	while ( 1 ) {
		//acquire buffer if any
		if ( acquire_sem_etc( fBuffer_free, 1, B_RELATIVE_TIMEOUT, 0 ) == B_BAD_SEM_ID ) {
			return B_OK;
		}
				
		//oldMBI = MBI;
		
		//send buffer
		fDevice->DoBufferExchange(&MBI);
		
		//PRINT(("MultiAudioNode::RunThread: buffer exchanged\n"));
		//PRINT(("MultiAudioNode::RunThread: played_real_time : %i\n", MBI.played_real_time));
		//PRINT(("MultiAudioNode::RunThread: played_frames_count : %i\n", MBI.played_frames_count));
		//PRINT(("MultiAudioNode::RunThread: buffer_cycle : %i\n", MBI.playback_buffer_cycle));

		node_input *input = NULL;
		
		for(int32 i=0; i<fInputs.CountItems(); i++) {
			input = (node_input *)fInputs.ItemAt(i);
			
			if((MBI._reserved_0 == input->fChannelId)
				&& (input->fOldMBI.playback_buffer_cycle != MBI.playback_buffer_cycle 
				|| fDevice->MBL.return_playback_buffers == 1)
				&& (input->fInput.source != media_source::null 
				|| input->fChannelId == 0)) {
				
				//PRINT(("playback_buffer_cycle ok input : %i %d\n", i, MBI.playback_buffer_cycle));
				
				input->fBufferCycle = (MBI.playback_buffer_cycle - 1 
					+ fDevice->MBL.return_playback_buffers) % fDevice->MBL.return_playback_buffers;
											
				// update the timesource
				if(input->fChannelId==0) {
					//PRINT(("updating timesource\n"));
					UpdateTimeSource(MBI, input->fOldMBI, *input);
				}
				
				input->fOldMBI = MBI;
				
				if(input->fBuffer!=NULL) {
					/*memcpy( fDevice->MBL.playback_buffers[input->fBufferCycle][input->fChannelId].base, 
						input->fBuffer->Data(), input->fBuffer->SizeUsed() );*/
					FillNextBuffer(*input, input->fBuffer);
					input->fBuffer->Recycle();
					input->fBuffer = NULL;
				} else {
					// put zeros in current buffer
					if(input->fInput.source != media_source::null)
						WriteZeros(*input, input->fBufferCycle);
					//PRINT(("MultiAudioNode::Runthread WriteZeros\n"));
				}
				
				//mark buffer free
				release_sem( fBuffer_free );
			} else {
				//PRINT(("playback_buffer_cycle non ok input : %i\n", i));
			}
		}
		
		node_output *output = NULL;
		for(int32 i=0; i<fOutputs.CountItems(); i++) {
			output = (node_output *)fOutputs.ItemAt(i);
			
			// make sure we're both started *and* connected before delivering a buffer		
			if ((RunState() == BMediaEventLooper::B_STARTED) && (output->fOutput.destination != media_destination::null)) {
				if((MBI._reserved_1 == output->fChannelId)
					&& (output->fOldMBI.record_buffer_cycle != MBI.record_buffer_cycle 
					|| fDevice->MBL.return_record_buffers == 1)) {
					//PRINT(("record_buffer_cycle ok\n"));
					
					// Get the next buffer of data
					BBuffer* buffer = FillNextBuffer(MBI, *output);
					if (buffer)
					{
						// send the buffer downstream if and only if output is enabled
						status_t err = B_ERROR;
						if (output->fOutputEnabled)
							err = SendBuffer(buffer, output->fOutput.destination);
						if (err) {
							buffer->Recycle();
						} else {
							// track how much media we've delivered so far
							size_t nSamples = output->fOutput.format.u.raw_audio.buffer_size
								/ output->fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK;
							output->fSamplesSent += nSamples;
						}
					}
					
					output->fOldMBI = MBI;
				} else {
					//PRINT(("record_buffer_cycle non ok\n"));
				}
			}
		}
	
	}
	
	return B_OK;
}

void
MultiAudioNode::WriteZeros(node_input &input, uint32 bufferCycle)
{
	//CALLED();
	/*int32 samples = input.fInput.format.u.raw_audio.buffer_size;
	if(input.fInput.format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_UCHAR) {
		uint8 *sample = (uint8*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].base;
		for(int32 i = samples-1; i>=0; i--)
			*sample++ = 128;
	
	} else {
		int32 *sample = (int32*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].base;
		for(int32 i = (samples / 4)-1; i>=0; i--)
			*sample++ = 0;*/
	
	size_t stride = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId].stride;
	uint32 channelCount = input.fFormat.u.raw_audio.channel_count;
	
	switch(input.fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			for(uint32 channel = 0; channel < channelCount; channel++) {
				char *sample_dest = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId + channel].base;
				for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
					*(float*)sample_dest = 0;
					sample_dest += stride;
				}
			}
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			for(uint32 channel = 0; channel < channelCount; channel++) {
				char *sample_dest = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId + channel].base;
				for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
					*(int32*)sample_dest = 0;
					sample_dest += stride;
				}
			}
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:{
			for(uint32 channel = 0; channel < channelCount; channel++) {
				char *sample_dest = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId + channel].base;
				for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
					*(int16*)sample_dest = 0;
					sample_dest += stride;
				}
			}
			break;
		}
		default:
			fprintf(stderr, "ERROR in WriteZeros format not handled\n");
	}
	
}

void
MultiAudioNode::FillWithZeros(node_input &input)
{
	CALLED();
	for(int32 i=0; i<fDevice->MBL.return_playback_buffers; i++)
		WriteZeros(input, i);
}

void
MultiAudioNode::FillNextBuffer(node_input &input, BBuffer* buffer)
{
	switch(input.fInput.format.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:

			switch(input.fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				if(input.fInput.format.u.raw_audio.channel_count == 2) {
					int16 *sample_dest1 = (int16*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].base;
					int16 *sample_dest2 = (int16*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].base;
					float *sample_src = (float*)buffer->Data();
					if(fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride == sizeof(int16)
						&& fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride == sizeof(int16)) {
						//PRINT(("FillNextBuffer : 2 channels strides 2\n"));
						for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
							*sample_dest1++ = int16(32767 * *sample_src++);
							*sample_dest2++ = int16(32767 * *sample_src++);
						}
					} else if(fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride == 2 * sizeof(int16)
						&& fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride == 2 * sizeof(int16)
						&& sample_dest1 + 1 == sample_dest2) {
							//PRINT(("FillNextBuffer : 2 channels strides 4\n"));
							for(uint32 i=2*fDevice->MBL.return_playback_buffer_size; i>0; i--)
								*sample_dest1++ = int16(32767 * *sample_src++);
						} else {
							//PRINT(("FillNextBuffer : 2 channels strides != 2\n"));
							size_t stride1 = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride / sizeof(int16);
							size_t stride2 = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride / sizeof(int16);
							for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
								*sample_dest1 = int16(32767 * *sample_src++);
								*sample_dest2 = int16(32767 * *sample_src++);
								sample_dest1 += stride1;
								sample_dest2 += stride2;
							}
						}
				} else {
					size_t frame_size = input.fInput.format.u.raw_audio.channel_count * sizeof(int16);
					size_t stride = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride;
					//PRINT(("stride : %i, frame_size : %i, return_playback_buffer_size : %i\n", stride, frame_size, fDevice->MBL.return_playback_buffer_size));
					for(uint32 channel = 0; channel < input.fInput.format.u.raw_audio.channel_count; channel++) {
						char *sample_dest = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + channel].base;
						//char *sample_src = (char*)buffer->Data() + (input.fInput.format.u.raw_audio.channel_count - 1 - channel) * sizeof(int16);
						char *sample_src = (char*)buffer->Data() + channel * sizeof(int16);
						for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
							*(int16*)sample_dest = int16(32767 * *(float*)sample_src);
							sample_dest += stride;
							sample_src += frame_size;
						}
					}
				}
				break;
			default:
				break;
			}

			break;
		case media_raw_audio_format::B_AUDIO_INT:
		
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
		
			switch(input.fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				if(input.fInput.format.u.raw_audio.channel_count == 2) {
					int16 *sample_dest1 = (int16*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].base;
					int16 *sample_dest2 = (int16*)fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].base;
					int16 *sample_src = (int16*)buffer->Data();
					if(fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride == sizeof(int16)
						&& fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride == sizeof(int16)) {
						//PRINT(("FillNextBuffer : 2 channels strides 2\n"));
						for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
							*sample_dest1++ = *sample_src++;
							*sample_dest2++ = *sample_src++;
						}
					} else if(fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride == 2 * sizeof(int16)
						&& fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride == 2 * sizeof(int16)
						&& sample_dest1 + 1 == sample_dest2) {
							//PRINT(("FillNextBuffer : 2 channels strides 4\n"));
							memcpy(sample_dest1, sample_src, fDevice->MBL.return_playback_buffer_size * 2 * sizeof(int16));
						} else {
							//PRINT(("FillNextBuffer : 2 channels strides != 2\n"));
							size_t stride1 = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride / 2;
							size_t stride2 = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + 1].stride / 2;
							for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
								*sample_dest1 = *sample_src++;
								*sample_dest2 = *sample_src++;
								sample_dest1 += stride1;
								sample_dest2 += stride2;
							}
						}
				} else {
					size_t frame_size = input.fInput.format.u.raw_audio.channel_count * sizeof(int16);
					size_t stride = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride;
					//PRINT(("stride : %i, frame_size : %i, return_playback_buffer_size : %i\n", stride, frame_size, fDevice->MBL.return_playback_buffer_size));
					for(uint32 channel = 0; channel < input.fInput.format.u.raw_audio.channel_count; channel++) {
						char *sample_dest = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + channel].base;
						//char *sample_src = (char*)buffer->Data() + (input.fInput.format.u.raw_audio.channel_count - 1 - channel) * sizeof(int16);
						char *sample_src = (char*)buffer->Data() + channel * sizeof(int16);
						for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
							*(int16*)sample_dest = *(int16*)sample_src;
							sample_dest += stride;
							sample_src += frame_size;
						}
					}
				}
				break;
			default:
				break;
			}
			
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR:
		default:
			break;	
	}
}

status_t
MultiAudioNode::StartThread()
{
	CALLED();
	// the thread is already started ?
	if(fThread > B_OK)
		return B_OK;
	
	//allocate buffer free semaphore
	fBuffer_free = create_sem( fDevice->MBL.return_playback_buffers - 1, "multi_audio out buffer free" );
	
	if ( fBuffer_free < B_OK ) {
		return B_ERROR;
	}

	fThread = spawn_thread( _run_thread_, "multi_audio audio output", B_REAL_TIME_PRIORITY, this );

	if ( fThread < B_OK ) {
		delete_sem( fBuffer_free );
		return B_ERROR;
	}

	resume_thread( fThread );
	
	return B_OK;
}

status_t
MultiAudioNode::StopThread()
{
	CALLED();
	delete_sem( fBuffer_free );
	wait_for_thread( fThread, &fThread );
	return B_OK;
}

void 
MultiAudioNode::AllocateBuffers(node_output &channel)
{
	CALLED();

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = channel.fOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(fLatency / BufferDuration() + 1 + 1);

	PRINT(("\tlatency = %Ld, buffer duration = %Ld\n", fLatency, BufferDuration()));
	PRINT(("\tcreating group of %ld buffers, size = %lu\n", count, size));
	channel.fBufferGroup = new BBufferGroup(size, count);
}

void
MultiAudioNode::UpdateTimeSource(multi_buffer_info &MBI, multi_buffer_info &oldMBI, node_input &input)
{
	//CALLED();
	if(fTimeSourceStarted) {
		bigtime_t perf_time = (bigtime_t)(MBI.played_frames_count / 
						input.fInput.format.u.raw_audio.frame_rate * 1000000);
		bigtime_t real_time = MBI.played_real_time;
		float drift = ((MBI.played_frames_count - oldMBI.played_frames_count)
						/ input.fInput.format.u.raw_audio.frame_rate * 1000000)
						/ (MBI.played_real_time - oldMBI.played_real_time);
	
		PublishTime(perf_time, real_time, drift);
		//PRINT(("UpdateTimeSource() perf_time : %lli, real_time : %lli, drift : %f\n", perf_time, real_time, drift));
	}
}

BBuffer*
MultiAudioNode::FillNextBuffer(multi_buffer_info &MBI, node_output &channel)
{
	//CALLED();
	// get a buffer from our buffer group
	//PRINT(("buffer size : %i, buffer duration : %i\n", fOutput.format.u.raw_audio.buffer_size, BufferDuration()));
	//PRINT(("MBI.record_buffer_cycle : %i\n", MBI.record_buffer_cycle));
	//PRINT(("MBI.recorded_real_time : %i\n", MBI.recorded_real_time));
	//PRINT(("MBI.recorded_frames_count : %i\n", MBI.recorded_frames_count));
	if(!channel.fBufferGroup)
		return NULL;
	
	BBuffer* buffer = channel.fBufferGroup->RequestBuffer(channel.fOutput.format.u.raw_audio.buffer_size, BufferDuration());

	// if we fail to get a buffer (for example, if the request times out), we skip this
	// buffer and go on to the next, to avoid locking up the control thread
	if (!buffer)
		return NULL;
	
	if(fDevice==NULL)
		fprintf(stderr,"fDevice NULL\n");
	if(buffer->Header()==NULL)
		fprintf(stderr,"buffer->Header() NULL\n");
	if(TimeSource()==NULL)
		fprintf(stderr,"TimeSource() NULL\n");
	
	// now fill it with data, continuing where the last buffer left off
	memcpy( buffer->Data(), 
		fDevice->MBL.record_buffers[MBI.record_buffer_cycle][channel.fChannelId - fDevice->MD.output_channel_count].base, 
		channel.fOutput.format.u.raw_audio.buffer_size );
		
	// fill in the buffer header
	media_header* hdr = buffer->Header();
	hdr->type = B_MEDIA_RAW_AUDIO;
	hdr->size_used = channel.fOutput.format.u.raw_audio.buffer_size;
	hdr->time_source = TimeSource()->ID();
	hdr->start_time = PerformanceTimeFor(MBI.recorded_real_time);
	
	return buffer;
}

status_t
MultiAudioNode::GetConfigurationFor(BMessage * into_message)
{
	CALLED();
	
	BParameter *parameter = NULL;
	void *buffer;
	size_t size = 128;
	bigtime_t last_change;
	status_t err;
	
	if(!into_message)
		return B_BAD_VALUE;
	
	buffer = malloc(size);
	
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER
			&& parameter->Type() != BParameter::B_DISCRETE_PARAMETER)
			continue;
			
		PRINT(("getting parameter %i\n", parameter->ID()));
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
			PRINT(("parameter err : %s\n", strerror(err)));
		}
	}
	
	PRINT_OBJECT(*into_message);
	
	return B_OK;
}

node_output*
MultiAudioNode::FindOutput(media_source source)
{
	node_output *channel = NULL;

	for(int32 i=0; i<fOutputs.CountItems(); i++) {
		channel = (node_output *)fOutputs.ItemAt(i);
		if(source == channel->fOutput.source)
			break;
	}
	
	if (source != channel->fOutput.source)
		return NULL;
	else
		return channel;
	
}

node_input*
MultiAudioNode::FindInput(media_destination dest)
{
	node_input *channel = NULL;

	for(int32 i=0; i<fInputs.CountItems(); i++) {
		channel = (node_input *)fInputs.ItemAt(i);
		if(dest == channel->fInput.destination)
			break;
	}
	
	if (dest != channel->fInput.destination)
		return NULL;
	else
		return channel;
	
}

node_input*
MultiAudioNode::FindInput(int32 destinationId)
{
	node_input *channel = NULL;

	for(int32 i=0; i<fInputs.CountItems(); i++) {
		channel = (node_input *)fInputs.ItemAt(i);
		if(destinationId == channel->fInput.destination.id)
			break;
	}
	
	if (destinationId != channel->fInput.destination.id)
		return NULL;
	else
		return channel;
	
}

// static:

int32
MultiAudioNode::_run_thread_( void *data )
{
	CALLED();
	return static_cast<MultiAudioNode *>( data )->RunThread();
}

void MultiAudioNode::GetFlavor(flavor_info * outInfo, int32 id)
{
	CALLED();
	if (outInfo == 0) {
		return;
	}

	outInfo->flavor_flags = 0;
	outInfo->possible_count = 1;	// one flavor at a time
	outInfo->in_format_count = 0; // no inputs
	outInfo->in_formats = 0;
	outInfo->out_format_count = 0; // no outputs
	outInfo->out_formats = 0;
	outInfo->internal_id = id;
	
	outInfo->name = "MultiAudioNode Node";
	outInfo->info = "The MultiAudioNode node outputs to multi_audio drivers.";
	outInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_TIME_SOURCE 
						| B_PHYSICAL_OUTPUT | B_PHYSICAL_INPUT | B_CONTROLLABLE;
	outInfo->in_format_count = 1; // 1 input
	media_format * informats = new media_format[outInfo->in_format_count];
	GetFormat(&informats[0]);
	outInfo->in_formats = informats;
	
	outInfo->out_format_count = 1; // 1 output
	media_format * outformats = new media_format[outInfo->out_format_count];
	GetFormat(&outformats[0]);
	outInfo->out_formats = outformats;
}

void MultiAudioNode::GetFormat(media_format * outFormat)
{
	CALLED();
	if (outFormat == 0) {
		return;
	}
	outFormat->type = B_MEDIA_RAW_AUDIO;
	outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
	outFormat->u.raw_audio = media_raw_audio_format::wildcard;
}
