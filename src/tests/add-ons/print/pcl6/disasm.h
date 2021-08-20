/*

PCL6 Disassembler

Copyright (c) 2003 Haiku.

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

#ifndef _DISASM_H
#define _DISASM_H

#include "jetlib.h"
#include <SupportDefs.h>
#include <List.h>

class InputStream {
	uint8 fBuffer; // one byte only
	int8 fBufferSize;
	uint32 fPos;
	
public:
	InputStream() : fBufferSize(0), fPos(0) { }
	virtual ~InputStream() {};
	virtual int RawRead(void* buffer, int size) = 0;
	int Read(void* buffer, int size);
	int Pos() const { return fPos; }
	bool PutUByte(uint8 v);
	bool ReadUByte(uint8& v);
	bool ReadUInt16(uint16& v);
	bool ReadUInt32(uint32& v);
	bool ReadSInt16(int16& v);
	bool ReadSInt32(int32& v);
	bool ReadReal32(float& v);
};

class File : public InputStream {
	FILE* fFile;
public:
	File(const char* filename);
	~File();
	bool InitCheck() { return fFile != NULL; }
	int RawRead(void* buffer, int size);
};

typedef struct {
	uint8 value;
	const char* name;
} AttrValue;

#define NUM_OF_ELEMS(array, type) (sizeof(array) / sizeof(type))

class Disasm {
public:
	Disasm(InputStream* stream) : fStream(stream), fVerbose(false) { };
	void SetVerbose(bool v) { fVerbose = v; }
	void Print();
	
private:
	InputStream* fStream;
	BList fArgs;
	bool fVerbose;

	void Error(const char* text);
	bool Expect(const char* text);
	bool SkipTo(const char* text);
	bool ReadNumber(int32& n);
	bool ParsePJL();
	
	void PushArg(struct ATTRIBUTE* attr) { fArgs.AddItem(attr); }
	int  NumOfArgs() { return fArgs.CountItems(); }
	struct ATTRIBUTE* ArgAt(int i) { return (struct ATTRIBUTE*)fArgs.ItemAt(i); }
	void DeleteAttr(struct ATTRIBUTE* attr);
	void PrintAttr(struct ATTRIBUTE* attr);
	void ClearAttrs();
	
	bool DecodeOperator(uint8 byte);
	struct ATTRIBUTE* ReadData(uint8 byte);
	bool PushData(uint8 byte);
	bool ReadArrayLength(uint32& length);
	bool ReadArray(uint8 type, uint32 length, struct ATTRIBUTE* attr);
	const char* AttributeName(uint8 id);
	void GenericPrintAttribute(uint8 id);
	void PrintAttributeValue(uint8 id, const AttrValue* table, int tableSize);
	bool DecodeAttribute(uint8 byte);
	bool DecodeEmbedData(uint8 byte);
	bool ParsePCL6();
	
	bool IsOperator(uint8 byte) const { return byte >= 0x41 && byte <= 0xb9; }
	bool IsDataType(uint8 byte) const { return byte >= 0xc0 && byte <= 0xef; }
	bool IsAttribute(uint8 byte) const { return byte >= 0xf8 && byte <= 0xf9; }
	bool IsEmbedData(uint8 byte) const { return byte >= 0xfa && byte <= 0xfb; }
	bool IsWhiteSpace(uint8 byte) const { return byte == 0 || byte >= 0x09 && byte <= 0x0d || byte == 0x20; }
	bool IsEsc(uint8 byte) const { return byte == 0x1b; }
};

#endif
