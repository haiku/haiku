// AudioMixer.cpp
/*

	By David Shipman, 2002

*/

#include "AudioMixer.h"
#include "IOStructures.h"

#include <media/RealtimeAlloc.h>
#include <media/Buffer.h>
#include <media/TimeSource.h>
#include <media/ParameterWeb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MALLOC_DEBUG = 1;

//

#define SEND_NEW_BUFFER_EVENT 	(BTimedEventQueue::B_USER_EVENT + 1)

mixer_input::mixer_input(media_input &input)
{

	fInput = input;

	fEventOffset = 0;
	fDataSize = 0;
	
	enabled = false;
	
	int channelCount = fInput.format.u.raw_audio.channel_count;
	
	fGainDisplay = new float[channelCount];
	fGainScale = new float[channelCount];
	
	for (int i = 0; i < channelCount; i++)
	{
		fGainScale[i] = 1.0;
		fGainDisplay[i] = 0.0;
	}
		
	fMuteValue = 0;
	fPanValue = 0;

	fGainDisplayLastChange = 0;
	fMuteValueLastChange = 0;
	fPanValueLastChange = 0;

}

mixer_input::~mixer_input()
{
	if (fGainDisplay)
		delete fGainDisplay;
	if (fGainScale)
		delete fGainScale;
	
	if (fData)
		rtm_free(fData);
	
	printf("mixer_input deleted\n");

}


// AudioMixer support classes

bool				
AudioMixer::IsValidDest( media_destination dest)
{

	int32 input_cookie = 0;
	media_input input;
	
	while (GetNextInput(&input_cookie, &input) == B_OK)
	{
		
		if (dest == input.destination)
			return true;
			
	}

	return false;
	
}

bool
AudioMixer::IsValidSource( media_source source)
{

	int32 output_cookie = 0;
	media_output output;
	
	while (GetNextOutput(&output_cookie, &output) == B_OK)
	{

		if (source == output.source)
			return true;
			
	}
	
	return false;
	
}

char *
StringForFormat(const media_format format)
{

	char *name = new char[B_MEDIA_NAME_LENGTH];

	sprintf(name, "%g kHz ", format.u.raw_audio.frame_rate / 1000.0);

	switch (format.u.raw_audio.format)
	{
		case media_raw_audio_format::B_AUDIO_FLOAT:
		
			strncat(name, "float", 5);
			break;
			
		case media_raw_audio_format::B_AUDIO_SHORT:
		
			strncat(name, "16bit", 5);
			break;

	}
	
	return name;

}

/*
// Interface
*/

BParameterWeb *
AudioMixer::BuildParameterWeb()
{

	BParameterWeb *web = new BParameterWeb();

	BParameterGroup *top = web->MakeGroup(Name()); // top level group
	
	BParameterGroup *masterGroup = top->MakeGroup("Master"); // 'Master' group split
	BParameterGroup *channelsGroup = top->MakeGroup("Channels"); // 'Channels' group split
	
	BParameterGroup *headerGroup = masterGroup->MakeGroup("Master"); // The 'header group' for this split... 
	
	headerGroup->MakeNullParameter(100, B_MEDIA_RAW_AUDIO, "Master Gain", B_WEB_BUFFER_OUTPUT);
	headerGroup->MakeNullParameter(100, B_MEDIA_RAW_AUDIO, "44.1kHz 16bit", B_GENERIC); // set our initial display like R5's mixer... 
	headerGroup->MakeDiscreteParameter(2, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);
	headerGroup->MakeContinuousParameter(3, B_MEDIA_RAW_AUDIO, "Gain", B_MASTER_GAIN, "dB", -60.0, 18.0, 1.0)->SetChannelCount(2);
	headerGroup->MakeNullParameter(100, B_MEDIA_RAW_AUDIO, "To Output", B_WEB_BUFFER_OUTPUT);
	
	(headerGroup->ParameterAt(0))->AddOutput(headerGroup->ParameterAt(1));
	(headerGroup->ParameterAt(1))->AddOutput(headerGroup->ParameterAt(2));
	(headerGroup->ParameterAt(2))->AddOutput(headerGroup->ParameterAt(3));
	(headerGroup->ParameterAt(3))->AddOutput(headerGroup->ParameterAt(4));
		
	int inputsCount = fMixerInputs.CountItems();
	
	for (int which_input = 1; which_input < inputsCount; which_input ++)
	{
	
		mixer_input *mixerInput = (mixer_input *)fMixerInputs.ItemAt(which_input);
	
		char *groupNumLabel = new char[4];
	
		sprintf(groupNumLabel, "%d", which_input);

		BParameterGroup *group = channelsGroup->MakeGroup(groupNumLabel);
	
		int baseID = (which_input) * 65536;
	
		char *formatString = StringForFormat(mixerInput->fInput.format);
		
		group->MakeNullParameter(baseID + 100, B_MEDIA_RAW_AUDIO, mixerInput->fInput.name, B_WEB_BUFFER_OUTPUT); // name of attached producer
		group->MakeNullParameter(baseID + 300, B_MEDIA_RAW_AUDIO, formatString, B_GENERIC); // display information about format
		group->MakeDiscreteParameter(baseID + 3, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);
		group->MakeContinuousParameter(baseID + 4, B_MEDIA_RAW_AUDIO, "Gain", B_GAIN, "dB", -60.0, 18.0, 1.0)
			->SetChannelCount(mixerInput->fInput.format.u.raw_audio.channel_count);

		bool panning = false;
	
		if (fOutput.destination != media_destination::null)
		{
			if (mixerInput->fInput.format.u.raw_audio.channel_count < fOutput.format.u.raw_audio.channel_count)
			{
			
				group->MakeContinuousParameter(baseID + 1, B_MEDIA_RAW_AUDIO, "Pan", B_BALANCE, "L R", -1.0, 1.0, 0.02);
				(group->ParameterAt(0))->AddOutput(group->ParameterAt(1));	
				(group->ParameterAt(1))->AddOutput(group->ParameterAt(4));
				(group->ParameterAt(4))->AddOutput(group->ParameterAt(2));
				(group->ParameterAt(2))->AddOutput(group->ParameterAt(3));
				
				panning = true;
					
			}
		}
		
		if (!panning)
		{
			(group->ParameterAt(0))->AddOutput(group->ParameterAt(1));
			(group->ParameterAt(1))->AddOutput(group->ParameterAt(2));
			(group->ParameterAt(2))->AddOutput(group->ParameterAt(3));
		}
		
	}

	return web;		
}

