#include <MediaNode.h>
#include "MixerCore.h"
#include "MixerInput.h"
#include "MixerOutput.h"

MixerCore::MixerCore()
 :	fLocker(new BLocker)
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
