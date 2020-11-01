/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */


#include "OpenSoundNode.h"

#include <AutoDeleter.h>
#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Debug.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <scheduler.h>
#include <String.h>

#include <new>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
  #define PRINTING
#endif
#include "debug.h"

#include "OpenSoundDevice.h"
#include "OpenSoundDeviceEngine.h"
#include "OpenSoundDeviceMixer.h"
#include "SupportFunctions.h"

using std::nothrow;


class FunctionTracer {
public:
	FunctionTracer(const char* functionName)
		: fFunctionName(functionName)
	{
		printf("OpenSoundNode::%s()\n", fFunctionName.String());
	}
	 ~FunctionTracer()
	{
		printf("OpenSoundNode::%s() - leave\n", fFunctionName.String());
	}
	BString	fFunctionName;
};


// debugging
#ifdef TRACE
#	undef TRACE
#endif
#ifdef CALLED
#	undef CALLED
#endif
//#define TRACE_OSS_NODE
#ifdef TRACE_OSS_NODE
#	define TRACE(x...)		printf(x)
#	define CALLED(x...)		FunctionTracer _ft(__FUNCTION__)
#	define PRINTING
#else
#	define TRACE(x...)
#	define CALLED(x...)
#endif


class OpenSoundNode::NodeInput {
public:
	NodeInput(const media_input& input, int engineIndex, int ossFormatFlags,
			OpenSoundNode* node)
		: fNode(node),
		  fEngineIndex(engineIndex),
		  fRealEngine(NULL),
		  fOSSFormatFlags(ossFormatFlags),

		  fInput(input),
		  fPreferredFormat(input.format),
		  	// Keep a version of the original preferred format,
		  	// in case we are re-connected and need to start "clean"

		  fThread(-1),
		  fBuffers(4),

		  fTestTonePhase(0)
	{
		CALLED();

		fInput.format.u.raw_audio.format
			= media_raw_audio_format::wildcard.format;
	}

	~NodeInput()
	{
		CALLED();

		fNode->_StopPlayThread(this);

		RecycleAllBuffers();
	}

	status_t Write(void* data, size_t size)
	{
		CALLED();

		ssize_t written = fRealEngine->Write(data, size);

		if (written < 0)
			return (status_t)written;
		if (written < (ssize_t)size)
			return B_IO_ERROR;

		return B_OK;
	}

	void WriteTestTone(size_t bytes)
	{
		// phase of the sine wave
		uint8 buffer[bytes];
		float sampleRate = fInput.format.u.raw_audio.frame_rate;

		const static int kSineBuffer[48] = {
			0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
			28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
			28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
			0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
			-28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
			-28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
		};

		short* b = (short*)buffer;
			// TODO: assumes 16 bit samples!
		int32 channels = fInput.format.u.raw_audio.channel_count;
		int32 frames = bytes / bytes_per_frame(fInput.format);
		for (int32 i = 0; i < frames; i ++) {
			// convert sample rate from 48000 to connected format
			uint32 p = (uint32)((fTestTonePhase * sampleRate) / 48000);

			// prevent phase from integer overflow
			fTestTonePhase = (fTestTonePhase + 1) % 4800;
			for (int32 k = 0; k < channels; k++)
				b[k] = kSineBuffer[p % 48];
			b += channels;
		}

		ssize_t written = fRealEngine->Write(buffer, bytes);
		if (written != (ssize_t)bytes) {
			// error
		}
	}

	void RecycleAllBuffers()
	{
		CALLED();

		// make sure all buffers are recycled, or we might hang
		// when told to quit
		while (BBuffer* buffer = (BBuffer*)fBuffers.RemoveItem((int32)0))
			buffer->Recycle();
	}

	OpenSoundNode*			fNode;
	int32					fEngineIndex;
	OpenSoundDeviceEngine*	fRealEngine;
		// engine it's connected to. can be a shadow one (!= fEngineIndex)
	int						fOSSFormatFlags;
		// AFMT_* flags for this input
	media_input				fInput;
	media_format 			fPreferredFormat;

	thread_id				fThread;
	BList					fBuffers;
		// contains BBuffer* pointers that have not yet played

	uint32					fTestTonePhase;
};


class OpenSoundNode::NodeOutput {
public:
	NodeOutput(const media_output& output, const media_format& format)
		: fNode(NULL),
		  fEngineIndex(-1),
		  fRealEngine(NULL),
		  fOSSFormatFlags(0),

		  fOutput(output),
		  fPreferredFormat(format),

		  fThread(-1),
		  fBufferGroup(NULL),
		  fUsingOwnBufferGroup(true),
		  fOutputEnabled(true),

		  fSamplesSent(0)
	{
		CALLED();
	}

	~NodeOutput()
	{
		CALLED();

		fNode->_StopRecThread(this);

		FreeBuffers();
	}

	status_t AllocateBuffers(bigtime_t bufferDuration, bigtime_t latency)
	{
		TRACE("NodeOutput::AllocateBuffers(bufferDuration = %lld, "
			"latency = %lld)\n", bufferDuration, latency);

		FreeBuffers();

		// allocate enough buffers to span our downstream latency, plus one
		size_t size = fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(latency / bufferDuration + 1 + 1);

		fBufferGroup = new (nothrow) BBufferGroup(size, count);
		fUsingOwnBufferGroup = true;
		return fBufferGroup != NULL ? fBufferGroup->InitCheck() : B_NO_MEMORY;
	}

	status_t SetExternalBuffers(BBufferGroup* bufferGroup)
	{
		TRACE("NodeOutput::SetExternalBuffers(%p)\n", bufferGroup);

		fBufferGroup = bufferGroup;
		fUsingOwnBufferGroup = false;
		return fBufferGroup->InitCheck();
	}

	void FreeBuffers()
	{
		TRACE("NodeOutput::FreeBuffers(): %p (own %d)\n", fBufferGroup,
			fUsingOwnBufferGroup);
// TODO: it is not clear to me how buffer group responsibility is supposed
// to work properly. Appearantly, a consumer can use SetOutputBuffers(),
// which is a deprecated call in the BeOS API, with "willReclaim == true".
// In that case, we would not be responsible for deleting these buffers,
// but I don't understand what mechanism makes sure that we know about this.
// The documentation for SetBufferGroup() says you are supposed to delete
// the given buffer group. In any case, the fUsingOwnBufferGroup is correclty
// maintained as far as we are concerned, but I delete the buffers anyways,
// which is what the code was doing from the beginning and that worked. I
// have not tested yet, whether an external buffer group is passed to the node
// from the system mixer.

//		if (fUsingOwnBufferGroup)
			delete fBufferGroup;
		fBufferGroup = NULL;
	}

	BBuffer* FillNextBuffer(bigtime_t bufferDuration)
	{
		if (fBufferGroup == NULL)
			return NULL;

		BBuffer* buffer = fBufferGroup->RequestBuffer(
			fOutput.format.u.raw_audio.buffer_size, bufferDuration);

		// if we fail to get a buffer (for example, if the request times out),
		// we skip this buffer and go on to the next, to avoid locking up the
		// control thread
		if (!buffer)
			return NULL;

		// now fill it with data
		ssize_t sizeUsed = fRealEngine->Read(buffer->Data(),
			fOutput.format.u.raw_audio.buffer_size);
		if (sizeUsed < 0) {
			TRACE("NodeOutput::%s: %s\n", __FUNCTION__,
				strerror(sizeUsed));
			buffer->Recycle();
			return NULL;
		}
		if (sizeUsed < (ssize_t)fOutput.format.u.raw_audio.buffer_size) {
			TRACE("NodeOutput::%s: requested %d, got %d\n", __FUNCTION__,
				fOutput.format.u.raw_audio.buffer_size, sizeUsed);
		}

		media_header* hdr = buffer->Header();
		if (hdr != NULL) {
			hdr->type = B_MEDIA_RAW_AUDIO;
			hdr->size_used = sizeUsed;
		}

		return buffer;
	}

	OpenSoundNode*			fNode;
	int32					fEngineIndex;
	OpenSoundDeviceEngine*	fRealEngine;
		// engine it's connected to. can be a shadow one (!= fEngineIndex)
	int						fOSSFormatFlags;
		// AFMT_* flags for this output
	media_output			fOutput;
	media_format 			fPreferredFormat;

	thread_id				fThread;
	BBufferGroup*			fBufferGroup;
	bool					fUsingOwnBufferGroup;
	bool 					fOutputEnabled;
	uint64 					fSamplesSent;
};


// #pragma mark - OpenSoundNode