// 'structors... 

AudioMixer::AudioMixer(BMediaAddOn *addOn)
		:	BMediaNode("Audio Mixer"),
			BBufferConsumer(B_MEDIA_RAW_AUDIO),
			BBufferProducer(B_MEDIA_RAW_AUDIO),
			BControllable(),
			BMediaEventLooper(),
			fAddOn(addOn),
			fWeb(NULL),
			fLatency(1),
			fInternalLatency(1),
			fStartTime(0),
			fFramesSent(0),
			fOutputEnabled(true),
			fBufferGroup(NULL),
			fNextFreeID(1)
			
			
{

	BMediaNode::AddNodeKind(B_SYSTEM_MIXER);
	
	// set up the preferred output format (although we will accept any format)
	memset(&fPrefOutputFormat, 0, sizeof(fPrefOutputFormat)); // set everything to wildcard first
	fPrefOutputFormat.type = B_MEDIA_RAW_AUDIO;
	fPrefOutputFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	fPrefOutputFormat.u.raw_audio.channel_count = 2;
	
	// set up the preferred input format (although we will accept any format)
	memset(&fPrefInputFormat, 0, sizeof(fPrefInputFormat)); // set everything to wildcard first
	fPrefInputFormat.type = B_MEDIA_RAW_AUDIO;
	fPrefInputFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fPrefInputFormat.u.raw_audio.frame_rate = 96000; //wildcard?
	fPrefInputFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

	fPrefInputFormat.u.raw_audio.buffer_size = 1024;
	fPrefInputFormat.u.raw_audio.channel_count = 2;

	//fInput.source = media_source::null;

	// init i/o
	
	fOutput.destination = media_destination::null;
	fOutput.format = fPrefOutputFormat;
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	
	strcpy(fOutput.name, "Audio Mix");

	fMasterGainScale = new float[2];
	fMasterGainDisplay = new float[2];
	
	fMasterGainDisplay[0] = 0.0;
	fMasterGainDisplay[1] = 0.0;
	
	fMasterGainScale[0] = 1.0;
	fMasterGainScale[1] = 1.0;
	
	fMasterGainDisplayLastChange = TimeSource()->Now();
	
}

AudioMixer::~AudioMixer()
{
	
	BMediaEventLooper::Quit();
	SetParameterWeb(NULL); 
	fWeb = NULL;	

	// any other cleanup goes here

}

//
// BMediaNode methods
//

BMediaAddOn * 
AudioMixer::AddOn(int32 *internal_id) const
{
	if(fAddOn) 
	{
		*internal_id = 0;
		return fAddOn;
	}
	else 
		return NULL;
}

//
// BControllable methods
//

