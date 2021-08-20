/*

SynthFileReader.cpp

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

#include "SynthFileReader.h"
#include "SynthFile.h"
#include <string.h>
#include <ctype.h>

#define DEBUG 1
#include <Debug.h>

SSynthFileReader::SSynthFileReader(const char* synthFile) {
	fFile = fopen(synthFile, "r+b");
	tag tag;
	if (fFile) {
		if (Read(tag) && TagEquals(tag, "IREZ")) {
			return;
		}
		fclose(fFile); fFile = NULL;
	}
	
}

SSynthFileReader::~SSynthFileReader() {
	if (fFile) {
		fclose(fFile); fFile = NULL;
	}
}

status_t SSynthFileReader::InitCheck() const {
	return fFile != NULL ? B_OK : B_ERROR;
}

bool SSynthFileReader::TagEquals(const char* tag1, const char* tag2) const {
	return strncmp(tag1, tag2, 4) == 0;
}

bool SSynthFileReader::Read(void* data, uint32 size) {
	return 1 == fread(data, size, 1, fFile);
}

bool SSynthFileReader::Read(tag &tag) {
	return Read((void*)tag, sizeof(tag));
}

bool SSynthFileReader::Read(uint64 &n, uint32 size) {
	uint8 number[8];
	ASSERT(size <= sizeof(number));
	if (Read((void*)number, size)) {
		n = 0;
		for (unsigned int i = 0; i < size; i ++) {
			n <<= 8;
			n += number[i];
		}
		return true;
	}
	return false;	
}

bool SSynthFileReader::Read(uint32 &n) {
	uint64 num;
	bool ok = Read(num, 4);
	n = num;
	return ok;
}


bool SSynthFileReader::Read(uint16 &n) {
	uint64 num;
	bool ok = Read(num, 2);
	n = num;
	return ok;
}


bool SSynthFileReader::Read(uint8 &n) {
	return Read((void*)&n, 1);
}

bool SSynthFileReader::Read(BString& s, uint32 len) {
	char* str = s.LockBuffer(len+1);
	str[len] = 0;
	bool ok = Read((void*)str, len);
	s.UnlockBuffer(len+1);
	return ok;	
}

bool SSynthFileReader::Skip(uint32 bytes) {
	fseek(fFile, bytes, SEEK_CUR);
	return true;
}


bool SSynthFileReader::ReadHeader(uint32& version, uint32& chunks, uint32& nextChunk) {
	tag tag;
	fseek(fFile, 0, SEEK_SET);
	if (Read(tag) && TagEquals(tag, "IREZ") && Read(version) && Read(chunks)) {
		nextChunk = ftell(fFile);
		return true;
	}
	return false;
}


bool SSynthFileReader::NextChunk(tag& tag, uint32& nextChunk) {
	fseek(fFile, nextChunk, SEEK_SET);
	return Read(nextChunk) && Read(tag);
}


bool SSynthFileReader::ReadInstHeader(uint16& inst, uint16& snd, uint16& snds, BString& name, uint32& size) {
	uint8 len;
	
	if (Skip(2) && Read(inst) && Read(len) && Read(name, len) && Read(size) && Read(snd) && Skip(10) && Read(snds)) {
		size -= 14;
		return true;
	}
	return false;
}


bool SSynthFileReader::ReadSoundInRange(uint8& start, uint8& end, uint16& snd, uint32& size) {
	size -= 4;
	return Read(start) && Read(end) && Read(snd) && Skip(4);
}


bool SSynthFileReader::ReadSoundHeader(uint16& inst, BString& name, uint32& size) {
	uint8 len;
	return Skip(2) && Read(inst) && Read(len) && Read(name, len) && Read(size);
}


status_t SSynthFileReader::Initialize(SSynthFile* synth) {
	uint32   version;
	uint32   chunks;
	uint32   nextChunk;
	tag      t;

	if (ReadHeader(version, chunks, nextChunk)) {
		for (uint32 chunk = 0; chunk < chunks; chunk ++) {
			if (NextChunk(t, nextChunk)) {
				if (TagEquals(t, "INST")) {
					uint32  offset = Tell();
					uint16  inst;
					uint16  snd;
					uint16  snds;
					BString name;
					uint32  size;
					if (ReadInstHeader(inst, snd, snds, name, size)) {
						SInstrument* instr = synth->GetInstrument(inst);
						instr->SetOffset(offset);
						instr->SetName(name.String());
						instr->SetDefaultSound(synth->GetSound(snd));
						for (int s = 0; s < snds; s ++) {
							uint8 start, end;
							if (ReadSoundInRange(start, end, snd, size)) {
								instr->Sounds()->AddItem(new SSoundInRange(start, end, synth->GetSound(snd)));
							} else {
								return B_ERROR;
							}
						}
					} else {
						return B_ERROR;
					}
				} else if (TagEquals(t, "snd ")) {
					uint32  offset = Tell();
					uint16  inst;
					BString name;
					uint32  size; 
					if (ReadSoundHeader(inst, name, size)) {
						SSound* s = synth->GetSound(inst);
						s->SetOffset(offset);
						s->SetName(name.String());
					} else {
						return B_ERROR;
					}
				} else {
					// skip
				}
			} else {
				return B_ERROR;
			}
		}		
		return B_OK;	
	}
	return B_ERROR;
}

// debugging
void SSynthFileReader::Print(tag tag) {
	printf("%.*s", 4, tag);
}


void SSynthFileReader::Dump(uint32 bytes) {
	uint8 byte;
	int   col = 0;
	const int cols = 16;
	bool  first = true;
	long  start = ftell(fFile);
	
	for (;bytes > 0 && Read(byte); bytes --, col = (col + 1) % cols) {
		if (col == 0) {
			if (first) first = false;
			else printf("\n");
			printf("%6.6lx(%3.3lx)  ", ftell(fFile)-1, ftell(fFile)-start-1);
		}
		printf("%2.2x ", (uint)byte);
		if (isprint(byte)) printf("'%c' ", (char)byte);
		else printf("    ");
	} 
}

void SSynthFileReader::Dump(bool play, uint32 instrOnly) {
	tag    tag;
	uint32 version;
	uint32 nEntries;
	uint32 next;
	uint32 cur;
	
	printf("SynthFileReader::Dump\n");
	printf("=================\n\n");
	
	// read header
	fseek(fFile, 0, SEEK_SET);
	Read(tag); ASSERT(TagEquals(tag, "IREZ"));	
	if (Read(version) && Read(nEntries)) {
		printf("version= %ld entries= %ld\n", version, nEntries);

		// dump rest of file
//		for (uint32 i = 0; i < nEntries; i++) {
		while (Read(next)) {
			printf("next=%lx cur=%lx ", next, cur);
			cur = ftell(fFile);
			if (Read(tag)) {
				Print(tag);
				if (!play && TagEquals(tag, "INST")) {
					uint32  inst;
					uint8   len;
					BString name;
					uint32  size;
					
					if (Read(inst) && Read(len) && Read(name, len) && Read(size)) {
						uint32 rest = size;
						printf(" inst=%d '%s' size=%lx", (int)inst, name.String(), size);

						printf("\n"); Dump(12); printf("\n");
						rest -= 10;
						
						uint16 elems;
						Read(elems);
						rest -= 4;
						
						printf("elems = %d\n", elems);
						
						if (elems > 0 || rest >= 16) {
							Dump(elems * 8 + 21); printf("\n");
						
							rest -= elems * 8 + 21;
						} else {
							printf("rest %ld\n", rest);
							Dump(rest);
							rest = 0;
						}
						
						bool prev_was_sust = false;
						while (rest > 0) {
							Read(tag); rest -= 4;
							Print(tag); printf("\n");
							int s = 0;
							if (TagEquals(tag, "ADSR")) {
								s = 1+4+4;
							} else if (TagEquals(tag, "LINE")) {
								s = 8;
							} else if (TagEquals(tag, "SUST")) {
								s = 8;
							} else if (TagEquals(tag, "LAST")) {
								s = 0;
								if (rest > 0) {
									SSynthFileReader::tag tag2;
									long pos = ftell(fFile);
									Read(tag2);
									fseek(fFile, pos, SEEK_SET);
									if (!isalpha(tag2[0])) {
										s = 4;
									}
								}
							} else if (TagEquals(tag, "LPGF")) {
								s = 12;
							} else if (TagEquals(tag, "LPFR")) {
								s = 9;
							} else if (TagEquals(tag, "SINE")) {
								s = 8;
							} else if (TagEquals(tag, "LPRE")) {
								s = 9;
							} else if (TagEquals(tag, "PITC")) {
								s = 9;
							} else if (TagEquals(tag, "LPAM")) {
								s = 9;
							} else if (TagEquals(tag, "VOLU")) {
								s = 9;
							} else if (TagEquals(tag, "SPAN")) {
								s = 9;
							} else if (TagEquals(tag, "TRIA")) {
								s = 8;
							} else if (TagEquals(tag, "SQU2")) {
								s = 8;
							} else if (TagEquals(tag, "SQUA")) {
								s = 8;
							// Patches*.hsb
							} else if (TagEquals(tag, "CURV")) {
								s = 0;
							} else {
								// ASSERT(false);
								printf("unknown tag "); Print(tag); printf("\n");
								// ASSERT(false);
								break;
							}
							prev_was_sust = TagEquals(tag, "SUST");
							Dump(s); printf("\n");
							if (rest < s) {
								printf("wrong size: rest=%ld size= %d\n", rest, s);
								break;
							}
							rest -= s;
						}
						if (rest > 0) {		
							printf("Rest:\n");
							Dump(rest); printf("\n");
						}
					}
				} else if (TagEquals(tag, "snd ")) {
					uint32  inst;
					uint8   len;
					BString name;
					uint32  size;
					
					if (Read(inst) && Read(len) && Read(name, len) && Read(size)) {
						printf(" inst=%lx '%s' size=%lx\n", inst, name.String(), size);
						uint32 rest = size;
						Dump(28); printf("\n"); rest -= 28;
						uint16 rate;
						Read(rate); rest -= 2;
						printf("rate=%d\n", (int)rate);
						Dump(16*3+7); printf("\n"); rest -= 16*3+7;
						printf("size=%ld offsetToNext=%ld\n", size, next-cur);
						if (play && (instrOnly==0xffff || instrOnly == inst)) Play(rate, 0, rest);
					}
				}
				printf("\n");
			} else {
				exit(-1);
			}
			fseek(fFile, next, SEEK_SET);
		}		
	} else {
		printf("Could not read header\n");
	}
}

#include <GameSoundDefs.h>
#include <SimpleGameSound.h>

void SSynthFileReader::Play(uint16 rate, uint32 offset, uint32 size) {
	uint8* samples = new uint8[size];
	fseek(fFile, offset, SEEK_CUR);
	Read((void*)samples, size);

	BSimpleGameSound* s;
	gs_audio_format format = { 
		rate,
		1,
		gs_audio_format::B_GS_S16,
		2,
		0
	};
	s = new BSimpleGameSound((void*)samples, size/2, &format);
	if (s->InitCheck() == B_OK) {
		s->StartPlaying();
		s->SetIsLooping(true);
		printf("hit enter "); while (getchar() != '\n');
		s->StopPlaying();
	} else {
		printf("Could not initialize BSimpleGameSound!\n");
	}
	delete s;
}
