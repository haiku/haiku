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
	if (be_synth == NULL)
	{
		new BSynth();
	}
}

//------------------------------------------------------------------------------

BSamples::~BSamples()
{
	// do nothing
}

//------------------------------------------------------------------------------

void BSamples::Start(
	void* sampleData, int32 frames, int16 bytes_per_sample, 
	int16 channel_count, double pitch, int32 loopStart, int32 loopEnd, 
	double sampleVolume, double stereoPosition, int32 hook_arg, 
	sample_loop_hook pLoopContinueProc, sample_exit_hook pDoneProc)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

bool BSamples::IsPaused(void) const
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
	return false;
}

//------------------------------------------------------------------------------

void BSamples::Pause(void)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

void BSamples::Resume(void)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

void BSamples::Stop(void)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

bool BSamples::IsPlaying(void) const
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
	return false;
}

//------------------------------------------------------------------------------

void BSamples::SetVolume(double newVolume)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

double BSamples::Volume(void) const
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::SetSamplingRate(double newRate)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

double BSamples::SamplingRate(void) const
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::SetPlacement(double stereoPosition)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

double BSamples::Placement(void) const
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
	return 0;
}

//------------------------------------------------------------------------------

void BSamples::EnableReverb(bool useReverb)
{
	fprintf(stderr, "[midi] BSamples is not supported\n");
}

//------------------------------------------------------------------------------

void BSamples::_ReservedSamples1() { }
void BSamples::_ReservedSamples2() { }
void BSamples::_ReservedSamples3() { }

//------------------------------------------------------------------------------
