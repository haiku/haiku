/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_KEY_H
#define STRING_KEY_H


#include "String.h"


class StringKey {
public:
	StringKey(const ::String& string)
		:
		fString(string.Data()),
		fHash(string.Hash())
	{
	}

	explicit StringKey(const char* string)
		:
		fString(string),
		fHash(hash_hash_string(string))
	{
	}

	const char* String() const
	{
		return fString;
	}

	uint32 Hash() const
	{
		return fHash;
	}

	inline int Compare(const ::StringKey& other) const
	{
		if (fHash == other.fHash) {
			if (fString == other.fString)
				return 0;
			return strcmp(fString, other.fString);
		}
		return (fHash < other.fHash) ? -1 : 1;
	}

	bool operator==(const ::String& other) const
	{
		return Compare(StringKey(other)) == 0;
	}

	bool operator!=(const ::String& other) const
	{
		return !(*this == other);
	}

private:
	const char*	fString;
	uint32		fHash;
};


#endif	// STRING_KEY_H
