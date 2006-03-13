#include <RealtimeAlloc.h>
#include <MediaNode.h>
#include <BufferProducer.h>
#include <TimeSource.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <StopWatch.h>
#include <string.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerUtils.h"
#include "AudioMixer.h"
#include "Resampler.h"
#include "RtList.h"

#define DOUBLE_RATE_MIXING 	0

#if DEBUG > 1
	#define ASSERT_LOCKED()		if (fLocker->IsLocked()) {} else debugger("core not locked, meltdown occurred")
#else
	#define ASSERT_LOCKED()		((void)0)
#endif

/* Mixer channels are identified by a type number, each type number corresponds
 * to the one of the channel masks of enum media_multi_channels.
 *
 * The mixer buffer uses either the same frame rate and same count of frames as the
 * output buffer, or the double frame rate and frame count.
 *
 * All mixer input ring buffers must be an exact multiple of the mixer buffer size,
 * so that we do not get any buffer wrap around during reading from the input buffers.
 * The mixer input is told by constructor (or after a format change by SetMixBufferFormat())
 * of the current mixer buffer propertys, and must allocate a buffer that is an exact multiple,
 */


MixerCore::MixerCore(AudioMixer *node)
 :	fLocker(new BLocker),
	fInputs(new BList),
	fOutput(0),
	fNextInputID(1),
	fRunning(false),
	fStarted(false),
	fOutputEnabled(true),
	fResampler(0),
	fMixBuffer(0),
	fMixBufferFrameRate(0),
	fMixBufferFrameCount(0),
	fMixBufferChannelCount(0),
	fMixBufferChannelTypes(0),
 	fDoubleRateMixing(DOUBLE_RATE_MIXING),
 	fDownstreamLatency(1),
 	fSettings(new MixerSettings),
 	fNode(node),
	fBufferGroup(0),
	fTimeSource(0),
	fMixThread(-1),
	fMixThreadWaitSem(-1),
	fOutputGain(1.0)
{
}

MixerCore::~MixerCore()
{
	delete fSettings;

	delete fLocker;
	delete fInputs;

	ASSERT(fMixThreadWaitSem == -1);
	ASSERT(fMixThread == -1);

	if (fMixBuffer)
		rtm_free(fMixBuffer);

	if (fTimeSource)
		fTimeSource->Release();

	// delete resamplers
	if (fResampler) {
		for (int i = 0; i < fMixBufferChannelCount; i++)
			delete fResampler[i];
		delete [] fResampler;
	}
	
	delete fMixBufferChannelTypes;
}

MixerSettings *
MixerCore::Settings()
{
	return fSettings;
}

void
MixerCore::SetOutputAttenuation(float gain)
{
	ASSERT_LOCKED();
	fOutputGain = gain;
}

MixerInput *
MixerCore::AddInput(const media_input &input)
{
	ASSERT_LOCKED();
	MixerInput *in = new MixerInput(this, input, fMixBufferFrameRate, fMixBufferFrameCount);
	fInputs->AddItem(in);
	return in;
}

MixerOutput *
MixerCore::AddOutput(const media_output &output)
{
	ASSERT_LOCKED();
	if (fOutput) {
		ERROR("MixerCore::AddOutput: already connected\n");
		return fOutput;
	}
	fOutput = new MixerOutput(this, output);
	// the output format might have been adjusted inside MixerOutput
	ApplyOutputFormat();
	
	ASSERT(!fRunning);
	if (fStarted && fOutputEnabled)
		StartMixThread();
		
	return fOutput;
}

bool
MixerCore::RemoveInput(int32 inputID)
{
	ASSERT_LOCKED();
	MixerInput *input;
	for (int i = 0; (input = Input(i)) != 0; i++) {
		if (input->ID() == inputID) {
			fInputs->RemoveItem(i);
			delete input;
			return true;
		}
	}
	return false;
}

bool
MixerCore::RemoveOutput()
{
	ASSERT_LOCKED();
	if (!fOutput)
		return false;

	if (fStarted)
		StopMixThread();

	delete fOutput;
	fOutput = 0;
	fOutputEnabled = true;
	return true;
}

