/* AudioMixer
 *
 * First implementation by David Shipman, 2002
 * Rewritten by Marcus Overhagen, 2003
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
#include "MixerUtils.h"
#include "debug.h"

#define USE_MEDIA_FORMAT_WORKAROUND 1

#if USE_MEDIA_FORMAT_WORKAROUND
static void multi_audio_format_specialize(media_multi_audio_format *format, const media_multi_audio_format *other);
#endif

AudioMixer::AudioMixer(BMediaAddOn *addOn)
		:	BMediaNode("Audio Mixer"),
			BBufferConsumer(B_MEDIA_RAW_AUDIO),
			BBufferProducer(B_MEDIA_RAW_AUDIO),
			BControllable(),
			BMediaEventLooper(),
			fAddOn(addOn),
			fCore(new MixerCore(this)),
			fWeb(0),
			fBufferGroup(0),
			fDownstreamLatency(1),
			fInternalLatency(1),
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

	// stop the mixer
	fCore->Lock();
	fCore->Stop();
	fCore->Unlock();

	// disconnect all nodes from the mixer
	// XXX todo
	
	delete fCore;
	delete fBufferGroup;
	
	DEBUG_ONLY(fCore = 0; fBufferGroup = 0; fWeb = 0);
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
	*internal_id = 0;
	return fAddOn;
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
							  const void *value, size_t size)
{
}

//
// BBufferConsumer methods
//

status_t
AudioMixer::HandleMessage(int32 message, const void *data, size_t size)
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

	//printf("buffer received at %12Ld, should arrive at %12Ld, delta %12Ld\n", TimeSource()->Now(), buffer->Header()->start_time, TimeSource()->Now() - buffer->Header()->start_time);

//	HandleInputBuffer(buffer, 0);
//	buffer->Recycle();
//	return;


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

//	printf("Received buffer with lateness %Ld\n", lateness);
	
	fCore->Lock();
	fCore->BufferReceived(buffer, lateness);
	fCore->Unlock();

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
	// we have multiple inputs with different IDs, but
	// the port number must match our ControlPort()
	if (for_whom.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;	
		
	// return our event latency - this includes our internal + downstream 
	// latency, but _not_ the scheduling latency
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();

	//printf("AudioMixer::GetLatencyFor %Ld, timesource is %ld\n", *out_latency, *out_timesource);

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
AudioMixer::FormatChanged(const media_source &producer, const media_destination &consumer, 
						  int32 change_tag, const media_format &format)
{
	// at some point in the future (indicated by change_tag and RequestCompleted()),
	// we will receive buffers in a different format

	printf("AudioMixer::FormatChanged\n");
	
	if (consumer.port != ControlPort() || consumer.id == 0)
		return B_MEDIA_BAD_DESTINATION;
	
	// XXX we should not apply the format change at this point

	// tell core about format change
	fCore->Lock();
	fCore->InputFormatChanged(consumer.id, format.u.raw_audio);
	fCore->Unlock();
	
	return B_OK;
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

	printf("AudioMixer::FormatProposal\n");
	
	// we only have one output (id=0, port=ControlPort())
	if (output.id != 0 || output.port != ControlPort())
		return B_MEDIA_BAD_SOURCE;

	// specialize to raw audio format if necessary
	if (ioFormat->type == B_MEDIA_UNKNOWN_TYPE)
		ioFormat->type = B_MEDIA_RAW_AUDIO;
	
	// we require a raw audio format
	if (ioFormat->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
		
	return B_OK;
}

status_t
AudioMixer::FormatChangeRequested(const media_source &source, const media_destination &destination, 
								  media_format *io_format, int32 *_deprecated_)
{
	// the downstream consumer node (soundcard) requested that we produce
	// another format, we need to check if the format is acceptable and
	// remove any wildcards before returning OK.
	
	return B_ERROR;
	
	fCore->Lock();
	
	printf("AudioMixer::FormatChangeRequested\n");
	
	MixerOutput *output = fCore->Output();
	if (!output) {
		ERROR("AudioMixer::FormatChangeRequested: no output\n");
		goto err;
	}
	if (source != output->MediaOutput().source) {
		ERROR("AudioMixer::FormatChangeRequested: wrong output source\n");
		goto err;
	}
	if (destination != output->MediaOutput().destination) {
		ERROR("AudioMixer::FormatChangeRequested: wrong output destination (port %ld, id %ld), our is (port %ld, id %ld)\n", destination.port, destination.id, output->MediaOutput().destination.port, output->MediaOutput().destination.id);
		if (destination.port == output->MediaOutput().destination.port && destination.id == output->MediaOutput().destination.id + 1) {
			ERROR("AudioMixer::FormatChangeRequested: this might be the broken R5 multi audio add-on\n");
			goto err;
//			fCore->Unlock();
//			return B_OK;
		} else {
			goto err;
		}
	}
	if (io_format->type != B_MEDIA_RAW_AUDIO && io_format->type != B_MEDIA_UNKNOWN_TYPE) {
		ERROR("AudioMixer::FormatChangeRequested: wrong format type\n");
		goto err;
	}
	
	/* remove wildcards */
	#if USE_MEDIA_FORMAT_WORKAROUND
		multi_audio_format_specialize(&io_format->u.raw_audio, &fDefaultFormat.u.raw_audio);
	#else 
		io_format->SpecializeTo(&fDefaultFormat);
	#endif
	
	media_node_id id;
	FindLatencyFor(destination, &fDownstreamLatency, &id);
	printf("AudioMixer: Downstream Latency is %Ld usecs\n", fDownstreamLatency);

	// SetDuration of one buffer
	SetBufferDuration(buffer_duration(io_format->u.raw_audio));
	printf("AudioMixer: buffer duration is %Ld usecs\n", BufferDuration());

	// Our internal latency is at least the length of a full output buffer
	fInternalLatency = bigtime_t(1.6 * BufferDuration());
	printf("AudioMixer: Internal latency is %Ld usecs\n", fInternalLatency);
	
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	
	//printf("AudioMixer: SendLatencyChange %Ld\n", EventLatency());
	//SendLatencyChange(source, destination, EventLatency());

	// apply latency change
	fCore->SetTimingInfo(TimeSource(), fDownstreamLatency);

	// apply format change
	fCore->OutputFormatChanged(io_format->u.raw_audio);

	delete fBufferGroup;
	fBufferGroup = CreateBufferGroup();
	fCore->SetOutputBufferGroup(fBufferGroup);

	fCore->Unlock();
	return B_OK;
	