status_t 
AudioMixer::GetParameterValue(int32 id, bigtime_t *last_change, 
								void *value, size_t *ioSize)
{

	int idGroup = int(id/65536.0);
	
	if (idGroup == 0)
	{
		if (id == 3)
		{						
			float *valueOut = (float *)value;
			valueOut[0] = fMasterGainDisplay[0];
			valueOut[1] = fMasterGainDisplay[1];
			
			*last_change = fMasterGainDisplayLastChange;
			*ioSize = 8;
		
			return B_OK;
		}
		else if (id == 2)
		{
					
			int *valueOut = (int *)value;
			*valueOut = fMasterMuteValue;
							
			*last_change = fMasterMuteValueLastChange;
			*ioSize = 4;
		
			return B_OK;
		}
	}
	else
	{
	
		mixer_input *mixerInput = (mixer_input *)fMixerInputs.ItemAt(idGroup);
		
		int idBase = idGroup * 65536;
		
		if (id == idBase + 4)
		{
			float *valueOut = (float *)value;
			valueOut[0] = mixerInput->fGainDisplay[0];
			valueOut[1] = mixerInput->fGainDisplay[1];
			
			*last_change = mixerInput->fGainDisplayLastChange;
			*ioSize = 8;
		
			return B_OK;
		}
		else if (id == idBase + 3)
		{
			int *valueOut = (int *)value;
			*valueOut = mixerInput->fMuteValue;
							
			*last_change = mixerInput->fMuteValueLastChange;
			*ioSize = 4;
		
			return B_OK;
		}		
	
	}			
			
	return B_ERROR;

}

void 
AudioMixer::SetParameterValue(int32 id, bigtime_t when, 
								const void *value, size_t ioSize)
{
	
	int idGroup = int(id/65536.0);
	
	if (idGroup == 0)
	{
		if (id == 3)
		{			
					
			float *inValue = (float *)value;
			if (ioSize == 4) {
				fMasterGainDisplay[0] = inValue[0];
				fMasterGainDisplay[1] = inValue[0];
			}
			else if (ioSize == 8) {
				fMasterGainDisplay[0] = inValue[0];
				fMasterGainDisplay[1] = inValue[1];
			}
			
			fMasterGainDisplayLastChange = when;
			
			BroadcastNewParameterValue(fMasterGainDisplayLastChange,
				id, fMasterGainDisplay, ioSize);
		
			fMasterGainScale[0] = pow(2, (fMasterGainDisplay[0] / 6));
			fMasterGainScale[1] = pow(2, (fMasterGainDisplay[1] / 6));
			
		}
		else if (id == 2)
		{
			
			int *inValue = (int *)value;
			
			fMasterMuteValue = *inValue;
							
			fMasterMuteValueLastChange = when;
			
			BroadcastNewParameterValue(fMasterMuteValueLastChange,
				id, &fMasterMuteValue, ioSize);
		
		}
	}
	else
	{
	
		mixer_input *mixerInput = (mixer_input *)fMixerInputs.ItemAt(idGroup);
		
		int idBase = idGroup * 65536;
		
		if (id == idBase + 4)
		{
		
			float *inValue = (float *)value;
			if (ioSize == 4) {
				mixerInput->fGainDisplay[0] = inValue[0];
				mixerInput->fGainDisplay[1] = inValue[0];
			}
			else if (ioSize == 8) {
				mixerInput->fGainDisplay[0] = inValue[0];
				mixerInput->fGainDisplay[1] = inValue[1];
			}
			
			mixerInput->fGainDisplayLastChange = when;
			
			BroadcastNewParameterValue(mixerInput->fGainDisplayLastChange,
				id, mixerInput->fGainDisplay, ioSize);
		
			mixerInput->fGainScale[0] = pow(2, (mixerInput->fGainDisplay[0] / 6));
			mixerInput->fGainScale[1] = pow(2, (mixerInput->fGainDisplay[1] / 6));
			
		}
		else if (id == idBase + 3)
		{
			int *inValue = (int *)value;
			
			mixerInput->fMuteValue = *inValue;
							
			mixerInput->fMuteValueLastChange = when;
			
			BroadcastNewParameterValue(mixerInput->fMuteValueLastChange,
				id, &mixerInput->fMuteValue, ioSize);
		}		
	
	}			

}

//
// BBufferConsumer methods
//

status_t
AudioMixer::HandleMessage( int32 message, const void *data, size_t size)
{
	
	// since we're using a mediaeventlooper, there shouldn't be any messages
	
	printf("Received message to BBufferConsumer - this should not be!\n");
	
	return B_ERROR;

}

status_t
AudioMixer::AcceptFormat(const media_destination &dest, media_format *format)
{
	// check that the specified format is reasonable for the specified destination, and
	// fill in any wildcard fields for which our BBufferConsumer has specific requirements. 

	if (!IsValidDest(dest))
		return B_MEDIA_BAD_DESTINATION;	

	if ((format->type != B_MEDIA_UNKNOWN_TYPE) && (format->type != B_MEDIA_RAW_AUDIO))
		return B_MEDIA_BAD_FORMAT;
	else
		return B_OK;
}

status_t
AudioMixer::GetNextInput(int32 *cookie, media_input *out_input)
{

	if ((*cookie < fMixerInputs.CountItems()) && (*cookie >= 0))
	{
		mixer_input *channel = (mixer_input *)fMixerInputs.ItemAt(*cookie);
		*out_input = channel->fInput;
		*cookie += 1;
		return B_OK;
	}
	else
		return B_BAD_INDEX;
	
}

