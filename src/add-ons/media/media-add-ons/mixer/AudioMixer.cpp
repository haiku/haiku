/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007-2011 Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "AudioMixer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Buffer.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <Path.h>
#include <RealtimeAlloc.h>
#include <TimeSource.h>

#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AudioMixer"


// the range of the gain sliders (in dB)
#define DB_MAX	18.0
#define DB_MIN	-60.0
// when using non linear sliders, we use a power function with
#define DB_EXPONENT_POSITIVE 1.4	// for dB values > 0
#define DB_EXPONENT_NEGATIVE 1.8	// for dB values < 0

#define USE_MEDIA_FORMAT_WORKAROUND 1

#define DB_TO_GAIN(db)			dB_to_Gain((db))
#define GAIN_TO_DB(gain)		Gain_to_dB((gain))
#define PERCENT_TO_GAIN(pct)	((pct) / 100.0)
#define GAIN_TO_PERCENT(gain)	((gain)  * 100.0)

// the id is encoded with 16 bits
// then chan and src (or dst) are encoded with 6 bits
// the unique number with 4 bits
// the PARAM_ETC etc is encoded with 26 bits
#define PARAM_SRC_ENABLE(id, chan, src)		(((id) << 16) | ((chan) << 10) | ((src) << 4) | 0x1)
#define PARAM_SRC_GAIN(id, chan, src)		(((id) << 16) | ((chan) << 10) | ((src) << 4) | 0x2)
#define PARAM_DST_ENABLE(id, chan, dst)		(((id) << 16) | ((chan) << 10) | ((dst) << 4) | 0x3)
#define PARAM_ETC(etc)						(((etc) << 16) | 0x4)
#define PARAM_SRC_STR(id, chan)				(((id) << 16) | ((chan) << 10) | 0x5)
#define PARAM_DST_STR(id, chan)				(((id) << 16) | ((chan) << 10) | 0x6)
#define PARAM_MUTE(id)						(((id) << 16) | 0x7)
#define PARAM_GAIN(id)						(((id) << 16) | 0x8)
#define PARAM_BALANCE(id)					(((id) << 16) | 0x9)
#define PARAM_STR1(id)						(((id) << 16) | ((1) << 10) | 0xa)
#define PARAM_STR2(id)						(((id) << 16) | ((2) << 10) | 0xa)
#define PARAM_STR3(id)						(((id) << 16) | ((3) << 10) | 0xa)
#define PARAM_STR4(id)						(((id) << 16) | ((4) << 10) | 0xa)
#define PARAM_STR5(id)						(((id) << 16) | ((5) << 10) | 0xa)
#define PARAM_STR6(id)						(((id) << 16) | ((6) << 10) | 0xa)
#define PARAM_STR7(id)						(((id) << 16) | ((7) << 10) | 0xa)

#define PARAM(id)							((id) >> 16)
#define ETC(id)								((id) >> 16)
#define PARAM_CHAN(id)						(((id) >> 10) & 0x3f)
#define PARAM_SRC(id)						(((id) >> 4) & 0x3f)
#define PARAM_DST(id)						(((id) >> 4) & 0x3f)
#define PARAM_IS_SRC_ENABLE(id)				(((id) & 0xf) == 0x1)
#define PARAM_IS_SRC_GAIN(id)				(((id) & 0xf) == 0x2)
#define PARAM_IS_DST_ENABLE(id)				(((id) & 0xf) == 0x3)
#define PARAM_IS_ETC(id)					(((id) & 0xf) == 0x4)
#define PARAM_IS_MUTE(id)					(((id) & 0xf) == 0x7)
#define PARAM_IS_GAIN(id)					(((id) & 0xf) == 0x8)
#define PARAM_IS_BALANCE(id)				(((id) & 0xf) == 0x9)

#if USE_MEDIA_FORMAT_WORKAROUND
static void
multi_audio_format_specialize(media_multi_audio_format *format,
	const media_multi_audio_format *other);
#endif

#define FORMAT_USER_DATA_TYPE 		0x7294a8f3
#define FORMAT_USER_DATA_MAGIC_1	0xc84173bd
#define FORMAT_USER_DATA_MAGIC_2	0x4af62b7d


const static bigtime_t kMaxLatency = 150000;
	// 150 ms is the maximum latency we publish

const bigtime_t kMinMixingTime = 3500;
const bigtime_t kMaxJitter = 1500;


AudioMixer::AudioMixer(BMediaAddOn *addOn, bool isSystemMixer)
	:
	BMediaNode("Audio Mixer"),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),
	fAddOn(addOn),
	fCore(new MixerCore(this)),
	fWeb(NULL),
	fBufferGroup(NULL),
	fDownstreamLatency(1),
	fInternalLatency(1),
	fDisableStop(false),
	fLastLateNotification(0),
	fLastLateness(0)
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

	if (isSystemMixer) {
		// to get persistent settings, assign a settings file
		BPath path;
		if (B_OK != find_directory (B_USER_SETTINGS_DIRECTORY, &path))
			path.SetTo("/boot/home/config/settings/");
		path.Append("System Audio Mixer");
		fCore->Settings()->SetSettingsFile(path.Path());

		// disable stop on the auto started (system) mixer
		DisableNodeStop();
	}

	ApplySettings();
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
AudioMixer::ApplySettings()
{
	fCore->Lock();
	fCore->SetOutputAttenuation(fCore->Settings()->AttenuateOutput() ? 0.708 : 1.0);
	fCore->Unlock();
}


void
AudioMixer::DisableNodeStop()
{
	fDisableStop = true;
}


//	#pragma mark - BMediaNode methods


void
AudioMixer::Stop(bigtime_t performance_time, bool immediate)
{
	if (fDisableStop) {
		TRACE("AudioMixer STOP is disabled\n");
		return;
	} else {
		BMediaEventLooper::Stop(performance_time, immediate);
	}
}


BMediaAddOn*
AudioMixer::AddOn(int32 *internal_id) const
{
	*internal_id = 0;
	return fAddOn;
}


//	#pragma mark - BBufferConsumer methods


status_t
AudioMixer::HandleMessage(int32 message, const void *data, size_t size)
{
	// since we're using a mediaeventlooper, there shouldn't be any messages
	// except the message we are using to schedule output events for the
	// process thread.

	if (message == MIXER_SCHEDULE_EVENT) {
		RealTimeQueue()->AddEvent(*(const media_timed_event*)data);
		return B_OK;
	}

	return B_ERROR;
}


status_t
AudioMixer::AcceptFormat(const media_destination &dest, media_format *ioFormat)
{
	PRINT_FORMAT("AudioMixer::AcceptFormat: ", *ioFormat);

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

	// We do not have special requirements, but just in case
	// another mixer is connecting to us and may need a hint
	// to create a connection at optimal frame rate and
	// channel count, we place this information in the user_data
	fCore->Lock();
	MixerOutput *output = fCore->Output();
	uint32 channel_count = output ? output->MediaOutput().format.u.raw_audio.channel_count : 0;
	float frame_rate = output ? output->MediaOutput().format.u.raw_audio.frame_rate : 0.0;
	fCore->Unlock();
	ioFormat->user_data_type = FORMAT_USER_DATA_TYPE;
	*(uint32 *)&ioFormat->user_data[0] = FORMAT_USER_DATA_MAGIC_1;
	*(uint32 *)&ioFormat->user_data[4] = channel_count;
	*(float *)&ioFormat->user_data[20] = frame_rate;
	*(uint32 *)&ioFormat->user_data[44] = FORMAT_USER_DATA_MAGIC_2;

	return B_OK;
}