err:
	fCore->Unlock();
	return B_ERROR;
}

status_t 
AudioMixer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	if (*cookie != 0)
		return B_BAD_INDEX;

	fCore->Lock();
	MixerOutput *output = fCore->Output();
	if (output) {
		*out_output = output->MediaOutput();
	} else {
		out_output->node = Node();
		out_output->source.port = ControlPort();
		out_output->source.id = 0;
		out_output->destination = media_destination::null;
		memset(&out_output->format, 0, sizeof(out_output->format));
		out_output->format.type = B_MEDIA_RAW_AUDIO;
		strcpy(out_output->name, "Mixer Output");
	}
	fCore->Unlock();

	*cookie += 1;
	return B_OK;
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
	printf("#############################AudioMixer::SetBufferGroup\n");
	// the downstream consumer (soundcard) node asks us to use another
	// BBufferGroup (might be NULL). We only have one output (id 0)
	if (for_source.port != ControlPort() || for_source.id != 0)
		return B_MEDIA_BAD_SOURCE;

	if (newGroup == fBufferGroup) // we're already using this buffergroup
		return B_OK;
	
	fCore->Lock();
	if (!newGroup)
		newGroup = CreateBufferGroup();
	fCore->SetOutputBufferGroup(newGroup);
	delete fBufferGroup;
	fBufferGroup = newGroup;
	fCore->Unlock();

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
	// inside BMediaRoster::Connect(). At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format. It may also have left wildcards in the format. PrepareToConnect()
	// *must* fully specialize the format before returning!
	// we also create the new output connection and return it in out_source.

	PRINT_FORMAT("AudioMixer::PrepareToConnect: suggested format", *format);
	
	// is the source valid?
	if (what.port != ControlPort() || what.id != 0)
		return B_MEDIA_BAD_SOURCE;

	// is the format acceptable?
	if (format->type != B_MEDIA_RAW_AUDIO && format->type != B_MEDIA_UNKNOWN_TYPE) {
		PRINT_FORMAT("AudioMixer::PrepareToConnect: bad format", *format);
		return B_MEDIA_BAD_FORMAT;
	}

	fCore->Lock();

	// are we already connected?
	if (fCore->Output() != 0) {
		fCore->Unlock();
		ERROR("AudioMixer::PrepareToConnect: already connected\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	/* set source and suggest a name */
	*out_source = what;
	strcpy(out_name, "Mixer Output");

	/* remove wildcards */
	#if USE_MEDIA_FORMAT_WORKAROUND
		multi_audio_format_specialize(&format->u.raw_audio, &fDefaultFormat.u.raw_audio);
	#else 
		format->SpecializeTo(&fDefaultFormat);
	#endif

	PRINT_FORMAT("AudioMixer::PrepareToConnect: final format", *format);

	/* add output to core */	
	media_output output;
	output.node = Node();
	output.source = *out_source;
	output.destination = where;
	output.format = *format;
	strcpy(output.name, out_name);
	
	fCore->EnableOutput(false);
	fCore->AddOutput(output);

	fCore->Unlock();

	return B_OK;
}


void 
AudioMixer::Connect(status_t error, const media_source &source, const media_destination &dest, 
					const media_format &format, char *io_name)
{
	printf("AudioMixer::Connect\n");

	fCore->Lock();
	// are we still connected?
	if (fCore->Output() == 0) {
		fCore->Unlock();
		ERROR("AudioMixer::Connect: no longer connected\n");
		return;
	}
	fCore->Unlock();

	if (error != B_OK) {
		// if an error occured, remove output from core
		printf("AudioMixer::Connect failed with error 0x%08lX, removing connction\n", error);
		fCore->Lock();
		fCore->RemoveOutput();
		fCore->Unlock();
		return;
	}

	// if the connection has no name, we set it now
	if (strlen(io_name) == 0)
		strcpy(io_name, "Mixer Output");
		
	// Now that we're connected, we can determine our downstream latency.
	media_node_id id;
	FindLatencyFor(dest, &fDownstreamLatency, &id);
	printf("AudioMixer: Downstream Latency is %Ld usecs\n", fDownstreamLatency);

	// SetDuration of one buffer
	SetBufferDuration(buffer_duration(format.u.raw_audio));

	printf("AudioMixer: buffer duration is %Ld usecs\n", BufferDuration());

	// Our internal latency is at least the length of a full output buffer
	fInternalLatency = bigtime_t(1.6 * BufferDuration());
	printf("AudioMixer: Internal latency is %Ld usecs\n", fInternalLatency);
	
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	
	//printf("AudioMixer: SendLatencyChange %Ld\n", EventLatency());
	//SendLatencyChange(source, dest, EventLatency());

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!fBufferGroup)
		fBufferGroup = CreateBufferGroup();

	fCore->Lock();

	ASSERT(fCore->Output() != 0);
	
	// our source should still be valid, too
	ASSERT(fCore->Output()->MediaOutput().source.id == 0);
	ASSERT(fCore->Output()->MediaOutput().source.port == ControlPort());
	
	// BBufferConsumer::Connected() may return a different input for the
	// newly created connection. The destination can have changed since
	// AudioMixer::PrepareToConnect() and we need to update it.
	fCore->Output()->MediaOutput().destination = dest;

	fCore->EnableOutput(true);
	fCore->SetTimingInfo(TimeSource(), fDownstreamLatency);
	fCore->SetOutputBufferGroup(fBufferGroup);
	fCore->Unlock();
}


