#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

class MixerCore;

class MixerOutput
{
public:
	MixerOutput(MixerCore *core);
	~MixerOutput();
	
private:
	MixerCore *fCore;
};

#endif
