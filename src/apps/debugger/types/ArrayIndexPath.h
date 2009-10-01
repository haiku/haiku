/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARRAY_INDEX_PATH_H
#define ARRAY_INDEX_PATH_H


#include "Array.h"
#include "Types.h"


class BString;


class ArrayIndexPath {
public:
								ArrayIndexPath();
								ArrayIndexPath(const ArrayIndexPath& other);
								~ArrayIndexPath();

			bool				SetTo(const char* path);
			void				Clear();

			bool				GetPathString(BString& path) const;

	inline	int32				CountIndices() const;
	inline	int64				IndexAt(int32 index) const;
	inline	bool				AddIndex(int64 index);


			ArrayIndexPath&		operator=(const ArrayIndexPath& other);

private:
			typedef Array<int64> IndexArray;

private:
			IndexArray			fIndices;
};


int32
ArrayIndexPath::CountIndices() const
{
	return fIndices.Count();
}


int64
ArrayIndexPath::IndexAt(int32 index) const
{
	return index >= 0 && index < fIndices.Count()
		? fIndices.ElementAt(index) : -1;
}


bool
ArrayIndexPath::AddIndex(int64 index)
{
	return fIndices.Add(index);
}


#endif	// ARRAY_INDEX_PATH_H