void 
AudioMixer::Disconnect(const media_source &what, const media_destination &where)
{

	printf("AudioMixer::Disconnect\n");
	
	fCore->Lock();

	// Make sure that our connection is the one being disconnected
	MixerOutput * output = fCore->Output();
	if (!output || output->MediaOutput().node != Node() || output->MediaOutput().source != what || output->MediaOutput().destination != where) {
		ERROR("AudioMixer::Disconnect can't disconnect (wrong connection)\n");
		fCore->Unlock();
		return;
	}
	
	// force a stop
	fCore->Stop();
	
	fCore->RemoveOutput();

	// destroy buffer group
	delete fBufferGroup;
	fBufferGroup = 0;
	fCore->SetOutputBufferGroup(0);
	
	fCore->Unlock();
}


void 
AudioMixer::LateNoticeReceived(const media_source &what, bigtime_t how_much, bigtime_t performance_time)
{
	// We've produced some late buffers... Increase Latency 
	// is the only runmode in which we can do anything about this

	printf("AudioMixer::LateNoticeReceived, %Ld too late at %Ld\n", how_much, performance_time);

/*	
	if (what == fOutput.source) {
		if (RunMode() == B_INCREASE_LATENCY) {
			fInternalLatency += how_much;

			if (fInternalLatency > 50000)
				fInternalLatency = 50000;

			printf("AudioMixer: increasing internal latency to %Ld usec\n", fInternalLatency);
			SetEventLatency(fDownstreamLatency + fInternalLatency);
		
//			printf("AudioMixer: SendLatencyChange %Ld (2)\n", EventLatency());
//			SendLatencyChange(source, dest, EventLatency());
		}
	}
*/
}