void
AudioMixer::DisposeInputCookie(int32 cookie)
{
	// nothing yet
}

void
AudioMixer::BufferReceived(BBuffer *buffer)
{
	if (buffer->Header()->type == B_MEDIA_PARAMETERS) {
		printf("Control Buffer Received\n");
		ApplyParameterData(buffer->Data(), buffer->SizeUsed());
		buffer->Recycle();
		return;
	}

//	printf("buffer received at %12Ld, should arrive at %12Ld, delta %12Ld\n", TimeSource()->Now(), buffer->Header()->start_time, TimeSource()->Now() - buffer->Header()->start_time);

	// to receive the buffer at the right time,
	// push it through the event looper
	media_timed_event event(buffer->Header()->start_time,
							BTimedEventQueue::B_HANDLE_BUFFER,
							buffer,
							BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


void
AudioMixer::HandleInputBuffer(BBuffer *buffer, bigtime_t lateness)
{
	media_header *hdr = buffer->Header();

//	printf("latency = %12Ld, event = %12Ld, sched = %5Ld, arrive at %12Ld, now %12Ld, current lateness %12Ld\n", EventLatency() + SchedulingLatency(), EventLatency(), SchedulingLatency(), buffer->Header()->start_time, TimeSource()->Now(), lateness);

	// check input
	int inputcount = fMixerInputs.CountItems();
	
	for (int i = 0; i < inputcount; i++) {
	
		mixer_input *channel = (mixer_input *)fMixerInputs.ItemAt(i);
				
		if (channel->fInput.destination.id != hdr->destination)
			continue;

		if (lateness > 5000) {
			printf("Received buffer with way to high lateness %Ld\n", lateness);
			if (RunMode() != B_DROP_DATA) {
				printf("sending notify\n");
				NotifyLateProducer(channel->fInput.source, lateness / 2, TimeSource()->Now());
			} else if (RunMode() == B_DROP_DATA) {
				printf("dropping buffer\n");
				return;
			}
		}

		size_t sample_size = channel->fInput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	
		// calculate total byte offset for writing to the ringbuffer
		// this takes account of the Event offset, as well as the buffer's start time			
/*		
		size_t total_offset = int(channel->fEventOffset + ((((perf_time - fNextEventTime) / 1000000) * 
				channel->fInput.format.u.raw_audio.frame_rate) * 
				sample_size * channel->fInput.format.u.raw_audio.channel_count)) % int(channel->fDataSize);
*/
		if (channel->fDataSize == 0)
			break;
		
		// XXX this is probably broken now
		size_t total_offset = int(channel->fEventOffset + channel->fInput.format.u.raw_audio.buffer_size)  % int(channel->fDataSize);
		
		char *indata = (char *)buffer->Data();
														
		if (buffer->SizeUsed() > (channel->fDataSize - total_offset))
		{
			memcpy(channel->fData + total_offset, indata, channel->fDataSize - total_offset);
			memcpy(channel->fData, indata + (channel->fDataSize - total_offset), buffer->SizeUsed() - (channel->fDataSize - total_offset));	
		} 
		else
			memcpy(channel->fData + total_offset, indata, buffer->SizeUsed());

		if ((B_OFFLINE == RunMode()) && (B_DATA_AVAILABLE == channel->fProducerDataStatus))
		{
			RequestAdditionalBuffer(channel->fInput.source, buffer);
		}
		
		break;
	}
}

void
AudioMixer::SendNewBuffer(bigtime_t event_time)
{
	// XXX All of this should be handled in a different thread to avoid blocking the control loop
	
	bigtime_t start = system_time();
	BBuffer *outbuffer = fBufferGroup->RequestBuffer(fOutput.format.u.raw_audio.buffer_size, BufferDuration() / 2);
	bigtime_t delta = system_time() - start;
	if (delta > 1000)
		printf("AudioMixer::SendNewBuffer: RequestBuffer took %Ld usec\n", delta);

	if (!outbuffer) {
		printf("AudioMixer::SendNewBuffer: Could not allocate buffer\n");
		return;
	}
				
	FillMixBuffer(outbuffer->Data(), outbuffer->SizeAvailable());
					
	media_header *outheader = outbuffer->Header();
	outheader->type = B_MEDIA_RAW_AUDIO;
	outheader->size_used = fOutput.format.u.raw_audio.buffer_size;
	outheader->time_source = TimeSource()->ID();
	outheader->start_time = event_time;
	
	bigtime_t start2 = system_time();
	if (B_OK != SendBuffer(outbuffer, fOutput.destination)) {
		printf("AudioMixer: Could not send buffer to output : %s\n", fOutput.name);
		outbuffer->Recycle();
	}
	bigtime_t delta2 = system_time() - start2;
	if (delta2 > 1000)
		printf("AudioMixer::SendNewBuffer: SendBuffer took %Ld usec\n", delta2);
}


void
AudioMixer::ProducerDataStatus( const media_destination &for_whom, 
								int32 status, bigtime_t at_performance_time)
{
	
	if (IsValidDest(for_whom))
	{
		media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			(void *)(&for_whom), BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
		
		// FIX_THIS
		// the for_whom destination is not being sent correctly - verify in HandleEvent loop
		
	}

}

status_t
AudioMixer::GetLatencyFor( const media_destination &for_whom, bigtime_t *out_latency, 
							media_node_id *out_timesource)
{
	printf("AudioMixer::GetLatencyFor\n");

	if (! IsValidDest(for_whom))
		return B_MEDIA_BAD_DESTINATION;
		
	// return our event latency - this includes our internal + downstream 
	// latency, but _not_ the scheduling latency
	
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();

	printf("AudioMixer::GetLatencyFor %Ld, timesource is %ld\n", *out_latency, *out_timesource);

	return B_OK;

}

status_t
AudioMixer::Connected( const media_source &producer, const media_destination &where,
						const media_format &with_format, media_input *out_input)
{
	if (!IsValidDest(where))
		return B_MEDIA_BAD_DESTINATION;
		
	// We need to make sure that the outInput's name field contains a valid name,
	// the name given the connection by the producer may still be an empty string.

	// If we want the producer to use a specific BBufferGroup, we now need to call
	// BMediaRoster::SetOutputBuffersFor() here to set the producer's buffer group
	
	char *name = out_input->name;
	mixer_input *mixerInput;
	
	// We use fNextFreeID to keep track of the next available input
	// 1-4 are our initial "free" inputs
	
	if (fNextFreeID < 5)
	{
	
		mixerInput = (mixer_input *)fMixerInputs.ItemAt(fNextFreeID);

		mixerInput->fInput.format = with_format;
		mixerInput->fInput.source = producer;

		strcpy(mixerInput->fInput.name, name);

	}
	else
	{
	
		media_input *newInput = new media_input;
	
		newInput->format = with_format;
		newInput->source = producer;
		newInput->destination.port = ControlPort();
		newInput->destination.id = fNextFreeID;
		newInput->node = Node();
		
		strcpy(newInput->name, name);
		
		mixerInput = new mixer_input(*newInput);
		
		fMixerInputs.AddItem(mixerInput, fNextFreeID);	
	
	}

	// Recalculate the next available input
	
	bool idSet = 0;
	int testID = fNextFreeID;
	int inputsCount = fMixerInputs.CountItems();
	
	for (int testID = fNextFreeID; testID < inputsCount; testID ++)
	{
		
		mixer_input *testInput = (mixer_input *)fMixerInputs.ItemAt(testID);
		
		if (testID < 5)
		{
			if (testInput->fInput.source == media_source::null) {
				fNextFreeID = testID;
				idSet = 1;
				testID = inputsCount;
			}
		}
		else
		{
			if (testInput->fInput.destination.id > testID)
			{
				fNextFreeID = testID;
				idSet = 1;
				testID = inputsCount;		
			}	
		
		}
	
	}
	if (!idSet)
		fNextFreeID = inputsCount;
	
	// Set up the ringbuffer in which to write incoming data
	
	mixerInput->fData = (char *)rtm_alloc(NULL, 65536); // CHANGE_THIS - we only need to allocate space for as many
	mixerInput->fDataSize = 65536;			// input buffers as would fill an output - plus one
		
	memset(mixerInput->fData, 0, mixerInput->fDataSize);
		
	mixerInput->enabled = true;

	*out_input = mixerInput->fInput;
	
	fWeb = BuildParameterWeb();
	SetParameterWeb(fWeb);
				
	return B_OK;

}

void
AudioMixer::Disconnected( const media_source &producer, const media_destination &where)
{
	
	// One of our inputs has been disconnected
	// If its one of our initial "Free" inputs, we need to set it back to its original state 
	// Otherwise, we can delete the input
	
	mixer_input *mixerInput;

	int inputcount = fMixerInputs.CountItems();
		
	for (int i = 1; i < inputcount; i++)
	{
		mixerInput = (mixer_input *)fMixerInputs.ItemAt(i);
		if (mixerInput->fInput.destination == where)
		{	
			if (mixerInput->fInput.source != media_source::null) // input already disconnected?
			{
				if ((mixerInput->fInput.destination.id > 0) && (mixerInput->fInput.destination.id < 5))
				{
					mixerInput->fInput.source = media_source::null;
					strcpy(mixerInput->fInput.name, "Free");
					mixerInput->fInput.format = fPrefInputFormat;
									
					rtm_free(mixerInput->fData);
					
					mixerInput->fData = NULL;
					
					mixerInput->fEventOffset = 0;
					mixerInput->fDataSize = 0;
					
					delete mixerInput->fGainDisplay, mixerInput->fGainScale;
					
					mixerInput->fMuteValue = 0;
					mixerInput->fPanValue = 0.0;
					
					mixerInput->enabled = false;
					
					int channelCount = mixerInput->fInput.format.u.raw_audio.channel_count;
	
					mixerInput->fGainDisplay = new float[channelCount];
					mixerInput->fGainScale = new float[channelCount];
	
					for (int i = 0; i < channelCount; i++)
					{
						mixerInput->fGainScale[i] = 1.0;
						mixerInput->fGainDisplay[i] = 0.0;
					}
					
				}
				else if (mixerInput->fInput.destination.id > 4)
				{
				
					delete mixerInput;
					fMixerInputs.RemoveItem(mixerInput);
					
				}
			}
			
			fWeb = BuildParameterWeb();
			SetParameterWeb(fWeb);
			
			if (mixerInput->fInput.destination.id < fNextFreeID)
				fNextFreeID = mixerInput->fInput.destination.id;
			
			i = inputcount;
		
		}
		else printf("Disconnection requested for input with no source\n");
		
	}					

}

status_t
AudioMixer::FormatChanged( const media_source &producer, const media_destination &consumer, 
							int32 change_tag, const media_format &format)
{

	// Format changed, we need to update our local info
	// and change our ringbuffer size if needs be
	// Not supported (yet)

	printf("Format changed\n");
	return B_ERROR;
	
}

//
// BBufferProducer methods
//

status_t
AudioMixer::FormatSuggestionRequested( media_type type,	int32 quality, media_format *format)
{

	// downstream consumers are asking for a preferred format
	// if suitable, return our previously constructed format def

	if (!format)
		return B_BAD_VALUE;

	*format = fPrefOutputFormat;
	
	if (type == B_MEDIA_UNKNOWN_TYPE)
		type = B_MEDIA_RAW_AUDIO;
	
	if (type != B_MEDIA_RAW_AUDIO) 
		return B_MEDIA_BAD_FORMAT;
	else return B_OK;

}

status_t
AudioMixer::FormatProposal( const media_source &output,	media_format *format)
{

	// check to see if format is valid for the specified output
	
	if (! IsValidSource(output))
		return B_MEDIA_BAD_SOURCE;
	
	if ((format->type != B_MEDIA_UNKNOWN_TYPE) && (format->type != B_MEDIA_RAW_AUDIO))
		return B_MEDIA_BAD_FORMAT;
	else return B_OK;

}

status_t
AudioMixer::FormatChangeRequested( const media_source &source, const media_destination &destination, 
									media_format *io_format, int32 *_deprecated_)
{
	
	// try to change the format of the specified source to io_format
	// not supported (yet)
	
	return B_ERROR;
	
}

status_t 
AudioMixer::GetNextOutput( int32 *cookie, media_output *out_output)
{

	// iterate through available outputs
	
	if (*cookie == 0)
	{
		*out_output = fOutput;
		*cookie = 1;
		return B_OK;
	}
	else return B_BAD_INDEX;
	
}

status_t 
AudioMixer::DisposeOutputCookie( int32 cookie)
{

	// we don't need to do anything here, since we haven't 
	// used the cookie for anything special... 
	
	return B_OK;

}

status_t 
AudioMixer::SetBufferGroup(const media_source &for_source, BBufferGroup *newGroup)
{

	if (! IsValidSource(for_source))
		return B_MEDIA_BAD_SOURCE;

	if (newGroup == fBufferGroup) // we're already using this buffergroup
		return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point we delete
	// the one we are using and use the specified one instead.  If the specified group is
	// NULL, we need to recreate one ourselves, and use *that*.  Note that if we're
	// caching a BBuffer that we requested earlier, we have to Recycle() that buffer
	// *before* deleting the buffer group, otherwise we'll deadlock waiting for that
	// buffer to be recycled!
	delete fBufferGroup;		// waits for all buffers to recycle
	if (newGroup != NULL)
	{
		// we were given a valid group; just use that one from now on
		fBufferGroup = newGroup;
	}
	else
	{
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		AllocateBuffers();
	}


	return B_OK;
}

status_t
AudioMixer::GetLatency(bigtime_t *out_latency)
{
	// report our *total* latency:  internal plus downstream plus scheduling
	*out_latency = EventLatency() + SchedulingLatency();

	printf("AudioMixer::GetLatency %Ld\n", *out_latency);

	return B_OK;
}

status_t
AudioMixer::PrepareToConnect(const media_source &what, const media_destination &where, 
								media_format *format, media_source *out_source, char *out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!

	// trying to connect something that isn't our source?
	if (! IsValidSource(what))
		return B_MEDIA_BAD_SOURCE;

	// are we already connected?
	if (fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
		
		// CHANGE_THIS  -  we're messing around with formats, need to clean up
		// still need to check u.raw_audio.format
	
	if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format)
		format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	
	if ((format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT) &&  // currently support floating point
		(format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT))		// and 16bit int audio
		return B_MEDIA_BAD_FORMAT;

	// all fields MUST be validated here
	// or else the consumer is prone to divide-by-zero errors

	if(format->u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate)
		format->u.raw_audio.frame_rate = 44100;

	if(format->u.raw_audio.channel_count == media_raw_audio_format::wildcard.channel_count)
		format->u.raw_audio.channel_count = 2;

	if(format->u.raw_audio.byte_order == media_raw_audio_format::wildcard.byte_order)
		format->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
		format->u.raw_audio.buffer_size = 1024;		// pick something comfortable to suggest

	
	// Now reserve the connection, and return information about it
	fOutput.destination = where;
	fOutput.format = *format;
	*out_source = fOutput.source;
	strncpy(out_name, fOutput.name, B_MEDIA_NAME_LENGTH); // strncpy?

	return B_OK;
}


void 
AudioMixer::Connect( status_t error, const media_source &source, const media_destination &dest, 
						const media_format &format, char *io_name)
{
	printf("AudioMixer::Connect\n");

	// we need to check which output dest refers to - we only have one for now
	
	if (error)
	{
		fOutput.destination = media_destination::null;
		fOutput.format = fPrefOutputFormat;
		return;
	}

	// connection is confirmed, record information and send output name
	
	fOutput.destination = dest;
	fOutput.format = format;
	strncpy(io_name, fOutput.name, B_MEDIA_NAME_LENGTH);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);
	printf("Downstream Latency is %Ld usecs\n", fLatency);

	// we need at least the length of a full output buffer's latency (I think?)

	size_t sample_size = fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK;

	size_t framesPerBuffer = (fOutput.format.u.raw_audio.buffer_size / sample_size) / fOutput.format.u.raw_audio.channel_count;
	
	//fInternalLatency = (framesPerBuffer / fOutput.format.u.raw_audio.frame_rate); // test * 1000000
	
	void *mouse = malloc(fOutput.format.u.raw_audio.buffer_size);
	
	bigtime_t latency_start = TimeSource()->RealTime();
	FillMixBuffer(mouse, fOutput.format.u.raw_audio.buffer_size);
	bigtime_t latency_end = TimeSource()->RealTime();
	
	fInternalLatency = latency_end - latency_start;
	printf("Internal latency is %Ld usecs\n", fInternalLatency);
	
	// use a higher internal latency to be able to process buffers that arrive late
	// XXX does this make sense?
	if (fInternalLatency < 5000)
		fInternalLatency = 5000;
	
	delete mouse;
	
	// might need to tweak the latency
	SetEventLatency(fLatency + fInternalLatency);
	
	printf("AudioMixer: SendLatencyChange %Ld\n", EventLatency());
	SendLatencyChange(source, dest, EventLatency());

	// calculate buffer duration and set it	
	if (fOutput.format.u.raw_audio.frame_rate == 0) {
		// XXX must be adjusted later when the format is known
		SetBufferDuration((framesPerBuffer * 1000000LL) / 44100);
	} else {
		SetBufferDuration((framesPerBuffer * 1000000LL) / fOutput.format.u.raw_audio.frame_rate);
	}

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!fBufferGroup)
		AllocateBuffers();
}


