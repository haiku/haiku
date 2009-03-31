/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTF8_CHAR_H
#define UTF8_CHAR_H

#include <ctype.h>
#include <string.h>


struct UTF8Char {
	char	bytes[4];

	UTF8Char()
	{
	}

	UTF8Char(char c)
	{
		bytes[0] = c;
	}

	UTF8Char(const char* c, int32 count)
	{
		SetTo(c, count);
	}

	void SetTo(const char* c, int32 count)
	{
		bytes[0] = c[0];
		if (count > 1) {
			bytes[1] = c[1];
			if (count > 2) {
				bytes[2] = c[2];
				if (count > 3)
					bytes[3] = c[3];
			}
		}
	}

	static int32 ByteCount(char firstChar)
	{
		// Note, this does not recognize invalid chars
		uchar c = firstChar;
		if (c < 0x80)
			return 1;
		if (c < 0xe0)
			return 2;
		return c < 0xf0 ? 3 : 4;
	}

	int32 ByteCount() const
	{
		return ByteCount(bytes[0]);
	}

	bool IsFullWidth() const
	{
		// TODO: Implement!
		return false;
	}

	bool IsSpace() const
	{
		// TODO: Support multi-byte chars!
		return ByteCount() == 1 ? isspace(bytes[0]) : false;
	}

	UTF8Char ToLower() const
	{
		// TODO: Support multi-byte chars!
		if (ByteCount() > 1)
			return *this;

		return UTF8Char((char)tolower(bytes[0]));
	}

	bool operator==(const UTF8Char& other) const
	{
		int32 byteCount = ByteCount();
		bool equals = bytes[0] == other.bytes[0];
		if (byteCount > 1 && equals) {
			equals = bytes[1] == other.bytes[1];
			if (byteCount > 2 && equals) {
				equals = bytes[2] == other.bytes[2];
				if (byteCount > 3 && equals)
					equals = bytes[3] == other.bytes[3];
			}
		}
		return equals;
	}

	bool operator!=(const UTF8Char& other) const
	{
		return !(*this == other);
	}
};


#endif	// UTF8_CHAR_H