void 
AudioMixer::EnableOutput(const media_source &what, bool enabled, int32 *_deprecated_)
{
	// we only have one output
	if (what.id != 0 || what.port != ControlPort())
		return;
		
	fCore->Lock();
	fCore->EnableOutput(enabled);
	fCore->Unlock();
}


//
// BMediaEventLooper methods
//

void
AudioMixer::NodeRegistered()
{
		Run();
//		SetPriority(120);
		SetPriority(12);
}

void
AudioMixer::SetTimeSource(BTimeSource * time_source)
{
	printf("AudioMixer::SetTimeSource: timesource is now %ld\n", time_source->ID());
	fCore->Lock();
	fCore->SetTimingInfo(time_source, fDownstreamLatency);
	fCore->Unlock();
}

void 
AudioMixer::HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
	switch (event->type)
	{
	
		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			HandleInputBuffer((BBuffer *)event->pointer, lateness);
			((BBuffer *)event->pointer)->Recycle();
			break;
		}

		case BTimedEventQueue::B_START:
		{
			printf("AudioMixer::HandleEvent: B_START\n");
			if (RunState() != B_STARTED) {
				fCore->Lock();
				fCore->Start();
				fCore->Unlock();
			}
			break;
		}
			
		case BTimedEventQueue::B_STOP:
		{
			printf("AudioMixer::HandleEvent: B_STOP\n");
			// stopped - don't process any more buffers, flush all buffers from eventqueue
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			fCore->Lock();
			fCore->Stop();
			fCore->Unlock();
			break;
		}

		case BTimedEventQueue::B_DATA_STATUS:
		{
			printf("DataStatus message\n");
			break;
		}
		
		default:
			break;	
	
	}
}
								
//
// AudioMixer methods
//		

BBufferGroup *
AudioMixer::CreateBufferGroup()
{
	// allocate enough buffers to span our downstream latency (plus one for rounding up), plus one extra
	int32 count = int32(fDownstreamLatency / BufferDuration()) + 2;
	
	fCore->Lock();
	uint32 size = fCore->Output()->MediaOutput().format.u.raw_audio.buffer_size;
	fCore->Unlock();
	
	printf("AudioMixer: allocating %ld buffers of %ld bytes each\n", count, size);
	return new BBufferGroup(size, count);
}

#if USE_MEDIA_FORMAT_WORKAROUND
static void
raw_audio_format_specialize(media_raw_audio_format *format, const media_raw_audio_format *other)
{
	if (format->frame_rate == 0)
		format->frame_rate = other->frame_rate;
	if (format->channel_count == 0)
		format->channel_count = other->channel_count;
	if (format->format == 0)
		format->format = other->format;
	if (format->byte_order == 0)
		format->byte_order = other->byte_order;
	if (format->buffer_size == 0)
		format->buffer_size = other->buffer_size;
	if (format->frame_rate == 0)
		format->frame_rate = other->frame_rate;
}

static void
multi_audio_info_specialize(media_multi_audio_info *format, const media_multi_audio_info *other)
{
	if (format->channel_mask == 0)
		format->channel_mask = other->channel_mask;
	if (format->valid_bits == 0)
		format->valid_bits = other->valid_bits;
	if (format->matrix_mask == 0)
		format->matrix_mask = other->matrix_mask;
}

static void
multi_audio_format_specialize(media_multi_audio_format *format, const media_multi_audio_format *other)
{
	raw_audio_format_specialize(format, other);
	multi_audio_info_specialize(format, other);
}
#endif
