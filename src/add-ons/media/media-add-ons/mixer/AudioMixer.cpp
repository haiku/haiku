// AudioMixer.cpp
/*

	By David Shipman, 2002

*/

#include <RealtimeAlloc.h>
#include <Buffer.h>
#include <TimeSource.h>
#include <ParameterWeb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "AudioMixer.h"
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "debug.h"

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
			fDisableStop(false)
{
	BMediaNode::AddNodeKind(B_SYSTEM_MIXER);

	// this is the default format used for all wildcard format SpecializeTo()s
	fDefaultFormat.type = B_MEDIA_RAW_AUDIO;
	fDefaultFormat.u.raw_audio.frame_rate = 96000;
	fDefaultFormat.u.raw_audio.channel_count = 2;
	fDefaultFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fDefaultFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	fDefaultFormat.u.raw_audio.buffer_size = 4096;
	fDefaultFormat.u.raw_audio.channel_mask = 0;
	fDefaultFormat.u.raw_audio.valid_bits = 0;
	fDefaultFormat.u.raw_audio.matrix_mask = 0;
}

AudioMixer::~AudioMixer()
{
	
	BMediaEventLooper::Quit();
	SetParameterWeb(NULL); 
	fWeb = NULL;	

	// any other cleanup goes here

}

void
AudioMixer::DisableNodeStop()
{
	fDisableStop = true;
}

//
// BMediaNode methods
//

void AudioMixer::Stop(bigtime_t performance_time, bool immediate)
{
	if (fDisableStop) {
		printf("AudioMixer STOP is disabled\n");
		return;
	} else {
		BMediaEventLooper::Stop(performance_time, immediate);
	}
}

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
	return B_ERROR;
}

void 
AudioMixer::SetParameterValue(int32 id, bigtime_t when, 
								const void *value, size_t ioSize)
{
}

//
// BBufferConsumer methods
//

status_t
AudioMixer::HandleMessage( int32 message, const void *data, size_t size)
{
	// since we're using a mediaeventlooper, there shouldn't be any messages
	return B_ERROR;

}

