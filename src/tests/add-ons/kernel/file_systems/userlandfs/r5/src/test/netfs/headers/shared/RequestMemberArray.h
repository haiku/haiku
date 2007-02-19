// RequestMemberArray.h

#ifndef NET_FS_REQUEST_MEMBER_ARRAY_H
#define NET_FS_REQUEST_MEMBER_ARRAY_H

#include <new>

#include <stdlib.h>

#include "Request.h"
#include "RequestFlattener.h"
#include "RequestUnflattener.h"

template<typename Member>
class RequestMemberArray : public FlattenableRequestMember {
public:
	RequestMemberArray()
		: fElements(NULL),
		  fSize(0),
		  fCapacity(0)
	{
	}

	virtual ~RequestMemberArray()
	{
		for (int32 i = 0; i < fSize; i++)
			fElements[i].~Member();
		free(fElements);
	}

	virtual void ShowAround(RequestMemberVisitor* visitor)
	{
		visitor->Visit(this, fSize);
		for (int32 i = 0; i < fSize; i++)
			visitor->Visit(this, fElements[i]);
	}

	virtual status_t Flatten(RequestFlattener* flattener)
	{
		if (flattener->WriteInt32(fSize) != B_OK)
			return flattener->GetStatus();

		for (int32 i = 0; i < fSize; i++)
			flattener->Visit(this, fElements[i]);

		return flattener->GetStatus();
	}

	virtual status_t Unflatten(RequestUnflattener* unflattener)
	{
		if (fSize > 0) {
			for (int32 i = 0; i < fSize; i++)
				fElements[i].~Member();
			fSize = 0;
		}

		int32 size;
		if (unflattener->ReadInt32(size) != B_OK)
			return unflattener->GetStatus();

		status_t error = _EnsureCapacity(size);
		if (error != B_OK)
			return error;

		for (int32 i = 0; i < size; i++) {
			Member* element = new(fElements + i) Member;
			fSize = i + 1;
			unflattener->Visit(this, *element);
		}

		return unflattener->GetStatus();
	}

	status_t Append(const Member& element)
	{
		status_t error = _EnsureCapacity(fSize + 1);
		if (error != B_OK)
			return error;
		new(fElements + fSize) Member(element);
		fSize++;
		return B_OK;
	}

	int32 CountElements() const
	{
		return fSize;
	}

	Member* GetElements() const
	{
		return fElements;
	}

private:
	status_t _EnsureCapacity(int32 capacity)
	{
		const int32 kMinCapacity = 10;
		if (capacity < kMinCapacity)
			capacity = kMinCapacity;

		if (capacity > fCapacity) {
			if (capacity < 2 * fCapacity)
				capacity = 2 * fCapacity;

			Member* elements
				= (Member*)realloc(fElements, capacity * sizeof(Member));
			if (!elements)
				return B_NO_MEMORY;

			fElements = elements;
			fCapacity = capacity;
		}

		return B_OK;
	}

private:
	Member*	fElements;
	int32	fSize;
	int32	fCapacity;
};

#endif	// NET_FS_REQUEST_MEMBER_ARRAY_H