int32
MixerCore::CreateInputID()
{
	ASSERT_LOCKED();
	return fNextInputID++;
}

MixerInput *
MixerCore::Input(int i)
{
	ASSERT_LOCKED();
	return (MixerInput *)fInputs->ItemAt(i);
}

MixerOutput *
MixerCore::Output()
{
	ASSERT_LOCKED();
	return fOutput;
}

void
MixerCore::BufferReceived(BBuffer *buffer, bigtime_t lateness)
{
	ASSERT_LOCKED();
	MixerInput *input;
	int32 id = buffer->Header()->destination;
	for (int i = 0; (input = Input(i)) != 0; i++) {
		if (input->ID() == id) {
			input->BufferReceived(buffer);
			return;
		}
	}
	ERROR("MixerCore::BufferReceived: received buffer for unknown id %ld\n", id);
}
	
void
MixerCore::InputFormatChanged(int32 inputID, const media_multi_audio_format &format)
{
	ASSERT_LOCKED();
	ERROR("MixerCore::InputFormatChanged not handled\n");
}

void
MixerCore::OutputFormatChanged(const media_multi_audio_format &format)
{
	ASSERT_LOCKED();
	
	bool was_started = fStarted;

	if (was_started)
		Stop();

	fOutput->ChangeFormat(format);
	ApplyOutputFormat();

	if (was_started)
		Start();
}

void
MixerCore::ApplyOutputFormat()
{
	ASSERT_LOCKED();
	
	media_multi_audio_format format = fOutput->MediaOutput().format.u.raw_audio;
	
	if (fMixBuffer)
		rtm_free(fMixBuffer);
		
	delete fMixBufferChannelTypes;

	fMixBufferFrameRate = (int32)(0.5 + format.frame_rate);
	fMixBufferFrameCount = frames_per_buffer(format);
	if (fDoubleRateMixing) {
		fMixBufferFrameRate *= 2;
		fMixBufferFrameCount *= 2;
	}
	fMixBufferChannelCount = format.channel_count;
	ASSERT(fMixBufferChannelCount == fOutput->GetOutputChannelCount());
	fMixBufferChannelTypes = new int32 [format.channel_count];
	
	for (int i = 0; i < fMixBufferChannelCount; i++)
		 fMixBufferChannelTypes[i] = ChannelMaskToChannelType(GetChannelMask(i, format.channel_mask));

	fMixBuffer = (float *)rtm_alloc(NULL, sizeof(float) * fMixBufferFrameCount * fMixBufferChannelCount);
	ASSERT(fMixBuffer);

	// delete resamplers
	if (fResampler) {
		for (int i = 0; i < fMixBufferChannelCount; i++)
			delete fResampler[i];
		delete [] fResampler;
	}
	// create new resamplers
	fResampler = new Resampler * [fMixBufferChannelCount];
	for (int i = 0; i < fMixBufferChannelCount; i++)
		fResampler[i] = new Resampler(media_raw_audio_format::B_AUDIO_FLOAT, format.format, format.valid_bits);
	
	TRACE("MixerCore::OutputFormatChanged:\n");
	TRACE("  fMixBufferFrameRate %ld\n", fMixBufferFrameRate);
	TRACE("  fMixBufferFrameCount %ld\n", fMixBufferFrameCount);
	TRACE("  fMixBufferChannelCount %ld\n", fMixBufferChannelCount);
	for (int i = 0; i < fMixBufferChannelCount; i++)
		TRACE("  fMixBufferChannelTypes[%i] %ld\n", i, fMixBufferChannelTypes[i]);

	MixerInput *input;
	for (int i = 0; (input = Input(i)); i++)
		input->SetMixBufferFormat(fMixBufferFrameRate, fMixBufferFrameCount);
}

void
MixerCore::SetOutputBufferGroup(BBufferGroup *group)
{
	ASSERT_LOCKED();

	fBufferGroup = group;
}

void
MixerCore::SetTimingInfo(BTimeSource *ts, bigtime_t downstream_latency)
{
	ASSERT_LOCKED();

	if (fTimeSource)
		fTimeSource->Release();
		
	fTimeSource = dynamic_cast<BTimeSource *>(ts->Acquire());
	fDownstreamLatency = downstream_latency;

	TRACE("MixerCore::SetTimingInfo, now = %Ld, downstream latency %Ld\n", fTimeSource->Now(), fDownstreamLatency);
}

