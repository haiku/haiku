/**
 * @file MidiSynth.cpp
 *
 * @author Matthijs Hollemans
 */

#include "debug.h"
#include "MidiSynth.h"

//------------------------------------------------------------------------------

BMidiSynth::BMidiSynth()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

BMidiSynth::~BMidiSynth()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiSynth::EnableInput(bool enable, bool loadInstruments)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

bool BMidiSynth::IsInputEnabled(void) const
{
	UNIMPLEMENTED
	return false;
}

//------------------------------------------------------------------------------

void BMidiSynth::SetVolume(double volume)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

double BMidiSynth::Volume(void) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BMidiSynth::SetTransposition(int16 offset)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

int16 BMidiSynth::Transposition(void) const
{
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BMidiSynth::MuteChannel(int16 channel, bool do_mute)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::GetMuteMap(char* pChannels) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::SoloChannel(int16 channel, bool do_solo)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::GetSoloMap(char* pChannels) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

status_t BMidiSynth::LoadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiSynth::UnloadInstrument(int16 instrument)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BMidiSynth::RemapInstrument(int16 from, int16 to)
{
	UNIMPLEMENTED
	return B_ERROR;
}

//------------------------------------------------------------------------------

void BMidiSynth::FlushInstrumentCache(bool startStopCache)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

uint32 BMidiSynth::Tick(void) const
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::AllNotesOff(bool controlOnly, uint32 time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiSynth::_ReservedMidiSynth1() { }
void BMidiSynth::_ReservedMidiSynth2() { }
void BMidiSynth::_ReservedMidiSynth3() { }
void BMidiSynth::_ReservedMidiSynth4() { }

//------------------------------------------------------------------------------

void BMidiSynth::Run()
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

