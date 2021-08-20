/*

SynthFile.h

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

#ifndef _SYNTH_FILE_H
#define _SYNTH_FILE_H

#include "SynthFileReader.h"
#include <ObjectList.h>


class SSound {
	uint32  fOffset;
	uint16  fId;
	BString fName;
	void*   fSamples;
	int32   fFrameCount;
	int16   fSampleSize;
	int16   fChannelCount;

	SSound() { }	
public:
	SSound(uint16 id);

	uint16 Id() const               { return fId; }

	void        SetName(const char* name) { fName = name; }
	const char* Name() const              { return fName.String(); }

	void   SetSample(void* samples, int32 frameCount, int16 sampleSize, int16 channelCount);
	void*  Samples() const          { return fSamples; }
	int32  FrameCount() const       { return fFrameCount; }
	int16  SampleSize() const       { return fSampleSize; }
	int16  ChannelCount() const     { return fChannelCount; }

	bool   HasSamples() const       { return fSamples != NULL; }

	void   SetOffset(uint32 offset) { fOffset = offset; }
	uint32 Offset() const           { return fOffset; }
};


class SSoundInRange {
	uint8   fNoteStart;
	uint8   fNoteEnd;
	SSound* fSound;

	SSoundInRange() { }
public:
	SSoundInRange(uint8 start, uint8 end, SSound* sound);
	
	uint8  Start() const    { return fNoteStart; }
	uint8  End() const      { return fNoteEnd; }
	SSound* Sound() const   { return fSound; }
};


class SInstrument {
	uint32               fOffset;
	uint8                fId;
	BString              fName;
	SSound*              fDefaultSound;
	BObjectList<SSoundInRange> fSounds;
	
public:
	SInstrument(uint8 instrument);
	
	void SetOffset(uint32 offset)              { fOffset = offset; }
	uint32 Offset() const                      { return fOffset; }
	
	uint8 Id() const                           { return fId; }
	
	void SetName(const char* name)             { fName = name; }
	const char* Name() const                   { return fName.String(); }
	
	void SetDefaultSound(SSound* sound)        { fDefaultSound = sound; }
	SSound*               DefaultSound() const { return fDefaultSound; }
	
	BObjectList<SSoundInRange>* Sounds()       { return &fSounds; }
};


class SSynthFile;

class SSynthFile {
	BObjectList<SInstrument> fInstruments;
	BObjectList<SSound>      fSounds;
	SSynthFileReader   fReader;
	status_t           fStatus;
	
	void InstrumentAtPut(int i, SInstrument* instr);
	SSound* FindSound(uint16 sound);
	
public:
	SSynthFile(const char* fileName);
	status_t InitCheck() const { return fReader.InitCheck(); }
	

	SSound*      GetSound(uint16 sound);
	bool         HasInstrument(uint8 instrument) const;
	SInstrument* GetInstrument(uint8 instrument);

	void Dump();
};

#endif
