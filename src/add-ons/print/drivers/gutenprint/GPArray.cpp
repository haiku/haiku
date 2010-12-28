/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include<Debug.h>

template<typename TYPE>
GPArray<TYPE>::GPArray()
	:
	fArray(NULL),
	fSize(0)
{

}

template<typename TYPE>
GPArray<TYPE>::~GPArray()
{
	if (fArray != NULL) {
		for (int i = 0; i < fSize; i ++)
			delete fArray[i];
		delete[] fArray;
		fArray = NULL;
	}
}


template<typename TYPE>
void
GPArray<TYPE>::SetSize(int size)
{
	ASSERT(fSize == NULL);
	fArray = new PointerType[size];
	if (fArray == NULL)
		return;

	fSize = size;
	for (int i = 0; i < size; i ++) {
		fArray[i] = NULL;
	}
}


template<typename TYPE>
int
GPArray<TYPE>::Size() const
{
	return fSize;
}


template<typename TYPE>
void
GPArray<TYPE>::DecreaseSize()
{
	ASSERT(fArray != NULL);
	ASSERT(fArray[fSize-1] == NULL);
	fSize --;
}

template<typename TYPE>
TYPE**
GPArray<TYPE>::Array()
{
	return fArray;
}


template<typename TYPE>
TYPE **
GPArray<TYPE>::Array() const
{
	return fArray;
}

template<typename TYPE>
bool
GPArray<TYPE>::IsEmpty() const
{
	return fSize == 0;
}
