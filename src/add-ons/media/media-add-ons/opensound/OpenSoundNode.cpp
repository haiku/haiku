/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 * 
 * Based on MultiAudio media addon
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
#include <Debug.h>
#include <Autolock.h>
#include <String.h>

#include "OpenSoundNode.h"
#include "driver_io.h"
#ifdef DEBUG
  #define PRINTING
#endif
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
	fEngineId = -1;
	fRealEngine = NULL;
	fAFmt = 0;
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
	fEngineId = -1;
	fRealEngine = NULL;
	fAFmt = 0;
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

OpenSoundNode::~OpenSoundNode(void)
{
	CALLED();
	fAddOn->GetConfigurationFor(this, NULL);
	
	StopThread();
	BMediaEventLooper::Quit();
		
	fWeb = NULL;	
}

OpenSoundNode::OpenSoundNode(BMediaAddOn *addon, char* name, OpenSoundDevice *device, 
	int32 internal_id, BMessage * config)
	: BMediaNode(name),
	  BBufferConsumer(B_MEDIA_RAW_AUDIO),
	  BBufferProducer(B_MEDIA_RAW_AUDIO),
	  BTimeSource(),
	  BMediaEventLooper(),
	  fThread(-1),
	  fDevice(device),
	  fTimeSourceStarted(false),
	  fOldPlayedFramesCount(0LL),
	  fOldPlayedRealTime(0LL),
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
	//XXX: this should go away! should use engine's preferred for each afmt.
#if 1
	memset(&fPreferredFormat, 0, sizeof(fPreferredFormat)); // set everything to wildcard first
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio = media_multi_audio_format::wildcard;
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	OpenSoundDeviceEngine *engine = fDevice->EngineAt(0);
	if (engine) {
		const oss_audioinfo *ai = engine->Info();
		int fmt = OpenSoundDevice::select_oss_format(ai->oformats);
		fPreferredFormat.u.raw_audio.format = OpenSoundDevice::convert_oss_format_to_media_format(fmt);
		fPreferredFormat.u.raw_audio.valid_bits = OpenSoundDevice::convert_oss_format_to_valid_bits(fmt);
		//XXX:engine->PreferredChannels() ? (caps & DSP_CH*)
		fPreferredFormat.u.raw_audio.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(ai->max_rate);		// measured in Hertz
	}

	// we'll use the consumer's preferred buffer size, if any
