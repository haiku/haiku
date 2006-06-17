/*
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
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

#include <MidiSynth.h>

#include "debug.h"
#include "SoftSynth.h"

using namespace BPrivate;

//------------------------------------------------------------------------------

BMidiSynth::BMidiSynth()
{
	if (be_synth == NULL)
	{
		new BSynth();
	}

	be_synth->clientCount++;

	inputEnabled = false;
	transpose = 0;
	creationTime = system_time();
}

//------------------------------------------------------------------------------

BMidiSynth::~BMidiSynth()
{
	be_synth->clientCount--;
}

//------------------------------------------------------------------------------

status_t BMidiSynth::EnableInput(bool enable, bool loadInstruments)
{
	status_t err = B_OK;
	inputEnabled = enable;
	
	if (loadInstruments) {
		err = be_synth->synth->LoadAllInstruments();
	}

	return err;
}

//------------------------------------------------------------------------------

bool BMidiSynth::IsInputEnabled(void) const
{
	return inputEnabled;
}

//------------------------------------------------------------------------------

void BMidiSynth::SetVolume(double volume)
{
	be_synth->synth->SetVolume(volume);
}

//------------------------------------------------------------------------------

double BMidiSynth::Volume(void) const
{
	return be_synth->synth->Volume();
}

//------------------------------------------------------------------------------

void BMidiSynth::SetTransposition(int16 offset)
{
	transpose = offset;
}

//------------------------------------------------------------------------------

int16 BMidiSynth::Transposition(void) const
{
	return transpose;
}

//------------------------------------------------------------------------------

void BMidiSynth::MuteChannel(int16 channel, bool do_mute)
{
	fprintf(stderr, "[midi] MuteChannel is broken; don't use it\n");
}

//------------------------------------------------------------------------------

void BMidiSynth::GetMuteMap(char* pChannels) const
{
	fprintf(stderr, "[midi] GetMuteMap is broken; don't use it\n");
}

//------------------------------------------------------------------------------

void BMidiSynth::SoloChannel(int16 channel, bool do_solo)
{
	fprintf(stderr, "[midi] SoloChannel is broken; don't use it\n");
}

//------------------------------------------------------------------------------

void BMidiSynth::GetSoloMap(char* pChannels) const
{
	fprintf(stderr, "[midi] GetSoloMap is broken; don't use it\n");
}

//------------------------------------------------------------------------------

status_t BMidiSynth::LoadInstrument(int16 instrument)
{
	return be_synth->synth->LoadInstrument(instrument);
}

//------------------------------------------------------------------------------

status_t BMidiSynth::UnloadInstrument(int16 instrument)
{
	return be_synth->synth->UnloadInstrument(instrument);
}

//------------------------------------------------------------------------------

status_t BMidiSynth::RemapInstrument(int16 from, int16 to)
{
	return be_synth->synth->RemapInstrument(from, to);
}

//------------------------------------------------------------------------------

void BMidiSynth::FlushInstrumentCache(bool startStopCache)
{
	be_synth->synth->FlushInstrumentCache(startStopCache);
}

//------------------------------------------------------------------------------

uint32 BMidiSynth::Tick(void) const
{
	return (uint32) (system_time() - creationTime);
}

//------------------------------------------------------------------------------

void BMidiSynth::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->NoteOff(channel, note + transpose, velocity, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->NoteOn(channel, note + transpose, velocity, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->KeyPressure(
			channel, note + transpose, pressure, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->ControlChange(
			channel, controlNumber, controlValue, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->ProgramChange(channel, programNumber, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->ChannelPressure(channel, pressure, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->PitchBend(channel, lsb, msb, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::AllNotesOff(bool justChannel, uint32 time)
{
	if (inputEnabled)
	{
		be_synth->synth->AllNotesOff(justChannel, time);
	}
}

//------------------------------------------------------------------------------

void BMidiSynth::_ReservedMidiSynth1() { }
void BMidiSynth::_ReservedMidiSynth2() { }
void BMidiSynth::_ReservedMidiSynth3() { }
void BMidiSynth::_ReservedMidiSynth4() { }

//------------------------------------------------------------------------------

void BMidiSynth::Run()
{
	// do nothing
}

//------------------------------------------------------------------------------
