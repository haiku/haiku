/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SUPPORT_BSTRING_PRIVATE_H_
#define _SUPPORT_BSTRING_PRIVATE_H_


#include <stdlib.h>

#include <String.h>


class BString::Private {
public:
	static const uint32 kPrivateDataOffset = 2 * sizeof(int32);

public:
	Private(const BString& string)
		:
		fString(string)
	{
	}

	char* Data()
	{
		return fString.fPrivateData;
	}

	static vint32& DataRefCount(char* data)
	{
		return *(((int32 *)data) - 2);
	}

	vint32& DataRefCount()
	{
		return DataRefCount(Data());
	}

	static int32& DataLength(char* data)
	{
		return *(((int32*)data) - 1);
	}

	int32& DataLength()
	{
		return DataLength(Data());
	}

	static void IncrementDataRefCount(char* data)
	{
		if (data != NULL)
			atomic_add(&DataRefCount(data), 1);
	}

	static void DecrementDataRefCount(char* data)
	{
		if (data != NULL) {
			if (atomic_add(&DataRefCount(data), -1) == 1)
				free(data - kPrivateDataOffset);
		}
	}

	static BString StringFromData(char* data)
	{
		return BString(data, BString::PRIVATE_DATA);
	}

private:
	const BString&	fString;
};


#endif	// _SUPPORT_BSTRING_PRIVATE_H_

