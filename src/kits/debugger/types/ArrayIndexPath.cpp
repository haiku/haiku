/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ArrayIndexPath.h"

#include <stdlib.h>

#include <String.h>


static const char kIndexSeparator = ';';


ArrayIndexPath::ArrayIndexPath()
{
}


ArrayIndexPath::ArrayIndexPath(const ArrayIndexPath& other)
	:
	fIndices(other.fIndices)
{
}


ArrayIndexPath::~ArrayIndexPath()
{
}


status_t
ArrayIndexPath::SetTo(const char* path)
{
	fIndices.Clear();

	if (path == NULL)
		return B_OK;

	while (*path != '\0') {
		char* numberEnd;
		int64 index = strtoll(path, &numberEnd, 0);
		if (numberEnd == path)
			return B_BAD_VALUE;
		path = numberEnd;

		if (!fIndices.Add(index))
			return B_NO_MEMORY;

		if (*path == '\0')
			break;

		if (*path != kIndexSeparator)
			return B_BAD_VALUE;
		path++;
	}

	return B_OK;
}


void
ArrayIndexPath::Clear()
{
	fIndices.Clear();
}


bool
ArrayIndexPath::GetPathString(BString& path) const
{
	path.Truncate(0);

	int32 count = CountIndices();
	for (int32 i = 0; i < count; i++) {
		// append separator for all but the first index
		if (i > 0) {
			int32 oldLength = path.Length();
			if (path.Append(kIndexSeparator, 1).Length() != oldLength + 1)
				return false;
		}

		// append index
		int32 oldLength = path.Length();
		if ((path << IndexAt(i)).Length() == oldLength)
			return false;
	}

	return true;
}


ArrayIndexPath&
ArrayIndexPath::operator=(const ArrayIndexPath& other)
{
	fIndices = other.fIndices;
	return *this;
}
