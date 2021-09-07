/*
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */


//! Media add-on for drivers that use the multi audio interface


#include "MultiAudioNode.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Catalog.h>
#include <ParameterWeb.h>
#include <String.h>

#include <Referenceable.h>

#include "MultiAudioUtility.h"
#ifdef DEBUG
#	define PRINTING
#endif
#include "debug.h"
#include "Resampler.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MultiAudio"

#define PARAMETER_ID_INPUT_FREQUENCY	1
#define PARAMETER_ID_OUTPUT_FREQUENCY	2


// This represents a hardware output.
class node_input {
public:
	node_input(media_input& input, media_format preferredFormat);
	~node_input();

	int32				fChannelId;
	media_input			fInput;
	media_format 		fPreferredFormat;
	media_format		fFormat;

	volatile uint32		fBufferCycle;
	int32				fOldBufferCycle;
	BBuffer*			fBuffer;
	Resampler			*fResampler;
};


// This represents a hardware input.
class node_output {
public:
	node_output(media_output& output, media_format preferredFormat);
	~node_output();

	int32				fChannelId;
	media_output		fOutput;
	media_format 		fPreferredFormat;

	BBufferGroup*		fBufferGroup;
	bool 				fOutputEnabled;
	uint64 				fSamplesSent;
	volatile uint32 	fBufferCycle;
	int32				fOldBufferCycle;
	Resampler*			fResampler;
};


struct FrameRateChangeCookie : public BReferenceable {
	float	oldFrameRate;
	uint32	id;
};


struct sample_rate_info {
	uint32		multiAudioRate;
	const char*	name;
};


static const sample_rate_info kSampleRateInfos[] = {
	{B_SR_8000,		"8000"},
	{B_SR_11025,	"11025"},
	{B_SR_12000,	"12000"},
	{B_SR_16000,	"16000"},
	{B_SR_22050,	"22050"},
	{B_SR_24000,	"24000"},
	{B_SR_32000,	"32000"},
	{B_SR_44100,	"44100"},
	{B_SR_48000,	"48000"},
	{B_SR_64000,	"64000"},
	{B_SR_88200,	"88200"},
	{B_SR_96000,	"96000"},
	{B_SR_176400,	"176400"},
	{B_SR_192000,	"192000"},
	{B_SR_384000,	"384000"},
	{B_SR_1536000,	"1536000"},
	{}
};


const char* kMultiControlString[] = {
	"NAME IS ATTACHED",
	B_TRANSLATE("Output"), B_TRANSLATE("Input"), B_TRANSLATE("Setup"),
	B_TRANSLATE("Tone control"), B_TRANSLATE("Extended Setup"),
	B_TRANSLATE("Enhanced Setup"), B_TRANSLATE("Master"), B_TRANSLATE("Beep"),
	B_TRANSLATE("Phone"), B_TRANSLATE("Mic"), B_TRANSLATE("Line"),
	B_TRANSLATE("CD"), B_TRANSLATE("Video"), B_TRANSLATE("Aux"),
	B_TRANSLATE("Wave"), B_TRANSLATE("Gain"), B_TRANSLATE("Level"),
	B_TRANSLATE("Volume"), B_TRANSLATE("Mute"), B_TRANSLATE("Enable"),
	B_TRANSLATE("Stereo mix"), B_TRANSLATE("Mono mix"),
	B_TRANSLATE("Output stereo mix"), B_TRANSLATE("Output mono mix"),
	B_TRANSLATE("Output bass"), B_TRANSLATE("Output treble"),
	B_TRANSLATE("Output 3D center"), B_TRANSLATE("Output 3D depth"),
	B_TRANSLATE("Headphones"), B_TRANSLATE("SPDIF")
};


//	#pragma mark -


node_input::node_input(media_input& input, media_format preferredFormat)
{
	CALLED();
	fInput = input;
	fPreferredFormat = preferredFormat;
	fBufferCycle = 1;
	fOldBufferCycle = -1;
	fBuffer = NULL;
	fResampler = NULL;
}


node_input::~node_input()
{
	CALLED();
}


//	#pragma mark -


node_output::node_output(media_output& output, media_format preferredFormat)
	:
	fBufferGroup(NULL),
	fOutputEnabled(true)
{
	CALLED();
	fOutput = output;
	fPreferredFormat = preferredFormat;
	fBufferCycle = 1;
	fOldBufferCycle = -1;
	fResampler = NULL;
}


node_output::~node_output()
{
	CALLED();
}


//	#pragma mark -


MultiAudioNode::MultiAudioNode(BMediaAddOn* addon, const char* name,
		MultiAudioDevice* device, int32 internalID, BMessage* config)
	:
	BMediaNode(name),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	fBufferLock("multi audio buffers"),
	fQuitThread(0),
	fThread(-1),
	fDevice(device),
	fTimeSourceStarted(false),
	fWeb(NULL),
	fConfig()
{
	CALLED();
	fInitStatus = B_NO_INIT;

	if (!device)
		return;

	fAddOn = addon;
	fId = internalID;

	if (fDevice->Description().output_channel_count > 0) {
		// initialize our preferred format objects
		fOutputPreferredFormat.type = B_MEDIA_RAW_AUDIO;
		fOutputPreferredFormat.u.raw_audio.format
			= MultiAudio::convert_to_media_format(
				fDevice->FormatInfo().output.format);
		fOutputPreferredFormat.u.raw_audio.valid_bits
			= MultiAudio::convert_to_valid_bits(
				fDevice->FormatInfo().output.format);
		fOutputPreferredFormat.u.raw_audio.channel_count = 2;
		fOutputPreferredFormat.u.raw_audio.frame_rate
			= MultiAudio::convert_to_sample_rate(fDevice->FormatInfo().output.rate);
			// measured in Hertz
		fOutputPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

		if (fOutputPreferredFormat.u.raw_audio.format != 0) {
			AddNodeKind(B_PHYSICAL_OUTPUT);

			// we'll use the consumer's preferred buffer size, if any
			fOutputPreferredFormat.u.raw_audio.buffer_size
				= fDevice->BufferList().return_playback_buffer_size
					* (fOutputPreferredFormat.u.raw_audio.format
							& media_raw_audio_format::B_AUDIO_SIZE_MASK)
					* fOutputPreferredFormat.u.raw_audio.channel_count;
		}
	}

	if (fDevice->Description().input_channel_count > 0) {
		// initialize our preferred format objects
		fInputPreferredFormat.type = B_MEDIA_RAW_AUDIO;
		fInputPreferredFormat.u.raw_audio.format
			= MultiAudio::convert_to_media_format(
				fDevice->FormatInfo().input.format);
		fInputPreferredFormat.u.raw_audio.valid_bits
			= MultiAudio::convert_to_valid_bits(fDevice->FormatInfo().input.format);
		fInputPreferredFormat.u.raw_audio.channel_count = 2;
		fInputPreferredFormat.u.raw_audio.frame_rate
			= MultiAudio::convert_to_sample_rate(fDevice->FormatInfo().input.rate);
			// measured in Hertz
		fInputPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;

		if (fInputPreferredFormat.u.raw_audio.format != 0) {
			AddNodeKind(B_PHYSICAL_INPUT);

			// we'll use the consumer's preferred buffer size, if any
			fInputPreferredFormat.u.raw_audio.buffer_size
				= fDevice->BufferList().return_record_buffer_size
					* (fInputPreferredFormat.u.raw_audio.format
							& media_raw_audio_format::B_AUDIO_SIZE_MASK)
					* fInputPreferredFormat.u.raw_audio.channel_count;
		}
	}

	if (config != NULL) {
		fConfig = *config;
		PRINT_OBJECT(*config);
	}

	fInitStatus = B_OK;
}


MultiAudioNode::~MultiAudioNode()
{
	CALLED();
	fAddOn->GetConfigurationFor(this, NULL);

	_StopOutputThread();
	BMediaEventLooper::Quit();
}


status_t
MultiAudioNode::InitCheck() const
{
	CALLED();
	return fInitStatus;
}


void
MultiAudioNode::GetFlavor(flavor_info* info, int32 id)
{
	CALLED();
	if (info == NULL)
		return;

	info->flavor_flags = 0;
	info->possible_count = 1;
		// one flavor at a time
	info->in_format_count = 0;
		// no inputs
	info->in_formats = 0;
	info->out_format_count = 0;
		// no outputs
	info->out_formats = 0;
	info->internal_id = id;

	info->name = const_cast<char*>("MultiAudioNode Node");
	info->info = const_cast<char*>("The MultiAudioNode node outputs to "
		"multi_audio drivers.");
	info->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_TIME_SOURCE
		| B_PHYSICAL_OUTPUT | B_PHYSICAL_INPUT | B_CONTROLLABLE;
	info->in_format_count = 1;
		// 1 input
	media_format* inFormats = new media_format[info->in_format_count];
	GetFormat(&inFormats[0]);
	info->in_formats = inFormats;

	info->out_format_count = 1;
		// 1 output
	media_format* outFormats = new media_format[info->out_format_count];
	GetFormat(&outFormats[0]);
	info->out_formats = outFormats;
}


