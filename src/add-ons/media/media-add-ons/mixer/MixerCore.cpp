#include <MediaNode.h>
#include <Buffer.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"
#include "MixerUtils.h"
#include "Debug.h"

#define MAX_OUTPUT_BUFFER_LENGTH	50000LL /* 50 ms */

#define ASSERT_LOCKED()		if (fLocker->IsLocked()) {} else debugger("core not locked, meltdown occurred")

MixerCore::MixerCore()
 :	fLocker(new BLocker),
	fOutputBufferLength(MAX_OUTPUT_BUFFER_LENGTH),
	fInputBufferLength(3 * MAX_OUTPUT_BUFFER_LENGTH),
	fMixFrameRate(96000),
	fMixStartTime(0),
	fInputs(new BList),
	fOutput(0),
	fNextInputID(1),
	fRunning(false)
{
}

MixerCore::~MixerCore()
{
	delete fLocker;
	delete fInputs;
}

bool
MixerCore::AddInput(const media_input &input)
{
	ASSERT_LOCKED();
	fInputs->AddItem(new MixerInput(this, input, fMixFrameRate, frames_for_duration(fMixFrameRate, fInputBufferLength), fMixStartTime));
	return true;
}

bool
MixerCore::AddOutput(const media_output &output)
{
	ASSERT_LOCKED();
	if (fOutput)
		return false;
	fOutput = new MixerOutput(this, output);
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
MixerCore::InputFormatChanged(int32 inputID, const media_format *format)
{
	ASSERT_LOCKED();
}

void
MixerCore::OutputFormatChanged(const media_format *format)
{
	ASSERT_LOCKED();
	
//		void ChangeMixBufferFormat(float samplerate, int32 frames, int32 channelcount, uint32 channel_mask);

}

void
MixerCore::SetOutputBufferGroup(BBufferGroup *group)
{
	ASSERT_LOCKED();
}

void
MixerCore::SetTimeSource(media_node_id id)
{
	ASSERT_LOCKED();
}

void
MixerCore::EnableOutput(bool enabled)
{
	ASSERT_LOCKED();
}

void
MixerCore::Start(bigtime_t time)
{
	ASSERT_LOCKED();
	if (fRunning)
		return;
		
	fRunning = true;
}

void
MixerCore::Stop()
{
	ASSERT_LOCKED();
	if (!fRunning)
		return;
		
	fRunning = false;
}

uint32
MixerCore::OutputBufferSize()
{
	ASSERT_LOCKED();
	return 1;
}

bool
MixerCore::IsStarted()
{
	ASSERT_LOCKED();
	return fRunning;
}

void
MixerCore::OutputBufferLengthChanged(bigtime_t length)
{
}


/*
	void BufferReceived(BBuffer *buffer, bigtime_t lateness);


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

*/