OpenSoundNode::OpenSoundNode(BMediaAddOn* addon, const char* name,
		OpenSoundDevice* device, int32 internal_id, BMessage* config)
	: BMediaNode(name),
	  BBufferConsumer(B_MEDIA_RAW_AUDIO),
	  BBufferProducer(B_MEDIA_RAW_AUDIO),
	  BTimeSource(),
	  BMediaEventLooper(),

	  fInitCheckStatus(B_NO_INIT),
	  fDevice(device),

	  fTimeSourceStarted(false),
	  fTimeSourceStartTime(0),

	  fWeb(NULL),
	  fConfig((uint32)0)
{
	CALLED();

	if (fDevice == NULL)
		return;

	fAddOn = addon;
	fId = internal_id;

	AddNodeKind(B_PHYSICAL_OUTPUT);
	AddNodeKind(B_PHYSICAL_INPUT);

	// initialize our preferred format object
	// TODO: this should go away! should use engine's preferred for each afmt.
#if 1
	fPreferredFormat = media_format();
		// set everything to wildcard first
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio = media_multi_audio_format::wildcard;
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	OpenSoundDeviceEngine *engine = fDevice->EngineAt(0);
	if (engine) {
		const oss_audioinfo* ai = engine->Info();
		int fmt = OpenSoundDevice::select_oss_format(ai->oformats);
		fPreferredFormat.u.raw_audio.format
			= OpenSoundDevice::convert_oss_format_to_media_format(fmt);
		fPreferredFormat.u.raw_audio.valid_bits
			= OpenSoundDevice::convert_oss_format_to_valid_bits(fmt);
		// TODO: engine->PreferredChannels() ? (caps & DSP_CH*)
		fPreferredFormat.u.raw_audio.frame_rate
			= OpenSoundDevice::convert_oss_rate_to_media_rate(ai->max_rate);		// measured in Hertz
	}

	// TODO: Use the OSS suggested buffer size via SNDCTL_DSP_GETBLKSIZE ?
	fPreferredFormat.u.raw_audio.buffer_size = DEFAULT_BUFFER_SIZE
		* (fPreferredFormat.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK)
		* fPreferredFormat.u.raw_audio.channel_count;
#endif

	if (config != NULL) {
		fConfig = *config;
		PRINT_OBJECT(fConfig);
	}

	fInitCheckStatus = B_OK;
}


OpenSoundNode::~OpenSoundNode()
{
	CALLED();

	fAddOn->GetConfigurationFor(this, NULL);

	int32 count = fInputs.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (NodeInput*)fInputs.ItemAtFast(i);
	count = fOutputs.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (NodeOutput*)fOutputs.ItemAtFast(i);

	BMediaEventLooper::Quit();

	fWeb = NULL;
}


status_t
OpenSoundNode::InitCheck() const
{
	CALLED();
	return fInitCheckStatus;
}


// #pragma mark - BMediaNode


BMediaAddOn*
OpenSoundNode::AddOn(int32* internal_id) const
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


void
OpenSoundNode::Preroll()
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}


status_t
OpenSoundNode::HandleMessage(int32 message, const void* data, size_t size)
{
	CALLED();
	return B_ERROR;
}


void
OpenSoundNode::NodeRegistered()
{
	CALLED();

	if (fInitCheckStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	TRACE("NodeRegistered: %d engines\n", fDevice->CountEngines());
	for (int32 i = 0; i < fDevice->CountEngines(); i++) {
		OpenSoundDeviceEngine* engine = fDevice->EngineAt(i);
		if (engine == NULL)
			continue;
		// skip shadow engines
		if (engine->Caps() & PCM_CAP_SHADOW)
			continue;
		// skip engines that don't have outputs
		if ((engine->Caps() & PCM_CAP_OUTPUT) == 0)
			continue;

		TRACE("NodeRegistered: engine[%d]: .caps=0x%08x, .oformats=0x%08x\n",
			i, engine->Caps(), engine->Info()->oformats);

		// iterate over all possible OSS formats/encodings and
		// create a NodeInput for each
		for (int32 f = 0; gSupportedFormats[f]; f++) {
			// figure out if the engine supports the given format
			int fmt = gSupportedFormats[f] & engine->Info()->oformats;
			if (fmt == 0)
				continue;
			TRACE("NodeRegistered() : creating an input for engine %i, "
				"format[%i]\n", i, f);

			media_input mediaInput;
			status_t err = engine->PreferredFormatFor(fmt, mediaInput.format);
			if (err < B_OK)
				continue;

			mediaInput.destination.port = ControlPort();
			mediaInput.destination.id = fInputs.CountItems();
			mediaInput.node = Node();
			const char *prefix = "";
			if (strstr(engine->Info()->name, "SPDIF"))
				prefix = "S/PDIF ";
			sprintf(mediaInput.name, "%sOutput %" B_PRId32 " (%s)", prefix,
				mediaInput.destination.id, gSupportedFormatsNames[f]);

			NodeInput* input = new (nothrow) NodeInput(mediaInput, i, fmt,
				this);
			if (input == NULL || !fInputs.AddItem(input)) {
				delete input;
				continue;
			}
		}
	}

	for (int32 i = 0; i < fDevice->CountEngines(); i++) {
		OpenSoundDeviceEngine* engine = fDevice->EngineAt(i);
		if (engine == NULL)
			continue;
		// skip shadow engines
		if (engine->Caps() & PCM_CAP_SHADOW)
			continue;
		// skip engines that don't have inputs
		if ((engine->Caps() & PCM_CAP_INPUT) == 0)
			continue;

		TRACE("NodeRegistered: engine[%d]: .caps=0x%08x, .iformats=0x%08x\n",
			i, engine->Caps(), engine->Info()->iformats);

		for (int32 f = 0; gSupportedFormats[f]; f++) {
			int fmt = gSupportedFormats[f] & engine->Info()->iformats;
			if (fmt == 0)
				continue;
			TRACE("NodeRegistered() : creating an output for engine %i, "
				"format[%i]\n", i, f);

			media_format preferredFormat;
			status_t err = engine->PreferredFormatFor(fmt, preferredFormat);
			if (err < B_OK)
				continue;

			media_output mediaOutput;

			mediaOutput.format = preferredFormat;
			mediaOutput.destination = media_destination::null;
			mediaOutput.source.port = ControlPort();
			mediaOutput.source.id = fOutputs.CountItems();
			mediaOutput.node = Node();
			const char *prefix = "";
			if (strstr(engine->Info()->name, "SPDIF"))
				prefix = "S/PDIF ";
			sprintf(mediaOutput.name, "%sInput %" B_PRId32 " (%s)", prefix,
				mediaOutput.source.id, gSupportedFormatsNames[f]);

			NodeOutput* output = new (nothrow) NodeOutput(mediaOutput,
				preferredFormat);
			if (output == NULL || !fOutputs.AddItem(output)) {
				delete output;
				continue;
			}
//			output->fPreferredFormat.u.raw_audio.channel_count
//				= engine->Info()->max_channels;
			output->fOutput.format = output->fPreferredFormat;
			output->fEngineIndex = i;
			output->fOSSFormatFlags = fmt;
			output->fNode = this;
		}
	}

	// set up our parameter web
	fWeb = MakeParameterWeb();
	SetParameterWeb(fWeb);

	// apply configuration
#ifdef TRACE_OSS_NODE
	bigtime_t start = system_time();
#endif

	int32 index = 0;
	int32 parameterID = 0;
	while (fConfig.FindInt32("parameterID", index, &parameterID) == B_OK) {
		const void* data;
		ssize_t size;
		if (fConfig.FindData("parameterData", B_RAW_TYPE, index, &data,
			&size) == B_OK) {
			SetParameterValue(parameterID, TimeSource()->Now(), data, size);
		}
		index++;
	}

	TRACE("apply configuration in : %lldµs\n", system_time() - start);

	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
}


status_t
OpenSoundNode::RequestCompleted(const media_request_info& info)
{
	CALLED();
	return B_OK;
}


void
OpenSoundNode::SetTimeSource(BTimeSource* timeSource)
{
	CALLED();
}


// #pragma mark - BBufferConsumer


//!	Someone, probably the producer, is asking you about this format.
//	Give your honest opinion, possibly modifying *format. Do not ask
//	upstream producer about the format, since he's synchronously
//	waiting for your reply.
status_t
OpenSoundNode::AcceptFormat(const media_destination& dest,
	media_format* format)
{
	// Check to make sure the format is okay, then remove
	// any wildcards corresponding to our requirements.

	CALLED();

	NodeInput* channel = _FindInput(dest);

	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::AcceptFormat()"
			" - B_MEDIA_BAD_DESTINATION");
		return B_MEDIA_BAD_DESTINATION;
			// we only have one input so that better be it
	}

	if (format == NULL) {
		fprintf(stderr, "OpenSoundNode::AcceptFormat() - B_BAD_VALUE\n");
		return B_BAD_VALUE;
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

	OpenSoundDeviceEngine* engine = fDevice->NextFreeEngineAt(
		channel->fEngineIndex, false);
	if (!engine)
		return B_BUSY;

	status_t err = engine->AcceptFormatFor(channel->fOSSFormatFlags, *format,
		false);
	if (err < B_OK)
		return err;

	channel->fRealEngine = engine;

/*
	if ( format->type != B_MEDIA_RAW_AUDIO ) {
		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
*/

	//channel->fInput.format = channel->fPreferredFormat;
	channel->fInput.format = *format;

	/*if(format->u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT
		&& channel->fPreferredFormat.u.raw_audio.format
			== media_raw_audio_format::B_AUDIO_SHORT)
		format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	else*/
/*
	format->u.raw_audio.format = channel->fPreferredFormat.u.raw_audio.format;
	format->u.raw_audio.valid_bits
		= channel->fPreferredFormat.u.raw_audio.valid_bits;

	format->u.raw_audio.frame_rate
		= channel->fPreferredFormat.u.raw_audio.frame_rate;
	format->u.raw_audio.channel_count
		= channel->fPreferredFormat.u.raw_audio.channel_count;
	format->u.raw_audio.byte_order
		= B_MEDIA_HOST_ENDIAN;

	format->u.raw_audio.buffer_size
		= DEFAULT_BUFFER_SIZE
			* (format->u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* format->u.raw_audio.channel_count;
*/

//	media_format myFormat;
//	GetFormat(&myFormat);
//	if (!format_is_acceptible(*format,myFormat)) {
//		fprintf(stderr,"<- B_MEDIA_BAD_FORMAT\n");
//		return B_MEDIA_BAD_FORMAT;
//	}

	return B_OK;
}