#if MA
	fPreferredFormat.u.raw_audio.buffer_size = fDevice->MBL.return_record_buffer_size 
						* (fPreferredFormat.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* fPreferredFormat.u.raw_audio.channel_count;
#endif
	//XXX: SNDCTL_DSP_GETBLKSIZE ?
	fPreferredFormat.u.raw_audio.buffer_size = DEFAULT_BUFFER_SIZE 
						* (fPreferredFormat.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* fPreferredFormat.u.raw_audio.channel_count;
#endif
	
	if(config) {
		PRINT_OBJECT(*config);
	}
		
	fInitCheckStatus = B_OK;
}

status_t OpenSoundNode::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * OpenSoundNode::AddOn(
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

void OpenSoundNode::Preroll(void)
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t OpenSoundNode::HandleMessage(
				int32 message,
				const void * data,
				size_t size)
{
	CALLED();
	return B_ERROR;
}

void OpenSoundNode::NodeRegistered(void)
{
	status_t err;
	int i, f, fmt;
	CALLED();
	
	if (fInitCheckStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}
	
	SetPriority(B_REAL_TIME_PRIORITY);
	
	Run();
	
	node_input *currentInput = NULL;
	PRINT(("NodeRegistered: %d engines\n", fDevice->CountEngines()));
	for (i = 0; i < fDevice->CountEngines(); i++) {
		OpenSoundDeviceEngine *engine = fDevice->EngineAt(i);
		if (!engine)
			continue;
		// skip shadow engines
		if (engine->Caps() & PCM_CAP_SHADOW)
			continue;
		// skip engines that don't have outputs
		if ((engine->Caps() & PCM_CAP_OUTPUT) == 0)
			continue;
		PRINT(("NodeRegistered: engine[%d]: .caps=0x%08x, .oformats=0x%08x\n", i, engine->Caps(), engine->Info()->oformats));
		for (f = 0; gSupportedFormats[f]; f++) {
			fmt = gSupportedFormats[f] & engine->Info()->oformats;
			if (fmt == 0)
				continue;
			PRINT(("NodeRegistered() : creating an input for engine %i, format[%i]\n", i, f));
			
			media_format preferredFormat;
			err = engine->PreferredFormatFor(fmt, preferredFormat);
			if (err < B_OK)
				continue;

			media_input *input = new media_input;
			
			//input->format = fPreferredFormat;//XXX
			input->format = preferredFormat;
			input->destination.port = ControlPort();
			input->destination.id = fInputs.CountItems();
			input->node = Node();
			char *prefix = "";
			if (strstr(engine->Info()->name, "SPDIF"))
				prefix = "S/PDIF ";
			sprintf(input->name, "%s%s output %ld", prefix, gSupportedFormatsNames[f], input->destination.id);
					
			currentInput = new node_input(*input, preferredFormat);
			//currentInput->fPreferredFormat.u.raw_audio.channel_count = engine->Info()->max_channels;
			currentInput->fInput.format = currentInput->fPreferredFormat;
			currentInput->fChannelId = i;//XXX:REMOVEME fDevice->MD.channels[i].channel_id;
			currentInput->fEngineId = i;
			currentInput->fAFmt = fmt;
			fInputs.AddItem(currentInput);
			
			currentInput->fInput.format.u.raw_audio.format = media_raw_audio_format::wildcard.format;
		}
	}
	
	node_output *currentOutput = NULL;
	
	for (i = 0; i < fDevice->CountEngines(); i++) {
		OpenSoundDeviceEngine *engine = fDevice->EngineAt(i);
		if (!engine)
			continue;
		// skip shadow engines
		if (engine->Caps() & PCM_CAP_SHADOW)
			continue;
		// skip engines that don't have inputs
		if ((engine->Caps() & PCM_CAP_INPUT) == 0)
			continue;
		PRINT(("NodeRegistered: engine[%d]: .caps=0x%08x, .iformats=0x%08x\n", i, engine->Caps(), engine->Info()->iformats));
		for (f = 0; gSupportedFormats[f]; f++) {
			fmt = gSupportedFormats[f] & engine->Info()->iformats;
			if (fmt == 0)
				continue;
			PRINT(("NodeRegistered() : creating an output for engine %i, format[%i]\n", i, f));
			
			media_format preferredFormat;
			err = engine->PreferredFormatFor(fmt, preferredFormat);
			if (err < B_OK)
				continue;

			media_output *output = new media_output;

			//output->format = fPreferredFormat; //XXX
			output->format = preferredFormat;
			output->destination = media_destination::null;
			output->source.port = ControlPort();
			output->source.id = fOutputs.CountItems();
			output->node = Node();
			char *prefix = "";
			if (strstr(engine->Info()->name, "SPDIF"))
				prefix = "S/PDIF ";
			sprintf(output->name, "%s%s input %ld", prefix, gSupportedFormatsNames[f], output->source.id);
					
			currentOutput = new node_output(*output, preferredFormat);
			//currentOutput->fPreferredFormat.u.raw_audio.channel_count = engine->Info()->max_channels;
			currentOutput->fOutput.format = currentOutput->fPreferredFormat;
			currentOutput->fChannelId = i;//XXX:REMOVEME fDevice->MD.channels[i].channel_id;
			currentOutput->fEngineId = i;
			currentOutput->fAFmt = fmt;
			fOutputs.AddItem(currentOutput);
		}
	}
		
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
	
	PRINT(("apply configuration in : %ld\n", system_time() - start));
}

status_t OpenSoundNode::RequestCompleted(const media_request_info &info)
{
	CALLED();
	return B_OK;
}

void OpenSoundNode::SetTimeSource(BTimeSource *timeSource)
{
	CALLED();
}

// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t OpenSoundNode::AcceptFormat(
				const media_destination & dest,
				media_format * format)
{
	status_t err;
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
	
	BAutolock L(fDevice->Locker());
	
	OpenSoundDeviceEngine *engine = fDevice->NextFreeEngineAt(channel->fEngineId, false);
	if (!engine) {
		return B_BUSY;
	}
	
	err = engine->AcceptFormatFor(channel->fAFmt, *format, false);
	if (err < B_OK)
		return err;
	
	channel->fRealEngine = engine;
	
/*
	if ( format->type != B_MEDIA_RAW_AUDIO ) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}	
*/
	
	//channel->fFormat = channel->fPreferredFormat;
	channel->fFormat = *format;
	
	/*if(format->u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
		&& channel->fPreferredFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT)
		format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	else*/ 
/*
	format->u.raw_audio.format = channel->fPreferredFormat.u.raw_audio.format;
	format->u.raw_audio.valid_bits = channel->fPreferredFormat.u.raw_audio.valid_bits;
	
	format->u.raw_audio.frame_rate    = channel->fPreferredFormat.u.raw_audio.frame_rate;
	format->u.raw_audio.channel_count = channel->fPreferredFormat.u.raw_audio.channel_count;
	format->u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;

	format->u.raw_audio.buffer_size   = DEFAULT_BUFFER_SIZE 
						* (format->u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* format->u.raw_audio.channel_count;
*/	
	
	/*media_format myFormat;
	GetFormat(&myFormat);
	if (!format_is_acceptible(*format,myFormat)) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}*/
	//AddRequirements(format);
	return B_OK;	
}

status_t OpenSoundNode::GetNextInput(
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

void OpenSoundNode::DisposeInputCookie(
				int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
}

void OpenSoundNode::BufferReceived(
				BBuffer * buffer)
{
	/**/CALLED();
	switch (buffer->Header()->type) {
		/*case B_MEDIA_PARAMETERS:
			{
			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
			if (status != B_OK) {
				fprintf(stderr,"ApplyParameterData in OpenSoundNode::BufferReceived failed\n");
			}			
			buffer->Recycle();
			}
			break;*/
		case B_MEDIA_RAW_AUDIO:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in OpenSoundNode::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in OpenSoundNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in OpenSoundNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void OpenSoundNode::ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	/**/CALLED();
	
	node_input *channel = FindInput(for_whom);
	
	if(channel==NULL) {
		fprintf(stderr,"invalid destination received in OpenSoundNode::ProducerDataStatus\n");
		return;
	}
	
	//PRINT(("************ ProducerDataStatus: queuing event ************\n"));
	//PRINT(("************ status=%d ************\n", status));
	
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&channel->fInput, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t OpenSoundNode::GetLatencyFor(
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

status_t OpenSoundNode::Connected(
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
	
	BAutolock L(fDevice->Locker());
	
	// use one buffer length latency
	fInternalLatency = with_format.u.raw_audio.buffer_size * 10000 / 2
			/ ( (with_format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
				* with_format.u.raw_audio.channel_count) 
			/ ((int32)(with_format.u.raw_audio.frame_rate / 100));
			
	PRINT(("  internal latency = %lld\n",fInternalLatency));
	//fInternalLatency = 0;
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	channel->fInput.source = producer;
	channel->fInput.format = with_format;
	*out_input = channel->fInput;
	
	// we are sure the thread is started
	StartThread();
	
	return B_OK;
}

void OpenSoundNode::Disconnected(
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
	if (channel->fRealEngine)
		channel->fRealEngine->Close();
	channel->fRealEngine = NULL;
	FillWithZeros(*channel);
	//GetFormat(&channel->fInput.format);
}

	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
status_t OpenSoundNode::FormatChanged(
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
status_t OpenSoundNode::SeekTagRequested(
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
OpenSoundNode::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
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
OpenSoundNode::FormatProposal(const media_source& output, media_format* format)
{
	status_t err;
	// FormatProposal() is the first stage in the BMediaRoster::Connect() process.  We hand
	// out a suggested format, with wildcards for any variations we support.
	CALLED();
	node_output *channel = FindOutput(output);
	
	// is this a proposal for our select output?
	if (channel == NULL)
	{
		fprintf(stderr, "OpenSoundNode::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	media_type requestedType = format->type;
#ifdef ENABLE_REC
	OpenSoundDeviceEngine *engine = fDevice->NextFreeEngineAt(channel->fEngineId, true);

	// we only support floating-point raw audio, so we always return that, but we
	// supply an error code depending on whether we found the proposal acceptable.
	err = engine->PreferredFormatFor(channel->fAFmt, *format, true);
	if (err < B_OK)
		return err;
#else
	*format = fPreferredFormat;
#endif
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO)
#ifdef ENABLE_NON_RAW_SUPPORT
		 && (requestedType != B_MEDIA_ENCODED_AUDIO)
#endif
		)
	{
		fprintf(stderr, "OpenSoundNode::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else return B_OK;		// raw audio or wildcard type, either is okay by us
}

status_t 
OpenSoundNode::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}

status_t 
OpenSoundNode::GetNextOutput(int32* cookie, media_output* out_output)
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
OpenSoundNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}

status_t 
OpenSoundNode::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	CALLED();

	node_output *channel = FindOutput(for_source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "OpenSoundNode::SetBufferGroup returning B_MEDIA_BAD_SOURCE\n");
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
OpenSoundNode::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!
	CALLED();

	status_t err;
	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "OpenSoundNode::PrepareToConnect returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (channel->fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	BAutolock L(fDevice->Locker());

	OpenSoundDeviceEngine *engine = fDevice->NextFreeEngineAt(channel->fEngineId, false);
	if (!engine) {
		return B_BUSY;
	}

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO) //XXX:NONRAW
	{
		fprintf(stderr, "\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}
	
	 // !!! validate all other fields except for buffer_size here, because the consumer might have
	// supplied different values from AcceptFormat()?
	err = engine->SpecializeFormatFor(channel->fAFmt, *format, true);
	if (err < B_OK)
		return err;
	
	channel->fRealEngine = engine;

#if 0
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
#endif

	// Now reserve the connection, and return information about it
	channel->fOutput.destination = where;
	channel->fOutput.format = *format;
	*out_source = channel->fOutput.source;
	strncpy(out_name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);
	return B_OK;
}

void 
OpenSoundNode::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	CALLED();
	
	node_output *channel = FindOutput(source);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "OpenSoundNode::Connect returning (cause : B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	OpenSoundDeviceEngine *engine = channel->fRealEngine;
	if (engine == NULL)
		return;
	
	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	if (error)
	{
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		engine->Close();
		channel->fRealEngine = NULL;
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
	
	engine->StartRecording();
	
	// we are sure the thread is started
	StartThread();
}

void 
OpenSoundNode::Disconnect(const media_source& what, const media_destination& where)
{
	CALLED();
	
	node_output *channel = FindOutput(what);
	
	// is this our output?
	if (channel == NULL)
	{
		fprintf(stderr, "OpenSoundNode::Disconnect() returning (cause : B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	BAutolock L(fDevice->Locker());
	
	OpenSoundDeviceEngine *engine = channel->fRealEngine;
	
	// Make sure that our connection is the one being disconnected
	if ((where == channel->fOutput.destination) && (what == channel->fOutput.source))
	{
		if (engine)
			engine->Close();
		channel->fRealEngine = NULL;
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
OpenSoundNode::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
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
OpenSoundNode::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
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
OpenSoundNode::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void OpenSoundNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent)
{
	/**/CALLED();
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
			//PRINT(("HandleEvent: B_HANDLE_BUFFER, RunState= %d\n", RunState()));
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
status_t OpenSoundNode::HandleBuffer(
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
			PRINT(("OpenSoundNode::HandleBuffer snoozing recycling channelId : %i, how_early:%lli\n", channel->fChannelId, how_early));
			//channel->fBuffer->Recycle();
			snooze(100);
			if(channel->fBuffer != NULL)
				buffer->Recycle();
			else
				channel->fBuffer = buffer;
		} else {
			//PRINT(("OpenSoundNode::HandleBuffer writing channelId : %i, how_early:%lli\n", channel->fChannelId, how_early));
			channel->fBuffer = buffer;
		}
	}
	return B_OK;
}

status_t OpenSoundNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	//CALLED();
	PRINT(("OpenSoundNode::HandleDataStatus status:%li, lateness:%li\n", event->data, lateness));
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

status_t OpenSoundNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	if (RunState() != B_STARTED) {
	
	}
	return B_OK;
}

status_t OpenSoundNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	PRINT(("OpenSoundNode::HandleSeek(t=%lld,d=%li,bd=%lld)\n",event->event_time,event->data,event->bigdata));
	return B_OK;
}
						
status_t OpenSoundNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent)
{
	CALLED();
	return B_OK;
}

status_t OpenSoundNode::HandleStop(
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

status_t OpenSoundNode::HandleParameter(
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

void
OpenSoundNode::SetRunMode(run_mode mode)
{
	CALLED();
	PRINT(("OpenSoundNode::SetRunMode mode:%i\n", mode));
	//BTimeSource::SetRunMode(mode);
}

status_t
OpenSoundNode::TimeSourceOp(const time_source_op_info &op, void *_reserved)
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
OpenSoundNode::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* ioSize)
{
	oss_mixext mixext;
	oss_mixer_value mixval;
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	status_t err;
	int i, chans = 1;
	int sliderShift = 8;
	CALLED();
	
	if (!mixer)
		return ENODEV;
	
	PRINT(("id : %i, *ioSize=%d\n", id, *ioSize));
	
	err = mixer->GetExtInfo(id, &mixext);
	if (err < B_OK)
		return err;
	mixval.ctrl = mixext.ctrl;
	mixval.timestamp = mixext.timestamp;
	
	err = mixer->GetMixerValue(&mixval);
	if (err < B_OK)
		return err;
	
	if (!(mixext.flags & MIXF_READABLE))
		return EINVAL;

	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}

	if (!parameter)
		return ENODEV;

	PRINT(("%s: value = 0x%08x\n", __FUNCTION__, mixval.value));

	*last_change = system_time();//??
	
	switch (mixext.type) {
	case MIXT_DEVROOT:
	case MIXT_GROUP:
		break;
	case MIXT_ONOFF:
		if (*ioSize < sizeof(bool))
			return EINVAL;
		*(int32 *)value = mixval.value?true:false;
		*ioSize = sizeof(bool);
		return B_OK;
	case MIXT_ENUM:
		if (*ioSize < sizeof(int32))
			return EINVAL;
		*(int32 *)value = mixval.value;
		*ioSize = sizeof(int32);
		return B_OK;
		break;
	case MIXT_STEREODB:
	case MIXT_STEREOSLIDER16:
	case MIXT_STEREOSLIDER:
		chans = 2;
	case MIXT_SLIDER:
	case MIXT_MONODB:
	case MIXT_MONOSLIDER16:
	case MIXT_MONOSLIDER:
		if (*ioSize < chans * sizeof(float))
			return EINVAL;
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
			return EINVAL;
		if (mixext.type == MIXT_STEREOSLIDER16 || 
			mixext.type == MIXT_MONOSLIDER16)
			sliderShift = 16;
		*ioSize = chans * sizeof(float);
		((float *)value)[0] = (float)(mixval.value & ((1 << sliderShift) - 1));
		PRINT(("%s: value[O] = %f\n", __FUNCTION__, ((float *)value)[0]));
		if (chans < 2)
			return B_OK;
		((float *)value)[1] = (float)((mixval.value >> sliderShift) & ((1 << sliderShift) - 1));
		PRINT(("%s: value[1] = %f\n", __FUNCTION__, ((float *)value)[1]));
		return B_OK;
		break;
	case MIXT_MESSAGE:
		break;
	case MIXT_MONOVU:
		break;
	case MIXT_STEREOVU:
		break;
	case MIXT_MONOPEAK:
		break;
	case MIXT_STEREOPEAK:
		break;
	case MIXT_RADIOGROUP:
		break;//??
	case MIXT_MARKER:
		break;// separator item: ignore
	case MIXT_VALUE:
		break;
	case MIXT_HEXVALUE:
		break;
/*	case MIXT_MONODB:
		break;
	case MIXT_STEREODB:
		break;*/
	case MIXT_3D:
		break;
/*	case MIXT_MONOSLIDER16:
		break;
	case MIXT_STEREOSLIDER16:
		break;*/
	default:
		PRINT(("OpenSoundNode::%s: unknown mixer control type %d\n", __FUNCTION__, mixext.type));
	}
	*ioSize = 0;
	return EINVAL;
#if MA
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
#endif
}

void 
OpenSoundNode::SetParameterValue(int32 id, bigtime_t performance_time, const void* value, size_t size)
{
	oss_mixext mixext;
	oss_mixer_value mixval;
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	status_t err;
	int i, chans = 1;
	int sliderShift = 8;
	CALLED();
	PRINT(("id : %i, performance_time : %lld, size : %i\n", id, performance_time, size));
	
	if (!mixer)
		return;
	
	if (mixer->GetExtInfo(id, &mixext) < B_OK)
		return;

	if (!(mixext.flags & MIXF_WRITEABLE))
		return;
	mixval.ctrl = mixext.ctrl;
	mixval.timestamp = mixext.timestamp;
	
	err = mixer->GetMixerValue(&mixval);
	if (err < B_OK)
		return;

	mixval.ctrl = mixext.ctrl;
	mixval.timestamp = mixext.timestamp;
	
	BParameter *parameter = NULL;
	for(int32 i=0; i<fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}

	if (!parameter)
		return;

	switch (mixext.type) {
	case MIXT_DEVROOT:
	case MIXT_GROUP:
		break;
	case MIXT_ONOFF:
		if (size < sizeof(bool))
			return;
		mixval.value = (int)*(int32 *)value;
		mixer->SetMixerValue(&mixval);
		// At least on my ATI IXP, recording selection can't be set to OFF,
		// you have to set another one to ON to actually do it,
		// and setting to ON changes others to OFF
		// So we have to let users know about it.
		// XXX: find something better, doesn't work correctly here.
		// XXX: try a timed event ?
		PropagateParameterChanges(mixext.ctrl, mixext.type, mixext.id);
		
		return;
	case MIXT_ENUM:
		if (size < sizeof(int32))
			return;
		mixval.value = (int)*(int32 *)value;
		mixer->SetMixerValue(&mixval);
		break;
	case MIXT_STEREODB:
	case MIXT_STEREOSLIDER16:
	case MIXT_STEREOSLIDER:
		chans = 2;
	case MIXT_SLIDER:
	case MIXT_MONODB:
	case MIXT_MONOSLIDER16:
	case MIXT_MONOSLIDER:
		if (size < chans * sizeof(float))
			return;
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
			return;
		if (mixext.type == MIXT_STEREOSLIDER16 || 
			mixext.type == MIXT_MONOSLIDER16)
			sliderShift = 16;
		mixval.value = 0;
		PRINT(("-------- sliderShift=%d, v = %08x, v & %08x = %08x\n", sliderShift, mixval.value, ((1 << sliderShift) - 1), mixval.value & ((1 << sliderShift) - 1)));
		mixval.value |= ((int)(((float *)value)[0])) & ((1 << sliderShift) - 1);
		if (chans > 1) {
			mixval.value |= (((int)(((float *)value)[1])) & ((1 << sliderShift) - 1)) << sliderShift;
		}
		PRINT(("%s: value = 0x%08x\n", __FUNCTION__, mixval.value));
		mixer->SetMixerValue(&mixval);
		return;
		break;
	case MIXT_MESSAGE:
		break;
	case MIXT_MONOVU:
		break;
	case MIXT_STEREOVU:
		break;
	case MIXT_MONOPEAK:
		break;
	case MIXT_STEREOPEAK:
		break;
	case MIXT_RADIOGROUP:
		break;//??
	case MIXT_MARKER:
		break;// separator item: ignore
	case MIXT_VALUE:
		break;
	case MIXT_HEXVALUE:
		break;
/*	case MIXT_MONODB:
		break;
	case MIXT_STEREODB:
		break;*/
	case MIXT_3D:
		break;
/*	case MIXT_MONOSLIDER16:
		break;
	case MIXT_STEREOSLIDER16:
		break;*/
	default:
		PRINT(("OpenSoundNode::%s: unknown mixer control type %d\n", __FUNCTION__, mixext.type));
	}
	
	return;
#if MA
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
	
#endif
}

BParameterWeb* 
OpenSoundNode::MakeParameterWeb()
{
	CALLED();
	BParameterWeb* web = new BParameterWeb;
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	oss_mixext mixext;
	int i, mixext_count;
	
	// XXX: TODO: the card might change the mixer controls at some point, 
	// we should detect it (poll) and recreate the parameter web and
	// re-set it. 
	
	// TODO: cache mixext[...] and poll for changes in their update_counter.
	
	// some cards don't have a mixer, just put a placeholder then
	if (!mixer) {
		BParameterGroup *child = web->MakeGroup("No mixer");
		child->MakeNullParameter(1, B_MEDIA_UNKNOWN_TYPE, "No Mixer", B_GENERIC);
		return web;
	}
	
	// 
	mixext_count = mixer->CountExtInfos();
	PRINT(("OpenSoundNode::MakeParameterWeb %i ExtInfos\n", mixext_count));

	for(i = 0; i < mixext_count; i++) {
		if (mixer->GetExtInfo(i, &mixext) < B_OK)
			continue;
		
		if (mixext.type == MIXT_DEVROOT) {
				oss_mixext_root *extroot = (oss_mixext_root *)mixext.data;
				PRINT(("OpenSoundNode: mixext[%d]: ROOT\n", i));
				int32 nb = 0;
				const char* childName = mixext.extname;
				childName = extroot->id;//extroot->name;
				BParameterGroup *child = web->MakeGroup(childName);
				ProcessGroup(child, i, nb);
		}
	}

	return web;
}

void 
OpenSoundNode::ProcessGroup(BParameterGroup *group, int32 index, int32 &nbParameters)
{
	CALLED();
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	oss_mixext mixext;
	int i, mixext_count;

	mixext_count = mixer->CountExtInfos();
	
	for(i = 0; i < mixext_count; i++) {
		if (mixer->GetExtInfo(i, &mixext) < B_OK)
			continue;
		// only keep direct children of that group
		if (mixext.parent != index)
			continue;
		
		const char *childName = mixext.extname;
		const char *sliderUnit = "";//"(linear)";
		const char *continuousKind = B_GAIN;
		
		int32 nb = 1;
		BParameterGroup *child;
		
		PRINT(("OpenSoundNode: mixext[%d]: { %s/%s, type=%d, parent=%d, min=%d, max=%d, flags=0x%08x, control_no=%d, desc=%d, update_counter=%d }\n",
			i, (mixext.type != MIXT_MARKER)?mixext.id:"", 
			(mixext.type != MIXT_MARKER)?mixext.extname:"", 
			mixext.type, mixext.parent, 
			mixext.minvalue, mixext.maxvalue, 
			mixext.flags, mixext.control_no, 
			mixext.desc, mixext.update_counter));
		
		// should actually rename the whole group but it's too late there.
		if (mixext.flags & MIXF_MAINVOL)
			childName = "Master Gain";
		
		if (mixext.flags & MIXF_HZ)
			sliderUnit = "Hz";
		
		switch (mixext.type) {
		case MIXT_DEVROOT:
			// root item, should be done already
			break;
		case MIXT_GROUP:
			PRINT(("OpenSoundNode: mixext[%d]: GROUP\n", i));
			child = group->MakeGroup(childName);
			child->MakeNullParameter(i, B_MEDIA_RAW_AUDIO, childName, B_WEB_BUFFER_OUTPUT);
			ProcessGroup(child, i, nb);
			break;
		case MIXT_ONOFF:
			PRINT(("OpenSoundNode: mixext[%d]: ONOFF\n", i));
			// multiaudio node adds 100 to IDs !?
			if(0/*MMC[i].string == S_MUTE*/)
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName, B_MUTE);
			else
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName, B_ENABLE);
			if(nbParameters>0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
			break;
		case MIXT_ENUM:
		{
			PRINT(("OpenSoundNode: mixext[%d]: ENUM\n", i));
			BDiscreteParameter *parameter = 
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName, B_INPUT_MUX);
			if(nbParameters>0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
			ProcessMux(parameter, i);
		}	
			break;
		case MIXT_MONODB:
		case MIXT_STEREODB:
			sliderUnit = "dB";
		case MIXT_SLIDER:
		case MIXT_MONOSLIDER16:
		case MIXT_STEREOSLIDER16:
		case MIXT_MONOSLIDER:
			//PRINT(("OpenSoundNode: mixext[%d]: MONOSLIDER\n", i));
			//break;
			// fall through
		case MIXT_STEREOSLIDER:
			PRINT(("OpenSoundNode: mixext[%d]: [MONO|STEREO]SLIDER\n", i));
			
			if (mixext.flags & MIXF_MAINVOL)
				continuousKind = B_MASTER_GAIN;

			if (mixext.flags & MIXF_CENTIBEL)
				true;//step size
			if (mixext.flags & MIXF_DECIBEL)
				true;//step size
			
			group->MakeContinuousParameter(i, B_MEDIA_RAW_AUDIO, childName, continuousKind, 
				sliderUnit, mixext.minvalue, mixext.maxvalue, /*MMC[i].gain.granularity*/1);
			
			if (mixext.type == MIXT_STEREOSLIDER || 
				mixext.type == MIXT_STEREOSLIDER16 || 
				mixext.type == MIXT_STEREODB)
				group->ParameterAt(nbParameters)->SetChannelCount(2);
			
			PRINT(("nb parameters : %d\n", nbParameters));
			if (nbParameters > 0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
			
			break;
		case MIXT_MESSAGE:
			break;
		case MIXT_MONOVU:
			break;
		case MIXT_STEREOVU:
			break;
		case MIXT_MONOPEAK:
			break;
		case MIXT_STEREOPEAK:
			break;
		case MIXT_RADIOGROUP:
			break;//??
		case MIXT_MARKER:
			break;// separator item: ignore
		case MIXT_VALUE:
			break;
		case MIXT_HEXVALUE:
			break;
/*		case MIXT_MONODB:
			PRINT(("OpenSoundNode::ProcessGroup: Skipping obsolete MIXT_MONODB\n"));
			break;
		case MIXT_STEREODB:
			PRINT(("OpenSoundNode::ProcessGroup: Skipping obsolete MIXT_STEREODB\n"));
			break;*/
/*		case MIXT_SLIDER:
			break;*/
		case MIXT_3D:
			break;
/*		case MIXT_MONOSLIDER16:
			break;
		case MIXT_STEREOSLIDER16:
			break;*/
		default:
			PRINT(("OpenSoundNode::ProcessGroup: unknown mixer control type %d\n", mixext.type));
		}
	}

#if MA
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
			
			PRINT(("nb parameters : %d\n", nbParameters));
			if (nbParameters > 0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(group->ParameterAt(nbParameters));
				nbParameters++;
			}
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
#endif
}

void 
OpenSoundNode::ProcessMux(BDiscreteParameter *parameter, int32 index)
{
	CALLED();
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	oss_mixer_enuminfo enuminfo;
	status_t err;
	int i;
	
	err = mixer->GetEnumInfo(index, &enuminfo);
	if (err < B_OK) {
		// maybe there is no list.
		// generate a count form 0
		oss_mixext mixext;
		if (mixer->GetExtInfo(index, &mixext) < B_OK)
			return;

		for (i = 0; i < mixext.maxvalue; i++) {
			BString s;
			s << i;
			parameter->AddItem(i, s.String());
		}
		return;
	}
	
	for (i = 0; i < enuminfo.nvalues; i++) {
		parameter->AddItem(i, &enuminfo.strings[enuminfo.strindex[i]]);
	}
	return;
	
#if MA
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
#endif
}

status_t 
OpenSoundNode::PropagateParameterChanges(int from, int type, const char *id)
{
	oss_mixext mixext;
	oss_mixer_value mixval;
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	status_t err;
	int i;
	char oldValues[128], newValues[128];
	size_t oldValuesSize, newValuesSize;
	CALLED();
	
	if (!mixer)
		return ENODEV;
	
	// XXX !!!
	// Cortex doesn't like that!
	// try timed event
	// try checking update_counter+caching
	return B_OK;
	
	PRINT(("OpenSoundNode::%s: from %i, type %i, id %s\n", __FUNCTION__, from, type, id));
	
	for (i = 0; i < mixer->CountExtInfos(); i++) {
		err = mixer->GetExtInfo(i, &mixext);
		if (err < B_OK)
			continue;
		
		// skip the caller
		//if (mixext.ctrl == from)
		//	continue;
		
		if (!(mixext.flags & MIXF_READABLE))
			continue;
		
		// match type ?
		if (type > -1 && mixext.type != type)
			continue;
		
		// match internal ID string
		if (id && strncmp(mixext.id, id, 16))
			continue;
		
		/*
		BParameter *parameter = NULL;
		for(int32 i=0; i<fWeb->CountParameters(); i++) {
			parameter = fWeb->ParameterAt(i);
			if(parameter->ID() == mixext.ctrl)
				break;
		}

		if (!parameter)
			continue;
		*/

		oldValuesSize = 128;
		newValuesSize = 128;
		bigtime_t last;
//		PRINT(("OpenSoundNode::%s: comparing mixer control %d\n", __FUNCTION__, mixext.ctrl));
//		if (parameter->GetValue(oldValues, &oldValuesSize, &last) < B_OK)
//			continue;
		if (GetParameterValue(mixext.ctrl, &last, newValues, &newValuesSize) < B_OK)
			continue;
//		if (oldValuesSize != newValuesSize || memcmp(oldValues, newValues, MIN(oldValuesSize, newValuesSize))) {
			PRINT(("OpenSoundNode::%s: updating mixer control %d\n", __FUNCTION__, mixext.ctrl));
			BroadcastNewParameterValue(last, mixext.ctrl, newValues, newValuesSize);
//			BroadcastChangedParameter(mixext.ctrl);
//		}
	}
	return B_OK;
}

// -------------------------------------------------------- //
// OpenSoundNode specific functions
// -------------------------------------------------------- //

int32
OpenSoundNode::RunThread()
{
	int32 i;
	CALLED();
	while ( 1 ) {
	
		// XXX: use select() here when it gets implemented!
		
		//acquire buffer if any
		if ( acquire_sem_etc( fBuffer_free, 1, B_RELATIVE_TIMEOUT, 0 ) == B_BAD_SEM_ID ) {
			return B_OK;
		}
		
		//PRINT(("OpenSoundNode::RunThread: RunState= %d, EventQ: %d events\n", RunState(), EventQueue()->EventCount()));
		
		fDevice->Locker()->Lock();
		
		node_input *input = NULL;
		
		for(i=0; i<fInputs.CountItems(); i++) {
			input = (node_input *)fInputs.ItemAt(i);
			OpenSoundDeviceEngine *engine = input->fRealEngine;
			// skip unconnected or non-busy engines
			if (!engine || !engine->InUse())
				continue;
			if (input->fInput.source == media_source::null 
				&& input->fChannelId == 0)
				continue;
			// must be open for write
			ASSERT (engine->OpenMode() & OPEN_WRITE);
			
			audio_buf_info abinfo;
			size_t avail = engine->GetOSpace(&abinfo);
			//PRINT(("OpenSoundNode::RunThread: avail: %d\n", avail));
			if (avail < 1)
				continue;

#if 0
			// update the timesource
			if(input->fChannelId==0) {
				//PRINT(("updating timesource\n"));
				UpdateTimeSource(&abinfo, *input);
			}
#endif
			
			if(input->fBuffer!=NULL) {
				if (avail < input->fBuffer->SizeUsed())
					continue;
				//PRINT(("OpenSoundNode::RunThread: input[%d]: sending buffer\n", i));
				FillNextBuffer(*input, input->fBuffer);
				input->fBuffer->Recycle();
				input->fBuffer = NULL;
			} else {
				if (avail < abinfo.fragsize)
					continue;
				//XXX: write silence
				// write a nulled fragment
				if(input->fInput.source != media_source::null)
					WriteZeros(*input, abinfo.fragsize);

			}

#if 1
			// update the timesource
			if(input->fChannelId==0) {
				//PRINT(("updating timesource\n"));
				UpdateTimeSource(&abinfo, *input);
			}
#endif
			
		}
		node_output *output = NULL;
		for(i=0; i<fOutputs.CountItems(); i++) {
			output = (node_output *)fOutputs.ItemAt(i);
			OpenSoundDeviceEngine *engine = output->fRealEngine;

			// skip unconnected or non-busy engines
			if (!engine || !engine->InUse())
				continue;
			// must be open for read
			ASSERT (engine->OpenMode() & OPEN_READ);
#ifdef ENABLE_REC
			// make sure we're both started *and* connected before delivering a buffer		
			if ((RunState() != BMediaEventLooper::B_STARTED) || (output->fOutput.destination == media_destination::null))
				continue;

			audio_buf_info abinfo;
			size_t avail = engine->GetISpace(&abinfo);
			//PRINT(("OpenSoundNode::RunThread: I avail: %d\n", avail));
			// skip if less than 1 buffer
			if (avail < output->fOutput.format.u.raw_audio.buffer_size)
				continue;

			//XXX: WRITEME!
			// Get the next buffer of data
			BBuffer* buffer = FillNextBuffer(&abinfo, *output);
			if (buffer)
			{
				// send the buffer downstream if and only if output is enabled
				status_t err = B_ERROR;
				if (output->fOutputEnabled)
					err = SendBuffer(buffer, output->fOutput.destination);
				//PRINT(("OpenSoundNode::RunThread: I avail: %d, OE %d, %s\n", avail, output->fOutputEnabled, strerror(err)));
				if (err) {
					buffer->Recycle();
				} else {
					// track how much media we've delivered so far
					size_t nSamples = output->fOutput.format.u.raw_audio.buffer_size
						/ (output->fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);
					output->fSamplesSent += nSamples;
					//PRINT(("OpenSoundNode::%s: sent %d samples\n", __FUNCTION__, nSamples));
				}
			}
#endif
		}

		fDevice->Locker()->Unlock();
		
		//XXX
		snooze(5000);
		release_sem(fBuffer_free);//XXX
	}
#if MA
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
		
		//PRINT(("OpenSoundNode::RunThread: buffer exchanged\n"));
		//PRINT(("OpenSoundNode::RunThread: played_real_time : %i\n", MBI.played_real_time));
		//PRINT(("OpenSoundNode::RunThread: played_frames_count : %i\n", MBI.played_frames_count));
		//PRINT(("OpenSoundNode::RunThread: buffer_cycle : %i\n", MBI.playback_buffer_cycle));

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
					//PRINT(("OpenSoundNode::Runthread WriteZeros\n"));
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
	
#endif
	return B_OK;
}

void
OpenSoundNode::WriteZeros(node_input &input, size_t len)
{
	char buffer[2048];
	OpenSoundDeviceEngine *engine = input.fRealEngine;
	CALLED();
	
	//size_t stride = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId].stride;
	uint32 channelCount = input.fFormat.u.raw_audio.channel_count;
	size_t chunk, left = len;
	ssize_t done;
	switch(input.fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
/*			for(uint32 channel = 0; channel < channelCount; channel++) {
				char *sample_dest = fDevice->MBL.playback_buffers[bufferCycle][input.fChannelId + channel].base;
				for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
					*(float*)sample_dest = 0;
					sample_dest += stride;
				}
			}*/
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			memset(buffer, 0, 2048);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			memset(buffer, 0, 2048);
			break;
		default:
			fprintf(stderr, "ERROR in WriteZeros format not handled\n");
	}
again:
	chunk = MIN(left, 2048);
	done = engine->Write(buffer, chunk);
	if (done < 0)
		return;
	left -= done;
	if (left)
		goto again;


#if MA
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
#endif
}

void
OpenSoundNode::FillWithZeros(node_input &input)
{
	CALLED();
#if MA
	for(int32 i=0; i<fDevice->MBL.return_playback_buffers; i++)
		WriteZeros(input, i);
#endif
}

void
OpenSoundNode::FillNextBuffer(node_input &input, BBuffer* buffer)
{
	status_t err;
	CALLED();
	OpenSoundDeviceEngine *engine = input.fRealEngine;
	err = engine->Write(buffer->Data(), buffer->SizeUsed());
	
#if MA
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
			switch(input.fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				break;
			case media_raw_audio_format::B_AUDIO_INT: {
					size_t frame_size = input.fInput.format.u.raw_audio.channel_count * sizeof(int32);
					size_t stride = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId].stride;
					//PRINT(("stride : %i, frame_size : %i, return_playback_buffer_size : %i\n", stride, frame_size, fDevice->MBL.return_playback_buffer_size));
					for(uint32 channel = 0; channel < input.fInput.format.u.raw_audio.channel_count; channel++) {
						char *sample_dest = fDevice->MBL.playback_buffers[input.fBufferCycle][input.fChannelId + channel].base;
						//char *sample_src = (char*)buffer->Data() + (input.fInput.format.u.raw_audio.channel_count - 1 - channel) * sizeof(int16);
						char *sample_src = (char*)buffer->Data() + channel * sizeof(int32);
						for(uint32 i=fDevice->MBL.return_playback_buffer_size; i>0; i--) {
							*(int32*)sample_dest = *(int32*)sample_src;
							sample_dest += stride;
							sample_src += frame_size;
						}
					}
				}
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
			break;
			default:
				break;
			}
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
#endif
}

status_t
OpenSoundNode::StartThread()
{
	CALLED();
	// the thread is already started ?
	if(fThread > B_OK)
		return B_OK;
	
	//allocate buffer free semaphore
	int bufcount = MAX(fDevice->fFragments.fragstotal, 2);//XXX
	
//	fBuffer_free = create_sem( fDevice->MBL.return_playback_buffers - 1, "multi_audio out buffer free" );
	fBuffer_free = create_sem( bufcount - 1, "OpenSound out buffer free" );
	
	if ( fBuffer_free < B_OK ) {
		return B_ERROR;
	}

	fThread = spawn_thread( _run_thread_, "OpenSound audio output", B_REAL_TIME_PRIORITY, this );

	if ( fThread < B_OK ) {
		delete_sem( fBuffer_free );
		return B_ERROR;
	}

	resume_thread( fThread );
	return B_OK;
}

status_t
OpenSoundNode::StopThread()
{
	CALLED();
	delete_sem( fBuffer_free );
	wait_for_thread( fThread, &fThread );
	return B_OK;
}

void 
OpenSoundNode::AllocateBuffers(node_output &channel)
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
OpenSoundNode::UpdateTimeSource(audio_buf_info *abinfo, node_input &input)
{
	OpenSoundDeviceEngine *engine = input.fRealEngine;
	/**/CALLED();
	if (!engine)
		return;
	if(fTimeSourceStarted) {
		int64 played_frames = engine->PlayedFramesCount();
		bigtime_t perf_time = (bigtime_t)(played_frames / 
						input.fInput.format.u.raw_audio.frame_rate * 1000000);
		bigtime_t real_time = engine->PlayedRealTime(); //XXX!
						//MBI.played_real_time;
		/*PRINT(("TS: frames: last %Ld curr %Ld diff %Ld, time: last %Ld curr %Ld diff %Ld\n", 
				fOldPlayedFramesCount, played_frames, played_frames - fOldPlayedFramesCount,
				fOldPlayedRealTime, real_time, real_time - fOldPlayedRealTime));
		*/
		float drift;
		if (real_time - fOldPlayedRealTime)
			drift = ((played_frames - fOldPlayedFramesCount)
						/ input.fInput.format.u.raw_audio.frame_rate * 1000000)
						/ (real_time - fOldPlayedRealTime);
		else
			drift = 1;
		if (!drift)
			drift = 1;
//			drift = 1;
/*		fprintf(stderr, "TS: frames: last %Ld curr %Ld diff %Ld, time: last %Ld curr %Ld diff %Ld, drift %f\n", 
				fOldPlayedFramesCount, played_frames, played_frames - fOldPlayedFramesCount,
				fOldPlayedRealTime, real_time, real_time - fOldPlayedRealTime, drift);*/

//		drift = 1 + (drift - 1) / 10;

		/*
		 * In theory we should pass it, but it seems to work better if we fake a perfect world...
		 * Maybe it interferes with OSS's queing.
		 */
		drift = 1;

		PublishTime(perf_time, real_time, drift);
		
		fOldPlayedFramesCount = played_frames;
		fOldPlayedRealTime = real_time;
		//PRINT(("UpdateTimeSource() perf_time : %lli, real_time : %lli, drift : %f\n", perf_time, real_time, drift));
	}
}

#if MA
void
OpenSoundNode::UpdateTimeSource(multi_buffer_info &MBI, multi_buffer_info &oldMBI, node_input &input)
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
OpenSoundNode::FillNextBuffer(multi_buffer_info &MBI, node_output &channel)
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
#endif

BBuffer*
OpenSoundNode::FillNextBuffer(audio_buf_info *abinfo, node_output &channel)
{
	OpenSoundDeviceEngine *engine = channel.fRealEngine;
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
	
	// now fill it with data
	ssize_t used = engine->Read(buffer->Data(), channel.fOutput.format.u.raw_audio.buffer_size);
	if (used < 0) {
		PRINT(("OpenSoundNode::%s: %s\n", __FUNCTION__, strerror(used)));
		buffer->Recycle();
		return NULL;
	}
	if (used < channel.fOutput.format.u.raw_audio.buffer_size) {
		PRINT(("OpenSoundNode::%s: requested %d, got %d\n", __FUNCTION__, channel.fOutput.format.u.raw_audio.buffer_size, used));
	}
	// fill in the buffer header
	media_header* hdr = buffer->Header();
	hdr->type = B_MEDIA_RAW_AUDIO;
	hdr->size_used = used;//channel.fOutput.format.u.raw_audio.buffer_size;
	hdr->time_source = TimeSource()->ID();
	//XXX: should be system_time() - latency_as_per(abinfo)
	hdr->start_time = PerformanceTimeFor(system_time());
	
	return buffer;
}

status_t
OpenSoundNode::GetConfigurationFor(BMessage * into_message)
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
OpenSoundNode::FindOutput(media_source source)
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
OpenSoundNode::FindInput(media_destination dest)
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
OpenSoundNode::FindInput(int32 destinationId)
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
OpenSoundNode::_run_thread_( void *data )
{
	CALLED();
	return static_cast<OpenSoundNode *>( data )->RunThread();
}

void OpenSoundNode::GetFlavor(flavor_info * outInfo, int32 id)
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
	
	outInfo->name = "OpenSoundNode Node";
	outInfo->info = "The OpenSoundNode node outputs to OpenSound System v4 drivers.";
	outInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_TIME_SOURCE 
						| B_PHYSICAL_OUTPUT | B_PHYSICAL_INPUT | B_CONTROLLABLE;
	//XXX:B_MEDIA_ENCODED_AUDIO
	outInfo->in_format_count = 1; // 1 input
	media_format * informats = new media_format[outInfo->in_format_count];
	GetFormat(&informats[0]);
	outInfo->in_formats = informats;
	
	outInfo->out_format_count = 1; // 1 output
	media_format * outformats = new media_format[outInfo->out_format_count];
	GetFormat(&outformats[0]);
	outInfo->out_formats = outformats;
}

void OpenSoundNode::GetFormat(media_format * outFormat)
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
