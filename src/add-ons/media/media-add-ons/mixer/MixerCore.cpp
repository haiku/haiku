#include <RealtimeAlloc.h>
#include <MediaNode.h>
#include <BufferProducer.h>
#include <TimeSource.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <string.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerUtils.h"
#include "AudioMixer.h"
#include "Resampler.h"
#include "debug.h"
#include "TList.h"

#define DOUBLE_RATE_MIXING 	0

#define ASSERT_LOCKED()		if (fLocker->IsLocked()) {} else debugger("core not locked, meltdown occurred")

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
	fMixBufferChannelTypes(0),
	fMixBufferChannelCount(0),
 	fDoubleRateMixing(DOUBLE_RATE_MIXING),
 	fDownstreamLatency(1),
 	fNode(node),
	fBufferGroup(0),
	fTimeSource(0),
	fMixThread(-1),
	fMixThreadWaitSem(-1)
{
}

MixerCore::~MixerCore()
{
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

bool
MixerCore::AddInput(const media_input &input)
{
	ASSERT_LOCKED();
	fInputs->AddItem(new MixerInput(this, input, fMixBufferFrameRate, fMixBufferFrameCount));
	return true;
}

bool
MixerCore::AddOutput(const media_output &output)
{
	ASSERT_LOCKED();
	if (fOutput)
		return false;
	fOutput = new MixerOutput(this, output);
	// the output format might have been adjusted inside MixerOutput
	ApplyOutputFormat();
	
	ASSERT(!fRunning);
	if (fStarted && fOutputEnabled)
		StartMixThread();
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
	printf("MixerCore::BufferReceived: received buffer for unknown id %ld\n", id);
}
	
void
MixerCore::InputFormatChanged(int32 inputID, const media_multi_audio_format &format)
{
	ASSERT_LOCKED();
	printf("MixerCore::InputFormatChanged not handled\n");
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
		fResampler[i] = new Resampler(media_raw_audio_format::B_AUDIO_FLOAT, format.format);
	
	printf("MixerCore::OutputFormatChanged:\n");
	printf("  fMixBufferFrameRate %ld\n", fMixBufferFrameRate);
	printf("  fMixBufferFrameCount %ld\n", fMixBufferFrameCount);
	printf("  fMixBufferChannelCount %ld\n", fMixBufferChannelCount);
	for (int i = 0; i < fMixBufferChannelCount; i++)
		printf("  fMixBufferChannelTypes[%i] %ld\n", i, fMixBufferChannelTypes[i]);

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

	printf("MixerCore::SetTimingInfo, now = %Ld, downstream latency %Ld\n", fTimeSource->Now(), fDownstreamLatency);
}

void
MixerCore::EnableOutput(bool enabled)
{
	ASSERT_LOCKED();
	printf("MixerCore::EnableOutput %d\n", enabled);
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
	printf("MixerCore::Start\n");
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
	printf("MixerCore::Stop\n");
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
#if DEBUG > 1
	fMixThread = spawn_thread(_mix_thread_, "Yeah baby, very shagadelic", 12, this);
#else
	fMixThread = spawn_thread(_mix_thread_, "Yeah baby, very shagadelic", 120, this);
#endif
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
		printf("MixerCore: delaying MixThread start, timesource is at %Ld\n", start);
		snooze(1000);
		if (!LockFromMixThread())
			return;
		start = fTimeSource->Now();
		Unlock();
	}

	if (!LockFromMixThread())
		return;
	latency = bigtime_t(0.4 * buffer_duration(fOutput->MediaOutput().format.u.raw_audio));
	
	printf("MixerCore: starting MixThread at %Ld with latency %Ld and downstream latency %Ld\n", start, latency, fDownstreamLatency);

	/* We must read from the input buffer at a position (pos) that is always a multiple of fMixBufferFrameCount.
	 */
	int64 temp = frames_for_duration(fMixBufferFrameRate, start	);
	frame_base = ((temp / fMixBufferFrameCount) + 1) * fMixBufferFrameCount;
	time_base = duration_for_frames(fMixBufferFrameRate, frame_base);
	Unlock();
	
	printf("starting MixThread, start %Ld, time_base %Ld, frame_base %Ld\n", start, time_base, frame_base);
	
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
			
		if (!LockWithTimeout(10000))
			continue;
			
		// mix all data from all inputs into the mix buffer
		ASSERT((frame_base + frame_pos) % fMixBufferFrameCount == 0);

		PRINT(4, "create new buffer event at %Ld, reading input frames at %Ld\n", event_time, frame_base + frame_pos);

		// XXX this is a test, copy the the left and right channel from input 1 or 0
		
		#define MAX_TYPES	18
		
		int64 cur_framepos = frame_base + frame_pos;
		
		List<chan_info> InputChanInfos[MAX_TYPES];
		List<chan_info> MixChanInfos[fOutput->GetOutputChannelCount()];

		MixerInput *input;
		for (int i = 0; (input = Input(i)) != 0; i++) {
			int count = input->GetMixerChannelCount();
			for (int chan = 0; chan < count; chan++) {
				chan_info info;
				int type;
				input->GetMixerChannelInfo(chan, cur_framepos, (const float **)&info.base, &info.sample_offset, &type, &info.gain);
				if (type < 0 || type >= MAX_TYPES)
					continue;
				InputChanInfos[type].Insert(info);
			}
		}
		
		int count = fOutput->GetOutputChannelCount();
		for (int chan = 0; chan < count; chan++) {
			int count2 = fOutput->GetOutputChannelSourceCount(chan);
			for (int i = 0; i < count2; i++) {
				int type;
				float gain;
				fOutput->GetMixerChannelInfo(chan, i, &type, &gain);
				if (type < 0 || type >= MAX_TYPES)
					continue;
				int count3 = InputChanInfos[type].CountItems();
				for (int j = 0; j < count3; j++) {
					chan_info *info;
					InputChanInfos[type].Get(j, &info);
					chan_info newinfo = *info;
					newinfo.gain *= gain;
					MixChanInfos[chan].Insert(newinfo);
				}
			}
		}

		uint32 dst_sample_offset = fMixBufferChannelCount * sizeof(float);
		
		count = fOutput->GetOutputChannelCount();
		for (int chan = 0; chan < count; chan++) {
			PRINT(5, "MixThread: chan %d has %d sources\n", chan, MixChanInfos[chan].CountItems());
			ZeroFill(&fMixBuffer[chan], dst_sample_offset, fMixBufferFrameCount);
			int count2 = MixChanInfos[chan].CountItems();
			for (int i = 0; i < count2; i++) {
				chan_info *info;
				MixChanInfos[chan].Get(i, &info);
				char *dst = (char *)&fMixBuffer[chan];
				for (int j = 0; j < fMixBufferFrameCount; j++) {
					*(float *)dst += *(const float *)info->base * info->gain;
					dst += dst_sample_offset;
					info->base += info->sample_offset;
				}
				PRINT(5, "MixThread:   base %p, sample-offset %2d, gain %.3f\n", info->base, info->sample_offset, info->gain);
			}
		}

		// request a buffer
		BBuffer* buf = fBufferGroup->RequestBuffer(fOutput->MediaOutput().format.u.raw_audio.buffer_size, 5000);

		if (buf) {
		
			// copy data from mix buffer into output buffer
			for (int i = 0; i < fMixBufferChannelCount; i++) {
				fResampler[i]->Resample(reinterpret_cast<char *>(fMixBuffer) + i *  sizeof(float),
										fMixBufferChannelCount * sizeof(float),
										fMixBufferFrameCount,
										reinterpret_cast<char *>(buf->Data()) + (i * bytes_per_sample(fOutput->MediaOutput().format.u.raw_audio)),
										bytes_per_frame(fOutput->MediaOutput().format.u.raw_audio),
										frames_per_buffer(fOutput->MediaOutput().format.u.raw_audio),
										1.0);
			}
			PRINT(4, "send buffer, inframes %ld, outframes %ld\n",fMixBufferFrameCount, frames_per_buffer(fOutput->MediaOutput().format.u.raw_audio));

			// fill in the buffer header
			media_header* hdr = buf->Header();
			hdr->type = B_MEDIA_RAW_AUDIO;
			hdr->size_used = fOutput->MediaOutput().format.u.raw_audio.buffer_size;
			hdr->time_source = fTimeSource->ID();
			hdr->start_time = event_time;
		
			// send the buffer
			if (B_OK != fNode->SendBuffer(buf, fOutput->MediaOutput().destination)) {
				printf("MixerCore: #### SendBuffer failed\n");
				buf->Recycle();
			}
		} else {
			printf("MixerCore: #### RequestBuffer failed\n");
		}
		
		// schedule next event
		frame_pos += fMixBufferFrameCount;
		event_time = time_base + bigtime_t((1000000LL * frame_pos) / fMixBufferFrameRate);
		Unlock();
	}
}