status_t
AudioMixer::AcceptFormat(const media_destination &dest, media_format *ioFormat)
{
	// check that the specified format is reasonable for the specified destination, and
	// fill in any wildcard fields for which our BBufferConsumer has specific requirements. 

	// we have multiple inputs with different IDs, but
	// the port number must match our ControlPort()
	if (dest.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;	

	// specialize to raw audio format if necessary
	if (ioFormat->type == B_MEDIA_UNKNOWN_TYPE)
		ioFormat->type = B_MEDIA_RAW_AUDIO;
	
	// we require a raw audio format
	if (ioFormat->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
	
	return B_OK;
}

status_t
AudioMixer::GetNextInput(int32 *cookie, media_input *out_input)
{
	// our 0th input is always a wildcard and free one
	if (*cookie == 0) {
		out_input->node = Node();
		out_input->source = media_source::null;
		out_input->destination.port = ControlPort();
		out_input->destination.id = 0;
		memset(&out_input->format, 0, sizeof(out_input->format));
		out_input->format.type = B_MEDIA_RAW_AUDIO;
		strcpy(out_input->name, "Free Input");
		*cookie += 1;
		return B_OK;
	}

	// the other inputs are the currently connected ones
	fCore->Lock();
	MixerInput *input;
	input = fCore->Input(*cookie - 1);
	if (!input) {
		fCore->Unlock();
		return B_BAD_INDEX;
	}
	*out_input = input->MediaInput();
	*cookie += 1;
	fCore->Unlock();
	return B_OK;
}

void
AudioMixer::DisposeInputCookie(int32 cookie)
{
	// nothing to do
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
/*
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
*/

	fCore->BufferReceived(buffer, lateness);

/*
		if ((B_OFFLINE == RunMode()) && (B_DATA_AVAILABLE == channel->fProducerDataStatus))
		{
			RequestAdditionalBuffer(channel->fInput.source, buffer);
		}
*/
}

void
AudioMixer::ProducerDataStatus( const media_destination &for_whom, 
								int32 status, bigtime_t at_performance_time)
{
/*	
	if (IsValidDest(for_whom))
	{
		media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			(void *)(&for_whom), BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
		
		// FIX_THIS
		// the for_whom destination is not being sent correctly - verify in HandleEvent loop
		
	}
*/
}

status_t
AudioMixer::GetLatencyFor(const media_destination &for_whom,
						  bigtime_t *out_latency,
						  media_node_id *out_timesource)
{
	printf("AudioMixer::GetLatencyFor\n");

	// we have multiple inputs with different IDs, but
	// the port number must match our ControlPort()
	if (for_whom.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;	
		
	// return our event latency - this includes our internal + downstream 
	// latency, but _not_ the scheduling latency
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();

	printf("AudioMixer::GetLatencyFor %Ld, timesource is %ld\n", *out_latency, *out_timesource);

	return B_OK;

}

status_t
AudioMixer::Connected(const media_source &producer, const media_destination &where,
					  const media_format &with_format, media_input *out_input)
{
	// a BBufferProducer is connection to our BBufferConsumer

	// incoming connections should always have an incoming ID=0,
	// and the port number must match our ControlPort()
	if (where.id != 0 || where.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;
	
	media_input input;
	
	fCore->Lock();
	
	// we assign a new id (!= 0) to the newly created input
	out_input->destination.id = fCore->CreateInputID();

	// We need to make sure that the outInput's name field contains a valid name,
	// the name given the connection by the producer may still be an empty string.
	// if the input has no name, assign one
	if (strlen(out_input->name) == 0)
		sprintf(out_input->name, "Input %ld", out_input->destination.id);

	// add a new input to the mixer engine
	fCore->AddInput(*out_input);

	fCore->Unlock();

	// If we want the producer to use a specific BBufferGroup, we now need to call
	// BMediaRoster::SetOutputBuffersFor() here to set the producer's buffer group.
	// But we have no special buffer requirements anyway...
	
	return B_OK;
}

void
AudioMixer::Disconnected(const media_source &producer, const media_destination &where)
{
	// One of our inputs has been disconnected
	
	// check if it is really belongs to us
	if (where.port != ControlPort()) {
		TRACE("AudioMixer::Disconnected wrong input port\n");
		return;
	}
	
	fCore->Lock();
	
	if (!fCore->RemoveInput(where.id)) {
		TRACE("AudioMixer::Disconnected can't remove input\n");
	}
	
	fCore->Unlock();
}

status_t
AudioMixer::FormatChanged( const media_source &producer, const media_destination &consumer, 
							int32 change_tag, const media_format &format)
{

	// XXX tell core about format change
	void InputFormatChanged(int32 inputID, const media_format *format);
	printf("Format changed\n");
	return B_ERROR;
}

//
// BBufferProducer methods
//

status_t
AudioMixer::FormatSuggestionRequested(media_type type, int32 quality, media_format *format)
{
	// BBufferProducer function, a downstream consumer
	// is asking for our output format

	if (type != B_MEDIA_RAW_AUDIO && type != B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;

	// we can produce any (wildcard) raw audio format
	memset(format, 0, sizeof(*format));
	format->type = B_MEDIA_RAW_AUDIO;
	return B_OK;
}

status_t
AudioMixer::FormatProposal(const media_source &output, media_format *ioFormat)
{
	// BBufferProducer function, we implement this function to verify that the
	// proposed media_format is suitable for the specified output. If any fields
	// in the format are wildcards, and we have a specific requirement, adjust
	// those fields to match our requirements before returning.
	
	// we only have one output (id=0, port=ControlPort())
	if (output.id != 0 || output.port != ControlPort())
		return B_MEDIA_BAD_SOURCE;

	// specialize to raw audio format if necessary
	if (ioFormat->type == B_MEDIA_UNKNOWN_TYPE)
		ioFormat->type = B_MEDIA_RAW_AUDIO;
	
	// we require a raw audio format
	if (ioFormat->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	// XXX tell core about format change
	void OutputFormatChanged(const media_format *format);


}

status_t
AudioMixer::FormatChangeRequested(const media_source &source, const media_destination &destination, 
								  media_format *io_format, int32 *_deprecated_)
{
	// the downstream consumer node (soundcard) requested that we produce
	// another format, we need to check if the format is accecptable and
	// remove any wildcards before returning OK.
	
	fCore->Lock();
	MixerOutput *output = fCore->Output();
	if (!output) {
		FATAL("AudioMixer::FormatChangeRequested: no output\n");
		goto err;
	}
	if (source != output->MediaOutput().source) {
		FATAL("AudioMixer::FormatChangeRequested: wrong output source\n");
		goto err;
	}
	if (destination != output->MediaOutput().destination) {
		FATAL("AudioMixer::FormatChangeRequested: wrong output destination\n");
		goto err;
	}
	if (io_format->type != B_MEDIA_RAW_AUDIO && io_format->type != B_MEDIA_UNKNOWN_TYPE) {
		FATAL("AudioMixer::FormatChangeRequested: wrong format type\n");
		goto err;
	}
	
	/* remove wildcards */
	io_format->SpecializeTo(&fDefaultFormat);

err:
	fCore->Unlock();
	return B_ERROR;
}

status_t 
AudioMixer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	if (*cookie != 0)
		return B_BAD_INDEX;

	MixerOutput *output;
	status_t rv;

	fCore->Lock();
	output = fCore->Output();
	if (output) {
		*out_output = output->MediaOutput();
	} else {
		out_output->source.port = ControlPort();
		out_output->source.id = 0;
		out_output->destination = media_destination::null;
		out_output->format.type = B_MEDIA_RAW_AUDIO;
		out_output->format.u.raw_audio = media_multi_audio_format::wildcard;
		strcpy(out_output->name, "Mixer Output");
	}
	fCore->Unlock();

	*cookie += 1;
	return B_OK
}

status_t 
AudioMixer::DisposeOutputCookie(int32 cookie)
{
	// nothin to do
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

/*

he PrepareToConnect() hook is called before a new connection between the source whichSource and the destination whichDestination is established, in 
order to give your producer one last chance to specialize any wildcards that remain in the format (although by this point there shouldn't be any, you should 
check anyway). 
Your implementation should, additionally, return in outSource the source to be used for the connection, and should fill the outName buffer with the name the 
connection will be given; the consumer will see this in the outInput->name argument specified to BBufferConsumer::Connected(). If your node doesn't 
care what the name is, you can leave the outName untouched.

*/

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
