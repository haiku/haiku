
#ifndef _SAMPLES_H
#define _SAMPLES_H

#include <BeBuild.h>
#include <Entry.h>

typedef void (*sample_exit_hook)(int32 arg);
typedef bool (*sample_loop_hook)(int32 arg);

class BSamples 
{
public:
	BSamples();
	virtual ~BSamples();

	/* start a sample playing */
	void Start(
		void* sampleData,        /* pointer to audio data*/
		int32 frames,            /* number of frames*/
		int16 bytes_per_sample,  /* btyes per sample 1 or 2*/
		int16 channel_count,     /* mono or stereo 1 or 2*/
		double pitch,            /* floating sample rate*/
		int32 loopStart,         /* loop start in frames*/
		int32 loopEnd,           /* loop end in frames*/
		double sampleVolume,     /* sample volume*/
		double stereoPosition,   /* stereo placement*/
		int32 hook_arg,          /* hook argument*/
		sample_loop_hook pLoopContinueProc,
		sample_exit_hook pDoneProc);

	/* currently paused */
	bool IsPaused(void) const;
	void Pause(void);
	void Resume(void);
	void Stop(void);
	bool IsPlaying(void) const;

	void SetVolume(double newVolume);
	double Volume(void) const;

	void SetSamplingRate(double newRate);
	double SamplingRate(void) const;

	void SetPlacement(double stereoPosition);

	/* Set the stereo position (-1.0 left to +1.0 right, 0 is middle) */
	double Placement(void) const;

	/* Enable/Disable reverb on this particular IgorSound */
	void EnableReverb(bool useReverb);

private:

	virtual void _ReservedSamples1();
	virtual void _ReservedSamples2();
	virtual void _ReservedSamples3();

	int32 fReference;
	double fPauseVariable;
	void* fFileVariables;
	uint32 _reserved[4];
};

#endif // _SAMPLES_H