status_t
AudioMixer::GetNextInput(int32 *cookie, media_input *out_input)
{
	TRACE("AudioMixer::GetNextInput\n");

	// our 0th input is always a wildcard and free one
	if (*cookie == 0) {
		out_input->node = Node();
		out_input->source = media_source::null;
		out_input->destination.port = ControlPort();
		out_input->destination.id = 0;
		out_input->format.Clear();
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
		TRACE("Control Buffer Received\n");
		ApplyParameterData(buffer->Data(), buffer->SizeUsed());
		buffer->Recycle();
		return;
	}

	//PRINT(4, "buffer received at %12Ld, should arrive at %12Ld, delta %12Ld\n", TimeSource()->Now(), buffer->Header()->start_time, TimeSource()->Now() - buffer->Header()->start_time);

	// to receive the buffer at the right time,
	// push it through the event looper
	media_timed_event event(buffer->Header()->start_time,
		BTimedEventQueue::B_HANDLE_BUFFER, buffer,
		BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


void
AudioMixer::HandleInputBuffer(BBuffer* buffer, bigtime_t lateness)
{
	bigtime_t variation = 0;
	if (lateness > fLastLateness)
		variation = lateness-fLastLateness;

	if (variation > kMaxJitter) {
		TRACE("AudioMixer: Dequeued input buffer %" B_PRIdBIGTIME
			" usec late\n", lateness);
		if (RunMode() == B_DROP_DATA || RunMode() == B_DECREASE_PRECISION
			|| RunMode() == B_INCREASE_LATENCY) {
			TRACE("AudioMixer: sending notify\n");

			// Build a media_source out of the header data
			media_source source = media_source::null;
			source.port = buffer->Header()->source_port;
			source.id = buffer->Header()->source;

			NotifyLateProducer(source, variation, TimeSource()->Now());

			if (RunMode() == B_DROP_DATA) {
				TRACE("AudioMixer: dropping buffer\n");
				return;
			}
		}
	}

	fLastLateness = lateness;

	fCore->Lock();
	fCore->BufferReceived(buffer, lateness);
	fCore->Unlock();
}


void
AudioMixer::ProducerDataStatus(const media_destination& for_whom,
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
	bigtime_t *out_latency, media_node_id *out_timesource)
{
	// we have multiple inputs with different IDs, but
	// the port number must match our ControlPort()
	if (for_whom.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;

	// return our event latency - this includes our internal + downstream
	// latency, but _not_ the scheduling latency
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();

	printf("AudioMixer::GetLatencyFor %" B_PRIdBIGTIME ", timesource is %"
		B_PRId32 "\n", *out_latency, *out_timesource);

	return B_OK;
}


status_t
AudioMixer::Connected(const media_source &producer,
	const media_destination &where, const media_format &with_format,
	media_input *out_input)
{
	PRINT_FORMAT("AudioMixer::Connected: ", with_format);

	// workaround for a crashing bug in RealPlayer.  to be proper, RealPlayer's
	// BBufferProducer::PrepareToConnect() should have removed all wildcards.
	if (out_input->format.u.raw_audio.frame_rate == 0) {
		fprintf(stderr, "Audio Mixer Warning: "
			"Producer (port %" B_PRId32 ", id %" B_PRId32 ") connected with "
			"frame_rate=0\n", producer.port, producer.id);
		MixerOutput *output = fCore->Output();
		float frame_rate = output
			? output->MediaOutput().format.u.raw_audio.frame_rate : 44100.0f;
		out_input->format.u.raw_audio.frame_rate = frame_rate;
	}

	// a BBufferProducer is connecting to our BBufferConsumer

	// incoming connections should always have an incoming ID=0,
	// and the port number must match our ControlPort()
	if (where.id != 0 || where.port != ControlPort())
		return B_MEDIA_BAD_DESTINATION;

	fCore->Lock();

	// we assign a new id (!= 0) to the newly created input
	out_input->destination.id = fCore->CreateInputID();

	// We need to make sure that the outInput's name field contains a valid
	// name, the name given the connection by the producer may still be an
	// empty string.
	// if the input has no name, assign one
	if (strlen(out_input->name) == 0) {
		sprintf(out_input->name, "Input %" B_PRId32,
			out_input->destination.id);
	}

	// add a new input to the mixer engine
	MixerInput *input;
	input = fCore->AddInput(*out_input);

	fCore->Settings()->LoadConnectionSettings(input);

	fCore->Unlock();

	// If we want the producer to use a specific BBufferGroup, we now need
	// to call BMediaRoster::SetOutputBuffersFor() here to set the producer's
	// buffer group.
	// But we have no special buffer requirements anyway...

	UpdateParameterWeb();

	return B_OK;
}


void
AudioMixer::Disconnected(const media_source &producer,
	const media_destination &where)
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
	UpdateParameterWeb();
}


status_t
AudioMixer::FormatChanged(const media_source &producer,
	const media_destination &consumer, int32 change_tag,
	const media_format &format)
{
	// at some point in the future (indicated by change_tag and RequestCompleted()),
	// we will receive buffers in a different format

	TRACE("AudioMixer::FormatChanged\n");

	if (consumer.port != ControlPort() || consumer.id == 0)
		return B_MEDIA_BAD_DESTINATION;

	if (fCore->Settings()->RefuseInputFormatChange()) {
		TRACE("AudioMixer::FormatChanged: input format change refused\n");
		return B_NOT_ALLOWED;
	}

	// TODO: We should not apply the format change at this point
	// TODO: At the moment, this is not implemented at the moment and will just
	// crash the media_server.
	return B_NOT_SUPPORTED;

	// tell core about format change
	fCore->Lock();
	fCore->InputFormatChanged(consumer.id, format.u.raw_audio);
	fCore->Unlock();

	return B_OK;
}


//	#pragma mark - BBufferProducer methods


status_t
AudioMixer::FormatSuggestionRequested(media_type type, int32 quality,
	media_format *format)
{
	TRACE("AudioMixer::FormatSuggestionRequested\n");

	// BBufferProducer function, a downstream consumer
	// is asking for our output format

	if (type != B_MEDIA_RAW_AUDIO && type != B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;

	// we can produce any (wildcard) raw audio format
	format->Clear();
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

	TRACE("AudioMixer::FormatProposal\n");

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

/*!	If the format isn't good, put a good format into *io_format and return error
	If format has wildcard, specialize to what you can do (and change).
	If you can change the format, return OK.
	The request comes from your destination sychronously, so you cannot ask it
	whether it likes it -- you should assume it will since it asked.
*/
status_t
AudioMixer::FormatChangeRequested(const media_source &source,
	const media_destination &destination, media_format *io_format,
	int32 *_deprecated_)
{
	// the downstream consumer node (soundcard) requested that we produce
	// another format, we need to check if the format is acceptable and
	// remove any wildcards before returning OK.

	TRACE("AudioMixer::FormatChangeRequested\n");

	if (fCore->Settings()->RefuseOutputFormatChange()) {
		TRACE("AudioMixer::FormatChangeRequested: output format change refused\n");
		return B_ERROR;
	}

	fCore->Lock();

	status_t status = B_OK;
	BBufferGroup *group = NULL;
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
		ERROR("AudioMixer::FormatChangeRequested: wrong output destination "
			"(port %ld, id %ld), our is (port %ld, id %ld)\n", destination.port,
			destination.id, output->MediaOutput().destination.port,
			output->MediaOutput().destination.id);
		if (destination.port == output->MediaOutput().destination.port
			&& destination.id == output->MediaOutput().destination.id + 1) {
			ERROR("AudioMixer::FormatChangeRequested: this might be the broken "
				"R5 multi audio add-on\n");
		}
		goto err;
	}
	if (io_format->type != B_MEDIA_RAW_AUDIO
		&& io_format->type != B_MEDIA_UNKNOWN_TYPE) {
		ERROR("AudioMixer::FormatChangeRequested: wrong format type\n");
		goto err;
	}

	/* remove wildcards */
#if USE_MEDIA_FORMAT_WORKAROUND
	multi_audio_format_specialize(&io_format->u.raw_audio,
		&fDefaultFormat.u.raw_audio);
#else
	io_format->SpecializeTo(&fDefaultFormat);
#endif

	media_node_id id;
	FindLatencyFor(destination, &fDownstreamLatency, &id);
	TRACE("AudioMixer: Downstream Latency is %" B_PRIdBIGTIME " usecs\n",
		fDownstreamLatency);

	// SetDuration of one buffer
	SetBufferDuration(buffer_duration(io_format->u.raw_audio));
	TRACE("AudioMixer: buffer duration is %" B_PRIdBIGTIME " usecs\n",
		BufferDuration());

	// Our internal latency is at least the length of a full output buffer
	fInternalLatency = BufferDuration()
		+ max((bigtime_t)4500, bigtime_t(0.5 * BufferDuration()));
	TRACE("AudioMixer: Internal latency is %" B_PRIdBIGTIME " usecs\n",
		fInternalLatency);

	SetEventLatency(fDownstreamLatency + fInternalLatency);

	// we need to inform all connected *inputs* about *our* change in latency
	PublishEventLatencyChange();

	// TODO: we might need to create more buffers, to span a larger downstream
	// latency

	// apply latency change
	fCore->SetTimingInfo(TimeSource(), fDownstreamLatency);

	// apply format change
	fCore->OutputFormatChanged(io_format->u.raw_audio);

	status = CreateBufferGroup(&group);
	if (status != B_OK)
		return status;
	else {
		delete fBufferGroup;
		fBufferGroup = group;
		fCore->SetOutputBufferGroup(fBufferGroup);
	}

err:
	fCore->Unlock();
	return status;
}


status_t
AudioMixer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	TRACE("AudioMixer::GetNextOutput\n");

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
		out_output->format.Clear();
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
	// nothing to do
	return B_OK;
}


status_t
AudioMixer::SetBufferGroup(const media_source &for_source,
	BBufferGroup *newGroup)
{
	TRACE("AudioMixer::SetBufferGroup\n");
	// the downstream consumer (soundcard) node asks us to use another
	// BBufferGroup (might be NULL). We only have one output (id 0)
	if (for_source.port != ControlPort() || for_source.id != 0)
		return B_MEDIA_BAD_SOURCE;

	if (newGroup == fBufferGroup) {
		// we're already using this buffergroup
		return B_OK;
	}

	fCore->Lock();
	if (!newGroup) {
		status_t status = CreateBufferGroup(&newGroup);
		if (status != B_OK)
			return status;
	}
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

	TRACE("AudioMixer::GetLatency %Ld\n", *out_latency);

	return B_OK;
}


void
AudioMixer::LatencyChanged(const media_source& source,
	const media_destination& destination, bigtime_t new_latency, uint32 flags)
{
	if (source.port != ControlPort() || source.id != 0) {
		ERROR("AudioMixer::LatencyChanged: received but has wrong source "
			"%ld/%ld\n", source.port, source.id);
		return;
	}

	TRACE("AudioMixer::LatencyChanged: downstream latency from %ld/%ld to "
		"%ld/%ld changed from %Ld to %Ld\n", source.port, source.id,
		destination.port, destination.id, fDownstreamLatency, new_latency);

#if DEBUG
	{
		media_node_id id;
		bigtime_t l;
		FindLatencyFor(destination, &l, &id);
		TRACE("AudioMixer: Reported downstream Latency is %Ld usecs\n", l);
	}
#endif

	fDownstreamLatency = new_latency;
	SetEventLatency(fDownstreamLatency + fInternalLatency);

	// XXX we might need to create more buffers, to span a larger downstream
	// latency

	fCore->Lock();
	fCore->SetTimingInfo(TimeSource(), fDownstreamLatency);
	PublishEventLatencyChange();
	fCore->Unlock();
}

status_t
AudioMixer::PrepareToConnect(const media_source &what,
	const media_destination &where, media_format *format,
	media_source *out_source, char *out_name)
{
	TRACE("AudioMixer::PrepareToConnect\n");
	// PrepareToConnect() is the second stage of format negotiations that
	// happens inside BMediaRoster::Connect(). At this point, the consumer's
	// AcceptFormat() method has been called, and that node has potentially
	// changed the proposed format.
	// It may also have left wildcards in the format. PrepareToConnect()
	// *must* fully specialize the format before returning!
	// we also create the new output connection and return it in out_source.

	PRINT_FORMAT("AudioMixer::PrepareToConnect: suggested format", *format);

	// avoid loop connections
	if (where.port == ControlPort())
		return B_MEDIA_BAD_SOURCE;

	// is the source valid?
	if (what.port != ControlPort() || what.id != 0)
		return B_MEDIA_BAD_SOURCE;

	// is the format acceptable?
	if (format->type != B_MEDIA_RAW_AUDIO
		&& format->type != B_MEDIA_UNKNOWN_TYPE) {
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

	// It is possible that another mixer is connecting.
	// To avoid using the default format, we use one of
	// a) the format that it indicated as hint in the user_data,
	// b) the output format of the system audio mixer
	// c) the input format of the system DAC device
	// d) if everything failes, keep the wildcard
	if (format->u.raw_audio.channel_count == 0
		&& format->u.raw_audio.frame_rate < 1
		&& format->user_data_type == FORMAT_USER_DATA_TYPE
		&& *(uint32 *)&format->user_data[0] == FORMAT_USER_DATA_MAGIC_1
		&& *(uint32 *)&format->user_data[44] == FORMAT_USER_DATA_MAGIC_2) {
		// ok, a mixer is connecting
		uint32 channel_count = *(uint32 *)&format->user_data[4];
		float frame_rate = *(float *)&format->user_data[20];
		if (channel_count > 0 && frame_rate > 0) {
			// format is good, use it
			format->u.raw_audio.channel_count = channel_count;
			format->u.raw_audio.frame_rate = frame_rate;
		} else {
			// other mixer's output is probably not connected
			media_node node;
			BMediaRoster *roster = BMediaRoster::Roster();
			media_output out;
			media_input in;
			int32 count;
			if (roster->GetAudioMixer(&node) == B_OK
				&& roster->GetConnectedOutputsFor(node, &out, 1, &count)
						== B_OK
				&& count == 1) {
				// use mixer output format
				format->u.raw_audio.channel_count
					= out.format.u.raw_audio.channel_count;
				format->u.raw_audio.frame_rate
					= out.format.u.raw_audio.frame_rate;
			} else if (roster->GetAudioOutput(&node) == B_OK
				&& roster->GetAllInputsFor(node, &in, 1, &count) == B_OK
				&& count == 1) {
				// use DAC input format
				format->u.raw_audio.channel_count
					= in.format.u.raw_audio.channel_count;
				format->u.raw_audio.frame_rate
					= in.format.u.raw_audio.frame_rate;
			}
		}
	}

	/* set source and suggest a name */
	*out_source = what;
	strcpy(out_name, "Mixer Output");

	/* remove wildcards */
#if USE_MEDIA_FORMAT_WORKAROUND
	multi_audio_format_specialize(&format->u.raw_audio,
		&fDefaultFormat.u.raw_audio);
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
AudioMixer::Connect(status_t error, const media_source &source,
	const media_destination &dest, const media_format &format, char *io_name)
{
	TRACE("AudioMixer::Connect\n");

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
		ERROR("AudioMixer::Connect failed with error 0x%08lX, removing "
			"connection\n", error);
		fCore->Lock();
		fCore->RemoveOutput();
		fCore->Unlock();
		return;
	}

	// Switch our prefered format to have the same
	// frame_rate and channel count as the output.
	fDefaultFormat.u.raw_audio.frame_rate = format.u.raw_audio.frame_rate;
	fDefaultFormat.u.raw_audio.channel_count = format.u.raw_audio.channel_count;

	// if the connection has no name, we set it now
	if (strlen(io_name) == 0)
		strcpy(io_name, "Mixer Output");

	// Now that we're connected, we can determine our downstream latency.
	media_node_id id;
	FindLatencyFor(dest, &fDownstreamLatency, &id);
	TRACE("AudioMixer: Downstream Latency is %Ld usecs\n", fDownstreamLatency);

	// SetDuration of one buffer
	SetBufferDuration(buffer_duration(format.u.raw_audio));
	TRACE("AudioMixer: buffer duration is %Ld usecs\n", BufferDuration());

	// Our internal latency is at least the length of a full output buffer
	// plus mixing time, plus jitter
	fInternalLatency = BufferDuration()
		+ max(kMinMixingTime, bigtime_t(0.5 * BufferDuration())) + kMaxJitter;
	TRACE("AudioMixer: Internal latency is %Ld usecs\n", fInternalLatency);

	SetEventLatency(fDownstreamLatency + fInternalLatency);

	// we need to inform all connected *inputs* about *our* change in latency
	PublishEventLatencyChange();

	fCore->Lock();

	// Set up the buffer group for our connection, as long as nobody handed
	// us a buffer group (via SetBufferGroup()) prior to this.  That can
	// happen, for example, if the consumer calls SetOutputBuffersFor() on
	// us from within its Connected() method.
	if (!fBufferGroup) {
		BBufferGroup *group = NULL;
		if (CreateBufferGroup(&group) != B_OK)
			return;
		fBufferGroup = group;
	}

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

	fCore->Settings()->LoadConnectionSettings(fCore->Output());

	fCore->Unlock();
	UpdateParameterWeb();
}


void
AudioMixer::Disconnect(const media_source& what, const media_destination& where)
{
	TRACE("AudioMixer::Disconnect\n");
	fCore->Lock();

	// Make sure that our connection is the one being disconnected
	MixerOutput* output = fCore->Output();
	if (!output
		|| output->MediaOutput().node != Node()
		|| output->MediaOutput().source != what
		|| output->MediaOutput().destination != where) {
		ERROR("AudioMixer::Disconnect can't disconnect (wrong connection)\n");
		fCore->Unlock();
		return;
	}

	// Switch our prefered format back to default
	// frame rate and channel count.
	fDefaultFormat.u.raw_audio.frame_rate = 96000;
	fDefaultFormat.u.raw_audio.channel_count = 2;

	// force a stop
	fCore->Stop();

	fCore->RemoveOutput();

	// destroy buffer group
	delete fBufferGroup;
	fBufferGroup = NULL;
	fCore->SetOutputBufferGroup(0);

	fCore->Unlock();
	UpdateParameterWeb();
}


void
AudioMixer::LateNoticeReceived(const media_source& what, bigtime_t howMuch,
	bigtime_t performanceTime)
{
	// We've produced some late buffers... Increase Latency
	// is the only runmode in which we can do anything about this
	// TODO: quality could be decreased, too

	ERROR("AudioMixer::LateNoticeReceived, %Ld too late at %Ld\n", howMuch,
		performanceTime);

	if (what == fCore->Output()->MediaOutput().source
		&& RunMode() == B_INCREASE_LATENCY) {
		// We need to ignore subsequent notices whose arrival time here
		// lies within the last lateness, because queued-up buffers will all be 'late'
		if (performanceTime < fLastLateNotification)
			return;

		fInternalLatency += howMuch;

		// At some point a too large latency can get annoying
		// (actually more than annoying, as there won't be enough buffers long before this!)
		if (fInternalLatency > kMaxLatency)
			fInternalLatency = kMaxLatency;

		fLastLateNotification = TimeSource()->Now() + howMuch;

		TRACE("AudioMixer: increasing internal latency to %"
			B_PRIdBIGTIME " usec\n", fInternalLatency);
		SetEventLatency(fDownstreamLatency + fInternalLatency);

		PublishEventLatencyChange();
	}
}


void
AudioMixer::EnableOutput(const media_source& what, bool enabled,
	int32 */*deprecated*/)
{
	// we only have one output
	if (what.id != 0 || what.port != ControlPort())
		return;

	fCore->Lock();
	fCore->EnableOutput(enabled);
	fCore->Unlock();
}


//	#pragma mark - BMediaEventLooper methods


void
AudioMixer::NodeRegistered()
{
	UpdateParameterWeb();
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
}


void
AudioMixer::SetTimeSource(BTimeSource* timeSource)
{
	TRACE("AudioMixer::SetTimeSource: timesource is now %ld\n",
		timeSource->ID());
	fCore->Lock();
	fCore->SetTimingInfo(timeSource, fDownstreamLatency);
	fCore->Unlock();
}


void
AudioMixer::HandleEvent(const media_timed_event *event, bigtime_t lateness,
	bool realTimeEvent)
{
	switch (event->type) {
		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			HandleInputBuffer((BBuffer *)event->pointer, lateness);
			((BBuffer *)event->pointer)->Recycle();
			break;
		}

		case BTimedEventQueue::B_START:
		{
			TRACE("AudioMixer::HandleEvent: B_START\n");
			if (RunState() != B_STARTED) {
				fCore->Lock();
				fCore->Start();
				fCore->Unlock();
			}
			break;
		}

		case BTimedEventQueue::B_STOP:
		{
			TRACE("AudioMixer::HandleEvent: B_STOP\n");
			// stopped - don't process any more buffers, flush all buffers
			// from event queue
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_HANDLE_BUFFER);
			fCore->Lock();
			fCore->Stop();
			fCore->Unlock();
			break;
		}

		case BTimedEventQueue::B_DATA_STATUS:
		{
			ERROR("DataStatus message\n");
			break;
		}

		case MIXER_PROCESS_EVENT:
			fCore->Process();
		break;

		default:
			break;
	}
}


//	#pragma mark - AudioMixer methods


void
AudioMixer::PublishEventLatencyChange()
{
	// our event (processing + downstream) latency has changed,
	// and we need tell all inputs about this

	TRACE("AudioMixer::PublishEventLatencyChange\n");

	fCore->Lock();

	MixerInput *input;
	for (int i = 0; (input = fCore->Input(i)) != 0; i++) {
		TRACE("AudioMixer::PublishEventLatencyChange: SendLatencyChange, "
			"connection %ld/%ld to %ld/%ld event latency is now %Ld\n",
			input->MediaInput().source.port, input->MediaInput().source.id,
			input->MediaInput().destination.port,
			input->MediaInput().destination.id, EventLatency());
		SendLatencyChange(input->MediaInput().source,
			input->MediaInput().destination, EventLatency());
	}

	fCore->Unlock();
}


status_t
AudioMixer::CreateBufferGroup(BBufferGroup** buffer) const
{
	// allocate enough buffers to span our downstream latency
	// (plus one for rounding up), plus one extra
	int32 count = int32(fDownstreamLatency / BufferDuration()) + 2;

	TRACE("AudioMixer::CreateBufferGroup: fDownstreamLatency %Ld, "
		"BufferDuration %Ld, buffer count = %ld\n", fDownstreamLatency,
		BufferDuration(), count);

	if (count < 3)
		count = 3;

	fCore->Lock();
	uint32 size = fCore->Output()->MediaOutput().format.u.raw_audio.buffer_size;
	fCore->Unlock();

	TRACE("AudioMixer: allocating %ld buffers of %ld bytes each\n",
		count, size);

	BBufferGroup* buf = new BBufferGroup(size, count);
	if (buf == NULL)
		return B_NO_MEMORY;

	status_t status = buf->InitCheck();
	if (status != B_OK)
		delete buf;
	else
		*buffer = buf;

	return status;
}


status_t
AudioMixer::SendBuffer(BBuffer* buffer, MixerOutput* output)
{
	return BBufferProducer::SendBuffer(buffer, output->MediaOutput().source,
		output->MediaOutput().destination);
}


float
AudioMixer::dB_to_Gain(float db)
{
	TRACE("dB_to_Gain: dB in: %01.2f ", db);
	if (fCore->Settings()->NonLinearGainSlider()) {
		if (db > 0) {
			db = db * (pow(abs(DB_MAX), (1.0 / DB_EXPONENT_POSITIVE))
				/ abs(DB_MAX));
			db = pow(db, DB_EXPONENT_POSITIVE);
		} else {
			db = -db;
			db = db * (pow(abs(DB_MIN), (1.0 / DB_EXPONENT_NEGATIVE))
				/ abs(DB_MIN));
			db = pow(db, DB_EXPONENT_NEGATIVE);
			db = -db;
		}
	}
	TRACE("dB out: %01.2f\n", db);
	return pow(10.0, db / 20.0);
}


float
AudioMixer::Gain_to_dB(float gain)
{
	float db;
	db = 20.0 * log10(gain);
	if (fCore->Settings()->NonLinearGainSlider()) {
		if (db > 0) {
			db = pow(db, (1.0 / DB_EXPONENT_POSITIVE));
			db = db * (abs(DB_MAX) / pow(abs(DB_MAX),
				(1.0 / DB_EXPONENT_POSITIVE)));
		} else {
			db = -db;
			db = pow(db, (1.0 / DB_EXPONENT_NEGATIVE));
			db = db * (abs(DB_MIN) / pow(abs(DB_MIN),
				(1.0 / DB_EXPONENT_NEGATIVE)));
			db = -db;
		}
	}
	return db;
}


// #pragma mark - BControllable methods


status_t
AudioMixer::GetParameterValue(int32 id, bigtime_t *last_change, void *value,
	size_t *ioSize)
{
	TRACE("GetParameterValue: id 0x%08lx, ioSize %ld\n", id, *ioSize);
	int param = PARAM(id);
	fCore->Lock();
	if (PARAM_IS_ETC(id)) {
		switch (ETC(id)) {
			case 10:	// Attenuate mixer output by 3dB
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->AttenuateOutput();
				break;
			case 20:	// Use non linear gain sliders
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->NonLinearGainSlider();
				break;
			case 30:	// Display balance control for stereo connections
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->UseBalanceControl();
				break;
			case 40:	// Allow output channel remapping
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->AllowOutputChannelRemapping();
				break;
			case 50:	// Allow input channel remapping
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->AllowInputChannelRemapping();
				break;
			case 60:	// Input gain controls
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->InputGainControls();
				break;
			case 70:	// Resampling algorithm
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->ResamplingAlgorithm();
				break;
			case 80:	// Refuse output format changes
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->RefuseOutputFormatChange();
				break;
			case 90:	// Refuse input format changes
				*ioSize = sizeof(int32);
				static_cast<int32 *>(value)[0] = fCore->Settings()->RefuseInputFormatChange();
				break;
			default:
				ERROR("unhandled ETC 0x%08lx\n", id);
				break;
		}
	} else if (param == 0) {
		MixerOutput *output = fCore->Output();
		if (!output || (!PARAM_IS_MUTE(id) && !PARAM_IS_GAIN(id) && !PARAM_IS_SRC_ENABLE(id) && !PARAM_IS_SRC_GAIN(id) && !PARAM_IS_BALANCE(id)))
			goto err;
		if (PARAM_IS_MUTE(id)) {
			// output mute control
			if (*ioSize < sizeof(int32))
				goto err;
			*ioSize = sizeof(int32);
			static_cast<int32 *>(value)[0] = output->IsMuted();
		}
		if (PARAM_IS_GAIN(id)) {
			// output gain control
			if (fCore->Settings()->UseBalanceControl() && output->GetOutputChannelCount() == 2 && 1 /*channel mask is stereo */) {
				// single channel control + balance
				if (*ioSize < sizeof(float))
					goto err;
				*ioSize = sizeof(float);
				static_cast<float *>(value)[0] = GAIN_TO_DB((output->GetOutputChannelGain(0) + output->GetOutputChannelGain(1)) / 2);
			} else {
				// multi channel control
				if (*ioSize == sizeof(float)) {
					// get combined gain for all controls
					float gain = 0;
					for (int channel = 0;
							channel < output->GetOutputChannelCount();
							channel++) {
						gain += GAIN_TO_DB(
							output->GetOutputChannelGain(channel));
					}
					static_cast<float *>(value)[0] = gain
						/ output->GetOutputChannelCount();
				} else {
					if (*ioSize < output->GetOutputChannelCount()
							* sizeof(float))
						goto err;

					*ioSize = output->GetOutputChannelCount() * sizeof(float);

					for (int channel = 0;
							channel < output->GetOutputChannelCount();
							channel++) {
						static_cast<float *>(value)[channel]
							= GAIN_TO_DB(output->GetOutputChannelGain(channel));
					}
				}
			}
		}
		if (PARAM_IS_BALANCE(id)) {
			float l = output->GetOutputChannelGain(0);
			float r = output->GetOutputChannelGain(1);
			float v = r / (l+r);
			TRACE("balance l %1.3f, r %1.3f, v %1.3f\n",l,r,v);
			if (*ioSize < sizeof(float))
				goto err;
			*ioSize = sizeof(float);
			static_cast<float *>(value)[0] = v * 100;
		}
		if (PARAM_IS_SRC_ENABLE(id)) {
			if (*ioSize < sizeof(int32))
				goto err;
			*ioSize = sizeof(int32);
			static_cast<int32 *>(value)[0] = output->HasOutputChannelSource(PARAM_CHAN(id), PARAM_SRC(id));
		}
		if (PARAM_IS_SRC_GAIN(id)) {
			if (*ioSize < sizeof(float))
				goto err;
			*ioSize = sizeof(float);
			static_cast<float *>(value)[0] = GAIN_TO_PERCENT(output->GetOutputChannelSourceGain(PARAM_CHAN(id), PARAM_SRC(id)));
		}
	} else {
		MixerInput *input;
		for (int i = 0; (input = fCore->Input(i)); i++)
			if (input->ID() == param)
				break;
		if (!input || (!PARAM_IS_MUTE(id) && !PARAM_IS_GAIN(id) && !PARAM_IS_DST_ENABLE(id) && !PARAM_IS_BALANCE(id)))
			goto err;
		if (PARAM_IS_MUTE(id)) {
			// input mute control
			if (*ioSize < sizeof(int32))
				goto err;
			*ioSize = sizeof(int32);
			static_cast<int32 *>(value)[0] = !input->IsEnabled();
		}
		if (PARAM_IS_GAIN(id)) {
			// input gain control
			if (fCore->Settings()->InputGainControls() == 0) {
				// Physical input channels
				if (fCore->Settings()->UseBalanceControl() && input->GetInputChannelCount() == 2 && 1 /*channel mask is stereo */) {
					// single channel control + balance
					if (*ioSize < sizeof(float))
						goto err;
					*ioSize = sizeof(float);
					static_cast<float *>(value)[0] = GAIN_TO_DB((input->GetInputChannelGain(0) + input->GetInputChannelGain(1)) / 2);
				} else {
					// multi channel control
					if (*ioSize < input->GetInputChannelCount() * sizeof(float))
						goto err;
					*ioSize = input->GetInputChannelCount() * sizeof(float);
					for (int chan = 0; chan < input->GetInputChannelCount(); chan++)
						static_cast<float *>(value)[chan] = GAIN_TO_DB(input->GetInputChannelGain(chan));
				}
			} else {
				// Virtual output channels
				if (fCore->Settings()->UseBalanceControl() && input->GetMixerChannelCount() == 2 && 1 /*channel mask is stereo */) {
					// single channel control + balance
					if (*ioSize < sizeof(float))
						goto err;
					*ioSize = sizeof(float);
					static_cast<float *>(value)[0] = GAIN_TO_DB((input->GetMixerChannelGain(0) + input->GetMixerChannelGain(1)) / 2);
				} else {
					// multi channel control
					if (*ioSize < input->GetMixerChannelCount() * sizeof(float))
						goto err;
					*ioSize = input->GetMixerChannelCount() * sizeof(float);
					for (int chan = 0; chan < input->GetMixerChannelCount(); chan++)
						static_cast<float *>(value)[chan] = GAIN_TO_DB(input->GetMixerChannelGain(chan));
				}
			}
		}
		if (PARAM_IS_BALANCE(id)) {
			if (fCore->Settings()->InputGainControls() == 0) {
				// Physical input channels
				float l = input->GetInputChannelGain(0);
				float r = input->GetInputChannelGain(1);
				float v = r / (l+r);
				TRACE("balance l %1.3f, r %1.3f, v %1.3f\n",l,r,v);
				if (*ioSize < sizeof(float))
					goto err;
				*ioSize = sizeof(float);
				static_cast<float *>(value)[0] = v * 100;
			} else {
				// Virtual output channels
				float l = input->GetMixerChannelGain(0);
				float r = input->GetMixerChannelGain(1);
				float v = r / (l+r);
				TRACE("balance l %1.3f, r %1.3f, v %1.3f\n",l,r,v);
				if (*ioSize < sizeof(float))
					goto err;
				*ioSize = sizeof(float);
				static_cast<float *>(value)[0] = v * 100;
			}
		}
		if (PARAM_IS_DST_ENABLE(id)) {
			if (*ioSize < sizeof(int32))
				goto err;
			*ioSize = sizeof(int32);
			static_cast<int32 *>(value)[0] = input->HasInputChannelDestination(PARAM_CHAN(id), PARAM_DST(id));
		}
	}
	*last_change = TimeSource()->Now(); // XXX we could do better
	fCore->Unlock();
	return B_OK;
err:
	fCore->Unlock();
	return B_ERROR;
}


void
AudioMixer::SetParameterValue(int32 id, bigtime_t when, const void *value,
	size_t size)
{
	TRACE("SetParameterValue: id 0x%08lx, size %ld\n", id, size);
	bool update = false;
	int param = PARAM(id);
	fCore->Lock();
	if (PARAM_IS_ETC(id)) {
		switch (ETC(id)) {
			case 10:	// Attenuate mixer output by 3dB
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetAttenuateOutput(static_cast<const int32 *>(value)[0]);
				// this value is special (see MixerCore.h) and we need to notify the core
				fCore->SetOutputAttenuation((static_cast<const int32 *>(value)[0]) ? 0.708 : 1.0);
				break;
			case 20:	// Use non linear gain sliders
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetNonLinearGainSlider(static_cast<const int32 *>(value)[0]);
				update = true; // XXX should use BroadcastChangedParameter()
				break;
			case 30:	// Display balance control for stereo connections
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetUseBalanceControl(static_cast<const int32 *>(value)[0]);
				update = true;
				break;
			case 40:	// Allow output channel remapping
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetAllowOutputChannelRemapping(static_cast<const int32 *>(value)[0]);
				update = true;
				break;
			case 50:	// Allow input channel remapping
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetAllowInputChannelRemapping(static_cast<const int32 *>(value)[0]);
				update = true;
				break;
			case 60:	// Input gain controls represent
						// (0, "Physical input channels")
						// (1, "Virtual output channels")
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetInputGainControls(static_cast<const int32 *>(value)[0]);
				update = true; // XXX should use BroadcastChangedParameter()
				break;
			case 70:	// Resampling algorithm
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetResamplingAlgorithm(static_cast<const int32 *>(value)[0]);
				fCore->UpdateResamplingAlgorithm();
				break;
			case 80:	// Refuse output format changes
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetRefuseOutputFormatChange(static_cast<const int32 *>(value)[0]);
				break;
			case 90:	// Refuse input format changes
				if (size != sizeof(int32))
					goto err;
				fCore->Settings()->SetRefuseInputFormatChange(static_cast<const int32 *>(value)[0]);
				break;
			default:
				ERROR("unhandled ETC 0x%08lx\n", id);
				break;
		}
	} else if (param == 0) {
		MixerOutput *output = fCore->Output();
		if (!output || (!PARAM_IS_MUTE(id) && !PARAM_IS_GAIN(id) && !PARAM_IS_SRC_ENABLE(id) && !PARAM_IS_SRC_GAIN(id) && !PARAM_IS_BALANCE(id)))
			goto err;
		if (PARAM_IS_MUTE(id)) {
			// output mute control
			if (size != sizeof(int32))
				goto err;
			output->SetMuted(static_cast<const int32 *>(value)[0]);
		}
		if (PARAM_IS_GAIN(id)) {
			// output gain control
			if (fCore->Settings()->UseBalanceControl()
				&& output->GetOutputChannelCount() == 2 && 1 /*channel mask is stereo */) {
				// single channel control + balance
				float l = output->GetOutputChannelGain(0);
				float r = output->GetOutputChannelGain(1);
				float m = (l + r) / 2;	// master volume
				float v = DB_TO_GAIN(static_cast<const float *>(value)[0]);
				float f = v / m;		// factor for both channels
				TRACE("gain set l %1.3f, r %1.3f, m %1.3f, v %1.3f, f %1.3f\n",l,r,m,v,f);
				output->SetOutputChannelGain(0, output->GetOutputChannelGain(0) * f);
				output->SetOutputChannelGain(1, output->GetOutputChannelGain(1) * f);
			} else {
				// multi channel control
				if (size == sizeof(float)) {
					// set same volume for all channels
					float gain = static_cast<const float *>(value)[0];
					for (int channel = 0;
							channel < output->GetOutputChannelCount();
							channel++) {
						output->SetOutputChannelGain(channel,
							DB_TO_GAIN(gain));
					}
				} else {
					if (size < output->GetOutputChannelCount() * sizeof(float))
						goto err;
					for (int channel = 0;
							channel < output->GetOutputChannelCount();
							channel++) {
						output->SetOutputChannelGain(channel,
							DB_TO_GAIN(static_cast<const float *>(
								value)[channel]));
					}
				}
			}
		}
		if (PARAM_IS_BALANCE(id)) {
			float l = output->GetOutputChannelGain(0);
			float r = output->GetOutputChannelGain(1);
			float m = (l + r) / 2;	// master volume
			float v = static_cast<const float *>(value)[0] / 100; // current balance value
			float fl = 2 * (1 - v);	// left channel factor of master volume
			float fr = 2 * v;		// right channel factor of master volume
			TRACE("balance set l %1.3f, r %1.3f, m %1.3f, v %1.3f, fl %1.3f, fr %1.3f\n",l,r,m,v,fl,fr);
			output->SetOutputChannelGain(0, m * fl);
			output->SetOutputChannelGain(1, m * fr);
		}
		if (PARAM_IS_SRC_ENABLE(id)) {
			if (size != sizeof(int32))
				goto err;
			if (static_cast<const int32 *>(value)[0]) {
				output->AddOutputChannelSource(PARAM_CHAN(id), PARAM_SRC(id));
			} else {
				output->RemoveOutputChannelSource(PARAM_CHAN(id), PARAM_SRC(id));
			}
		}
		if (PARAM_IS_SRC_GAIN(id)) {
			if (size != sizeof(float))
				goto err;
			output->SetOutputChannelSourceGain(PARAM_CHAN(id), PARAM_SRC(id), PERCENT_TO_GAIN(static_cast<const float *>(value)[0]));
		}
		fCore->Settings()->SaveConnectionSettings(output);
	} else {
		MixerInput *input;
		for (int i = 0; (input = fCore->Input(i)); i++)
			if (input->ID() == param)
				break;
		if (!input || (!PARAM_IS_MUTE(id) && !PARAM_IS_GAIN(id) && !PARAM_IS_DST_ENABLE(id) && !PARAM_IS_BALANCE(id)))
			goto err;
		if (PARAM_IS_MUTE(id)) {
			// input mute control
			if (size != sizeof(int32))
				goto err;
			input->SetEnabled(!static_cast<const int32 *>(value)[0]);
		}
		if (PARAM_IS_GAIN(id)) {
			// input gain control
			if (fCore->Settings()->InputGainControls() == 0) {
				// Physical input channels
				if (fCore->Settings()->UseBalanceControl() && input->GetInputChannelCount() == 2 && 1 /*channel mask is stereo */) {
					// single channel control + balance
					float l = input->GetInputChannelGain(0);
					float r = input->GetInputChannelGain(1);
					float m = (l + r) / 2;	// master volume
					float v = DB_TO_GAIN(static_cast<const float *>(value)[0]);
					float f = v / m;		// factor for both channels
					TRACE("gain set l %1.3f, r %1.3f, m %1.3f, v %1.3f, f %1.3f\n",l,r,m,v,f);
					input->SetInputChannelGain(0, input->GetInputChannelGain(0) * f);
					input->SetInputChannelGain(1, input->GetInputChannelGain(1) * f);
				} else {
					// multi channel control
					if (size < input->GetInputChannelCount() * sizeof(float))
						goto err;
					for (int chan = 0; chan < input->GetInputChannelCount(); chan++)
						input->SetInputChannelGain(chan, DB_TO_GAIN(static_cast<const float *>(value)[chan]));
				}
			} else {
				// Virtual output channels
				if (fCore->Settings()->UseBalanceControl() && input->GetMixerChannelCount() == 2 && 1 /*channel mask is stereo */) {
					// single channel control + balance
					float l = input->GetMixerChannelGain(0);
					float r = input->GetMixerChannelGain(1);
					float m = (l + r) / 2;	// master volume
					float v = DB_TO_GAIN(static_cast<const float *>(value)[0]);
					float f = v / m;		// factor for both channels
					TRACE("gain set l %1.3f, r %1.3f, m %1.3f, v %1.3f, f %1.3f\n",l,r,m,v,f);
					input->SetMixerChannelGain(0, input->GetMixerChannelGain(0) * f);
					input->SetMixerChannelGain(1, input->GetMixerChannelGain(1) * f);
				} else {
					// multi channel control
					if (size < input->GetMixerChannelCount() * sizeof(float))
						goto err;
					for (int chan = 0; chan < input->GetMixerChannelCount(); chan++)
						input->SetMixerChannelGain(chan, DB_TO_GAIN(static_cast<const float *>(value)[chan]));
				}
			}
		}
		if (PARAM_IS_BALANCE(id)) {
			if (fCore->Settings()->InputGainControls() == 0) {
				// Physical input channels
				float l = input->GetInputChannelGain(0);
				float r = input->GetInputChannelGain(1);
				float m = (l + r) / 2;	// master volume
				float v = static_cast<const float *>(value)[0] / 100; // current balance value
				float fl = 2 * (1 - v);	// left channel factor of master volume
				float fr = 2 * v;		// right channel factor of master volume
				TRACE("balance set l %1.3f, r %1.3f, m %1.3f, v %1.3f, fl %1.3f, fr %1.3f\n",l,r,m,v,fl,fr);
				input->SetInputChannelGain(0, m * fl);
				input->SetInputChannelGain(1, m * fr);
			} else {
				// Virtual output channels
				float l = input->GetMixerChannelGain(0);
				float r = input->GetMixerChannelGain(1);
				float m = (l + r) / 2;	// master volume
				float v = static_cast<const float *>(value)[0] / 100; // current balance value
				float fl = 2 * (1 - v);	// left channel factor of master volume
				float fr = 2 * v;		// right channel factor of master volume
				TRACE("balance set l %1.3f, r %1.3f, m %1.3f, v %1.3f, fl %1.3f, fr %1.3f\n",l,r,m,v,fl,fr);
				input->SetMixerChannelGain(0, m * fl);
				input->SetMixerChannelGain(1, m * fr);
			}
		}
		if (PARAM_IS_DST_ENABLE(id)) {
			if (size != sizeof(int32))
				goto err;
			if (static_cast<const int32 *>(value)[0]) {
				int oldchan = input->GetInputChannelForDestination(PARAM_DST(id));
				if (oldchan != -1) {
					input->RemoveInputChannelDestination(oldchan, PARAM_DST(id));
					int32 null = 0;
					BroadcastNewParameterValue(when, PARAM_DST_ENABLE(PARAM(id), oldchan, PARAM_DST(id)), &null, sizeof(null));
				}
				input->AddInputChannelDestination(PARAM_CHAN(id), PARAM_DST(id));
			} else {
				input->RemoveInputChannelDestination(PARAM_CHAN(id), PARAM_DST(id));
			}
			// TODO: this is really annoying
			// The slider count of the gain control needs to be changed,
			// but calling SetChannelCount(input->GetMixerChannelCount())
			// on it has no effect on remote BParameterWebs in other apps.
			// BroadcastChangedParameter() should be correct, but doesn't work
			BroadcastChangedParameter(PARAM_GAIN(PARAM(id)));
			// We trigger a complete ParameterWeb update as workaround
			// but it will change the focus from tab 3 to tab 1
			update = true;
		}
		fCore->Settings()->SaveConnectionSettings(input);
	}

	BroadcastNewParameterValue(when, id, const_cast<void *>(value), size);

err:
	fCore->Unlock();
	if (update)
		UpdateParameterWeb();
}


void
AudioMixer::UpdateParameterWeb()
{
	fCore->Lock();
	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *top;
	BParameterGroup *outputchannels;
	BParameterGroup *inputchannels;
	BParameterGroup *group;
	BParameterGroup *subgroup;
	BParameterGroup *subsubgroup;
	BDiscreteParameter *dp;
	MixerInput *in;
	MixerOutput *out;
	char buf[50];

	top = web->MakeGroup(B_TRANSLATE("Gain controls"));

	out = fCore->Output();
	group = top->MakeGroup("");
	group->MakeNullParameter(PARAM_STR1(0), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Master output"), B_WEB_BUFFER_INPUT);
	if (!out) {
		group->MakeNullParameter(PARAM_STR2(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("not connected"), B_GENERIC);
	} else {
		group->MakeNullParameter(PARAM_STR2(0), B_MEDIA_RAW_AUDIO,
			StringForFormat(buf, out), B_GENERIC);
		group->MakeDiscreteParameter(PARAM_MUTE(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("Mute"), B_MUTE);
		if (fCore->Settings()->UseBalanceControl()
			&& out->GetOutputChannelCount() == 2 && 1
			/*channel mask is stereo */) {
			// single channel control + balance
			group->MakeContinuousParameter(PARAM_GAIN(0), B_MEDIA_RAW_AUDIO,
				B_TRANSLATE("Gain"), B_MASTER_GAIN, B_TRANSLATE("dB"),
				DB_MIN, DB_MAX, 0.1);
			group->MakeContinuousParameter(PARAM_BALANCE(0), B_MEDIA_RAW_AUDIO,
				"", B_BALANCE, "", 0, 100, 1);
		} else {
			// multi channel control
			group->MakeContinuousParameter(PARAM_GAIN(0), B_MEDIA_RAW_AUDIO,
				B_TRANSLATE("Gain"), B_MASTER_GAIN, B_TRANSLATE("dB"),
				DB_MIN, DB_MAX, 0.1)
				   ->SetChannelCount(out->GetOutputChannelCount());
		}
		group->MakeNullParameter(PARAM_STR3(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("To output"), B_WEB_BUFFER_OUTPUT);
	}

	for (int i = 0; (in = fCore->Input(i)); i++) {
		group = top->MakeGroup("");
		group->MakeNullParameter(PARAM_STR1(in->ID()), B_MEDIA_RAW_AUDIO,
			in->MediaInput().name, B_WEB_BUFFER_INPUT);
		group->MakeNullParameter(PARAM_STR2(in->ID()), B_MEDIA_RAW_AUDIO,
			StringForFormat(buf, in), B_GENERIC);
		group->MakeDiscreteParameter(PARAM_MUTE(in->ID()), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("Mute"), B_MUTE);
		// XXX the gain control is ugly once you have more than two channels,
		//     as you don't know what channel each slider controls. Tooltips might help...
		if (fCore->Settings()->InputGainControls() == 0) {
			// Physical input channels
			if (fCore->Settings()->UseBalanceControl()
				&& in->GetInputChannelCount() == 2 && 1
				/*channel mask is stereo */) {
				// single channel control + balance
				group->MakeContinuousParameter(PARAM_GAIN(in->ID()),
					B_MEDIA_RAW_AUDIO, B_TRANSLATE("Gain"), B_GAIN,
					B_TRANSLATE("dB"), DB_MIN, DB_MAX, 0.1);
				group->MakeContinuousParameter(PARAM_BALANCE(in->ID()),
					B_MEDIA_RAW_AUDIO, "", B_BALANCE, "", 0, 100, 1);
			} else {
				// multi channel control
				group->MakeContinuousParameter(PARAM_GAIN(in->ID()),
					B_MEDIA_RAW_AUDIO, B_TRANSLATE("Gain"), B_GAIN,
					B_TRANSLATE("dB"), DB_MIN, DB_MAX, 0.1)
						->SetChannelCount(in->GetInputChannelCount());
			}
		} else {
			// Virtual output channels
			if (fCore->Settings()->UseBalanceControl()
				&& in->GetMixerChannelCount() == 2 && 1
				/*channel mask is stereo */) {
				// single channel control + balance
				group->MakeContinuousParameter(PARAM_GAIN(in->ID()),
					B_MEDIA_RAW_AUDIO, B_TRANSLATE("Gain"), B_GAIN,
					B_TRANSLATE("dB"), DB_MIN, DB_MAX, 0.1);
				group->MakeContinuousParameter(PARAM_BALANCE(in->ID()),
					B_MEDIA_RAW_AUDIO, "", B_BALANCE, "", 0, 100, 1);
			} else {
				// multi channel control
				group->MakeContinuousParameter(PARAM_GAIN(in->ID()),
					B_MEDIA_RAW_AUDIO, B_TRANSLATE("Gain"), B_GAIN,
					B_TRANSLATE("dB"), DB_MIN, DB_MAX, 0.1)
						->SetChannelCount(in->GetMixerChannelCount());
			}
		}
		group->MakeNullParameter(PARAM_STR3(in->ID()), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("To master"), B_WEB_BUFFER_OUTPUT);
	}

	if (fCore->Settings()->AllowOutputChannelRemapping()) {
		top = web->MakeGroup(B_TRANSLATE("Output mapping")); // top level group
		outputchannels = top->MakeGroup("");
		outputchannels->MakeNullParameter(PARAM_STR4(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("Output channel sources"), B_GENERIC);

		group = outputchannels->MakeGroup("");
		group->MakeNullParameter(PARAM_STR5(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("Master output"), B_GENERIC);
		group = group->MakeGroup("");
		if (!out) {
			group->MakeNullParameter(PARAM_STR6(0), B_MEDIA_RAW_AUDIO,
				B_TRANSLATE("not connected"), B_GENERIC);
		} else {
			for (int chan = 0; chan < out->GetOutputChannelCount(); chan++) {
				subgroup = group->MakeGroup("");
				subgroup->MakeNullParameter(PARAM_SRC_STR(0, chan),
					B_MEDIA_RAW_AUDIO, StringForChannelType(buf,
						out->GetOutputChannelType(chan)), B_GENERIC);
				for (int src = 0; src < MAX_CHANNEL_TYPES; src++) {
					subsubgroup = subgroup->MakeGroup("");
					subsubgroup->MakeDiscreteParameter(
						PARAM_SRC_ENABLE(0, chan, src), B_MEDIA_RAW_AUDIO, "",
						B_ENABLE);
					subsubgroup->MakeContinuousParameter(
						PARAM_SRC_GAIN(0, chan, src), B_MEDIA_RAW_AUDIO,
						StringForChannelType(buf, src), B_GAIN, "%", 0.0,
						100.0, 0.1);
				}
			}
		}
	}

	if (fCore->Settings()->AllowInputChannelRemapping()) {
		top = web->MakeGroup(B_TRANSLATE("Input mapping")); // top level group
		inputchannels = top->MakeGroup("");
		inputchannels->MakeNullParameter(PARAM_STR7(0), B_MEDIA_RAW_AUDIO,
			B_TRANSLATE("Input channel destinations"), B_GENERIC);

		for (int i = 0; (in = fCore->Input(i)); i++) {
			group = inputchannels->MakeGroup("");
			group->MakeNullParameter(PARAM_STR4(in->ID()), B_MEDIA_RAW_AUDIO,
				in->MediaInput().name, B_GENERIC);
			group = group->MakeGroup("");

			for (int chan = 0; chan < in->GetInputChannelCount(); chan++) {
				subgroup = group->MakeGroup("");
				subgroup->MakeNullParameter(PARAM_DST_STR(in->ID(), chan),
					B_MEDIA_RAW_AUDIO, StringForChannelType(buf,
					in->GetInputChannelType(chan)), B_GENERIC);
				for (int dst = 0; dst < MAX_CHANNEL_TYPES; dst++) {
					subgroup->MakeDiscreteParameter(PARAM_DST_ENABLE(in->ID(),
					chan, dst), B_MEDIA_RAW_AUDIO, StringForChannelType(buf, dst),
					B_ENABLE);
				}
			}
		}
	}

	top = web->MakeGroup(B_TRANSLATE("Setup")); // top level group
	group = top->MakeGroup("");

	group->MakeDiscreteParameter(PARAM_ETC(10), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Attenuate mixer output by 3dB (like BeOS R5)"), B_ENABLE);
	group->MakeDiscreteParameter(PARAM_ETC(20), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Use non linear gain sliders (like BeOS R5)"), B_ENABLE);
	group->MakeDiscreteParameter(PARAM_ETC(30), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Display balance control for stereo connections"),
		B_ENABLE);

	group->MakeDiscreteParameter(PARAM_ETC(40), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Allow output channel remapping"), B_ENABLE);
	group->MakeDiscreteParameter(PARAM_ETC(50), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Allow input channel remapping"), B_ENABLE);

	dp = group->MakeDiscreteParameter(PARAM_ETC(60), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Input gain controls represent"), B_INPUT_MUX);
	dp->AddItem(0, B_TRANSLATE("Physical input channels"));
	dp->AddItem(1, B_TRANSLATE("Virtual output channels"));

	dp = group->MakeDiscreteParameter(PARAM_ETC(70), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Resampling algorithm"), B_INPUT_MUX);
	dp->AddItem(0, B_TRANSLATE("Drop/repeat samples"));
	dp->AddItem(2, B_TRANSLATE("Linear interpolation"));

	// Note: The following code is outcommented on purpose
	// and is about to be modified at a later point
	/*
	dp->AddItem(1, B_TRANSLATE("Drop/repeat samples (template based)"));
	dp->AddItem(3, B_TRANSLATE("17th order filtering"));
	*/
	group->MakeDiscreteParameter(PARAM_ETC(80), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Refuse output format changes"), B_ENABLE);
	group->MakeDiscreteParameter(PARAM_ETC(90), B_MEDIA_RAW_AUDIO,
		B_TRANSLATE("Refuse input format changes"), B_ENABLE);

	fCore->Unlock();
	SetParameterWeb(web);
}


#if USE_MEDIA_FORMAT_WORKAROUND
static void
raw_audio_format_specialize(media_raw_audio_format *format,
	const media_raw_audio_format *other)
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
multi_audio_info_specialize(media_multi_audio_info *format,
	const media_multi_audio_info *other)
{
	if (format->channel_mask == 0)
		format->channel_mask = other->channel_mask;
	if (format->valid_bits == 0)
		format->valid_bits = other->valid_bits;
	if (format->matrix_mask == 0)
		format->matrix_mask = other->matrix_mask;
}


static void
multi_audio_format_specialize(media_multi_audio_format *format,
	const media_multi_audio_format *other)
{
	raw_audio_format_specialize(format, other);
	multi_audio_info_specialize(format, other);
}
#endif
