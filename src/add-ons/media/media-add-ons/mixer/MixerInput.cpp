#include <MediaNode.h>
#include "MixerInput.h"
#include "MixerCore.h"


MixerInput::MixerInput(MixerCore *core)
 :	fCore(core)
{
}

MixerInput::~MixerInput()
{
}
	
void
MixerInput::BufferReceived(BBuffer *buffer)
{
}

media_input &
MixerInput::MediaInput()
{
	return fInput;
}
