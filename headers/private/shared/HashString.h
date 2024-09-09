/*
 * Copyright 2004-2007, Ingo Weinhold <ingo_weinhold@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HASH_STRING_H
#define HASH_STRING_H

#include <SupportDefs.h>


static inline uint32
string_hash(const char *_string)
{
	const uint8* string = (const uint8*)_string;
	if (string == NULL)
		return 0;

	uint32 h = 5381;
	char c;
	while ((c = *string++) != 0)
		h = (h * 33) + c;
	return h;
}


namespace BPrivate {


// HashString
class HashString {
public:
	HashString();
	HashString(const HashString &string);
	HashString(const char *string, int32 length = -1);
	~HashString();

	bool SetTo(const char *string, int32 maxLength = -1);
	void Unset();

	void Truncate(int32 newLength);

	const char *GetString() const;
	int32 GetLength() const	{ return fLength; }

	uint32 GetHashCode() const	{ return string_hash(GetString()); }

	HashString &operator=(const HashString &string);
	bool operator==(const HashString &string) const;
	bool operator!=(const HashString &string) const { return !(*this == string); }

private:
	bool _SetTo(const char *string, int32 length);

private:
	int32	fLength;
	char	*fString;
};


}	// namespace BPrivate


using BPrivate::HashString;


#endif	// HASH_STRING_H
