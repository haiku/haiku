#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

class MixerCore;

class MixerOutput
{
public:
	MixerOutput(MixerCore *core, const media_output &output);
	~MixerOutput();
	
	media_output & MediaOutput();
	
	void ChangeFormat(const media_multi_audio_format &format);
	
private:
	MixerCore *fCore;
	media_output fOutput;
};

#endif
