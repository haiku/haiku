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
MixerCore::RemoveInput(const media_input &input)
{
	return true;
}

bool
MixerCore::RemoveOutput(const media_output &output)
{
	return true;
}

void
MixerCore::OutputBufferLengthChanged(bigtime_t length)
{
	Lock();
	
	Unlock();
}


MixerInput *
MixerCore::Input(int i)
{
	return (MixerInput *)fInputs->ItemAt(i);
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

*/