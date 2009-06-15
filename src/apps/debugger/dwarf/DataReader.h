/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_READER_H
#define DATA_READER_H

#include <SupportDefs.h>


class DataReader {
public:
	DataReader()
		:
		fData(NULL),
		fSize(0),
		fInitialSize(0),
		fOverflow(false)
	{
	}

	DataReader(const void* data, off_t size)
	{
		SetTo(data, size);
	}

	void SetTo(const void* data, off_t size)
	{
		fData = (const uint8*)data;
		fInitialSize = fSize = size;
		fOverflow = false;
	}

	bool HasData() const
	{
		return fSize > 0;
	}

	bool HasOverflow() const
	{
		return fOverflow;
	}

	const void* Data() const
	{
		return fData;
	}

	off_t BytesRemaining() const
	{
		return fSize;
	}

	off_t Offset() const
	{
		return fInitialSize - fSize;
	}

	void SeekAbsolute(off_t offset)
	{
		if (offset < 0)
			offset = 0;
		else if (offset > fInitialSize)
			offset = fInitialSize;

		fData += offset - Offset();
		fSize = fInitialSize - offset;
	}

	template<typename Type>
	Type Read(const Type& defaultValue)
	{
		if (fSize < sizeof(Type)) {
			fOverflow = true;
			fSize = 0;
			return defaultValue;
		}

		Type data;
		memcpy(&data, fData, sizeof(Type));

		fData += sizeof(Type);
		fSize -= sizeof(Type);

		return data;
	}

	uint64 ReadUnsignedLEB128(uint64 defaultValue)
	{
		uint64 result = 0;
		int shift = 0;
		while (true) {
			uint8 byte = Read<uint8>(0);
			result |= uint64(byte & 0x7f) << shift;
			if ((byte & 0x80) == 0)
				break;
			shift += 7;
		}

		return fOverflow ? defaultValue : result;
	}

	int64 ReadSignedLEB128(int64 defaultValue)
	{
		int64 result = 0;
		int shift = 0;
		while (true) {
			uint8 byte = Read<uint8>(0);
			result |= uint64(byte & 0x7f) << shift;
			shift += 7;

			if ((byte & 0x80) == 0) {
				// sign extend
				if ((byte & 0x40) != 0 && shift < 64)
					result |= -((uint64)1 << shift);
				break;
			}
		}

		return fOverflow ? defaultValue : result;
	}

	const char* ReadString()
	{
		const char* string = (const char*)fData;
		while (fSize > 0) {
			fData++;
			fSize--;

			if (fData[-1] == 0)
				return string;
		}

		fOverflow = true;
		return NULL;
	}

	bool Skip(off_t bytes)
	{
		if (bytes < 0)
			return false;

		if (bytes > fSize) {
			fSize = 0;
			fOverflow = true;
			return false;
		}

		fData += bytes;
		fSize -= bytes;

		return true;
	}

private:
	const uint8*	fData;
	off_t			fSize;
	off_t			fInitialSize;
	bool			fOverflow;
};


#endif	// DATA_READER_H
