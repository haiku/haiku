/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "debug.h"
#include "Samples.h"
#include "Synth.h"

//------------------------------------------------------------------------------

BSamples::BSamples()
{
	/* not complete yet */

	if (be_synth == NULL)
	{
		new BSynth();
	}
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