void
MultiAudioNode::GetFormat(media_format* format)
{
	CALLED();
	if (format == NULL)
		return;

	format->type = B_MEDIA_RAW_AUDIO;
	format->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	format->u.raw_audio = media_raw_audio_format::wildcard;
}


//#pragma mark - BMediaNode


BMediaAddOn*
MultiAudioNode::AddOn(int32* _internalID) const
{
	CALLED();
	// BeBook says this only gets called if we were in an add-on.
	if (fAddOn != 0 && _internalID != NULL)
		*_internalID = fId;

	return fAddOn;
}


void
MultiAudioNode::Preroll()
{
	CALLED();
	// TODO: Performance opportunity
	BMediaNode::Preroll();
}


status_t
MultiAudioNode::HandleMessage(int32 message, const void* data, size_t size)
{
	CALLED();
	return B_ERROR;
}


void
MultiAudioNode::NodeRegistered()
{
	CALLED();

	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	node_input *currentInput = NULL;
	int32 currentId = 0;

	for (int32 i = 0; i < fDevice->Description().output_channel_count; i++) {
		if (currentInput == NULL
			|| (fDevice->Description().channels[i].designations
					& B_CHANNEL_MONO_BUS) != 0
			|| ((fDevice->Description().channels[currentId].designations
					& B_CHANNEL_STEREO_BUS) != 0
				&& ((fDevice->Description().channels[i].designations
						& B_CHANNEL_LEFT) != 0
					|| (fDevice->Description().channels[i].designations
						& B_CHANNEL_STEREO_BUS) == 0))
			|| ((fDevice->Description().channels[currentId].designations
					& B_CHANNEL_SURROUND_BUS) != 0
				&& ((fDevice->Description().channels[i].designations
						& B_CHANNEL_LEFT) != 0
					|| (fDevice->Description().channels[i].designations
						& B_CHANNEL_SURROUND_BUS) == 0))) {
			PRINT(("NodeRegistered(): creating an input for %" B_PRIi32 "\n",
				i));
			PRINT(("%" B_PRId32 "\t%d\t0x%" B_PRIx32 "\t0x%" B_PRIx32 "\n",
				fDevice->Description().channels[i].channel_id,
				fDevice->Description().channels[i].kind,
				fDevice->Description().channels[i].designations,
				fDevice->Description().channels[i].connectors));

			media_input* input = new media_input;

			input->format = fOutputPreferredFormat;
			input->destination.port = ControlPort();
			input->destination.id = fInputs.CountItems();
			input->node = Node();
			sprintf(input->name, "output %" B_PRId32, input->destination.id);

			currentInput = new node_input(*input, fOutputPreferredFormat);
			currentInput->fPreferredFormat.u.raw_audio.channel_count = 1;
			currentInput->fFormat = currentInput->fPreferredFormat;
			currentInput->fInput.format = currentInput->fPreferredFormat;

			delete currentInput->fResampler;
			currentInput->fResampler = new
				Resampler(currentInput->fPreferredFormat.AudioFormat(),
					fOutputPreferredFormat.AudioFormat());

			currentInput->fChannelId
				= fDevice->Description().channels[i].channel_id;
			fInputs.AddItem(currentInput);

			currentId = i;
		} else {
			PRINT(("NodeRegistered(): adding a channel\n"));
			currentInput->fPreferredFormat.u.raw_audio.channel_count++;
			currentInput->fFormat = currentInput->fPreferredFormat;
			currentInput->fInput.format = currentInput->fPreferredFormat;
		}
		currentInput->fInput.format.u.raw_audio.format
			= media_raw_audio_format::wildcard.format;
	}

	node_output *currentOutput = NULL;
	currentId = 0;

	for (int32 i = fDevice->Description().output_channel_count;
			i < fDevice->Description().output_channel_count
				+ fDevice->Description().input_channel_count; i++) {
		if (currentOutput == NULL
			|| (fDevice->Description().channels[i].designations
					& B_CHANNEL_MONO_BUS) != 0
			|| ((fDevice->Description().channels[currentId].designations
					& B_CHANNEL_STEREO_BUS) != 0
				&& ((fDevice->Description().channels[i].designations
						& B_CHANNEL_LEFT) != 0
					|| (fDevice->Description().channels[i].designations
						& B_CHANNEL_STEREO_BUS) == 0))
			|| ((fDevice->Description().channels[currentId].designations
					& B_CHANNEL_SURROUND_BUS) != 0
				&& ((fDevice->Description().channels[i].designations
						& B_CHANNEL_LEFT) != 0
					|| (fDevice->Description().channels[i].designations
						& B_CHANNEL_SURROUND_BUS) == 0))) {
			PRINT(("NodeRegistered(): creating an output for %" B_PRIi32 "\n",
				i));
			PRINT(("%" B_PRId32 "\t%d\t0x%" B_PRIx32 "\t0x%" B_PRIx32 "\n",
				fDevice->Description().channels[i].channel_id,
				fDevice->Description().channels[i].kind,
				fDevice->Description().channels[i].designations,
				fDevice->Description().channels[i].connectors));

			media_output *output = new media_output;

			output->format = fInputPreferredFormat;
			output->destination = media_destination::null;
			output->source.port = ControlPort();
			output->source.id = fOutputs.CountItems();
			output->node = Node();
			sprintf(output->name, "input %" B_PRId32, output->source.id);

			currentOutput = new node_output(*output, fInputPreferredFormat);
			currentOutput->fPreferredFormat.u.raw_audio.channel_count = 1;
			currentOutput->fOutput.format = currentOutput->fPreferredFormat;

			delete currentOutput->fResampler;
			currentOutput->fResampler = new
				Resampler(fInputPreferredFormat.AudioFormat(),
					currentOutput->fPreferredFormat.AudioFormat());

			currentOutput->fChannelId
				= fDevice->Description().channels[i].channel_id;
			fOutputs.AddItem(currentOutput);

			currentId = i;
		} else {
			PRINT(("NodeRegistered(): adding a channel\n"));
			currentOutput->fPreferredFormat.u.raw_audio.channel_count++;
			currentOutput->fOutput.format = currentOutput->fPreferredFormat;
		}
	}

	// Set up our parameter web
	fWeb = MakeParameterWeb();
	SetParameterWeb(fWeb);

	// Apply configuration
#ifdef PRINTING
	bigtime_t start = system_time();
#endif

	int32 index = 0;
	int32 parameterID = 0;
	const void *data;
	ssize_t size;
	while (fConfig.FindInt32("parameterID", index, &parameterID) == B_OK) {
		if (fConfig.FindData("parameterData", B_RAW_TYPE, index, &data, &size)
				== B_OK) {
			SetParameterValue(parameterID, TimeSource()->Now(), data, size);
		}
		index++;
	}

	PRINT(("apply configuration in: %" B_PRIdBIGTIME "\n",
		system_time() - start));

	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
}


status_t
MultiAudioNode::RequestCompleted(const media_request_info& info)
{
	CALLED();

	if (info.what != media_request_info::B_REQUEST_FORMAT_CHANGE)
		return B_OK;

	FrameRateChangeCookie* cookie
		= (FrameRateChangeCookie*)info.user_data;
	if (cookie == NULL)
		return B_OK;

	BReference<FrameRateChangeCookie> cookieReference(cookie, true);

	// if the request failed, we reset the frame rate
	if (info.status != B_OK) {
		if (cookie->id == PARAMETER_ID_INPUT_FREQUENCY) {
			_SetNodeInputFrameRate(cookie->oldFrameRate);
			if (fDevice->Description().output_rates & B_SR_SAME_AS_INPUT)
				_SetNodeOutputFrameRate(cookie->oldFrameRate);
		} else if (cookie->id == PARAMETER_ID_OUTPUT_FREQUENCY)
			_SetNodeOutputFrameRate(cookie->oldFrameRate);

		// TODO: If we have multiple connections, we should request to change
		// the format back!
	}

	return B_OK;
}


void
MultiAudioNode::SetTimeSource(BTimeSource* timeSource)
{
	CALLED();
}


//	#pragma mark - BBufferConsumer


status_t
MultiAudioNode::AcceptFormat(const media_destination& dest,
	media_format* format)
{
	CALLED();

	// Check to make sure the format is okay, then remove
	// any wildcards corresponding to our requirements.
	if (format == NULL)
		return B_BAD_VALUE;

	node_input *channel = _FindInput(dest);
	if (channel == NULL)
		return B_MEDIA_BAD_DESTINATION;

	if (!format_is_compatible(*format, channel->fPreferredFormat))
		return B_MEDIA_BAD_FORMAT;

	format->SpecializeTo(&channel->fPreferredFormat);
	return B_OK;
}