void
MixerCore::EnableOutput(bool enabled)
{
	ASSERT_LOCKED();
	TRACE("MixerCore::EnableOutput %d\n", enabled);
	fOutputEnabled = enabled;

	if (fRunning && !fOutputEnabled)
		StopMixThread();

	if (!fRunning && fOutput && fStarted && fOutputEnabled)
		StartMixThread();
}

uint32
MixerCore::OutputChannelCount()
{
	return (fOutput) ? fOutput->GetOutputChannelCount() : 0;
}


bool
MixerCore::Start()
{
	ASSERT_LOCKED();
	TRACE("MixerCore::Start\n");
	if (fStarted)
		return false;
		
	fStarted = true;
	
	ASSERT(!fRunning);
	
	// only start the mix thread if we have an output
	if (fOutput && fOutputEnabled)
		StartMixThread();

	return true;
}

bool
MixerCore::Stop()
{
	ASSERT_LOCKED();
	TRACE("MixerCore::Stop\n");
	if (!fStarted)
		return false;
		
	if (fRunning)
		StopMixThread();

	fStarted = false;
	return true;
}

void
MixerCore::StartMixThread()
{
	ASSERT(fOutputEnabled == true);
	ASSERT(fRunning == false);
	ASSERT(fOutput);
	fRunning = true;
	fMixThreadWaitSem = create_sem(0, "mix thread wait");
	fMixThread = spawn_thread(_mix_thread_, "Yeah baby, very shagadelic", 120, this);
	resume_thread(fMixThread);
}

void
MixerCore::StopMixThread()
{
	ASSERT(fRunning == true);
	ASSERT(fMixThread > 0);
	ASSERT(fMixThreadWaitSem > 0);
	
	status_t unused;
	delete_sem(fMixThreadWaitSem);
	wait_for_thread(fMixThread, &unused);	
		
	fMixThread = -1;
	fMixThreadWaitSem = -1;
	fRunning = false;
}

int32
MixerCore::_mix_thread_(void *arg)
{
	static_cast<MixerCore *>(arg)->MixThread();
	return 0;
}

struct chan_info {
	const char 	*base;
	uint32		sample_offset;
	float		gain;
};

