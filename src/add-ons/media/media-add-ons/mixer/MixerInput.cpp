#include <MediaNode.h>
#include "MixerInput.h"
#include "MixerCore.h"
#include "MixerUtils.h"
#include "debug.h"

MixerInput::MixerInput(MixerCore *core, const media_input &input)
 :	fCore(core),
 	fInput(input)
{
	fix_multiaudio_format(&fInput.format.u.raw_audio);
	PRINT_INPUT("MixerInput::MixerInput", fInput);
	PRINT_CHANNEL_MASK(fInput.format);
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

int32
MixerInput::ID()
{
	return fInput.destination.id;
}