status_t
OpenSoundNode::GetNextInput(int32* cookie, media_input* out_input)
{
	CALLED();

	// let's not crash even if they are stupid
	if (out_input == NULL) {
		// no place to write!
		fprintf(stderr,"OpenSoundNode::GetNextInput() - B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}

	if (*cookie >= fInputs.CountItems() || *cookie < 0)
		return B_BAD_INDEX;

	NodeInput* channel = (NodeInput*)fInputs.ItemAt(*cookie);
	*out_input = channel->fInput;
	*cookie += 1;

	TRACE("input.format : %u\n", channel->fInput.format.u.raw_audio.format);

	return B_OK;
}


void
OpenSoundNode::DisposeInputCookie(int32 cookie)
{
	CALLED();
	// nothing to do since our cookies are just integers
}


void
OpenSoundNode::BufferReceived(BBuffer* buffer)
{
	CALLED();

	switch (buffer->Header()->type) {
//		case B_MEDIA_PARAMETERS:
//		{
//			status_t status = ApplyParameterData(buffer->Data(),
//				buffer->SizeUsed());
//			if (status != B_OK) {
//				fprintf(stderr, "ApplyParameterData() in "
//					"OpenSoundNode::BufferReceived() failed: %s\n",
//					strerror(status));
//			}
//			buffer->Recycle();
//			break;
//		}
		case B_MEDIA_RAW_AUDIO:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr, "OpenSoundNode::BufferReceived() - "
					"B_SMALL_BUFFER not implemented\n");
				// TODO: implement this part
				buffer->Recycle();
			} else {
				media_timed_event event(buffer->Header()->start_time,
					BTimedEventQueue::B_HANDLE_BUFFER, buffer,
					BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr, "OpenSoundNode::BufferReceived() - "
						"EventQueue()->AddEvent() failed: %s\n",
						strerror(status));
					buffer->Recycle();
				}
			}
			break;
		default:
			fprintf(stderr, "OpenSoundNode::BufferReceived() - unexpected "
				"buffer type\n");
			buffer->Recycle();
			break;
	}
}


void
OpenSoundNode::ProducerDataStatus(const media_destination& for_whom,
	int32 status, bigtime_t at_performance_time)
{
	CALLED();

	NodeInput* channel = _FindInput(for_whom);

	if (channel == NULL) {
		fprintf(stderr,"OpenSoundNode::ProducerDataStatus() - "
			"invalid destination\n");
		return;
	}

//	TRACE("************ ProducerDataStatus: queuing event ************\n");
//	TRACE("************ status=%d ************\n", status);

	media_timed_event event(at_performance_time,
		BTimedEventQueue::B_DATA_STATUS, &channel->fInput,
		BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);
}


