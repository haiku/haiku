/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H


#include <stdlib.h>

#include <OS.h>


template<typename Type>
class CircularBuffer {
public:
	CircularBuffer(size_t size)
		:
		fSize(0)
	{
		SetSize(size);
	}

	~CircularBuffer()
	{
		free(fBuffer);
	}

	status_t InitCheck() const
	{
		return fBuffer != NULL ? B_OK : B_NO_MEMORY;
	}

	status_t SetSize(size_t size)
	{
		MakeEmpty();

		if (fSize == size)
			return B_OK;

		fSize = size;
		fBuffer = (Type*)malloc(fSize * sizeof(Type));
		if (fBuffer == NULL) {
			fSize = 0;
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void MakeEmpty()
	{
		fIn = 0;
		fFirst = 0;
	}

	bool IsEmpty() const
	{
		return fIn == 0;
	}

	int32 CountItems() const
	{
		return fIn;
	}

	Type* ItemAt(int32 index) const
	{
		if (index >= (int32)fIn || index < 0 || fBuffer == NULL)
			return NULL;

		return &fBuffer[(fFirst + index) % fSize];
	}

	void AddItem(const Type& item)
	{
		uint32 index;
		if (fIn < fSize)
			index = fFirst + fIn++;
		else
			index = fFirst++;

		if (fBuffer != NULL)
			fBuffer[index % fSize] = item;
	}

	size_t Size() const
	{
		return fSize;
	}

private:
	uint32		fFirst;
	uint32		fIn;
	uint32		fSize;
	Type*		fBuffer;
};

#endif	// CIRCULAR_BUFFER_H
