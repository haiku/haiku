/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <debug_hex_dump.h>

#include <ctype.h>
#include <stdio.h>


namespace BKernel {


// #pragma mark - HexDumpDataProvider


HexDumpDataProvider::~HexDumpDataProvider()
{
}


bool
HexDumpDataProvider::GetAddressString(char* buffer, size_t bufferSize) const
{
	return false;
}


// #pragma mark - HexDumpBufferDataProvider


HexDumpBufferDataProvider::HexDumpBufferDataProvider(const void* data,
	size_t dataSize)
	:
	fData((const uint8*)data),
	fDataSize(dataSize)
{
}


bool
HexDumpBufferDataProvider::HasMoreData() const
{
	return fDataSize > 0;
}


uint8
HexDumpBufferDataProvider::NextByte()
{
	if (fDataSize == 0)
		return '\0';

	fDataSize--;
	return *fData++;
}


bool
HexDumpBufferDataProvider::GetAddressString(char* buffer,
	size_t bufferSize) const
{
	snprintf(buffer, bufferSize, "%p", fData);
	return true;
}


// #pragma mark -


void
print_hex_dump(HexDumpDataProvider& data, size_t maxBytes, uint32 flags)
{
	static const size_t kBytesPerBlock = 4;
	static const size_t kBytesPerLine = 16;

	size_t i = 0;
	for (; i < maxBytes && data.HasMoreData();) {
		if (i > 0)
			kputs("\n");

		// print address
		uint8 buffer[kBytesPerLine];
		if ((flags & HEX_DUMP_FLAG_OMIT_ADDRESS) == 0
			&& data.GetAddressString((char*)buffer, sizeof(buffer))) {
			kputs((char*)buffer);
			kputs(": ");
		}

		// get the line data
		size_t bytesInLine = 0;
		for (; i < maxBytes && bytesInLine < kBytesPerLine
				&& data.HasMoreData();
			i++) {
			buffer[bytesInLine++] = data.NextByte();
		}

		// print hex representation
		for (size_t k = 0; k < bytesInLine; k++) {
			if (k > 0 && k % kBytesPerBlock == 0)
				kputs(" ");
			kprintf("%02x", buffer[k]);
		}

		// pad to align the text representation, if line is incomplete
		if (bytesInLine < kBytesPerLine) {
			int missingBytes = int(kBytesPerLine - bytesInLine);
			kprintf("%*s",
				2 * missingBytes + int(missingBytes / kBytesPerBlock), "");
		}

		// print character representation
		kputs("  ");
		for (size_t k = 0; k < bytesInLine; k++)
			kprintf("%c", isprint(buffer[k]) ? buffer[k] : '.');
	}

	if (i > 0)
		kputs("\n");
}


void
print_hex_dump(const void* data, size_t maxBytes, uint32 flags)
{
	HexDumpBufferDataProvider dataProvider(data, maxBytes);
	print_hex_dump(dataProvider, maxBytes, flags);
}


}	// namespace BKernel
