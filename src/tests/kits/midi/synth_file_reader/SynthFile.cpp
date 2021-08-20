/*

SynthFile.cpp

Copyright (c) 2002 Haiku.

Author:
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <Debug.h>

#include "SynthFile.h"
#include "SynthFileReader.h"

// SSound

SSound::SSound(uint16 id)
	: fOffset(0)
	, fId(id)
	, fSamples(NULL)
	, fFrameCount(0)
	, fSampleSize(0)
	, fChannelCount(0)
{
}


void   
SSound::SetSample(void* samples, int32 frameCount, int16 sampleSize, int16 channelCount)
{
	ASSERT(fSamples == NULL);
	fSamples      = samples;
	fFrameCount   = frameCount;
	fSampleSize   = sampleSize;
	fChannelCount = channelCount;
}

// SSoundInRange
SSoundInRange::SSoundInRange(uint8 start, uint8 end, SSound* sound)
	: fNoteStart(start)
	, fNoteEnd(end)
	, fSound(sound)
{
}


// SInstrument

SInstrument::SInstrument(uint8 instrument) 
	: fOffset(0)
	, fId(instrument)
	, fDefaultSound(NULL)
 	, fSounds(false) // does not own items	
{
}


// SSynthFile

static int bySoundId(const SSound* a, const SSound* b)
{
	return (a)->Id() - (b)->Id();
}

SSynthFile::SSynthFile(const char* fileName)
	: fReader(fileName)
{
	fStatus = fReader.InitCheck();
	if (fStatus == B_OK) {
		for (int i = 0; i < 128; i++) {
			fInstruments.AddItem(NULL);
		}
		
		fStatus = fReader.Initialize(this);
		fSounds.SortItems(bySoundId);
	}
}

SSound*
SSynthFile::FindSound(uint16 sound)
{
	const int n = fSounds.CountItems();
	for (int i = 0; i < n; i ++) {
		SSound* s = fSounds.ItemAt(i);
		if (s->Id() == sound) return s;
	}
	return NULL;
}

SSound* 
SSynthFile::GetSound(uint16 sound)
{
	SSound* s = FindSound(sound);
	if (s == NULL) {
		s = new SSound(sound);
		fSounds.AddItem(s);
	}
	return s;
}


void
SSynthFile::InstrumentAtPut(int i, SInstrument* instr)
{
	ASSERT(fInstruments.CountItems() == 128);
	fInstruments.ReplaceItem(i, instr);
}


bool
SSynthFile::HasInstrument(uint8 instrument) const
{
	ASSERT(instrument < 128);
	return fInstruments.ItemAt(instrument) != NULL;
}

SInstrument* 
SSynthFile::GetInstrument(uint8 instrument)
{
	ASSERT(instrument < 128);
	SInstrument* i = fInstruments.ItemAt(instrument);
	if (i == NULL) {
		i = new SInstrument(instrument);
		InstrumentAtPut(instrument, i);
	}
	return i;
}

void
SSynthFile::Dump()
{
	printf("SynthFile:\n");
	printf("==========\n\n");
	for (uint8 i = 0; i < 128; i++) {
		if (HasInstrument(i)) {
			SInstrument* instr = GetInstrument(i);
			printf("Instrument id=%d name=%s sounds=%d\n", 
				(int)instr->Id(), 
				instr->Name(), 
				(int)instr->Sounds()->CountItems());
			SSound* sound = instr->DefaultSound();
			ASSERT(sound != NULL);
			printf("Default Sound id=%d name='%s'\n", 
				(int)sound->Id(), 
				sound->Name());
			for (int s = 0; s < instr->Sounds()->CountItems(); s ++) {
				SSoundInRange* range = instr->Sounds()->ItemAt(s);
				printf("Sound id=%d start=%d end=%d name='%s'\n", 
					(int)range->Sound()->Id(), 
					(int)range->Start(),
					(int)range->End(),
					range->Sound()->Name());			
			}
			printf("\n");
		}	
	}
	
	printf("\nSounds:\n");
	printf("-------\n\n");
	for (int i = 0; i < fSounds.CountItems(); i ++) {
		SSound* sound = fSounds.ItemAt(i);
		printf("Id=%d name='%s'\n", (int)sound->Id(), sound->Name());
	}
}