status_t
OpenSoundNode::GetLatencyFor(const media_destination& for_whom,
	bigtime_t* out_latency, media_node_id* out_timesource)
{
	CALLED();

	if (out_latency == NULL || out_timesource == NULL) {
		fprintf(stderr,"OpenSoundNode::GetLatencyFor() - B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}

	NodeInput* channel = _FindInput(for_whom);

	if (channel == NULL || channel->fRealEngine == NULL) {
		fprintf(stderr,"OpenSoundNode::GetLatencyFor() - "
			"B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	// start with the node latency (1 buffer duration)
	*out_latency = EventLatency();

	// add the OSS driver buffer's latency as well
	bigtime_t bufferLatency = channel->fRealEngine->PlaybackLatency();
	*out_latency += bufferLatency;

	TRACE("OpenSoundNode::GetLatencyFor() - EventLatency %lld, OSS %lld\n",
		EventLatency(), bufferLatency);

	*out_timesource = TimeSource()->ID();

	return B_OK;
}


status_t
OpenSoundNode::Connected(const media_source& producer,
	const media_destination& where, const media_format& with_format,
	media_input* out_input)
{
	CALLED();

	if (out_input == NULL) {
		fprintf(stderr,"OpenSoundNode::Connected() - B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}

	NodeInput* channel = _FindInput(where);

	if (channel == NULL) {
		fprintf(stderr,"OpenSoundNode::Connected() - "
			"B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	BAutolock L(fDevice->Locker());

	// use one half buffer length latency
	size_t bufferSize = channel->fRealEngine->DriverBufferSize() / 2;
	fInternalLatency = time_for_buffer(bufferSize, with_format);
	TRACE("  internal latency = %lld\n", fInternalLatency);

	// TODO: A global node value is assigned a channel specific value!
	// That can't be correct. For as long as there is only one output
	// in use at a time, this will not matter of course.
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	channel->fInput.source = producer;
	channel->fInput.format = with_format;

	*out_input = channel->fInput;

	// we are sure the thread is started
	_StartPlayThread(channel);

	return B_OK;
}


void
OpenSoundNode::Disconnected(const media_source& producer,
	const media_destination& where)
{
	CALLED();

	NodeInput* channel = _FindInput(where);
	if (channel == NULL) {
		fprintf(stderr,"OpenSoundNode::Disconnected() - "
			"B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	if (channel->fInput.source != producer) {
		fprintf(stderr,"OpenSoundNode::Disconnected() - "
			"B_MEDIA_BAD_SOURCE\n");
		return;
	}

	_StopPlayThread(channel);

	channel->RecycleAllBuffers();

	channel->fInput.source = media_source::null;
	channel->fInput.format = channel->fPreferredFormat;
	if (channel->fRealEngine)
		channel->fRealEngine->Close();
	channel->fRealEngine = NULL;
}


//! The notification comes from the upstream producer, so he's
//	already cool with the format; you should not ask him about it
//	in here.
status_t
OpenSoundNode::FormatChanged(const media_source& producer,
	const media_destination& consumer,  int32 change_tag,
	const media_format& format)
{
	CALLED();
	NodeInput* channel = _FindInput(consumer);

	if (channel == NULL) {
		fprintf(stderr,"OpenSoundNode::FormatChanged() - "
			"B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	if (channel->fInput.source != producer) {
		fprintf(stderr,"OpenSoundNode::FormatChanged() - "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// currently not supported, TODO: implement?
	return B_ERROR;
}


//!	Given a performance time of some previous buffer, retrieve the
//	remembered tag of the closest (previous or exact) performance
//	time. Set *out_flags to 0; the idea being that flags can be
//	added later, and the understood flags returned in *out_flags.
status_t
OpenSoundNode::SeekTagRequested(const media_destination& destination,
	bigtime_t in_target_time, uint32 in_flags, media_seek_tag* out_seek_tag,
	bigtime_t* out_tagged_time, uint32* out_flags)
{
	CALLED();
	return BBufferConsumer::SeekTagRequested(destination, in_target_time,
		in_flags, out_seek_tag, out_tagged_time, out_flags);
}


// #pragma mark - BBufferProducer


//!	FormatSuggestionRequested() is not necessarily part of the format
//	negotiation process; it's simply an interrogation -- the caller wants
//	to see what the node's preferred data format is, given a suggestion by
//	the caller.
status_t
OpenSoundNode::FormatSuggestionRequested(media_type type, int32 /*quality*/,
	media_format* format)
{
	CALLED();

	if (format == NULL) {
		fprintf(stderr, "\tERROR - NULL format pointer passed in!\n");
		return B_BAD_VALUE;
	}

	// this is the format we'll be returning (our preferred format)
	*format = fPreferredFormat;

	// a wildcard type is okay; we can specialize it
	if (type == B_MEDIA_UNKNOWN_TYPE)
		type = B_MEDIA_RAW_AUDIO;

	// TODO: For OSS engines that support encoded formats, we could
	// handle this here. For the time being, we only support raw audio.
	if (type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}


//!	FormatProposal() is the first stage in the BMediaRoster::Connect()
//	process.  We hand out a suggested format, with wildcards for any
//	variations we support.
status_t
OpenSoundNode::FormatProposal(const media_source& output, media_format* format)
{
	CALLED();

	NodeOutput* channel = _FindOutput(output);

	// is this a proposal for our select output?
	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::FormatProposal returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	media_type requestedType = format->type;
#ifdef ENABLE_REC
	OpenSoundDeviceEngine* engine = fDevice->NextFreeEngineAt(
		channel->fEngineIndex, true);

	// We only support raw audio, so we always return that, but we supply an
	// error code depending on whether we found the proposal acceptable.
	status_t err = engine->PreferredFormatFor(channel->fOSSFormatFlags, *format, true);
	if (err < B_OK)
		return err;
#else
	*format = fPreferredFormat;
#endif
	if (requestedType != B_MEDIA_UNKNOWN_TYPE
		&& requestedType != B_MEDIA_RAW_AUDIO
#ifdef ENABLE_NON_RAW_SUPPORT
		 && requestedType != B_MEDIA_ENCODED_AUDIO
#endif
		)
	{
		fprintf(stderr, "OpenSoundNode::FormatProposal returning "
			"B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	// raw audio or wildcard type, either is okay by us
	return B_OK;
}


status_t
OpenSoundNode::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* io_format,
	int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format
	// changes. TODO: implement?
	return B_ERROR;
}


status_t
OpenSoundNode::GetNextOutput(int32* cookie, media_output* out_output)
{
	CALLED();

	if (*cookie >= fOutputs.CountItems() || *cookie < 0)
		return B_BAD_INDEX;

	NodeOutput *channel = (NodeOutput*)fOutputs.ItemAt(*cookie);
	*out_output = channel->fOutput;
	*cookie += 1;
	return B_OK;
}


status_t
OpenSoundNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}


status_t
OpenSoundNode::SetBufferGroup(const media_source& for_source,
	BBufferGroup* newGroup)
{
	CALLED();

	NodeOutput* channel = _FindOutput(for_source);

	// is this our output?
	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::SetBufferGroup() returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Are we being passed the buffer group we're already using?
	if (newGroup == channel->fBufferGroup)
		return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point
	// we delete the one we are using and use the specified one instead.  If
	// the specified group is NULL, we need to recreate one ourselves, and
	// use *that*.  Note that if we're caching a BBuffer that we requested
	// earlier, we have to Recycle() that buffer *before* deleting the buffer
	// group, otherwise we'll deadlock waiting for that buffer to be recycled!
	channel->FreeBuffers();
		// waits for all buffers to recycle
	if (newGroup != NULL) {
		// we were given a valid group; just use that one from now on
		return channel->SetExternalBuffers(newGroup);
	} else {
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		return channel->AllocateBuffers(BufferDuration(), fLatency);
	}
}


//!	PrepareToConnect() is the second stage of format negotiations that happens
//	inside BMediaRoster::Connect().  At this point, the consumer's
//	AcceptFormat() method has been called, and that node has potentially
//	changed the proposed format.  It may also have left wildcards in the
//	format.  PrepareToConnect() *must* fully specialize the format before
//	returning!
status_t
OpenSoundNode::PrepareToConnect(const media_source& what,
	const media_destination& where, media_format* format,
	media_source* out_source, char* out_name)
{
	CALLED();

	status_t err;
	NodeOutput *channel = _FindOutput(what);

	// is this our output?
	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::PrepareToConnect returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (channel->fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	BAutolock L(fDevice->Locker());

	OpenSoundDeviceEngine *engine = fDevice->NextFreeEngineAt(
		channel->fEngineIndex, false);
	if (engine == NULL)
		return B_BUSY;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO) {
		fprintf(stderr, "\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}

	// !!! validate all other fields except for buffer_size here, because the
	// consumer might have supplied different values from AcceptFormat()?
	err = engine->SpecializeFormatFor(channel->fOSSFormatFlags, *format, true);
	if (err < B_OK)
		return err;

	channel->fRealEngine = engine;

#if 0
	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size
		== media_raw_audio_format::wildcard.buffer_size) {
		format->u.raw_audio.buffer_size = 2048;
			// pick something comfortable to suggest
		fprintf(stderr, "\tno buffer size provided, suggesting %lu\n",
			format->u.raw_audio.buffer_size);
	} else {
		fprintf(stderr, "\tconsumer suggested buffer_size %lu\n",
			format->u.raw_audio.buffer_size);
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
OpenSoundNode::Connect(status_t error, const media_source& source,
	const media_destination& destination, const media_format& format,
	char* io_name)
{
	CALLED();

	NodeOutput *channel = _FindOutput(source);

	// is this our output?
	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::Connect returning (cause : "
			"B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	OpenSoundDeviceEngine *engine = channel->fRealEngine;
	if (engine == NULL)
		return;

	// If something earlier failed, Connect() might still be called, but with
	// a non-zero error code.  When that happens we simply unreserve the
	// connection and do nothing else.
	if (error) {
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		engine->Close();
		channel->fRealEngine = NULL;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and
	// format that we agreed on, and report our connection name again.
	channel->fOutput.destination = destination;
	channel->fOutput.format = format;
	strncpy(io_name, channel->fOutput.name, B_MEDIA_NAME_LENGTH);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = channel->fOutput.format.u.raw_audio.buffer_size
		* 10000
		/ ( (channel->fOutput.format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* channel->fOutput.format.u.raw_audio.channel_count)
		/ ((int32)(channel->fOutput.format.u.raw_audio.frame_rate / 100));

	SetBufferDuration(duration);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(channel->fOutput.destination, &fLatency, &id);
	TRACE("\tdownstream latency = %Ld\n", fLatency);

	fInternalLatency = BufferDuration();
	TRACE("\tbuffer-filling took %Ld usec on this machine\n",
		fInternalLatency);
	//SetEventLatency(fLatency + fInternalLatency);

	// Set up the buffer group for our connection, as long as nobody handed us
	// a buffer group (via SetBufferGroup()) prior to this.  That can happen,
	// for example, if the consumer calls SetOutputBuffersFor() on us from
	// within its Connected() method.
	if (channel->fBufferGroup == NULL)
		channel->AllocateBuffers(BufferDuration(), fLatency);

	engine->StartRecording();

	// we are sure the thread is started
	_StartRecThread(channel);
}


void
OpenSoundNode::Disconnect(const media_source& what,
	const media_destination& where)
{
	CALLED();

	NodeOutput *channel = _FindOutput(what);

	// is this our output?
	if (channel == NULL) {
		fprintf(stderr, "OpenSoundNode::Disconnect() returning (cause : "
			"B_MEDIA_BAD_SOURCE)\n");
		return;
	}

	_StopRecThread(channel);

	BAutolock L(fDevice->Locker());

	OpenSoundDeviceEngine* engine = channel->fRealEngine;

	// Make sure that our connection is the one being disconnected
	if (where == channel->fOutput.destination
		&& what == channel->fOutput.source) {
		if (engine)
			engine->Close();
		channel->fRealEngine = NULL;
		channel->fOutput.destination = media_destination::null;
		channel->fOutput.format = channel->fPreferredFormat;
		channel->FreeBuffers();
	} else {
		fprintf(stderr, "\tDisconnect() called with wrong source/destination "
			"(%" B_PRId32 "/%" B_PRId32 "), ours is (%" B_PRId32 "/%" B_PRId32
			")\n", what.id, where.id, channel->fOutput.source.id,
			channel->fOutput.destination.id);
	}
}


void
OpenSoundNode::LateNoticeReceived(const media_source& what, bigtime_t how_much,
	bigtime_t performance_time)
{
	CALLED();

	// is this our output?
	NodeOutput *channel = _FindOutput(what);
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
		// are not properly reporting their latency, but there's not much we
		// can do about that at the moment, so we try to start producing
		// buffers earlier to compensate.
		fInternalLatency += how_much;
		SetEventLatency(fLatency + fInternalLatency);

		fprintf(stderr, "\tincreasing latency to %" B_PRIdBIGTIME "\n",
			fLatency + fInternalLatency);
	} else {
		// The other run modes dictate various strategies for sacrificing data
		// quality in the interests of timely data delivery.  The way *we* do
		// this is to skip a buffer, which catches us up in time by one buffer
		// duration.
//		size_t nSamples = fOutput.format.u.raw_audio.buffer_size
//			/ sizeof(float);
//		mSamplesSent += nSamples;

		fprintf(stderr, "\tskipping a buffer to try to catch up\n");
	}
}


void
OpenSoundNode::EnableOutput(const media_source& what, bool enabled,
	int32* _deprecated_)
{
	CALLED();

	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	NodeOutput *channel = _FindOutput(what);

	if (channel != NULL)
	{
		channel->fOutputEnabled = enabled;
	}
}

void
OpenSoundNode::AdditionalBufferRequested(const media_source& source,
	media_buffer_id prev_buffer, bigtime_t prev_time,
	const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}


// #pragma mark - BMediaEventLooper


void
OpenSoundNode::HandleEvent(const media_timed_event* event, bigtime_t lateness,
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
//			TRACE("HandleEvent: B_HANDLE_BUFFER, RunState= %d\n",
//				RunState());
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
			fprintf(stderr,"  unknown event type: %" B_PRId32 "\n",
				event->type);
			break;
	}
}


// protected
status_t
OpenSoundNode::HandleBuffer(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();

	// TODO: How should we handle late buffers? Drop them?
	// Notify the producer?

	BBuffer* buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == NULL) {
		fprintf(stderr,"OpenSoundNode::HandleBuffer() - B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}

	NodeInput *channel = _FindInput(buffer->Header()->destination);
//	TRACE("buffer->Header()->destination : %i\n",
//		buffer->Header()->destination);

	if (channel == NULL) {
		buffer->Recycle();
		fprintf(stderr,"OpenSoundNode::HandleBuffer() - "
			"B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	media_header* hdr = buffer->Header();
	bigtime_t now = TimeSource()->Now();
	bigtime_t perf_time = hdr->start_time;

	// the how_early calculated here doesn't include scheduling latency
	// because we've already been scheduled to handle the buffer
	bigtime_t how_early = perf_time - EventLatency() - now;

	// if the buffer is late, we ignore it and report the fact to the producer
	// who sent it to us
	if (RunMode() != B_OFFLINE
			// lateness doesn't matter in offline mode...
		&& RunMode() != B_RECORDING
			// ...or in recording mode
		&& how_early < 0LL
		&& false) {
			// TODO: Debug
		//mLateBuffers++;
		NotifyLateProducer(channel->fInput.source, -how_early, perf_time);
		fprintf(stderr,"	<- LATE BUFFER : %" B_PRIdBIGTIME "\n", how_early);
		buffer->Recycle();
	} else {
		fDevice->Locker()->Lock();
		if (channel->fBuffers.CountItems() > 10) {
			fDevice->Locker()->Unlock();
			TRACE("OpenSoundNode::HandleBuffer too many buffers, "
				"recycling\n");
			buffer->Recycle();
		} else {
//			TRACE("OpenSoundNode::HandleBuffer writing channelId : %i,
//				how_early:%lli\n", channel->fEngineIndex, how_early);
			if (!channel->fBuffers.AddItem(buffer))
				buffer->Recycle();
			fDevice->Locker()->Unlock();
		}
	}
	return B_OK;
}


status_t
OpenSoundNode::HandleDataStatus(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
//	CALLED();

	// TODO: this is called mostly whenever the system mixer
	// switches from not sending us buffers (no client connected)
	// to sending buffers, and vice versa. In a Terminal, this
	// can be nicely demonstrated by provoking a system beep while
	// nothing else is using audio playback. Any first beep will
	// start with a small glitch, while more beeps before the last
	// one finished will not have the glitch. I am not sure, but
	// I seem to remember that other audio nodes have the same
	// problem, so it might not be a problem of this implementation.

	BString message("OpenSoundNode::HandleDataStatus status: ");

	switch(event->data) {
		case B_DATA_NOT_AVAILABLE:
			message << "No data";
			break;
		case B_DATA_AVAILABLE:
			message << "Data";
			break;
		case B_PRODUCER_STOPPED:
			message << "Stopped";
			break;
		default:
			message << "???";
			break;
	}

	message << ", lateness: " << lateness;
	printf("%s\n", message.String());

	return B_OK;
}


status_t
OpenSoundNode::HandleStart(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	if (RunState() != B_STARTED) {
		// TODO: What should happen here?
	}
	return B_OK;
}


status_t
OpenSoundNode::HandleSeek(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	TRACE("OpenSoundNode::HandleSeek(t=%lld, d=%li, bd=%lld)\n",
		event->event_time,event->data,event->bigdata);
	return B_OK;
}


status_t
OpenSoundNode::HandleWarp(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


status_t
OpenSoundNode::HandleStop(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
		BTimedEventQueue::B_HANDLE_BUFFER);

	return B_OK;
}


status_t
OpenSoundNode::HandleParameter(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


// #pragma mark - BTimeSource


void
OpenSoundNode::SetRunMode(run_mode mode)
{
	CALLED();
	TRACE("OpenSoundNode::SetRunMode(%d)\n", mode);
	//BTimeSource::SetRunMode(mode);
}


status_t
OpenSoundNode::TimeSourceOp(const time_source_op_info& op, void* _reserved)
{
	CALLED();
	switch(op.op) {
		case B_TIMESOURCE_START:
			TRACE("TimeSourceOp op B_TIMESOURCE_START\n");
			if (RunState() != BMediaEventLooper::B_STARTED) {
				fTimeSourceStarted = true;
				fTimeSourceStartTime = RealTime();

				media_timed_event startEvent(0, BTimedEventQueue::B_START);
				EventQueue()->AddEvent(startEvent);
			}
			break;
		case B_TIMESOURCE_STOP:
			TRACE("TimeSourceOp op B_TIMESOURCE_STOP\n");
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			TRACE("TimeSourceOp op B_TIMESOURCE_STOP_IMMEDIATELY\n");
			if (RunState() == BMediaEventLooper::B_STARTED) {
				media_timed_event stopEvent(0, BTimedEventQueue::B_STOP);
				EventQueue()->AddEvent(stopEvent);
				fTimeSourceStarted = false;
				PublishTime(0, 0, 0);
			}
			break;
		case B_TIMESOURCE_SEEK:
//			TRACE("TimeSourceOp op B_TIMESOURCE_SEEK\n");
printf("TimeSourceOp op B_TIMESOURCE_SEEK, real %" B_PRIdBIGTIME ", "
	"perf %" B_PRIdBIGTIME "\n", op.real_time, op.performance_time);
			BroadcastTimeWarp(op.real_time, op.performance_time);
			break;
		default:
			break;
	}
	return B_OK;
}


// #pragma mark - BControllable


status_t
OpenSoundNode::GetParameterValue(int32 id, bigtime_t* last_change, void* value,
	size_t* ioSize)
{
	CALLED();

	int channelCount = 1;
	int sliderShift = 8;

	OpenSoundDeviceMixer* mixer = fDevice->MixerAt(0);
	if (!mixer)
		return ENODEV;

	TRACE("id : %i, *ioSize=%d\n", id, *ioSize);

	oss_mixext mixext;
	status_t err = mixer->GetExtInfo(id, &mixext);
	if (err < B_OK)
		return err;

	oss_mixer_value mixval;
	mixval.ctrl = mixext.ctrl;
	mixval.timestamp = mixext.timestamp;

	err = mixer->GetMixerValue(&mixval);
	if (err < B_OK)
		return err;

	if (!(mixext.flags & MIXF_READABLE))
		return EINVAL;

	BParameter *parameter = NULL;
	for (int32 i = 0; i < fWeb->CountParameters(); i++) {
		parameter = fWeb->ParameterAt(i);
		if(parameter->ID() == id)
			break;
	}

	if (!parameter)
		return ENODEV;

	TRACE("%s: value = 0x%08x\n", __FUNCTION__, mixval.value);

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
		channelCount = 2;
	case MIXT_SLIDER:
	case MIXT_MONODB:
	case MIXT_MONOSLIDER16:
	case MIXT_MONOSLIDER:
		if (*ioSize < channelCount * sizeof(float))
			return EINVAL;
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
			return EINVAL;
		if (mixext.type == MIXT_STEREOSLIDER16 ||
			mixext.type == MIXT_MONOSLIDER16)
			sliderShift = 16;
		*ioSize = channelCount * sizeof(float);
		((float *)value)[0] = (float)(mixval.value & ((1 << sliderShift) - 1));
		TRACE("%s: value[O] = %f\n", __FUNCTION__, ((float *)value)[0]);
		if (channelCount < 2)
			return B_OK;
		((float *)value)[1] = (float)((mixval.value >> sliderShift)
			& ((1 << sliderShift) - 1));
		TRACE("%s: value[1] = %f\n", __FUNCTION__, ((float *)value)[1]);
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
		TRACE("OpenSoundNode::%s: unknown mixer control type %d\n",
			__FUNCTION__, mixext.type);
	}
	*ioSize = 0;
	return EINVAL;
}


void
OpenSoundNode::SetParameterValue(int32 id, bigtime_t performance_time,
	const void* value, size_t size)
{
	CALLED();

	TRACE("id : %i, performance_time : %lld, size : %i\n", id,
		performance_time, size);

	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	if (mixer == NULL)
		return;

	oss_mixext mixext;
	if (mixer->GetExtInfo(id, &mixext) < B_OK)
		return;
	if (!(mixext.flags & MIXF_WRITEABLE))
		return;

	oss_mixer_value mixval;
	mixval.ctrl = mixext.ctrl;
	mixval.timestamp = mixext.timestamp;

	status_t err = mixer->GetMixerValue(&mixval);
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

	int channelCount = 1;
	int sliderShift = 8;

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
			_PropagateParameterChanges(mixext.ctrl, mixext.type, mixext.id);

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
			channelCount = 2;
		case MIXT_SLIDER:
		case MIXT_MONODB:
		case MIXT_MONOSLIDER16:
		case MIXT_MONOSLIDER:
			if (size < channelCount * sizeof(float))
				return;
			if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
				return;
			if (mixext.type == MIXT_STEREOSLIDER16 ||
				mixext.type == MIXT_MONOSLIDER16)
				sliderShift = 16;
			mixval.value = 0;

			TRACE("-------- sliderShift=%d, v = %08x, v & %08x = %08x\n",
				sliderShift, mixval.value, ((1 << sliderShift) - 1),
				mixval.value & ((1 << sliderShift) - 1));

			mixval.value |= ((int)(((float *)value)[0]))
				& ((1 << sliderShift) - 1);
			if (channelCount > 1) {
				mixval.value |= (((int)(((float *)value)[1]))
					& ((1 << sliderShift) - 1)) << sliderShift;
			}

			TRACE("%s: value = 0x%08x\n", __FUNCTION__, mixval.value);
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
//		case MIXT_MONODB:
//			break;
//		case MIXT_STEREODB:
//			break;
		case MIXT_3D:
			break;
//		case MIXT_MONOSLIDER16:
//			break;
//		case MIXT_STEREOSLIDER16:
//			break;
		default:
			TRACE("OpenSoundNode::%s: unknown mixer control type %d\n",
				__FUNCTION__, mixext.type);
	}

	return;
}


BParameterWeb*
OpenSoundNode::MakeParameterWeb()
{
	CALLED();
	BParameterWeb* web = new BParameterWeb;

	// TODO: the card might change the mixer controls at some point,
	// we should detect it (poll) and recreate the parameter web and
	// re-set it.

	// TODO: cache mixext[...] and poll for changes in their update_counter.

	OpenSoundDeviceMixer* mixer = fDevice->MixerAt(0);
	if (mixer == NULL) {
		// some cards don't have a mixer, just put a placeholder then
		BParameterGroup* child = web->MakeGroup("No mixer");
		child->MakeNullParameter(1, B_MEDIA_UNKNOWN_TYPE, "No Mixer",
			B_GENERIC);
		return web;
	}

	int mixext_count = mixer->CountExtInfos();
	TRACE("OpenSoundNode::MakeParameterWeb %i ExtInfos\n", mixext_count);

	for (int32 i = 0; i < mixext_count; i++) {
		oss_mixext mixext;
		if (mixer->GetExtInfo(i, &mixext) < B_OK)
			continue;

		if (mixext.type == MIXT_DEVROOT) {
			oss_mixext_root* extroot = (oss_mixext_root*)mixext.data;
			TRACE("OpenSoundNode: mixext[%d]: ROOT\n", i);
			int32 nb = 0;
			const char* childName = mixext.extname;
			childName = extroot->id; // extroot->name;
			BParameterGroup *child = web->MakeGroup(childName);
			_ProcessGroup(child, i, nb);
		}
	}

	return web;
}


// #pragma mark - OpenSoundNode specific


void
OpenSoundNode::_ProcessGroup(BParameterGroup *group, int32 index,
	int32& nbParameters)
{
	CALLED();
	// TODO: It looks wrong to use the same mixer in a recursive function!
	OpenSoundDeviceMixer* mixer = fDevice->MixerAt(0);

	int mixext_count = mixer->CountExtInfos();
	for (int32 i = 0; i < mixext_count; i++) {
		oss_mixext mixext;
		if (mixer->GetExtInfo(i, &mixext) < B_OK)
			continue;
		// only keep direct children of that group
		if (mixext.parent != index)
			continue;

		int32 nb = 1;

		TRACE("OpenSoundNode: mixext[%d]: { %s/%s, type=%d, parent=%d, "
			"min=%d, max=%d, flags=0x%08x, control_no=%d, desc=%d, "
			"update_counter=%d }\n", i,
			(mixext.type != MIXT_MARKER) ? mixext.id : "",
			(mixext.type != MIXT_MARKER) ? mixext.extname : "",
			mixext.type, mixext.parent,
			mixext.minvalue, mixext.maxvalue,
			mixext.flags, mixext.control_no,
			mixext.desc, mixext.update_counter);

		// should actually rename the whole group but it's too late there.
		const char *childName = mixext.extname;
		if (mixext.flags & MIXF_MAINVOL)
			childName = "Master Gain";

		const char *sliderUnit = "";//"(linear)";
		if (mixext.flags & MIXF_HZ)
			sliderUnit = "Hz";

		const char *continuousKind = B_GAIN;
		BParameterGroup* child;

		switch (mixext.type) {
		case MIXT_DEVROOT:
			// root item, should be done already
			break;
		case MIXT_GROUP:
			TRACE("OpenSoundNode: mixext[%d]: GROUP\n", i);
			child = group->MakeGroup(childName);
			child->MakeNullParameter(i, B_MEDIA_RAW_AUDIO, childName,
				B_WEB_BUFFER_OUTPUT);
			_ProcessGroup(child, i, nb);
			break;
		case MIXT_ONOFF:
			TRACE("OpenSoundNode: mixext[%d]: ONOFF\n", i);
			// multiaudio node adds 100 to IDs !?
			if (0/*MMC[i].string == S_MUTE*/) {
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName,
					B_MUTE);
			} else {
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName,
					B_ENABLE);
			}
			if (nbParameters > 0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(
					group->ParameterAt(nbParameters));
				nbParameters++;
			}
			break;
		case MIXT_ENUM:
		{
			TRACE("OpenSoundNode: mixext[%d]: ENUM\n", i);
			BDiscreteParameter *parameter =
				group->MakeDiscreteParameter(i, B_MEDIA_RAW_AUDIO, childName,
					B_INPUT_MUX);
			if (nbParameters > 0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(
					group->ParameterAt(nbParameters));
				nbParameters++;
			}
			_ProcessMux(parameter, i);
			break;
		}
		case MIXT_MONODB:
		case MIXT_STEREODB:
			sliderUnit = "dB";
		case MIXT_SLIDER:
		case MIXT_MONOSLIDER16:
		case MIXT_STEREOSLIDER16:
		case MIXT_MONOSLIDER:
			//TRACE("OpenSoundNode: mixext[%d]: MONOSLIDER\n", i);
			//break;
			// fall through
		case MIXT_STEREOSLIDER:
			TRACE("OpenSoundNode: mixext[%d]: [MONO|STEREO]SLIDER\n", i);

			if (mixext.flags & MIXF_MAINVOL)
				continuousKind = B_MASTER_GAIN;

// TODO: find out what this was supposed to do:
//			if (mixext.flags & MIXF_CENTIBEL)
//				true;//step size
//			if (mixext.flags & MIXF_DECIBEL)
//				true;//step size

			group->MakeContinuousParameter(i, B_MEDIA_RAW_AUDIO, childName,
				continuousKind,  sliderUnit, mixext.minvalue, mixext.maxvalue,
				/*TODO: should be "granularity"*/1);

			if (mixext.type == MIXT_STEREOSLIDER ||
				mixext.type == MIXT_STEREOSLIDER16 ||
				mixext.type == MIXT_STEREODB)
				group->ParameterAt(nbParameters)->SetChannelCount(2);

			TRACE("nb parameters : %d\n", nbParameters);
			if (nbParameters > 0) {
				(group->ParameterAt(nbParameters - 1))->AddOutput(
					group->ParameterAt(nbParameters));
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
//		case MIXT_MONODB:
//			TRACE("OpenSoundNode::_ProcessGroup: Skipping obsolete "
//				"MIXT_MONODB\n");
//			break;
//		case MIXT_STEREODB:
//			TRACE("OpenSoundNode::_ProcessGroup: Skipping obsolete "
//				"MIXT_STEREODB\n");
//			break;
//		case MIXT_SLIDER:
//			break;
		case MIXT_3D:
			break;
//		case MIXT_MONOSLIDER16:
//			break;
//		case MIXT_STEREOSLIDER16:
//			break;
		default:
			TRACE("OpenSoundNode::_ProcessGroup: unknown mixer control "
				"type %d\n", mixext.type);
		}
	}

}


void
OpenSoundNode::_ProcessMux(BDiscreteParameter* parameter, int32 index)
{
	CALLED();
	OpenSoundDeviceMixer *mixer = fDevice->MixerAt(0);
	oss_mixer_enuminfo enuminfo;
	status_t err = mixer->GetEnumInfo(index, &enuminfo);
	if (err < B_OK) {
		// maybe there is no list.
		// generate a count form 0
		oss_mixext mixext;
		if (mixer->GetExtInfo(index, &mixext) < B_OK)
			return;

		for (int32 i = 0; i < mixext.maxvalue; i++) {
			BString s;
			s << i;
			parameter->AddItem(i, s.String());
		}
		return;
	}

	for (int32 i = 0; i < enuminfo.nvalues; i++) {
		parameter->AddItem(i, &enuminfo.strings[enuminfo.strindex[i]]);
	}
	return;
}


status_t
OpenSoundNode::_PropagateParameterChanges(int from, int type, const char* id)
{
	CALLED();

	TRACE("OpenSoundNode::_PropagateParameterChanges(from %i, type %i, "
		"id %s)\n", from, type, id);

	OpenSoundDeviceMixer* mixer = fDevice->MixerAt(0);
	if (mixer == NULL)
		return ENODEV;

// TODO: Cortex doesn't like that!
// try timed event
// try checking update_counter+caching
return B_OK;

//	char oldValues[128];
	char newValues[128];
//	size_t oldValuesSize;
	size_t newValuesSize;

	for (int i = 0; i < mixer->CountExtInfos(); i++) {
		oss_mixext mixext;
		status_t err = mixer->GetExtInfo(i, &mixext);
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

//		BParameter *parameter = NULL;
//		for(int32 i=0; i<fWeb->CountParameters(); i++) {
//			parameter = fWeb->ParameterAt(i);
//			if(parameter->ID() == mixext.ctrl)
//				break;
//		}
//
//		if (!parameter)
//			continue;

//		oldValuesSize = 128;
		newValuesSize = 128;
		bigtime_t last;
//		TRACE("OpenSoundNode::%s: comparing mixer control %d\n",
//			__FUNCTION__, mixext.ctrl);
//		if (parameter->GetValue(oldValues, &oldValuesSize, &last) < B_OK)
//			continue;
		if (GetParameterValue(mixext.ctrl, &last, newValues,
				&newValuesSize) < B_OK) {
			continue;
		}
//		if (oldValuesSize != newValuesSize || memcmp(oldValues, newValues,
//				MIN(oldValuesSize, newValuesSize))) {
			TRACE("OpenSoundNode::%s: updating mixer control %d\n",
				__FUNCTION__, mixext.ctrl);
			BroadcastNewParameterValue(last, mixext.ctrl, newValues,
				newValuesSize);
//			BroadcastChangedParameter(mixext.ctrl);
//		}
	}
	return B_OK;
}


int32
OpenSoundNode::_PlayThread(NodeInput* input)
{
	CALLED();
	//set_thread_priority(find_thread(NULL), 5);// TODO:DEBUG
	signal(SIGUSR1, &_SignalHandler);

	OpenSoundDeviceEngine* engine = input->fRealEngine;
	if (!engine || !engine->InUse())
		return B_NO_INIT;
	// skip unconnected or non-busy engines
	if (input->fInput.source == media_source::null
		&& input->fEngineIndex == 0)
		return B_NO_INIT;
	// must be open for write
	ASSERT(engine->OpenMode() & OPEN_WRITE);

	// make writing actually block until the previous buffer played
	size_t driverBufferSize = engine->DriverBufferSize();
	size_t bufferSize = input->fInput.format.u.raw_audio.buffer_size;
	if (driverBufferSize != bufferSize) {
		printf("warning, OSS driver buffer size: %ld, audio buffer "
			"size: %ld", driverBufferSize, bufferSize);
	}

	// cache a silence buffer
	uint8* silenceBuffer = (uint8*)malloc(bufferSize);
	if (silenceBuffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter deleter(silenceBuffer);
	uint8 formatSilence = 0;
	if (input->fInput.format.u.raw_audio.format
			== media_raw_audio_format::B_AUDIO_UCHAR)
		formatSilence = 128;

	memset(silenceBuffer, formatSilence, bufferSize);

	// start by writing the OSS driver buffer size of silence
	// so that the first call to write() already blocks for (almost) the
	// buffer duration
	input->Write(silenceBuffer, bufferSize);

	int64 bytesWritten = 0;
	bigtime_t lastRealTime = RealTime();
	bigtime_t lastPerformanceTime = 0;

	const int32 driftValueCount = 64;
	int32 currentDriftValueIndex = 0;
	float driftValues[driftValueCount];
	for (int32 i = 0; i < driftValueCount; i++)
		driftValues[i] = 1.0;

	do {
		if (!fDevice->Locker()->Lock())
			break;

		TRACE("OpenSoundNode::_PlayThread: buffers: %ld\n",
			input->fBuffers.CountItems());

		BBuffer* buffer = (BBuffer*)input->fBuffers.RemoveItem((int32)0);

		fDevice->Locker()->Unlock();

		if (input->fThread < 0) {
			if (buffer)
				buffer->Recycle();
			break;
		}

//input->WriteTestTone();
//if (buffer)
//	buffer->Recycle();
//continue;

		int32 additionalBytesWritten = 0;
		if (buffer != NULL) {
			if (input->Write(buffer->Data(), buffer->SizeUsed()) == B_OK)
				additionalBytesWritten = buffer->SizeUsed();
			buffer->Recycle();
		} else {
			input->Write(silenceBuffer, bufferSize);
			additionalBytesWritten = bufferSize;
		}

		// TODO: do not assume channel 0 will always be running!
		// update the timesource
		if (input->fEngineIndex == 0 && input->fThread >= 0) {

			bigtime_t realTime = RealTime();
			bigtime_t realPlaybackDuration = realTime - lastRealTime;
			bigtime_t performanceTime
				= time_for_buffer(bytesWritten, input->fInput.format);
			float drift = (double)(performanceTime
				- lastPerformanceTime) / realPlaybackDuration;

			lastPerformanceTime = performanceTime;
			lastRealTime = realTime;

			driftValues[currentDriftValueIndex++] = drift;
			if (currentDriftValueIndex == driftValueCount)
				currentDriftValueIndex = 0;
			drift = 0.0;
			for (int32 i = 0; i < driftValueCount; i++)
				drift += driftValues[i];
			drift /= driftValueCount;

			if (fDevice->Locker()->Lock()) {
				if (input->fThread >= 0)
					_UpdateTimeSource(performanceTime, realTime, drift);
				fDevice->Locker()->Unlock();
			}
		}
		bytesWritten += additionalBytesWritten;

	} while (input->fThread > -1);

	return 0;
}


int32
OpenSoundNode::_RecThread(NodeOutput* output)
{
	CALLED();

	//set_thread_priority(find_thread(NULL), 5);// TODO:DEBUG
	signal(SIGUSR1, &_SignalHandler);

	OpenSoundDeviceEngine *engine = output->fRealEngine;
	if (!engine || !engine->InUse())
		return B_NO_INIT;
	// make sure we're both started *and* connected before delivering a buffer
	if ((RunState() != BMediaEventLooper::B_STARTED)
		|| (output->fOutput.destination == media_destination::null)) {
		return B_NO_INIT;
	}

	// must be open for read
	ASSERT(engine->OpenMode() & OPEN_READ);

#ifdef ENABLE_REC

	fDevice->Locker()->Lock();
	do {
		audio_buf_info abinfo;
//		size_t avail = engine->GetISpace(&abinfo);
//		TRACE("OpenSoundNode::_RunThread: I avail: %d\n", avail);
//
//		// skip if less than 1 buffer
//		if (avail < output->fOutput.format.u.raw_audio.buffer_size)
//			continue;

		fDevice->Locker()->Unlock();
		// Get the next buffer of data
		BBuffer* buffer = _FillNextBuffer(&abinfo, *output);
		fDevice->Locker()->Lock();

		if (buffer) {
			// send the buffer downstream if and only if output is enabled
			status_t err = B_ERROR;
			if (output->fOutputEnabled) {
				err = SendBuffer(buffer, output->fOutput.source,
					output->fOutput.destination);
			}
//			TRACE("OpenSoundNode::_RunThread: I avail: %d, OE %d, %s\n",
//				avail, output->fOutputEnabled, strerror(err));
			if (err != B_OK) {
				buffer->Recycle();
			} else {
				// track how much media we've delivered so far
				size_t nSamples = buffer->SizeUsed()
					/ (output->fOutput.format.u.raw_audio.format
						& media_raw_audio_format::B_AUDIO_SIZE_MASK);
				output->fSamplesSent += nSamples;
//				TRACE("OpenSoundNode::%s: sent %d samples\n",
//					__FUNCTION__, nSamples);
			}

		}
	} while (output->fThread > -1);
	fDevice->Locker()->Unlock();

#endif
	return 0;
}


status_t
OpenSoundNode::_StartPlayThread(NodeInput* input)
{
	CALLED();
	BAutolock L(fDevice->Locker());
	// the thread is already started ?
	if (input->fThread > B_OK)
		return B_OK;

	//allocate buffer free semaphore
//	int bufferCount = MAX(fDevice->fFragments.fragstotal, 2); // XXX

//	fBufferAvailableSem = create_sem(fDevice->MBL.return_playback_buffers - 1,
//		"multi_audio out buffer free");
//	fBufferAvailableSem = create_sem(bufferCount - 1,
//		"OpenSound out buffer free");

//	if (fBufferAvailableSem < B_OK)
//		return B_ERROR;

	input->fThread = spawn_thread(_PlayThreadEntry,
		"OpenSound audio output", B_REAL_TIME_PRIORITY, input);

	if (input->fThread < B_OK) {
//		delete_sem(fBufferAvailableSem);
		return B_ERROR;
	}

	resume_thread(input->fThread);
	return B_OK;
}


status_t
OpenSoundNode::_StopPlayThread(NodeInput* input)
{
	if (input->fThread < 0)
		return B_OK;

	CALLED();

	thread_id th;
	{
		BAutolock L(fDevice->Locker());
		th = input->fThread;
		input->fThread = -1;
		//kill(th, SIGUSR1);
	}
	status_t ret;
	wait_for_thread(th, &ret);

	return B_OK;
}


status_t
OpenSoundNode::_StartRecThread(NodeOutput* output)
{
	CALLED();
	// the thread is already started ?
	if (output->fThread > B_OK)
		return B_OK;

	//allocate buffer free semaphore
//	int bufferCount = MAX(fDevice->fFragments.fragstotal, 2); // XXX

//	fBufferAvailableSem = create_sem(fDevice->MBL.return_playback_buffers - 1,
//		"multi_audio out buffer free");
//	fBufferAvailableSem = create_sem(bufferCount - 1,
//		"OpenSound out buffer free");

//	if (fBufferAvailableSem < B_OK)
//		return B_ERROR;

	output->fThread = spawn_thread(_RecThreadEntry, "OpenSound audio input",
		B_REAL_TIME_PRIORITY, output);

	if (output->fThread < B_OK) {
		//delete_sem(fBufferAvailableSem);
		return B_ERROR;
	}

	resume_thread(output->fThread);
	return B_OK;
}


status_t
OpenSoundNode::_StopRecThread(NodeOutput* output)
{
	if (output->fThread < 0)
		return B_OK;

	CALLED();

	thread_id th = output->fThread;
	output->fThread = -1;
	{
		BAutolock L(fDevice->Locker());
		//kill(th, SIGUSR1);
	}
	status_t ret;
	wait_for_thread(th, &ret);

	return B_OK;
}


void
OpenSoundNode::_UpdateTimeSource(bigtime_t performanceTime,
	bigtime_t realTime, float drift)
{
//	CALLED();

	if (!fTimeSourceStarted)
		return;

	PublishTime(performanceTime, realTime, drift);

//	TRACE("_UpdateTimeSource() perfTime : %lli, realTime : %lli, "
//		"drift : %f\n", perfTime, realTime, drift);
}


BBuffer*
OpenSoundNode::_FillNextBuffer(audio_buf_info* abinfo, NodeOutput& channel)
{
	CALLED();

	BBuffer* buffer = channel.FillNextBuffer(BufferDuration());
	if (!buffer)
		return NULL;

	if (fDevice == NULL)
		fprintf(stderr, "OpenSoundNode::_FillNextBuffer() - fDevice NULL\n");
	if (buffer->Header() == NULL) {
		fprintf(stderr, "OpenSoundNode::_FillNextBuffer() - "
			"buffer->Header() NULL\n");
	}
	if (TimeSource() == NULL) {
		fprintf(stderr, "OpenSoundNode::_FillNextBuffer() - "
			"TimeSource() NULL\n");
	}

	// fill in the buffer header
	media_header* hdr = buffer->Header();
	if (hdr != NULL) {
		hdr->time_source = TimeSource()->ID();
		// TODO: should be system_time() - latency_as_per(abinfo)
		hdr->start_time = PerformanceTimeFor(system_time());
	}

	return buffer;
}


status_t
OpenSoundNode::GetConfigurationFor(BMessage* into_message)
{
	CALLED();

	if (!into_message)
		return B_BAD_VALUE;

	size_t size = 128;
	void* buffer = malloc(size);

	for (int32 i = 0; i < fWeb->CountParameters(); i++) {
		BParameter* parameter = fWeb->ParameterAt(i);
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER
			&& parameter->Type() != BParameter::B_DISCRETE_PARAMETER)
			continue;

		TRACE("getting parameter %i\n", parameter->ID());
		size = 128;
		bigtime_t last_change;
		status_t err;
		while ((err = GetParameterValue(parameter->ID(), &last_change, buffer,
				&size)) == B_NO_MEMORY) {
			size += 128;
			free(buffer);
			buffer = malloc(size);
		}

		if (err == B_OK && size > 0) {
			into_message->AddInt32("parameterID", parameter->ID());
			into_message->AddData("parameterData", B_RAW_TYPE, buffer, size,
				false);
		} else {
			TRACE("parameter err : %s\n", strerror(err));
		}
	}

	free(buffer);

	PRINT_OBJECT(*into_message);

	return B_OK;
}


OpenSoundNode::NodeOutput*
OpenSoundNode::_FindOutput(const media_source& source) const
{
	int32 count = fOutputs.CountItems();
	for (int32 i = 0; i < count; i++) {
		NodeOutput* channel = (NodeOutput*)fOutputs.ItemAtFast(i);
		if (source == channel->fOutput.source)
			return channel;
	}
	return NULL;
}


OpenSoundNode::NodeInput*
OpenSoundNode::_FindInput(const media_destination& dest) const
{
	int32 count = fInputs.CountItems();
	for (int32 i = 0; i < count; i++) {
		NodeInput* channel = (NodeInput*)fInputs.ItemAtFast(i);
		if (dest == channel->fInput.destination)
			return channel;
	}
	return NULL;
}


OpenSoundNode::NodeInput*
OpenSoundNode::_FindInput(int32 destinationId)
{
	int32 count = fInputs.CountItems();
	for (int32 i = 0; i < count; i++) {
		NodeInput* channel = (NodeInput*)fInputs.ItemAtFast(i);
		if (destinationId == channel->fInput.destination.id)
			return channel;
	}
	return NULL;
}


// pragma mark - static


void
OpenSoundNode::_SignalHandler(int sig)
{
	// TODO: what was this intended for, just stopping the threads?
	// (see _StopThreadXXX(), there is a kill call commented out there)
}


int32
OpenSoundNode::_PlayThreadEntry(void* data)
{
	CALLED();
	NodeInput* channel = static_cast<NodeInput*>(data);
	return channel->fNode->_PlayThread(channel);
}


int32
OpenSoundNode::_RecThreadEntry(void* data)
{
	CALLED();
	NodeOutput* channel = static_cast<NodeOutput*>(data);
	return channel->fNode->_RecThread(channel);
}


void
OpenSoundNode::GetFlavor(flavor_info* outInfo, int32 id)
{
	CALLED();
	if (outInfo == NULL)
		return;

	outInfo->flavor_flags = 0;
	outInfo->possible_count = 1;
		// one flavor at a time
	outInfo->in_format_count = 0;
		// no inputs
	outInfo->in_formats = 0;
	outInfo->out_format_count = 0;
		// no outputs
	outInfo->out_formats = 0;
	outInfo->internal_id = id;

	outInfo->name = (char *)"OpenSoundNode Node";
	outInfo->info = (char *)"The OpenSoundNode outputs to OpenSound System v4 "
		"drivers.";
	outInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER | B_TIME_SOURCE
		| B_PHYSICAL_OUTPUT | B_PHYSICAL_INPUT | B_CONTROLLABLE;
	// TODO: If the OSS engine supports outputing encoded audio,
	// we would need to setup a B_MEDIA_ENCODED_AUDIO format here
	outInfo->in_format_count = 1;
		// 1 input
	media_format * informats = new media_format[outInfo->in_format_count];
	GetFormat(&informats[0]);
	outInfo->in_formats = informats;

	outInfo->out_format_count = 1;
		// 1 output
	media_format * outformats = new media_format[outInfo->out_format_count];
	GetFormat(&outformats[0]);
	outInfo->out_formats = outformats;
}


void
OpenSoundNode::GetFormat(media_format* outFormat)
{
	CALLED();
	if (outFormat == NULL)
		return;

	outFormat->type = B_MEDIA_RAW_AUDIO;
	outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	outFormat->u.raw_audio = media_raw_audio_format::wildcard;
}