status_t
MultiAudioNode::GetNextInput(int32* cookie, media_input* _input)
{
	CALLED();
	if (_input == NULL)
		return B_BAD_VALUE;

	if (*cookie >= fInputs.CountItems() || *cookie < 0)
		return B_BAD_INDEX;

	node_input* channel = (node_input*)fInputs.ItemAt(*cookie);
	*_input = channel->fInput;
	*cookie += 1;
	PRINT(("input.format: %" B_PRIu32 "\n",
		channel->fInput.format.u.raw_audio.format));
	return B_OK;
}


void
MultiAudioNode::DisposeInputCookie(int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
}


void
MultiAudioNode::BufferReceived(BBuffer* buffer)
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
			if ((buffer->Flags() & BBuffer::B_SMALL_BUFFER) != 0) {
				fprintf(stderr, "NOT IMPLEMENTED: B_SMALL_BUFFER in "
					"MultiAudioNode::BufferReceived\n");
				// TODO: implement this part
				buffer->Recycle();
			} else {
				media_timed_event event(buffer->Header()->start_time,
					BTimedEventQueue::B_HANDLE_BUFFER, buffer,
					BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr, "EventQueue()->AddEvent(event) in "
						"MultiAudioNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default:
			fprintf(stderr, "unexpected buffer type in "
				"MultiAudioNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}


void
MultiAudioNode::ProducerDataStatus(const media_destination& forWhom,
	int32 status, bigtime_t atPerformanceTime)
{
	node_input* channel = _FindInput(forWhom);
	if (channel == NULL) {
		fprintf(stderr, "invalid destination received in "
			"MultiAudioNode::ProducerDataStatus\n");
		return;
	}

	media_timed_event event(atPerformanceTime, BTimedEventQueue::B_DATA_STATUS,
		&channel->fInput, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);
}


status_t
MultiAudioNode::GetLatencyFor(const media_destination& forWhom,
	bigtime_t* _latency, media_node_id* _timeSource)
{
	CALLED();
	if (_latency == NULL || _timeSource == NULL)
		return B_BAD_VALUE;

	node_input* channel = _FindInput(forWhom);
	if (channel == NULL)
		return B_MEDIA_BAD_DESTINATION;

	*_latency = EventLatency();
	*_timeSource = TimeSource()->ID();
	return B_OK;
}


status_t
MultiAudioNode::Connected(const media_source& producer,
	const media_destination& where, const media_format& with_format,
	media_input* out_input)
{
	CALLED();
	if (out_input == 0) {
		fprintf(stderr, "<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}

	node_input* channel = _FindInput(where);
	if (channel == NULL) {
		fprintf(stderr, "<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	if (with_format.u.raw_audio.frame_rate <= 0
		|| with_format.u.raw_audio.channel_count <= 0
		|| ((with_format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK) == 0))
		return B_BAD_VALUE;

	_UpdateInternalLatency(with_format);

	// record the agreed upon values
	channel->fInput.source = producer;
	channel->fInput.format = with_format;
	*out_input = channel->fInput;

	_StartOutputThreadIfNeeded();

	return B_OK;
}


void
MultiAudioNode::Disconnected(const media_source& producer,
	const media_destination& where)
{
	CALLED();

	node_input* channel = _FindInput(where);
	if (channel == NULL || channel->fInput.source != producer)
		return;

	channel->fInput.source = media_source::null;
	channel->fInput.format = channel->fPreferredFormat;

	BAutolock locker(fBufferLock);
	_FillWithZeros(*channel);
	//GetFormat(&channel->fInput.format);
}


status_t
MultiAudioNode::FormatChanged(const media_source& producer,
	const media_destination& consumer, int32 change_tag,
	const media_format& format)
{
	CALLED();

	node_input* channel = _FindInput(consumer);

	if (channel==NULL) {
		fprintf(stderr, "<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (channel->fInput.source != producer)
		return B_MEDIA_BAD_SOURCE;

	return B_ERROR;
}


status_t
MultiAudioNode::SeekTagRequested(const media_destination& destination,
	bigtime_t targetTime, uint32 flags, media_seek_tag* _seekTag,
	bigtime_t* _taggedTime, uint32* _flags)
{
	CALLED();
	return BBufferConsumer::SeekTagRequested(destination, targetTime, flags,
		_seekTag, _taggedTime, _flags);
}


//	#pragma mark - BBufferProducer


status_t
MultiAudioNode::FormatSuggestionRequested(media_type type, int32 /*quality*/,
	media_format* format)
{
	// FormatSuggestionRequested() is not necessarily part of the format
	// negotiation process; it's simply an interrogation -- the caller
	// wants to see what the node's preferred data format is, given a
	// suggestion by the caller.
	CALLED();

	if (format == NULL) {
		fprintf(stderr, "\tERROR - NULL format pointer passed in!\n");
		return B_BAD_VALUE;
	}

	// this is the format we'll be returning (our preferred format)
	*format = fInputPreferredFormat;

	// a wildcard type is okay; we can specialize it
	if (type == B_MEDIA_UNKNOWN_TYPE)
		type = B_MEDIA_RAW_AUDIO;

	// we only support raw audio
	if (type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}


status_t
MultiAudioNode::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect()
	// process.  We hand out a suggested format, with wildcards for any
	// variations we support.
	CALLED();

	// is this a proposal for our select output?
	node_output* channel = _FindOutput(output);
	if (channel == NULL) {
		fprintf(stderr, "MultiAudioNode::FormatProposal returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// We only support floating-point raw audio, so we always return that,
	// but we supply an error code depending on whether we found the proposal
	// acceptable.
	media_type requestedType = format->type;
	*format = channel->fPreferredFormat;
	if (requestedType != B_MEDIA_UNKNOWN_TYPE
		&& requestedType != B_MEDIA_RAW_AUDIO) {
		fprintf(stderr, "MultiAudioNode::FormatProposal returning "
			"B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	// raw audio or wildcard type, either is okay by us
	return B_OK;
}


status_t
MultiAudioNode::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* format,
	int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}


status_t
MultiAudioNode::GetNextOutput(int32* cookie, media_output* _output)
{
	CALLED();

	if (*cookie < fOutputs.CountItems() && *cookie >= 0) {
		node_output* channel = (node_output*)fOutputs.ItemAt(*cookie);
		*_output = channel->fOutput;
		*cookie += 1;
		return B_OK;
	}
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
MultiAudioNode::SetBufferGroup(const media_source& forSource,
	BBufferGroup* newGroup)
{
	CALLED();

	// is this our output?
	node_output* channel = _FindOutput(forSource);
	if (channel == NULL) {
		fprintf(stderr, "MultiAudioNode::SetBufferGroup returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Are we being passed the buffer group we're already using?
	if (newGroup == channel->fBufferGroup)
		return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point
	// we delete the one we are using and use the specified one instead.
	// If the specified group is NULL, we need to recreate one ourselves, and
	// use *that*.  Note that if we're caching a BBuffer that we requested
	// earlier, we have to Recycle() that buffer *before* deleting the buffer
	// group, otherwise we'll deadlock waiting for that buffer to be recycled!
	delete channel->fBufferGroup;
		// waits for all buffers to recycle
	if (newGroup != NULL) {
		// we were given a valid group; just use that one from now on
		channel->fBufferGroup = newGroup;
	} else {
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		size_t size = channel->fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(fLatency / BufferDuration() + 1 + 1);
		BBufferGroup* group = new BBufferGroup(size, count);
		if (group == NULL || group->InitCheck() != B_OK) {
			delete group;
			fprintf(stderr, "MultiAudioNode::SetBufferGroup failed to"
				"instantiate a new group.\n");
			return B_ERROR;
		}
		channel->fBufferGroup = group;
	}

	return B_OK;
}


status_t
MultiAudioNode::PrepareToConnect(const media_source& what,
	const media_destination& where, media_format* format,
	media_source* source, char* name)
{
	CALLED();

	// is this our output?
	node_output* channel = _FindOutput(what);
	if (channel == NULL) {
		fprintf(stderr, "MultiAudioNode::PrepareToConnect returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (channel->fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// Allow buffer sizes other than our preferred one.
	media_format compatible = channel->fPreferredFormat;
	compatible.u.raw_audio.buffer_size
		= media_raw_audio_format::wildcard.buffer_size;

	if (!format_is_compatible(*format, compatible))
		return B_MEDIA_BAD_FORMAT;

	format->SpecializeTo(&channel->fPreferredFormat);

	// Now reserve the connection, and return information about it
	channel->fOutput.destination = where;
	channel->fOutput.format = *format;

	*source = channel->fOutput.source;
	strlcpy(name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);
	return B_OK;
}


void
MultiAudioNode::Connect(status_t error, const media_source& source,
	const media_destination& destination, const media_format& format,
	char* name)
{
	CALLED();

	// is this our output?
	node_output* channel = _FindOutput(source);
	if (channel == NULL) {
		fprintf(stderr, "MultiAudioNode::Connect returning (cause: "
			"B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	// If something earlier failed, Connect() might still be called, but with
	// a non-zero error code.  When that happens we simply unreserve the
	// connection and do nothing else.
	if (error != B_OK) {
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and
	// format that we agreed on, and report our connection name again.
	channel->fOutput.destination = destination;
	channel->fOutput.format = format;
	strlcpy(name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = channel->fOutput.format.u.raw_audio.buffer_size * 10000
		/ ((channel->fOutput.format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* channel->fOutput.format.u.raw_audio.channel_count)
		/ ((int32)(channel->fOutput.format.u.raw_audio.frame_rate / 100));

	SetBufferDuration(duration);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(channel->fOutput.destination, &fLatency, &id);
	PRINT(("\tdownstream latency = %" B_PRIdBIGTIME "\n", fLatency));

	fInternalLatency = BufferDuration();
	PRINT(("\tbuffer-filling took %" B_PRIdBIGTIME " usec on this machine\n",
		fInternalLatency));
	//SetEventLatency(fLatency + fInternalLatency);

	// Set up the buffer group for our connection, as long as nobody handed us
	// a buffer group (via SetBufferGroup()) prior to this.  That can happen,
	// for example, if the consumer calls SetOutputBuffersFor() on us from
	// within its Connected() method.
	if (channel->fBufferGroup == NULL)
		_AllocateBuffers(*channel);

	_StartOutputThreadIfNeeded();
}


void
MultiAudioNode::Disconnect(const media_source& what,
	const media_destination& where)
{
	CALLED();

	// is this our output?
	node_output* channel = _FindOutput(what);
	if (channel == NULL) {
		fprintf(stderr, "MultiAudioNode::Disconnect() returning (cause: "
			"B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	// Make sure that our connection is the one being disconnected
	if (where == channel->fOutput.destination
		&& what == channel->fOutput.source) {
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		delete channel->fBufferGroup;
		channel->fBufferGroup = NULL;
	} else {
		fprintf(stderr, "\tDisconnect() called with wrong source/destination ("
			"%" B_PRId32 "/%" B_PRId32 "), ours is (%" B_PRId32 "/%" B_PRId32
			")\n", what.id, where.id, channel->fOutput.source.id,
			channel->fOutput.destination.id);
	}
}


void
MultiAudioNode::LateNoticeReceived(const media_source& what, bigtime_t howMuch,
	bigtime_t performanceTime)
{
	CALLED();

	// is this our output?
	node_output* channel = _FindOutput(what);
	if (channel == NULL)
		return;

	// If we're late, we need to catch up.  Respond in a manner appropriate
	// to our current run mode.
	if (RunMode() == B_RECORDING) {
		// A hardware capture node can't adjust; it simply emits buffers at
		// appropriate points.  We (partially) simulate this by not adjusting
		// our behavior upon receiving late notices -- after all, the hardware
		// can't choose to capture "sooner"....
	} else if (RunMode() == B_INCREASE_LATENCY) {
		// We're late, and our run mode dictates that we try to produce buffers
		// earlier in order to catch up.  This argues that the downstream nodes
		// are not properly reporting their latency, but there's not much we can
		// do about that at the moment, so we try to start producing buffers
		// earlier to compensate.
		fInternalLatency += howMuch;
		SetEventLatency(fLatency + fInternalLatency);

		fprintf(stderr, "\tincreasing latency to %" B_PRIdBIGTIME"\n",
			fLatency + fInternalLatency);
	} else {
		// The other run modes dictate various strategies for sacrificing data
		// quality in the interests of timely data delivery.  The way *we* do
		// this is to skip a buffer, which catches us up in time by one buffer
		// duration.
		/*size_t nSamples = fOutput.format.u.raw_audio.buffer_size / sizeof(float);
		mSamplesSent += nSamples;*/

		fprintf(stderr, "\tskipping a buffer to try to catch up\n");
	}
}


void
MultiAudioNode::EnableOutput(const media_source& what, bool enabled,
	int32* _deprecated_)
{
	CALLED();

	// If I had more than one output, I'd have to walk my list of output
	// records to see which one matched the given source, and then
	// enable/disable that one.  But this node only has one output, so I
	// just make sure the given source matches, then set the enable state
	// accordingly.
	node_output* channel = _FindOutput(what);
	if (channel != NULL)
		channel->fOutputEnabled = enabled;
}


void
MultiAudioNode::AdditionalBufferRequested(const media_source& source,
	media_buffer_id previousBuffer, bigtime_t previousTime,
	const media_seek_tag* previousTag)
{
	CALLED();
	// we don't support offline mode
	return;
}


//	#pragma mark - BMediaEventLooper


void
MultiAudioNode::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	switch (event->type) {
		case BTimedEventQueue::B_START:
			_HandleStart(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_SEEK:
			_HandleSeek(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_WARP:
			_HandleWarp(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_STOP:
			_HandleStop(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			if (RunState() == BMediaEventLooper::B_STARTED)
				_HandleBuffer(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			_HandleDataStatus(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_PARAMETER:
			_HandleParameter(event, lateness, realTimeEvent);
			break;
		default:
			fprintf(stderr,"  unknown event type: %" B_PRId32 "\n",
				event->type);
			break;
	}
}


status_t
MultiAudioNode::_HandleBuffer(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	BBuffer* buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == NULL)
		return B_BAD_VALUE;

	//PRINT(("buffer->Header()->destination: %i\n", buffer->Header()->destination));

	node_input* channel = _FindInput(buffer->Header()->destination);
	if (channel == NULL) {
		buffer->Recycle();
		return B_MEDIA_BAD_DESTINATION;
	}

	// if the buffer is late, we ignore it and report the fact to the producer
	// who sent it to us
	if (RunMode() != B_OFFLINE && RunMode() != B_RECORDING && lateness > 0) {
		// lateness doesn't matter in offline mode or in recording mode
		//mLateBuffers++;
		NotifyLateProducer(channel->fInput.source, lateness, event->event_time);
		fprintf(stderr,"	<- LATE BUFFER: %" B_PRIdBIGTIME "\n", lateness);
		buffer->Recycle();
	} else {
		//WriteBuffer(buffer, *channel);
		// TODO: This seems like a very fragile mechanism to wait until
		// the previous buffer for this channel has been processed...
		if (channel->fBuffer != NULL) {
			PRINT(("MultiAudioNode::HandleBuffer snoozing recycling channelId: "
				"%" B_PRIi32 ", how_early:%" B_PRIdBIGTIME "\n",
				channel->fChannelId, lateness));
			//channel->fBuffer->Recycle();
			snooze(100);
			if (channel->fBuffer != NULL)
				buffer->Recycle();
			else
				channel->fBuffer = buffer;
		} else {
			//PRINT(("MultiAudioNode::HandleBuffer writing channelId: %li, how_early:%Ld\n", channel->fChannelId, howEarly));
			channel->fBuffer = buffer;
		}
	}
	return B_OK;
}


status_t
MultiAudioNode::_HandleDataStatus(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	PRINT(("MultiAudioNode::HandleDataStatus status:%" B_PRIi32 ", lateness:%"
		B_PRIiBIGTIME "\n", event->data, lateness));
	switch (event->data) {
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


status_t
MultiAudioNode::_HandleStart(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	if (RunState() != B_STARTED) {
	}
	return B_OK;
}


status_t
MultiAudioNode::_HandleSeek(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	PRINT(("MultiAudioNode::HandleSeek(t=%" B_PRIdBIGTIME ",d=%" B_PRIi32
			",bd=%" B_PRId64 ")\n",
		event->event_time,event->data,event->bigdata));
	return B_OK;
}


status_t
MultiAudioNode::_HandleWarp(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


status_t
MultiAudioNode::_HandleStop(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
		BTimedEventQueue::B_HANDLE_BUFFER);

	//_StopOutputThread();
	return B_OK;
}


status_t
MultiAudioNode::_HandleParameter(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


//	#pragma mark - BTimeSource


void
MultiAudioNode::SetRunMode(run_mode mode)
{
	CALLED();
	PRINT(("MultiAudioNode::SetRunMode mode:%i\n", mode));
	//BTimeSource::SetRunMode(mode);
}


status_t
MultiAudioNode::TimeSourceOp(const time_source_op_info& op, void* _reserved)
{
	CALLED();
	switch (op.op) {
		case B_TIMESOURCE_START:
			PRINT(("TimeSourceOp op B_TIMESOURCE_START\n"));
			if (RunState() != BMediaEventLooper::B_STARTED) {
				fTimeSourceStarted = true;
				_StartOutputThreadIfNeeded();

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
				_StopOutputThread();
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			PRINT(("TimeSourceOp op B_TIMESOURCE_STOP_IMMEDIATELY\n"));
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				_StopOutputThread();
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


//	#pragma mark - BControllable


status_t
MultiAudioNode::GetParameterValue(int32 id, bigtime_t* lastChange, void* value,
	size_t* size)
{
	CALLED();

	PRINT(("id: %" B_PRIi32 "\n", id));
	BParameter* parameter = NULL;
	for (int32 i = 0; i < fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if (parameter->ID() == id)
			break;
	}

	if (parameter == NULL) {
		// Hmmm, we were asked for a parameter that we don't actually
		// support.  Report an error back to the caller.
		PRINT(("\terror - asked for illegal parameter %" B_PRId32 "\n", id));
		return B_ERROR;
	}

	if (id == PARAMETER_ID_INPUT_FREQUENCY
		|| id == PARAMETER_ID_OUTPUT_FREQUENCY) {
		const multi_format_info& info = fDevice->FormatInfo();

		uint32 rate = id == PARAMETER_ID_INPUT_FREQUENCY
			? info.input.rate : info.output.rate;

		if (*size < sizeof(rate))
			return B_ERROR;

		memcpy(value, &rate, sizeof(rate));
		*size = sizeof(rate);
		return B_OK;
	}

	multi_mix_value_info info;
	multi_mix_value values[2];
	info.values = values;
	info.item_count = 0;
	multi_mix_control* controls = fDevice->MixControlInfo().controls;
	int32 control_id = controls[id - 100].id;

	if (*size < sizeof(float))
		return B_ERROR;

	if (parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
		info.item_count = 1;
		values[0].id = control_id;

		if (parameter->CountChannels() == 2) {
			if (*size < 2*sizeof(float))
				return B_ERROR;
			info.item_count = 2;
			values[1].id = controls[id + 1 - 100].id;
		}
	} else if (parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
		info.item_count = 1;
		values[0].id = control_id;
	}

	if (info.item_count > 0) {
		status_t status = fDevice->GetMix(&info);
		if (status != B_OK) {
			fprintf(stderr, "Failed on DRIVER_GET_MIX\n");
		} else {
			if (parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
				((float*)value)[0] = values[0].gain;
				*size = sizeof(float);

				if (parameter->CountChannels() == 2) {
					((float*)value)[1] = values[1].gain;
					*size = 2*sizeof(float);
				}

				for (uint32 i = 0; i < *size / sizeof(float); i++) {
					PRINT(("GetParameterValue B_CONTINUOUS_PARAMETER value[%"
						B_PRIi32 "]: %f\n", i, ((float*)value)[i]));
				}
			} else if (parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
				BDiscreteParameter* discrete = (BDiscreteParameter*)parameter;
				if (discrete->CountItems() <= 2)
					((int32*)value)[0] = values[0].enable ? 1 : 0;
				else
					((int32*)value)[0] = values[0].mux;

				*size = sizeof(int32);

				for (uint32 i = 0; i < *size / sizeof(int32); i++) {
					PRINT(("GetParameterValue B_DISCRETE_PARAMETER value[%"
						B_PRIi32 "]: %" B_PRIi32 "\n", i, ((int32*)value)[i]));
				}
			}
		}
	}
	return B_OK;
}


void
MultiAudioNode::SetParameterValue(int32 id, bigtime_t performanceTime,
	const void* value, size_t size)
{
	CALLED();
	PRINT(("id: %" B_PRIi32 ", performance_time: %" B_PRIdBIGTIME
		", size: %" B_PRIuSIZE "\n", id, performanceTime, size));

	BParameter* parameter = NULL;
	for (int32 i = 0; i < fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if (parameter->ID() == id)
			break;
	}

	if (parameter == NULL)
		return;

	if (id == PARAMETER_ID_OUTPUT_FREQUENCY
		|| (id == PARAMETER_ID_INPUT_FREQUENCY
			&& (fDevice->Description().output_rates
				& B_SR_SAME_AS_INPUT) != 0)) {
		uint32 rate;
		if (size < sizeof(rate))
			return;
		memcpy(&rate, value, sizeof(rate));

		if (rate == fOutputPreferredFormat.u.raw_audio.frame_rate)
			return;

		// create a cookie RequestCompleted() can get the old frame rate from,
		// if anything goes wrong
		FrameRateChangeCookie* cookie = new(std::nothrow) FrameRateChangeCookie;
		if (cookie == NULL)
			return;

		cookie->oldFrameRate = fOutputPreferredFormat.u.raw_audio.frame_rate;
		cookie->id = id;
		BReference<FrameRateChangeCookie> cookieReference(cookie, true);

		// NOTE: What we should do is call RequestFormatChange() for all
		// connections and change the device's format in RequestCompleted().
		// Unfortunately we need the new buffer size first, which we only get
		// from the device after changing the format. So we do that now and
		// reset it in RequestCompleted(), if something went wrong. This causes
		// the buffers we receive until then to be played incorrectly leading
		// to unpleasant noise.
		float frameRate = MultiAudio::convert_to_sample_rate(rate);
		if (_SetNodeInputFrameRate(frameRate) != B_OK)
			return;

		for (int32 i = 0; i < fInputs.CountItems(); i++) {
			node_input* channel = (node_input*)fInputs.ItemAt(i);
			if (channel->fInput.source == media_source::null)
				continue;

			media_format newFormat = channel->fInput.format;
			newFormat.u.raw_audio.frame_rate = frameRate;
			newFormat.u.raw_audio.buffer_size
				= fOutputPreferredFormat.u.raw_audio.buffer_size;

			int32 changeTag = 0;
			status_t error = RequestFormatChange(channel->fInput.source,
				channel->fInput.destination, newFormat, NULL, &changeTag);
			if (error == B_OK)
				cookie->AcquireReference();
		}

		if (id != PARAMETER_ID_INPUT_FREQUENCY)
			return;
		//Do not return cause we should go in the next if
	}

	if (id == PARAMETER_ID_INPUT_FREQUENCY) {
		uint32 rate;
		if (size < sizeof(rate))
			return;
		memcpy(&rate, value, sizeof(rate));

		if (rate == fInputPreferredFormat.u.raw_audio.frame_rate)
			return;

		// create a cookie RequestCompleted() can get the old frame rate from,
		// if anything goes wrong
		FrameRateChangeCookie* cookie = new(std::nothrow) FrameRateChangeCookie;
		if (cookie == NULL)
			return;

		cookie->oldFrameRate = fInputPreferredFormat.u.raw_audio.frame_rate;
		cookie->id = id;
		BReference<FrameRateChangeCookie> cookieReference(cookie, true);

		// NOTE: What we should do is call RequestFormatChange() for all
		// connections and change the device's format in RequestCompleted().
		// Unfortunately we need the new buffer size first, which we only get
		// from the device after changing the format. So we do that now and
		// reset it in RequestCompleted(), if something went wrong. This causes
		// the buffers we receive until then to be played incorrectly leading
		// to unpleasant noise.
		float frameRate = MultiAudio::convert_to_sample_rate(rate);
		if (_SetNodeOutputFrameRate(frameRate) != B_OK)
			return;

		for (int32 i = 0; i < fOutputs.CountItems(); i++) {
			node_output* channel = (node_output*)fOutputs.ItemAt(i);
			if (channel->fOutput.source == media_source::null)
				continue;

			media_format newFormat = channel->fOutput.format;
			newFormat.u.raw_audio.frame_rate = frameRate;
			newFormat.u.raw_audio.buffer_size
				= fInputPreferredFormat.u.raw_audio.buffer_size;

			int32 changeTag = 0;
			status_t error = RequestFormatChange(channel->fOutput.source,
				channel->fOutput.destination, newFormat, NULL, &changeTag);
			if (error == B_OK)
				cookie->AcquireReference();
		}

		return;
	}

	multi_mix_value_info info;
	multi_mix_value values[2];
	info.values = values;
	info.item_count = 0;
	multi_mix_control* controls = fDevice->MixControlInfo().controls;
	int32 control_id = controls[id - 100].id;

	if (parameter->Type() == BParameter::B_CONTINUOUS_PARAMETER) {
		for (uint32 i = 0; i < size / sizeof(float); i++) {
			PRINT(("SetParameterValue B_CONTINUOUS_PARAMETER value[%" B_PRIi32
				"]: %f\n", i, ((float*)value)[i]));
		}
		info.item_count = 1;
		values[0].id = control_id;
		values[0].gain = ((float*)value)[0];

		if (parameter->CountChannels() == 2) {
			info.item_count = 2;
			values[1].id = controls[id + 1 - 100].id;
			values[1].gain = ((float*)value)[1];
		}
	} else if (parameter->Type() == BParameter::B_DISCRETE_PARAMETER) {
		for (uint32 i = 0; i < size / sizeof(int32); i++) {
			PRINT(("SetParameterValue B_DISCRETE_PARAMETER value[%" B_PRIi32
				"]: %" B_PRIi32 "\n", i, ((int32*)value)[i]));
		}

		BDiscreteParameter* discrete = (BDiscreteParameter*)parameter;
		if (discrete->CountItems() <= 2) {
			info.item_count = 1;
			values[0].id = control_id;
			values[0].enable = ((int32*)value)[0] == 1;
		} else {
			info.item_count = 1;
			values[0].id = control_id;
			values[0].mux = ((uint32*)value)[0];
		}
	}

	if (info.item_count > 0) {
		status_t status = fDevice->SetMix(&info);
		if (status != B_OK)
			fprintf(stderr, "Failed on DRIVER_SET_MIX\n");
	}
}


BParameterWeb*
MultiAudioNode::MakeParameterWeb()
{
	CALLED();
	BParameterWeb* web = new BParameterWeb();

	PRINT(("MixControlInfo().control_count: %" B_PRIi32 "\n",
		fDevice->MixControlInfo().control_count));

	BParameterGroup* generalGroup = web->MakeGroup(B_TRANSLATE("General"));

	const multi_description& description = fDevice->Description();

	if ((description.output_rates & B_SR_SAME_AS_INPUT) != 0) {
		_CreateFrequencyParameterGroup(generalGroup,
			B_TRANSLATE("Input & Output"), PARAMETER_ID_INPUT_FREQUENCY,
			description.input_rates);
	} else {
		_CreateFrequencyParameterGroup(generalGroup, B_TRANSLATE("Input"),
			PARAMETER_ID_INPUT_FREQUENCY, description.input_rates);
		_CreateFrequencyParameterGroup(generalGroup, B_TRANSLATE("Output"),
			PARAMETER_ID_OUTPUT_FREQUENCY, description.output_rates);
	}

	multi_mix_control* controls = fDevice->MixControlInfo().controls;

	for (int i = 0; i < fDevice->MixControlInfo().control_count; i++) {
		if ((controls[i].flags & B_MULTI_MIX_GROUP) != 0
			&& controls[i].parent == 0) {
			PRINT(("NEW_GROUP\n"));
			BParameterGroup* child = web->MakeGroup(
				_GetControlName(controls[i]));

			int32 numParameters = 0;
			_ProcessGroup(child, i, numParameters);
		}
	}

	return web;
}


const char*
MultiAudioNode::_GetControlName(multi_mix_control& control)
{
	if (control.string != S_null)
		return kMultiControlString[control.string];

	return control.name;
}


void
MultiAudioNode::_ProcessGroup(BParameterGroup* group, int32 index,
	int32& numParameters)
{
	CALLED();
	multi_mix_control* parent = &fDevice->MixControlInfo().controls[index];
	multi_mix_control* controls = fDevice->MixControlInfo().controls;

	for (int32 i = 0; i < fDevice->MixControlInfo().control_count; i++) {
		if (controls[i].parent != parent->id)
			continue;

		const char* name = _GetControlName(controls[i]);

		if (controls[i].flags & B_MULTI_MIX_GROUP) {
			PRINT(("NEW_GROUP\n"));
			BParameterGroup* child = group->MakeGroup(name);
			child->MakeNullParameter(100 + i, B_MEDIA_RAW_AUDIO, name,
				B_WEB_BUFFER_OUTPUT);

			int32 num = 1;
			_ProcessGroup(child, i, num);
		} else if (controls[i].flags & B_MULTI_MIX_MUX) {
			PRINT(("NEW_MUX\n"));
			BDiscreteParameter* parameter = group->MakeDiscreteParameter(
				100 + i, B_MEDIA_RAW_AUDIO, name, B_INPUT_MUX);
			if (numParameters > 0) {
				(group->ParameterAt(numParameters - 1))->AddOutput(
					group->ParameterAt(numParameters));
				numParameters++;
			}
			_ProcessMux(parameter, i);
		} else if (controls[i].flags & B_MULTI_MIX_GAIN) {
			PRINT(("NEW_GAIN\n"));
			group->MakeContinuousParameter(100 + i,
				B_MEDIA_RAW_AUDIO, "", B_MASTER_GAIN, "dB",
				controls[i].gain.min_gain, controls[i].gain.max_gain,
				controls[i].gain.granularity);

			if (i + 1 < fDevice->MixControlInfo().control_count
				&& controls[i + 1].master == controls[i].id
				&& (controls[i + 1].flags & B_MULTI_MIX_GAIN) != 0) {
				group->ParameterAt(numParameters)->SetChannelCount(
					group->ParameterAt(numParameters)->CountChannels() + 1);
				i++;
			}

			PRINT(("num parameters: %" B_PRId32 "\n", numParameters));
			if (numParameters > 0) {
				group->ParameterAt(numParameters - 1)->AddOutput(
					group->ParameterAt(numParameters));
				numParameters++;
			}
		} else if (controls[i].flags & B_MULTI_MIX_ENABLE) {
			PRINT(("NEW_ENABLE\n"));
			if (controls[i].string == S_MUTE) {
				group->MakeDiscreteParameter(100 + i,
					B_MEDIA_RAW_AUDIO, name, B_MUTE);
			} else {
				group->MakeDiscreteParameter(100 + i,
					B_MEDIA_RAW_AUDIO, name, B_ENABLE);
			}
			if (numParameters > 0) {
				group->ParameterAt(numParameters - 1)->AddOutput(
					group->ParameterAt(numParameters));
				numParameters++;
			}
		}
	}
}


void
MultiAudioNode::_ProcessMux(BDiscreteParameter* parameter, int32 index)
{
	CALLED();
	multi_mix_control* parent = &fDevice->MixControlInfo().controls[index];
	multi_mix_control* controls = fDevice->MixControlInfo().controls;
	int32 itemIndex = 0;

	for (int32 i = 0; i < fDevice->MixControlInfo().control_count; i++) {
		if (controls[i].parent != parent->id)
			continue;

		if ((controls[i].flags & B_MULTI_MIX_MUX_VALUE) != 0) {
			PRINT(("NEW_MUX_VALUE\n"));
			parameter->AddItem(itemIndex, _GetControlName(controls[i]));
			itemIndex++;
		}
	}
}


void
MultiAudioNode::_CreateFrequencyParameterGroup(BParameterGroup* parentGroup,
	const char* name, int32 parameterID, uint32 rateMask)
{
	BParameterGroup* group = parentGroup->MakeGroup(name);
	BDiscreteParameter* frequencyParam = group->MakeDiscreteParameter(
		parameterID, B_MEDIA_NO_TYPE,
		BString(name) << B_TRANSLATE(" frequency:"),
		B_GENERIC);

	for (int32 i = 0; kSampleRateInfos[i].name != NULL; i++) {
		const sample_rate_info& info = kSampleRateInfos[i];
		if ((rateMask & info.multiAudioRate) != 0) {
			frequencyParam->AddItem(info.multiAudioRate,
				BString(info.name) << " Hz");
		}
	}
}


//	#pragma mark - MultiAudioNode specific functions


int32
MultiAudioNode::_OutputThread()
{
	CALLED();
	multi_buffer_info bufferInfo;
	bufferInfo.info_size = sizeof(multi_buffer_info);
	bufferInfo.playback_buffer_cycle = 0;
	bufferInfo.record_buffer_cycle = 0;

	// init the performance time computation
	{
		BAutolock locker(fBufferLock);
		fTimeComputer.Init(fOutputPreferredFormat.u.raw_audio.frame_rate,
			system_time());
	}

	while (atomic_get(&fQuitThread) == 0) {
		BAutolock locker(fBufferLock);
			// make sure the buffers don't change while we're playing with them

		// send buffer
		fDevice->BufferExchange(&bufferInfo);

		//PRINT(("MultiAudioNode::RunThread: buffer exchanged\n"));
		//PRINT(("MultiAudioNode::RunThread: played_real_time: %Ld\n", bufferInfo.played_real_time));
		//PRINT(("MultiAudioNode::RunThread: played_frames_count: %Ld\n", bufferInfo.played_frames_count));
		//PRINT(("MultiAudioNode::RunThread: buffer_cycle: %li\n", bufferInfo.playback_buffer_cycle));

		for (int32 i = 0; i < fInputs.CountItems(); i++) {
			node_input* input = (node_input*)fInputs.ItemAt(i);

			if (bufferInfo.playback_buffer_cycle >= 0
				&& bufferInfo.playback_buffer_cycle
						< fDevice->BufferList().return_playback_buffers
				&& (input->fOldBufferCycle != bufferInfo.playback_buffer_cycle
					|| fDevice->BufferList().return_playback_buffers == 1)
				&& (input->fInput.source != media_source::null
					|| input->fChannelId == 0)) {
				//PRINT(("playback_buffer_cycle ok input: %li %ld\n", i, bufferInfo.playback_buffer_cycle));

				input->fBufferCycle = (bufferInfo.playback_buffer_cycle - 1
						+ fDevice->BufferList().return_playback_buffers)
					% fDevice->BufferList().return_playback_buffers;

				// update the timesource
				if (input->fChannelId == 0) {
					//PRINT(("updating timesource\n"));
					_UpdateTimeSource(bufferInfo, *input);
				}

				input->fOldBufferCycle = bufferInfo.playback_buffer_cycle;

				if (input->fBuffer != NULL) {
					_FillNextBuffer(*input, input->fBuffer);
					input->fBuffer->Recycle();
					input->fBuffer = NULL;
				} else {
					// put zeros in current buffer
					if (input->fInput.source != media_source::null)
						_WriteZeros(*input, input->fBufferCycle);
					//PRINT(("MultiAudioNode::Runthread WriteZeros\n"));
				}
			} else {
				//PRINT(("playback_buffer_cycle non ok input: %i\n", i));
			}
		}

		PRINT(("MultiAudioNode::RunThread: recorded_real_time: %" B_PRIdBIGTIME
				"\n", bufferInfo.recorded_real_time));
		PRINT(("MultiAudioNode::RunThread: recorded_frames_count: %"
				B_PRId64 "\n", bufferInfo.recorded_frames_count));
		PRINT(("MultiAudioNode::RunThread: record_buffer_cycle: %" B_PRIi32
				"\n", bufferInfo.record_buffer_cycle));

		for (int32 i = 0; i < fOutputs.CountItems(); i++) {
			node_output* output = (node_output*)fOutputs.ItemAt(i);

			// make sure we're both started *and* connected before delivering a
			// buffer
			if (RunState() == BMediaEventLooper::B_STARTED
				&& output->fOutput.destination != media_destination::null) {
				if (bufferInfo.record_buffer_cycle >= 0
					&& bufferInfo.record_buffer_cycle
							< fDevice->BufferList().return_record_buffers
					&& (output->fOldBufferCycle
							!= bufferInfo.record_buffer_cycle
						|| fDevice->BufferList().return_record_buffers == 1)) {
					//PRINT(("record_buffer_cycle ok\n"));

					output->fBufferCycle = bufferInfo.record_buffer_cycle;

					// Get the next buffer of data
					BBuffer* buffer = _FillNextBuffer(bufferInfo, *output);
					if (buffer != NULL) {
						// send the buffer downstream if and only if output is
						// enabled
						status_t err = B_ERROR;
						if (output->fOutputEnabled) {
							err = SendBuffer(buffer, output->fOutput.source,
								output->fOutput.destination);
						}
						if (err != B_OK) {
							buffer->Recycle();
						} else {
							// track how much media we've delivered so far
							size_t numSamples
								= output->fOutput.format.u.raw_audio.buffer_size
									/ (output->fOutput.format.u.raw_audio.format
										& media_raw_audio_format
											::B_AUDIO_SIZE_MASK);
							output->fSamplesSent += numSamples;
						}
					}

					output->fOldBufferCycle = bufferInfo.record_buffer_cycle;
				} else {
					//PRINT(("record_buffer_cycle non ok\n"));
				}
			}
		}
	}

	return B_OK;
}


void
MultiAudioNode::_WriteZeros(node_input& input, uint32 bufferCycle)
{
	//CALLED();

	uint32 channelCount = input.fFormat.u.raw_audio.channel_count;
	uint32 bufferSize = fDevice->BufferList().return_playback_buffer_size;
	size_t stride = fDevice->BufferList().playback_buffers[bufferCycle]
		[input.fChannelId].stride;

	switch (input.fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(float*)dest = 0;
					dest += stride;
				}
			}
			break;

		case media_raw_audio_format::B_AUDIO_DOUBLE:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(double*)dest = 0;
					dest += stride;
				}
			}
			break;

		case media_raw_audio_format::B_AUDIO_INT:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(int32*)dest = 0;
					dest += stride;
				}
			}
			break;

		case media_raw_audio_format::B_AUDIO_SHORT:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(int16*)dest = 0;
					dest += stride;
				}
			}
			break;

		case media_raw_audio_format::B_AUDIO_UCHAR:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(uint8*)dest = 128;
					dest += stride;
				}
			}
			break;

		case media_raw_audio_format::B_AUDIO_CHAR:
			for (uint32 channel = 0; channel < channelCount; channel++) {
				char* dest = _PlaybackBuffer(bufferCycle,
					input.fChannelId + channel);
				for (uint32 i = bufferSize; i > 0; i--) {
					*(int8*)dest = 0;
					dest += stride;
				}
			}
			break;

		default:
			fprintf(stderr, "ERROR in WriteZeros format not handled\n");
	}
}


void
MultiAudioNode::_FillWithZeros(node_input& input)
{
	CALLED();
	for (int32 i = 0; i < fDevice->BufferList().return_playback_buffers; i++)
		_WriteZeros(input, i);
}


void
MultiAudioNode::_FillNextBuffer(node_input& input, BBuffer* buffer)
{
	uint32 channelCount = input.fInput.format.u.raw_audio.channel_count;
	size_t inputSampleSize = input.fInput.format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK;

	uint32 bufferSize = fDevice->BufferList().return_playback_buffer_size;

	if (buffer->SizeUsed() / inputSampleSize / channelCount != bufferSize) {
		_WriteZeros(input, input.fBufferCycle);
		return;
	}

	if (channelCount != input.fFormat.u.raw_audio.channel_count) {
		PRINT(("Channel count is different"));
		return;
	}

	if (input.fResampler != NULL) {
		size_t srcStride = channelCount * inputSampleSize;

		for (uint32 channel = 0; channel < channelCount; channel++) {
			char* src = (char*)buffer->Data() + channel * inputSampleSize;
			char* dst = _PlaybackBuffer(input.fBufferCycle,
							input.fChannelId + channel);
			size_t dstStride = _PlaybackStride(input.fBufferCycle,
							input.fChannelId + channel);

			input.fResampler->Resample(src, srcStride,
				dst, dstStride, bufferSize);
		}
	}
}


status_t
MultiAudioNode::_StartOutputThreadIfNeeded()
{
	CALLED();
	// the thread is already started ?
	if (fThread >= 0)
		return B_OK;

	PublishTime(-50, 0, 0);

	fThread = spawn_thread(_OutputThreadEntry, "multi_audio audio output",
		B_REAL_TIME_PRIORITY, this);
	if (fThread < 0)
		return fThread;

	resume_thread(fThread);
	return B_OK;
}


status_t
MultiAudioNode::_StopOutputThread()
{
	CALLED();
	atomic_set(&fQuitThread, 1);

	wait_for_thread(fThread, NULL);
	fThread = -1;
	return B_OK;
}


void
MultiAudioNode::_AllocateBuffers(node_output &channel)
{
	CALLED();

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = channel.fOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(fLatency / BufferDuration() + 1 + 1);

	PRINT(("\tlatency = %" B_PRIdBIGTIME ", buffer duration = %" B_PRIdBIGTIME
			"\n", fLatency, BufferDuration()));
	PRINT(("\tcreating group of %" B_PRId32 " buffers, size = %" B_PRIuSIZE
			"\n", count, size));
	channel.fBufferGroup = new BBufferGroup(size, count);
}


void
MultiAudioNode::_UpdateTimeSource(multi_buffer_info& info, node_input& input)
{
	//CALLED();
	if (!fTimeSourceStarted)
		return;

	fTimeComputer.AddTimeStamp(info.played_real_time,
		info.played_frames_count);
	PublishTime(fTimeComputer.PerformanceTime(), fTimeComputer.RealTime(),
		fTimeComputer.Drift());
}


BBuffer*
MultiAudioNode::_FillNextBuffer(multi_buffer_info& info, node_output& output)
{
	//CALLED();
	// get a buffer from our buffer group
	//PRINT(("buffer size: %i, buffer duration: %i\n", fOutput.format.u.raw_audio.buffer_size, BufferDuration()));
	//PRINT(("MBI.record_buffer_cycle: %i\n", MBI.record_buffer_cycle));
	//PRINT(("MBI.recorded_real_time: %i\n", MBI.recorded_real_time));
	//PRINT(("MBI.recorded_frames_count: %i\n", MBI.recorded_frames_count));
	if (output.fBufferGroup == NULL)
		return NULL;

	BBuffer* buffer = output.fBufferGroup->RequestBuffer(
		output.fOutput.format.u.raw_audio.buffer_size, BufferDuration());
	if (buffer == NULL) {
		// If we fail to get a buffer (for example, if the request times out),
		// we skip this buffer and go on to the next, to avoid locking up the
		// control thread.
		fprintf(stderr, "Buffer is null");
		return NULL;
	}

	if (fDevice == NULL)
		fprintf(stderr, "fDevice NULL\n");
	if (buffer->Header() == NULL)
		fprintf(stderr, "buffer->Header() NULL\n");
	if (TimeSource() == NULL)
		fprintf(stderr, "TimeSource() NULL\n");

	uint32 channelCount = output.fOutput.format.u.raw_audio.channel_count;
	size_t outputSampleSize = output.fOutput.format.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;

	uint32 bufferSize = fDevice->BufferList().return_record_buffer_size;

	if (output.fResampler != NULL) {
		size_t dstStride = channelCount * outputSampleSize;

		uint32 channelId = output.fChannelId
			- fDevice->Description().output_channel_count;

		for (uint32 channel = 0; channel < channelCount; channel++) {
			char* src = _RecordBuffer(output.fBufferCycle,
									channelId + channel);
			size_t srcStride = _RecordStride(output.fBufferCycle,
									channelId + channel);
			char* dst = (char*)buffer->Data() + channel * outputSampleSize;

			output.fResampler->Resample(src, srcStride, dst, dstStride,
				bufferSize);
		}
	}

	// fill in the buffer header
	media_header* header = buffer->Header();
	header->type = B_MEDIA_RAW_AUDIO;
	header->size_used = output.fOutput.format.u.raw_audio.buffer_size;
	header->time_source = TimeSource()->ID();
	header->start_time = PerformanceTimeFor(info.recorded_real_time);

	return buffer;
}


status_t
MultiAudioNode::GetConfigurationFor(BMessage* message)
{
	CALLED();
	if (message == NULL)
		return B_BAD_VALUE;

	size_t bufferSize = 128;
	void* buffer = malloc(bufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	for (int32 i = 0; i < fWeb->CountParameters(); i++) {
		BParameter* parameter = fWeb->ParameterAt(i);
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER
			&& parameter->Type() != BParameter::B_DISCRETE_PARAMETER)
			continue;

		PRINT(("getting parameter %" B_PRIi32 "\n", parameter->ID()));
		size_t size = bufferSize;
		bigtime_t lastChange;
		status_t err;
		while ((err = GetParameterValue(parameter->ID(), &lastChange, buffer,
				&size)) == B_NO_MEMORY && bufferSize < 128 * 1024) {
			bufferSize += 128;
			free(buffer);
			buffer = malloc(bufferSize);
			if (buffer == NULL)
				return B_NO_MEMORY;
		}

		if (err == B_OK && size > 0) {
			message->AddInt32("parameterID", parameter->ID());
			message->AddData("parameterData", B_RAW_TYPE, buffer, size, false);
		} else {
			PRINT(("parameter err: %s\n", strerror(err)));
		}
	}

	free(buffer);
	PRINT_OBJECT(*message);
	return B_OK;
}


node_output*
MultiAudioNode::_FindOutput(media_source source)
{
	node_output* channel = NULL;

	for (int32 i = 0; i < fOutputs.CountItems(); i++) {
		channel = (node_output*)fOutputs.ItemAt(i);
		if (source == channel->fOutput.source)
			break;
	}

	if (source != channel->fOutput.source)
		return NULL;

	return channel;
}


node_input*
MultiAudioNode::_FindInput(media_destination dest)
{
	node_input* channel = NULL;

	for (int32 i = 0; i < fInputs.CountItems(); i++) {
		channel = (node_input*)fInputs.ItemAt(i);
		if (dest == channel->fInput.destination)
			break;
	}

	if (dest != channel->fInput.destination)
		return NULL;

	return channel;
}


node_input*
MultiAudioNode::_FindInput(int32 destinationId)
{
	node_input* channel = NULL;

	for (int32 i = 0; i < fInputs.CountItems(); i++) {
		channel = (node_input*)fInputs.ItemAt(i);
		if (destinationId == channel->fInput.destination.id)
			break;
	}

	if (destinationId != channel->fInput.destination.id)
		return NULL;

	return channel;
}


/*static*/ status_t
MultiAudioNode::_OutputThreadEntry(void* data)
{
	CALLED();
	return static_cast<MultiAudioNode*>(data)->_OutputThread();
}


status_t
MultiAudioNode::_SetNodeInputFrameRate(float frameRate)
{
	// check whether the frame rate is supported
	uint32 multiAudioRate = MultiAudio::convert_from_sample_rate(frameRate);
	if ((fDevice->Description().output_rates & multiAudioRate) == 0)
		return B_BAD_VALUE;

	BAutolock locker(fBufferLock);

	// already set?
	if (fDevice->FormatInfo().output.rate == multiAudioRate)
		return B_OK;

	// set the frame rate on the device
	status_t error = fDevice->SetOutputFrameRate(multiAudioRate);
	if (error != B_OK)
		return error;

	if (frameRate <= 0)
		return B_BAD_VALUE;

	// it went fine -- update all formats
	fOutputPreferredFormat.u.raw_audio.frame_rate = frameRate;
	fOutputPreferredFormat.u.raw_audio.buffer_size
		= fDevice->BufferList().return_playback_buffer_size
			* (fOutputPreferredFormat.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* fOutputPreferredFormat.u.raw_audio.channel_count;

	for (int32 i = 0; node_input* channel = (node_input*)fInputs.ItemAt(i);
			i++) {
		channel->fPreferredFormat.u.raw_audio.frame_rate = frameRate;
		channel->fPreferredFormat.u.raw_audio.buffer_size
			= fOutputPreferredFormat.u.raw_audio.buffer_size;

		channel->fFormat.u.raw_audio.frame_rate = frameRate;
		channel->fFormat.u.raw_audio.buffer_size
			= fOutputPreferredFormat.u.raw_audio.buffer_size;

		channel->fInput.format.u.raw_audio.frame_rate = frameRate;
		channel->fInput.format.u.raw_audio.buffer_size
			= fOutputPreferredFormat.u.raw_audio.buffer_size;
	}

	// make sure the time base is reset
	fTimeComputer.SetFrameRate(frameRate);

	// update internal latency
	_UpdateInternalLatency(fOutputPreferredFormat);

	return B_OK;
}


status_t
MultiAudioNode::_SetNodeOutputFrameRate(float frameRate)
{
	// check whether the frame rate is supported
	uint32 multiAudioRate = MultiAudio::convert_from_sample_rate(frameRate);
	if ((fDevice->Description().input_rates & multiAudioRate) == 0)
		return B_BAD_VALUE;

	BAutolock locker(fBufferLock);

	// already set?
	if (fDevice->FormatInfo().input.rate == multiAudioRate)
		return B_OK;

	// set the frame rate on the device
	status_t error = fDevice->SetInputFrameRate(multiAudioRate);
	if (error != B_OK)
		return error;

	if (frameRate <= 0
		|| fInputPreferredFormat.u.raw_audio.channel_count <= 0
		|| ((fInputPreferredFormat.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK) == 0))
		return B_BAD_VALUE;

	// it went fine -- update all formats
	fInputPreferredFormat.u.raw_audio.frame_rate = frameRate;
	fInputPreferredFormat.u.raw_audio.buffer_size
		= fDevice->BufferList().return_record_buffer_size
			* (fInputPreferredFormat.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* fInputPreferredFormat.u.raw_audio.channel_count;

	for (int32 i = 0; node_output* channel = (node_output*)fOutputs.ItemAt(i);
			i++) {
		channel->fPreferredFormat.u.raw_audio.frame_rate = frameRate;
		channel->fPreferredFormat.u.raw_audio.buffer_size
			= fInputPreferredFormat.u.raw_audio.buffer_size;

		channel->fOutput.format.u.raw_audio.frame_rate = frameRate;
		channel->fOutput.format.u.raw_audio.buffer_size
			= fInputPreferredFormat.u.raw_audio.buffer_size;
	}

	// make sure the time base is reset
	fTimeComputer.SetFrameRate(frameRate);

	// update internal latency
	_UpdateInternalLatency(fInputPreferredFormat);

	return B_OK;
}


void
MultiAudioNode::_UpdateInternalLatency(const media_format& format)
{
	// use half a buffer length latency
	fInternalLatency = format.u.raw_audio.buffer_size * 10000 / 2
		/ ((format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* format.u.raw_audio.channel_count)
		/ ((int32)(format.u.raw_audio.frame_rate / 100));

	PRINT(("  internal latency = %" B_PRIdBIGTIME "\n", fInternalLatency));

	SetEventLatency(fInternalLatency);
}
