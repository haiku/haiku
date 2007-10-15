
#ifndef _SYNTH_H
#define _SYNTH_H

#include <BeBuild.h>
#include <Entry.h>
#include <MidiDefs.h>
#include <OS.h>

typedef enum interpolation_mode
{
	B_DROP_SAMPLE = 0,
	B_2_POINT_INTERPOLATION,
	B_LINEAR_INTERPOLATION
} 
interpolation_mode;

typedef enum reverb_mode
{
	B_REVERB_NONE = 1,		
	B_REVERB_CLOSET,	
	B_REVERB_GARAGE,	
	B_REVERB_BALLROOM,	
	B_REVERB_CAVERN,	
	B_REVERB_DUNGEON	
} 
reverb_mode;

typedef void (*synth_controller_hook) (
	int16 channel, int16 controller, int16 value);

class BMidiSynth;
class BMidiSynthFile;

namespace BPrivate { class BSoftSynth; }

class BSynth 
{
public:

	BSynth();
	BSynth(synth_mode synth);
	virtual ~BSynth();

	status_t LoadSynthData(entry_ref* instrumentsFile); 
	status_t LoadSynthData(synth_mode synth);
	synth_mode SynthMode(void);

	void Unload(void);
	bool IsLoaded(void) const;

	status_t SetSamplingRate(int32 sample_rate);
	int32 SamplingRate() const;

	status_t SetInterpolation(interpolation_mode interp_mode);
	interpolation_mode Interpolation() const;

	void SetReverb(reverb_mode rev_mode);
	reverb_mode Reverb() const;

	status_t EnableReverb(bool reverb_enabled);
	bool IsReverbEnabled() const;

	status_t SetVoiceLimits(
		int16 maxSynthVoices, int16 maxSampleVoices, 
		int16 limiterThreshhold);

	int16 MaxSynthVoices(void) const;
	int16 MaxSampleVoices(void) const;
	int16 LimiterThreshhold(void) const;

	void SetSynthVolume(double theVolume);
	double SynthVolume(void) const;

	void SetSampleVolume(double theVolume);
	double SampleVolume(void) const;

	status_t GetAudio(
		int16* pLeft, int16* pRight, int32 max_samples) const;

	void Pause(void);
	void Resume(void);

	void SetControllerHook(int16 controller, synth_controller_hook cback);

	int32 CountClients(void) const;

private:

	friend class BMidiSynth;
	friend class BMidiSynthFile;

	virtual void _ReservedSynth1();
	virtual void _ReservedSynth2();
	virtual void _ReservedSynth3();
	virtual void _ReservedSynth4();

	void _Init();
	
	BPrivate::BSoftSynth* fSynth;
	synth_mode fSynthMode;
	int32 fClientCount;

	uint32 _reserved[10];
};

extern BSynth* be_synth;

#endif // _SYNTH_H
