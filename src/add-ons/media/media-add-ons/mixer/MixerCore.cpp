#include <MediaNode.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"

#define MAX_OUTPUT_BUFFER_LENGTH	50000LL /* 50 ms */


MixerCore::MixerCore()
 :	fLocker(new BLocker),
	fOutputBufferLength(MAX_OUTPUT_BUFFER_LENGTH),
	fInputBufferLength(3 * MAX_OUTPUT_BUFFER_LENGTH),
	fOutput(0)
	
{
}

MixerCore::~MixerCore()
{
	delete fLocker;
}
	
bool
MixerCore::AddInput(const media_input &input)
{
	return true;
}

bool
MixerCore::AddOutput(const media_output &output)
{
	return true;
}

bool
MixerCore::RemoveInput(int32 inputID)
{
	return true;
}

bool
MixerCore::RemoveOutput()
{
	return true;
}

int32
MixerCore::CreateInputID()
{
	return 1;
}

MixerInput *
MixerCore::Input(int i)
{
	return (MixerInput *)fInputs->ItemAt(i);
}

MixerOutput *
MixerCore::Output()
{
	return fOutput;
}

void
MixerCore::BufferReceived(BBuffer *buffer, bigtime_t lateness)
{
}
	
void
MixerCore::InputFormatChanged(int32 inputID, const media_format *format)
{
}

void
MixerCore::OutputFormatChanged(const media_format *format)
{
}

void
MixerCore::SetOutputBufferGroup(BBufferGroup *group)
{
}

void
MixerCore::SetTimeSource(media_node_id id)
{
}

void
MixerCore::EnableOutput(bool enabled)
{
}

void
MixerCore::Start(bigtime_t time)
{
}

void
MixerCore::Stop()
{
}

uint32
MixerCore::OutputBufferSize()
{
	return 1;
}

bool
MixerCore::IsStarted()
{
	return false;
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