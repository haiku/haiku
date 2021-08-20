/*

SynthFileReader.h

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

#ifndef _SYNTH_FILE_READER_H
#define _SYNTH_FILE_READER_H

#include <stdio.h>
#include <SupportDefs.h>
#include <String.h>

class SSynthFile;

class SSynthFileReader {
private:
	FILE*   fFile;

	typedef char tag[4];

	bool TagEquals(const char* tag1, const char* tag2) const;
	bool Read(void* data, uint32 size);
	bool Read(BString& s, uint32 size);
	bool Read(tag &tag);
	bool Read(uint64 &n, uint32 size);
	bool Read(uint32 &n);
	bool Read(uint16 &n);
	bool Read(uint8  &n);
	bool Skip(uint32 bytes);
	uint32 Tell()            { return ftell(fFile); }
	void   Seek(uint32 pos)  { fseek(fFile, pos, SEEK_SET); }
	
	bool ReadHeader(uint32& version, uint32& chunks, uint32& nextChunk);
	bool NextChunk(tag& tag, uint32& nextChunk);
	bool ReadInstHeader(uint16& inst, uint16& snd, uint16& snds, BString& name, uint32& size);
	bool ReadSoundInRange(uint8& start, uint8& end, uint16& snd, uint32& size);
	bool ReadSoundHeader(uint16& inst, BString& name, uint32& size);

	// debugging support
	void Print(tag tag);
	void Dump(uint32 bytes);
	void Play(uint16 rate, uint32 offset, uint32 size);
	
public:
	SSynthFileReader(const char* synthFile);
	~SSynthFileReader();
	status_t InitCheck() const;	

	status_t Initialize(SSynthFile* synth);

	void Dump(bool play, uint32 instrOnly);
};

#endif
