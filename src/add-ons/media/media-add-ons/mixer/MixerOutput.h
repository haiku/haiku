#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

class MixerCore;

class MixerOutput
{
public:
	MixerOutput(MixerCore *core);
	~MixerOutput();
	
	media_output MediaOutput();
	
private:
	MixerCore *fCore;
};

#endif