void
MixerCore::MixThread()
{
	bigtime_t 	event_time;
	bigtime_t 	time_base;
	bigtime_t 	latency;
	bigtime_t	start;
	bigtime_t	buffer_request_timeout;
	int64		frame_base;
	int64		frame_pos;
	
	// The broken BeOS R5 multiaudio node starts with time 0,
	// then publishes negative times for about 50ms, publishes 0
	// again until it finally reaches time values > 0
	if (!LockFromMixThread())
		return;
	start = fTimeSource->Now();
	Unlock();
	while (start <= 0) {
		TRACE("MixerCore: delaying MixThread start, timesource is at %Ld\n", start);
		snooze(5000);
		if (!LockFromMixThread())
			return;
		start = fTimeSource->Now();
		Unlock();
	}

	if (!LockFromMixThread())
		return;
	latency = max(3600LL, bigtime_t(0.4 * buffer_duration(fOutput->MediaOutput().format.u.raw_audio)));
	
	// XXX when the format changes while running, everything is wrong!
	buffer_request_timeout = buffer_duration(fOutput->MediaOutput().format.u.raw_audio) / 2;
	
	TRACE("MixerCore: starting MixThread at %Ld with latency %Ld and downstream latency %Ld, buffer_request_timeout %Ld\n", start, latency, fDownstreamLatency, buffer_request_timeout);

	/* We must read from the input buffer at a position (pos) that is always a multiple of fMixBufferFrameCount.
	 */
	int64 temp = frames_for_duration(fMixBufferFrameRate, start	);
	frame_base = ((temp / fMixBufferFrameCount) + 1) * fMixBufferFrameCount;
	time_base = duration_for_frames(fMixBufferFrameRate, frame_base);
	Unlock();
	
	TRACE("MixerCore: starting MixThread, start %Ld, time_base %Ld, frame_base %Ld\n", start, time_base, frame_base);
	
	ASSERT(fMixBufferFrameCount > 0);
	
#if DEBUG
	uint64 buffer_num = 0;
#endif

	RtList<chan_info> InputChanInfos[MAX_CHANNEL_TYPES];
	RtList<chan_info> MixChanInfos[fMixBufferChannelCount]; // XXX this does not support changing output channel count
	
	event_time = time_base;
	frame_pos = 0;
	for (;;) {
		bigtime_t wait_until;
		if (!LockFromMixThread())
			return;
		wait_until = fTimeSource->RealTimeFor(event_time, 0) - latency - fDownstreamLatency;
		Unlock();
		status_t rv;
		rv = acquire_sem_etc(fMixThreadWaitSem, 1, B_ABSOLUTE_TIMEOUT, wait_until);
		if (rv == B_INTERRUPTED)
			continue;
		if (rv != B_TIMED_OUT && rv < B_OK)
			return;
		
		if (!LockWithTimeout(10000)) {
			ERROR("MixerCore: LockWithTimeout failed\n");
			continue;
		}
		
		// no inputs or output muted, skip further processing and just send an empty buffer
		if (fInputs->IsEmpty() || fOutput->IsMuted()) {
			int size = fOutput->MediaOutput().format.u.raw_audio.buffer_size;
			BBuffer* buf = fBufferGroup->RequestBuffer(size, buffer_request_timeout);
			if (buf) {
				memset(buf->Data(), 0, size);
				// fill in the buffer header
				media_header* hdr = buf->Header();
				hdr->type = B_MEDIA_RAW_AUDIO;
				hdr->size_used = size;
				hdr->time_source = fTimeSource->ID();
				hdr->start_time = event_time;
				if (B_OK != fNode->SendBuffer(buf, fOutput->MediaOutput().destination)) {
#if DEBUG
					ERROR("MixerCore: SendBuffer failed for buffer %Ld\n", buffer_num);
#else
					ERROR("MixerCore: SendBuffer failed\n");
#endif
					buf->Recycle();
				}
			} else {
#if DEBUG
				ERROR("MixerCore: RequestBuffer failed for buffer %Ld\n", buffer_num);
#else
				ERROR("MixerCore: RequestBuffer failed\n");
#endif
			}
			goto schedule_next_event;
		}

		int64 cur_framepos;
		cur_framepos = frame_base + frame_pos;
			
		// mix all data from all inputs into the mix buffer
		ASSERT(cur_framepos % fMixBufferFrameCount == 0);

		PRINT(4, "create new buffer event at %Ld, reading input frames at %Ld\n", event_time, cur_framepos);
		
		MixerInput *input;
		for (int i = 0; (input = Input(i)) != 0; i++) {
			int count = input->GetMixerChannelCount();
			for (int chan = 0; chan < count; chan++) {
				int type;
				const float *base;
				uint32 sample_offset;
				float gain;
				if (!input->GetMixerChannelInfo(chan, cur_framepos, event_time, &base, &sample_offset, &type, &gain))
					continue;
				if (type < 0 || type >= MAX_CHANNEL_TYPES)
					continue;
				chan_info *info = InputChanInfos[type].Create();
				info->base = (const char *)base;
				info->sample_offset = sample_offset;
				info->gain = gain;
			}
		}
		
		for (int chan = 0; chan < fMixBufferChannelCount; chan++) {
			int sourcecount = fOutput->GetOutputChannelSourceCount(chan);
			for (int i = 0; i < sourcecount; i++) {
				int type;
				float gain;
				fOutput->GetOutputChannelSourceInfoAt(chan, i, &type, &gain);
				if (type < 0 || type >= MAX_CHANNEL_TYPES)
					continue;
				int count = InputChanInfos[type].CountItems();
				for (int j = 0; j < count; j++) {
					chan_info *info = InputChanInfos[type].ItemAt(j);
					chan_info *newinfo = MixChanInfos[chan].Create();
					newinfo->base = info->base;
					newinfo->sample_offset = info->sample_offset;
					newinfo->gain = info->gain * gain;
				}
			}
		}

		memset(fMixBuffer, 0, fMixBufferChannelCount * fMixBufferFrameCount * sizeof(float));
		for (int chan = 0; chan < fMixBufferChannelCount; chan++) {
			PRINT(5, "MixThread: chan %d has %d sources\n", chan, MixChanInfos[chan].CountItems());
			int count = MixChanInfos[chan].CountItems();
			for (int i = 0; i < count; i++) {
				chan_info *info = MixChanInfos[chan].ItemAt(i);
				PRINT(5, "MixThread:   base %p, sample-offset %2d, gain %.3f\n", info->base, info->sample_offset, info->gain);
				// This looks slightly ugly, but the current GCC will generate the fastest
				// code this way. fMixBufferFrameCount is always > 0.
				uint32 dst_sample_offset = fMixBufferChannelCount * sizeof(float);
				uint32 src_sample_offset = info->sample_offset;
				register char *dst = (char *)&fMixBuffer[chan];
				register char *src = (char *)info->base;
				register float gain = info->gain;
				register int j = fMixBufferFrameCount;
				do {
					*(float *)dst += *(const float *)src * gain;
					dst += dst_sample_offset;
					src += src_sample_offset;
				 } while (--j);
			}
		}

		// request a buffer
		BBuffer	*buf;
		{
//		BStopWatch w("buffer requ");
		buf = fBufferGroup->RequestBuffer(fOutput->MediaOutput().format.u.raw_audio.buffer_size, buffer_request_timeout);
		}
		if (buf) {
		
			// copy data from mix buffer into output buffer
			for (int i = 0; i < fMixBufferChannelCount; i++) {
				fResampler[i]->Resample(reinterpret_cast<char *>(fMixBuffer) + i *  sizeof(float),
										fMixBufferChannelCount * sizeof(float),
										fMixBufferFrameCount,
										reinterpret_cast<char *>(buf->Data()) + (i * bytes_per_sample(fOutput->MediaOutput().format.u.raw_audio)),
										bytes_per_frame(fOutput->MediaOutput().format.u.raw_audio),
										frames_per_buffer(fOutput->MediaOutput().format.u.raw_audio),
										fOutputGain * fOutput->GetOutputChannelGain(i));
			}
			PRINT(4, "send buffer, inframes %ld, outframes %ld\n", fMixBufferFrameCount, frames_per_buffer(fOutput->MediaOutput().format.u.raw_audio));
			
			// fill in the buffer header
			media_header* hdr = buf->Header();
			hdr->type = B_MEDIA_RAW_AUDIO;
			hdr->size_used = fOutput->MediaOutput().format.u.raw_audio.buffer_size;
			hdr->time_source = fTimeSource->ID();
			hdr->start_time = event_time;
			
			// swap byte order if necessary
			fOutput->AdjustByteOrder(buf);
		
			// send the buffer
			status_t res;
			{
//				BStopWatch watch("buffer send");
				res = fNode->SendBuffer(buf, fOutput->MediaOutput().destination);
			}
			if (B_OK != res) {
#if DEBUG
				ERROR("MixerCore: SendBuffer failed for buffer %Ld\n", buffer_num);
#else
				ERROR("MixerCore: SendBuffer failed\n");
#endif
				buf->Recycle();
			}
		} else {
#if DEBUG
			ERROR("MixerCore: RequestBuffer failed for buffer %Ld\n", buffer_num);
#else
			ERROR("MixerCore: RequestBuffer failed\n");
#endif
		}

		// make all lists empty
		for (int i = 0; i < MAX_CHANNEL_TYPES; i++)
			InputChanInfos[i].MakeEmpty();
		for (int i = 0; i < fOutput->GetOutputChannelCount(); i++)
			MixChanInfos[i].MakeEmpty();

schedule_next_event:		
		// schedule next event
		frame_pos += fMixBufferFrameCount;
		event_time = time_base + bigtime_t((1000000LL * frame_pos) / fMixBufferFrameRate);
		Unlock();
#if DEBUG
		buffer_num++;
#endif
	}
}
