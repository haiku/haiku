#include <MediaNode.h>
#include "MixerOutput.h"
#include "MixerCore.h"


MixerOutput::MixerOutput(MixerCore *core)
 :	fCore(core)
{
}

MixerOutput::~MixerOutput()
{
}

media_output MediaOutput();