void 
AudioMixer::Disconnect(const media_source &what, const media_destination &where)
{

	printf("AudioMixer::Disconnect\n");
	// Make sure that our connection is the one being disconnected
	if ((where == fOutput.destination) && (what == fOutput.source)) // change later for multisource outputs
	{
		fOutput.destination = media_destination::null;
		fOutput.format = fPrefOutputFormat;
		delete fBufferGroup;
		fBufferGroup = NULL;
	}

}


void 
AudioMixer::LateNoticeReceived(const media_source &what, bigtime_t how_much, bigtime_t performance_time)
{
	// We've produced some late buffers... Increase Latency 
	// is the only runmode in which we can do anything about this

	printf("AudioMixer::LateNoticeReceived, %Ld too late at %Ld\n", how_much, performance_time);
	
	if (what == fOutput.source) {
		if (RunMode() == B_INCREASE_LATENCY) {
			fInternalLatency += how_much;

			if (fInternalLatency > 50000)
				fInternalLatency = 50000;

			printf("AudioMixer: increasing internal latency to %Ld usec\n", fInternalLatency);
			SetEventLatency(fLatency + fInternalLatency);
		
//			printf("AudioMixer: SendLatencyChange %Ld (2)\n", EventLatency());
//			SendLatencyChange(source, dest, EventLatency());
		}
	}
}


