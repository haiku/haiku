/*

SynthFileReader.h

Copyright (c) 2002 OpenBeOS.

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

class SynthFileReader {
	FILE* fFile;

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

	// debugging support
	void Print(tag tag);
	void Dump(uint32 bytes);
	void Play(uint16 rate, uint32 offset, uint32 size);
	
public:
	SynthFileReader(const char* synthFile);
	~SynthFileReader();
	bool InitCheck() const;	

	void Dump(bool play, uint32 instrOnly);
};

#endif
