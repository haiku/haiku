/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
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
#include "MidiSynthFile.h"

// -----------------------------------------------------------------------------

BMidiSynthFile::BMidiSynthFile()
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

BMidiSynthFile::~BMidiSynthFile()
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::LoadFile(const entry_ref* midi_entry_ref)
{
	UNIMPLEMENTED
	return B_ERROR;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::UnloadFile(void)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::Start(void)
{
	UNIMPLEMENTED
	return B_ERROR;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Stop(void)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Fade(void)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Pause(void)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::Resume(void)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Duration(void) const
{
	UNIMPLEMENTED
	return 0;
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Position(int32 ticks) const
{
	UNIMPLEMENTED
	return 0;
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Seek()
{
	UNIMPLEMENTED
	return 0;
}

// -----------------------------------------------------------------------------

status_t BMidiSynthFile::GetPatches(
	int16* pArray768, int16* pReturnedCount) const
{
	UNIMPLEMENTED
	return B_ERROR;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SetFileHook(synth_file_hook pSongHook, int32 arg)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

bool BMidiSynthFile::IsFinished(void) const
{
	UNIMPLEMENTED
	return false;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::ScaleTempoBy(double tempoFactor)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SetTempo(int32 newTempoBPM)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

int32 BMidiSynthFile::Tempo(void) const
{
	UNIMPLEMENTED
	return 0;
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::EnableLooping(bool loop)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::MuteTrack(int16 track, bool do_mute)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::GetMuteMap(char* pTracks) const
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::SoloTrack(int16 track, bool do_solo)
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::GetSoloMap(char* pTracks) const
{
	UNIMPLEMENTED
}

// -----------------------------------------------------------------------------

void BMidiSynthFile::_ReservedMidiSynthFile1() { }
void BMidiSynthFile::_ReservedMidiSynthFile2() { }
void BMidiSynthFile::_ReservedMidiSynthFile3() { }

// -----------------------------------------------------------------------------