void 
AudioMixer::EnableOutput(const media_source &what, bool enabled, int32 *_deprecated_)
{

	// right now we've only got one output... check this against the supplied
	// media_source and set its state accordingly... 

	if (what == fOutput.source)
	{
		fOutputEnabled = enabled;
	}
}


//
// BMediaEventLooper methods
//

void
AudioMixer::NodeRegistered()
{

		Run();
		
		fOutput.node = Node();
		
		
		for (int i = 0; i < 5; i++)
		{

			media_input *newInput = new media_input;
	
			newInput->format = fPrefInputFormat;
			newInput->source = media_source::null;
			newInput->destination.port = ControlPort();
			newInput->destination.id = i;
			newInput->node = Node();
			strcpy(newInput->name, "Free"); // 
			
			mixer_input *mixerInput = new mixer_input(*newInput);
			
			fMixerInputs.AddItem(mixerInput);
			
		}
		
		SetPriority(120);
		
		fWeb = BuildParameterWeb();
		
		SetParameterWeb(fWeb);

}


void 
AudioMixer::HandleEvent( const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
	switch (event->type)
	{
	
		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			HandleInputBuffer((BBuffer *)event->pointer, lateness);
			((BBuffer *)event->pointer)->Recycle();
			break;
		}

		case SEND_NEW_BUFFER_EVENT:
		{
			// if the output is connected and enabled, send a bufffer
			if (fOutputEnabled && fOutput.destination != media_destination::null)
				SendNewBuffer(event->event_time);
			
			// if this is the first buffer, mark with the start time
			// we need this to calculate the other buffer times
			if (fStartTime == 0) {
				fStartTime = event->event_time;
			}
			
			// count frames that have been played
			size_t sample_size = fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK;
			int framesperbuffer = (fOutput.format.u.raw_audio.buffer_size / (sample_size * fOutput.format.u.raw_audio.channel_count));

			fFramesSent += framesperbuffer;
			
			// calculate the start time for the next event and add the event
			bigtime_t nextevent = bigtime_t(fStartTime + double(fFramesSent / fOutput.format.u.raw_audio.frame_rate) * 1000000.0);
			media_timed_event nextBufferEvent(nextevent, SEND_NEW_BUFFER_EVENT);
			EventQueue()->AddEvent(nextBufferEvent);
			break;
		}
		
		case BTimedEventQueue::B_START:

			if (RunState() != B_STARTED)
			{
				// We want to start sending buffers now, so we set up the buffer-sending bookkeeping
				// and fire off the first "produce a buffer" event.
				fStartTime = 0;
				fFramesSent = 0;
				
				//fThread = spawn_thread(_mix_thread_, "audio mixer thread", B_REAL_TIME_PRIORITY, this);
				
				media_timed_event firstBufferEvent(event->event_time, SEND_NEW_BUFFER_EVENT);

				// Alternatively, we could call HandleEvent() directly with this event, to avoid a trip through
				// the event queue, like this:
				//
				//		this->HandleEvent(&firstBufferEvent, 0, false);
				//
				EventQueue()->AddEvent(firstBufferEvent);
				
				//	fStartTime = event->event_time;
	
			}
		
			break;
			
		case BTimedEventQueue::B_STOP:
		{
			// stopped - don't process any more buffers, flush all buffers from eventqueue

			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, SEND_NEW_BUFFER_EVENT);
			break;
		}

		case BTimedEventQueue::B_DATA_STATUS:
		{
		
			printf("DataStatus message\n");
			
			mixer_input *mixerInput;
			
			int inputcount = fMixerInputs.CountItems();
		
			for (int i = 0; i < inputcount; i++)
			{
				mixerInput = (mixer_input *)fMixerInputs.ItemAt(i);
				if (mixerInput->fInput.destination == (media_destination &)event->pointer)
				{
				
					printf("Valid DatasStatus destination\n");
				
					mixerInput->fProducerDataStatus = event->data;
					
					if (mixerInput->fProducerDataStatus == B_DATA_AVAILABLE)
						printf("B_DATA_AVAILABLE\n");
					else if (mixerInput->fProducerDataStatus == B_DATA_NOT_AVAILABLE)
						printf("B_DATA_NOT_AVAILABLE\n");
					else if (mixerInput->fProducerDataStatus == B_PRODUCER_STOPPED)
						printf("B_PRODUCER_STOPPED\n");			
					
					i = inputcount;
					
				}
			}
			break;
		}
		
		default:
		
			break;	
	
	}

}
								
//
// AudioMixer methods
//		
		
void 
AudioMixer::AllocateBuffers()
{

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = fOutput.format.u.raw_audio.buffer_size;
	int32 count = int32((fLatency / (BufferDuration() + 1)) + 1);
	
	if (count < 3) {
		printf("AudioMixer: calculated only %ld buffers, that's not enough\n", count);
		count = 3;
	}
	
	printf("AudioMixer: allocating %ld buffers\n", count);
	fBufferGroup = new BBufferGroup(size, count);
}

// use this later for separate threads

int32
AudioMixer::_mix_thread_(void *data)
{
	return ((AudioMixer *)data)->MixThread();
}

int32 
AudioMixer::MixThread()
{
	while (1)
	{
		snooze(500000);
	}
	
}
