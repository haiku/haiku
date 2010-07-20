/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHECK_SUM_H
#define CHECK_SUM_H


#include <string.h>

#include <SHA256.h>


struct CheckSum {
	const uint8* Data() const
	{
		return fData;
	}

	bool IsZero() const
	{
		for (size_t i = 0; i < sizeof(fData); i++) {
			if (fData[i] != 0)
				return false;
		}

		return true;
	}

	CheckSum& operator=(const CheckSum& other)
	{
		memcpy(fData, other.fData, sizeof(fData));
		return *this;
	}

	CheckSum& operator=(const void* buffer)
	{
		memcpy(fData, buffer, sizeof(fData));
		return *this;
	}

	bool operator==(const void* buffer) const
	{
		return memcmp(fData, buffer, sizeof(fData)) == 0;
	}

	bool operator!=(const void* buffer) const
	{
		return !(*this == buffer);
	}

private:
	uint8	fData[SHA_DIGEST_LENGTH];
};


#endif	// CHECK_SUM_H
