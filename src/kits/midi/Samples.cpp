/**
 * @file Samples.cpp
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "Samples.h"

//------------------------------------------------------------------------------

BSamples::BSamples()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BSamples::~BSamples()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSamples::Start(
	void* sampleData, int32 frames, int16 bytes_per_sample, 
	int16 channel_count, double pitch, int32 loopStart, int32 loopEnd, 
	double sampleVolume, double stereoPosition, int32 hook_arg, 
	sample_loop_hook pLoopContinueProc, sample_exit_hook pDoneProc)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

bool BSamples::IsPaused(void) const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

void BSamples::Pause(void)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSamples::Resume(void)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSamples::Stop(void)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

bool BSamples::IsPlaying(void) const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

void BSamples::SetVolume(double newVolume)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

double BSamples::Volume(void) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::SetSamplingRate(double newRate)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

double BSamples::SamplingRate(void) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::SetPlacement(double stereoPosition)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

double BSamples::Placement(void) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::EnableReverb(bool useReverb)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BSamples::_ReservedSamples1() { }
void BSamples::_ReservedSamples2() { }
void BSamples::_ReservedSamples3() { }

//------------------------------------------------------------------------------

