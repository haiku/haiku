/* Utility - some helper classes
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "Utility.h"
#include "Debug.h"
#include "cpp.h"

#include <stdlib.h>
#include <string.h>


bool
sorted_array::FindInternal(off_t value, int32 &index) const
{
	int32 min = 0, max = count-1;
	off_t cmp;
	while (min <= max) {
		index = (min + max) / 2;

		cmp = values[index] - value;
		if (cmp < 0)
			min = index + 1;
		else if (cmp > 0)
			max = index - 1;
		else
			return true;
	}
	return false;
}


void 
sorted_array::Insert(off_t value)
{
	// if there are more than 8 values in this array, use a
	// binary search, if not, just iterate linearly to find
	// the insertion point
	int32 i;
	if (count > 8 ) {
		if (!FindInternal(value,i)
			&& values[i] <= value)
			i++;
	} else {
		for (i = 0;i < count; i++)
			if (values[i] > value)
				break;
	}

	memmove(&values[i+1],&values[i],(count - i) * sizeof(off_t));
	values[i] = value;
	count++;
}


bool 
sorted_array::Remove(off_t value)
{
	int32 index = Find(value);
	if (index == -1)
		return false;

	memmove(&values[index],&values[index + 1],(count - index) * sizeof(off_t));
	count--;

	return true;
}


//	#pragma mark -


BlockArray::BlockArray(int32 blockSize)
	:
	fArray(NULL),
	fSize(0),
	fBlockSize(blockSize)
{
}


BlockArray::~BlockArray()
{
	if (fArray)
		free(fArray);
}


int32
BlockArray::Find(off_t value)
{
	if (fArray == NULL)
		return -1;
	
	return fArray->Find(value);
}


status_t
BlockArray::Insert(off_t value)
{
	if (fArray == NULL || fArray->count + 1 > fMaxBlocks) {
		sorted_array *array = (sorted_array *)realloc(fArray,fSize + fBlockSize);
		if (array == NULL)
			return B_NO_MEMORY;
		
		if (fArray == NULL)
			array->count = 0;

		fArray = array;
		fSize += fBlockSize;
		fMaxBlocks = fSize / sizeof(off_t) - 1;
	}

	fArray->Insert(value);
	return B_OK;
}


status_t
BlockArray::Remove(off_t value)
{
	if (fArray == NULL)
		return B_ENTRY_NOT_FOUND;

	return fArray->Remove(value) ? B_OK : B_ENTRY_NOT_FOUND;
}


void 
BlockArray::MakeEmpty()
{
	fArray->count = 0;
}

