#ifndef _MIXER_INPUT_H
#define _MIXER_INPUT_H

class MixerCore;

class MixerInput
{
public:
	MixerInput(MixerCore *core);
	~MixerInput();
	
	void BufferReceived(BBuffer *buffer);
	
	media_input & MediaInput();
	
private:
	MixerCore *fCore;
	media_input fInput;
};

#endif
