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

#include "debug.h"
#include "MidiStore.h"
#include "MidiSynthFile.h"
#include "SoftSynth.h"

using namespace BPrivate;

// -----------------------------------------------------------------------------

BMidiSynthFile::BMidiSynthFile()
{
	store = new BMidiStore();
}

// -----------------------------------------------------------------------------

BMidiSynthFile::~BMidiSynthFile()
{
	Stop();
	delete store;
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::LoadFile(const entry_ref* midi_entry_ref)
{
	if (midi_entry_ref == NULL)
	{
		return B_BAD_VALUE;
	}

	EnableInput(true, false);
	store->finished = true;

	status_t err = store->Import(midi_entry_ref);

	if (err == B_OK)
	{
		for (int t = 0; t < 128; ++t)
		{
			if (store->instruments[t])
			{
				be_synth->synth->LoadInstrument(t);
			}
		}
	}
	
	return err;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::UnloadFile(void)
{
	delete store;
	store = new BMidiStore();
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::Start(void)
{
	store->Connect(this);
	store->SetCurrentEvent(0);
	return store->Start();
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Stop(void)
{
	store->Stop();
	store->Disconnect(this);
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Fade(void)
{
	Stop();  // really quick fade :P
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Pause(void)
{
	store->paused = true;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Resume(void)
{
	store->paused = false;
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Duration(void) const
{
	return store->DeltaOfEvent(store->CountEvents());
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Position(int32 ticks) const
{
	return store->EventAtDelta(ticks);
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Seek()
{
	return store->CurrentEvent();
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::GetPatches(
	int16* pArray768, int16* pReturnedCount) const
{
	int16 count = 0;
	
	for (int t = 0; t < 128; ++t)
	{
		if (store->instruments[t])
		{
			pArray768[count++] = t;
		}
	}

	*pReturnedCount = count;
	return B_OK;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SetFileHook(synth_file_hook pSongHook, int32 arg)
{
	store->hookFunc = pSongHook;
	store->hookArg = arg;
}

// -----------------------------------------------------------------------------

bool BMidiSynthFile::IsFinished(void) const
{
	return store->finished;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::ScaleTempoBy(double tempoFactor)
{
	store->SetTempo((int32) (Tempo() * tempoFactor));
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SetTempo(int32 newTempoBPM)
{
	store->SetTempo(newTempoBPM);
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Tempo(void) const
{
	return store->Tempo();
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::EnableLooping(bool loop)
{
	store->looping = loop;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::MuteTrack(int16 track, bool do_mute)
{
	fprintf(stderr, "[midi] MuteTrack is broken; don't use it\n");
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::GetMuteMap(char* pTracks) const
{
	fprintf(stderr, "[midi] GetMuteMap is broken; don't use it\n");
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SoloTrack(int16 track, bool do_solo)
{
	fprintf(stderr, "[midi] SoloTrack is broken; don't use it\n");
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::GetSoloMap(char* pTracks) const
{
	fprintf(stderr, "[midi] GetSoloMap is broken; don't use it\n");
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::_ReservedMidiSynthFile1() { }
void BMidiSynthFile::_ReservedMidiSynthFile2() { }
void BMidiSynthFile::_ReservedMidiSynthFile3() { }

// -----------------------------------------------------------------------------
