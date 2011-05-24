/*
 * Copyright 2006, Haiku.
 *
 * Copyright (c) 2002-2004 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		
 *		Matthijs Hollemans
 *		Jérôme Leveque
 */

#include <MidiSynth.h>

#include "debug.h"
#include "SoftSynth.h"

using namespace BPrivate;


BMidiSynth::BMidiSynth()
{
	if (be_synth == NULL) {
		new BSynth();
	}

	be_synth->fClientCount++;

	fInputEnabled = false;
	fTranspose = 0;
	fCreationTime = system_time();
}


BMidiSynth::~BMidiSynth()
{
	be_synth->fClientCount--;
	if (be_synth->fClientCount == 0) {
		delete be_synth;
		be_synth = NULL;
	}
}


status_t 
BMidiSynth::EnableInput(bool enable, bool loadInstruments)
{
	status_t err = B_OK;
	fInputEnabled = enable;
	
	if (loadInstruments) {
		err = be_synth->fSynth->LoadAllInstruments();
	}

	return err;
}


bool 
BMidiSynth::IsInputEnabled() const
{
	return fInputEnabled;
}


void 
BMidiSynth::SetVolume(double volume)
{
	be_synth->fSynth->SetVolume(volume);
}


double 
BMidiSynth::Volume() const
{
	return be_synth->fSynth->Volume();
}


void 
BMidiSynth::SetTransposition(int16 offset)
{
	fTranspose = offset;
}


int16 
BMidiSynth::Transposition() const
{
	return fTranspose;
}


void 
BMidiSynth::MuteChannel(int16 channel, bool do_mute)
{
	fprintf(stderr, "[midi] MuteChannel is broken; don't use it\n");
}


void 
BMidiSynth::GetMuteMap(char* pChannels) const
{
	fprintf(stderr, "[midi] GetMuteMap is broken; don't use it\n");
}


void 
BMidiSynth::SoloChannel(int16 channel, bool do_solo)
{
	fprintf(stderr, "[midi] SoloChannel is broken; don't use it\n");
}


void 
BMidiSynth::GetSoloMap(char* pChannels) const
{
	fprintf(stderr, "[midi] GetSoloMap is broken; don't use it\n");
}


status_t 
BMidiSynth::LoadInstrument(int16 instrument)
{
	return be_synth->fSynth->LoadInstrument(instrument);
}


status_t 
BMidiSynth::UnloadInstrument(int16 instrument)
{
	return be_synth->fSynth->UnloadInstrument(instrument);
}


status_t 
BMidiSynth::RemapInstrument(int16 from, int16 to)
{
	return be_synth->fSynth->RemapInstrument(from, to);
}


void 
BMidiSynth::FlushInstrumentCache(bool startStopCache)
{
	be_synth->fSynth->FlushInstrumentCache(startStopCache);
}


uint32 
BMidiSynth::Tick() const
{
	return (uint32) (system_time() - fCreationTime);
}


void 
BMidiSynth::NoteOff(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->NoteOff(channel, note + fTranspose, velocity, time);
}


void 
BMidiSynth::NoteOn(
	uchar channel, uchar note, uchar velocity, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->NoteOn(channel, note + fTranspose, velocity, time);
}


void 
BMidiSynth::KeyPressure(
	uchar channel, uchar note, uchar pressure, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->KeyPressure(
			channel, note + fTranspose, pressure, time);
}


void 
BMidiSynth::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->ControlChange(
			channel, controlNumber, controlValue, time);
}


void 
BMidiSynth::ProgramChange(
	uchar channel, uchar programNumber, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->ProgramChange(channel, programNumber, time);
}


void 
BMidiSynth::ChannelPressure(uchar channel, uchar pressure, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->ChannelPressure(channel, pressure, time);
}


void 
BMidiSynth::PitchBend(uchar channel, uchar lsb, uchar msb, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->PitchBend(channel, lsb, msb, time);
}


void 
BMidiSynth::AllNotesOff(bool justChannel, uint32 time)
{
	if (fInputEnabled)
		be_synth->fSynth->AllNotesOff(justChannel, time);
}


void BMidiSynth::_ReservedMidiSynth1() { }
void BMidiSynth::_ReservedMidiSynth2() { }
void BMidiSynth::_ReservedMidiSynth3() { }
void BMidiSynth::_ReservedMidiSynth4() { }


void 
BMidiSynth::Run()
{
	// do nothing
}

