
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

	void Start(
		void* sampleData, int32 frames, int16 bytes_per_sample,
		int16 channel_count, double pitch, int32 loopStart, int32 loopEnd,
		double sampleVolume, double stereoPosition, int32 hook_arg,
		sample_loop_hook pLoopContinueProc, sample_exit_hook pDoneProc);

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
	double Placement(void) const;

	void EnableReverb(bool useReverb);

private:

	virtual void _ReservedSamples1();
	virtual void _ReservedSamples2();
	virtual void _ReservedSamples3();

	uint32 _reserved[8];
};

#endif // _SAMPLES_H
