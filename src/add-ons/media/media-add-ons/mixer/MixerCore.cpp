#include <RealtimeAlloc.h>
#include <MediaNode.h>
#include <BufferProducer.h>
#include <TimeSource.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerUtils.h"
#include "AudioMixer.h"
#include "Resampler.h"
#include "Debug.h"

#define DOUBLE_RATE_MIXING 	1

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
	fResampler(0),
	fMixBuffer(0),
	fMixBufferFrameRate(0),
	fMixBufferFrameCount(0),
	fMixBufferChannelTypes(0),
	fMixBufferChannelCount(0),
 	fDoubleRateMixing(DOUBLE_RATE_MIXING),
 	fMixStartTime(0),
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
	fInputs->AddItem(new MixerInput(this, input, fMixBufferFrameRate, fMixBufferFrameCount, fMixStartTime));
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
	OutputFormatChanged(fOutput->MediaOutput().format.u.raw_audio);
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
	delete fOutput;
	fOutput = 0;
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
	
	if (fMixBuffer)
		rtm_free(fMixBuffer);
		
	delete fMixBufferChannelTypes;

	fMixBufferFrameRate = (int32)format.frame_rate;
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
		input->SetMixBufferFormat(fMixBufferFrameRate, fMixBufferFrameCount, fMixStartTime);
}

void
MixerCore::SetOutputBufferGroup(BBufferGroup *group)
{
	ASSERT_LOCKED();

	fBufferGroup = group;
}

void
MixerCore::SetTimeSource(BTimeSource *ts)
{
	ASSERT_LOCKED();

	if (fTimeSource)
		fTimeSource->Release();
		
	fTimeSource = dynamic_cast<BTimeSource *>(ts->Acquire());
//	fMixStartTime = fTimeSource->Now();

	printf("MixerCore::SetTimeSource, now = %Ld\n", fTimeSource->Now());

//	MixerInput *input;
//	for (int i = 0; (input = Input(i)); i++)
//		input->SetMixBufferFormat(fMixBufferFrameRate, fMixBufferFrameCount, fMixStartTime);
}

void
MixerCore::EnableOutput(bool enabled)
{
	ASSERT_LOCKED();
	printf("MixerCore::EnableOutput %d\n", enabled);
}

void
MixerCore::Start(bigtime_t time)
{
	ASSERT_LOCKED();
	printf("MixerCore::Start\n");
	if (fRunning)
		return;
		
	fRunning = true;
	fMixThreadWaitSem = create_sem(0, "mix thread wait");
	fMixThread = spawn_thread(_mix_thread_, "Yeah baby, very shagadelic", 12, this);
	resume_thread(fMixThread);
}

void
MixerCore::Stop()
{
	ASSERT_LOCKED();
	printf("MixerCore::Stop\n");
	if (!fRunning)
		return;

	ASSERT(fMixThread > 0);
	ASSERT(fMixThreadWaitSem > 0);
	
	status_t unused;
	delete_sem(fMixThreadWaitSem);
	wait_for_thread(fMixThread, &unused);	
		
	fMixThread = -1;
	fMixThreadWaitSem = -1;
	fRunning = false;
}

uint32
MixerCore::OutputBufferSize()
{
	ASSERT_LOCKED();
	
	uint32 size = sizeof(float) * fMixBufferFrameCount * fMixBufferChannelCount;
	return fDoubleRateMixing ? (size / 2) : size;
}

bool
MixerCore::IsStarted()
{
	ASSERT_LOCKED();
	return fRunning;
}

int32
MixerCore::_mix_thread_(void *arg)
{
	static_cast<MixerCore *>(arg)->MixThread();
	return 0;
}

void
MixerCore::MixThread()
{
	bigtime_t 	event_time;
	bigtime_t 	time_base;
	bigtime_t 	latency;
	int64		frame_base;
	int64		frame_pos;
	
	latency = 30000;
	
	while (fTimeSource->Now() <= 0) {
		printf("delay MixThread start, now %Ld\n", (int64)fTimeSource->Now());
		snooze(1000);
	}
	
	printf("starting MixThread, now %Ld\n", (int64)fTimeSource->Now());

	/* We must read from the input buffer at a position (pos) that is always a multiple of fMixBufferFrameCount.
	 */
	int64 temp = frames_for_duration(fMixBufferFrameRate, fTimeSource->Now() - fMixStartTime);
	frame_base = ((temp / fMixBufferFrameCount) + 1) * fMixBufferFrameCount;
	time_base = duration_for_frames(fMixBufferFrameRate, frame_base) + fMixStartTime;
	
	printf("starting MixThread, now %Ld, time_base %Ld, fMixStartTime %Ld, frame_base %Ld\n", fTimeSource->Now(), time_base, fMixStartTime, frame_base);
	
	event_time = time_base;
	frame_pos = 0;
	for (;;) {
		status_t rv;
		rv = acquire_sem_etc(fMixThreadWaitSem, 1, B_ABSOLUTE_TIMEOUT, fTimeSource->RealTimeFor(event_time, latency));
		if (rv == B_INTERRUPTED)
			continue;
		if (rv != B_TIMED_OUT && rv < B_OK)
			return;
			
		// mix all data from all inputs into the mix buffer
		ASSERT((frame_base + frame_pos) % fMixBufferFrameCount == 0);

		printf("create new buffer event at %Ld, reading input frames at %Ld\n", event_time, frame_base + frame_pos);

		for (int i = 0; i < fMixBufferChannelCount; i++) {
			for (int j = 0; j < fMixBufferFrameCount; j++) {
				fMixBuffer[(i * fMixBufferChannelCount)+j] = (i*j) / (float)(fMixBufferChannelCount * fMixBufferFrameCount);
			}
		}
		
		// request a buffer
		BBuffer* buf = fBufferGroup->RequestBuffer(fOutput->MediaOutput().format.u.raw_audio.buffer_size, 10000);
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

			// fill in the buffer header
			media_header* hdr = buf->Header();
			hdr->type = B_MEDIA_RAW_AUDIO;
			hdr->size_used = fOutput->MediaOutput().format.u.raw_audio.buffer_size;
			hdr->time_source = fTimeSource->ID();
			hdr->start_time = event_time;
		
			// send the buffer
			if (B_OK != fNode->SendBuffer(buf, fOutput->MediaOutput().destination)) {
				printf("SendBuffer failed\n");
				buf->Recycle();
			}
		}
		
		// shedule next event
		frame_pos += fMixBufferFrameCount;
		event_time = time_base + bigtime_t((1000000LL * frame_pos) / fMixBufferFrameRate);
	}
}
