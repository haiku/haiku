/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARRAY_H
#define _ARRAY_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

#if DEBUG
#	include <OS.h>
#endif


namespace BPrivate {


template<typename Element>
class Array {
public:
	inline						Array();
								Array(const Array<Element>& other);
	inline						~Array();

	inline	int32				Size() const		{ return fSize; }
	inline	int32				Count() const		{ return fSize; }
	inline	bool				IsEmpty() const		{ return fSize == 0; }
	inline	Element*			Elements() const	{ return fElements; }

	inline	bool				Add(const Element& element);
	inline	bool				AddUninitialized(int32 elementCount);
	inline	bool				Insert(const Element& element, int32 index);
	inline	bool				InsertUninitialized(int32 index, int32 count);
	inline	bool				Remove(int32 index, int32 count = 1);

			void				Clear();
	inline	void				MakeEmpty();

	inline	Element&			ElementAt(int32 index);
	inline	const Element&		ElementAt(int32 index) const;

	inline	Element&			operator[](int32 index);
	inline	const Element&		operator[](int32 index) const;

			Array<Element>&		operator=(const Array<Element>& other);

private:
	static	const int32			kMinCapacity = 8;

			bool				_Resize(int32 index, int32 delta);

private:
			Element*			fElements;
			int32				fSize;
			int32				fCapacity;
};


template<typename Element>
Array<Element>::Array()
	:
	fElements(NULL),
	fSize(0),
	fCapacity(0)
{
}


template<typename Element>
Array<Element>::Array(const Array<Element>& other)
	:
	fElements(NULL),
	fSize(0),
	fCapacity(0)
{
	*this = other;
}


template<typename Element>
Array<Element>::~Array()
{
	free(fElements);
}


template<typename Element>
bool
Array<Element>::Add(const Element& element)
{
	if (!_Resize(fSize, 1))
		return false;

	fElements[fSize] = element;
	fSize++;
	return true;
}


template<typename Element>
inline bool
Array<Element>::AddUninitialized(int32 elementCount)
{
	return InsertUninitialized(fSize, elementCount);
}


template<typename Element>
bool
Array<Element>::Insert(const Element& element, int32 index)
{
	if (index < 0 || index > fSize)
		index = fSize;

	if (!_Resize(index, 1))
		return false;

	fElements[index] = element;
	fSize++;
	return true;
}


template<typename Element>
bool
Array<Element>::InsertUninitialized(int32 index, int32 count)
{
	if (index < 0 || index > fSize || count < 0)
		return false;
	if (count == 0)
		return true;

	if (!_Resize(index, count))
		return false;

	fSize += count;
	return true;
}


template<typename Element>
bool
Array<Element>::Remove(int32 index, int32 count)
{
	if (index < 0 || count < 0 || index + count > fSize) {
#if DEBUG
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Array::Remove(): index: %d, "
			"count: %d, size: %d", index, count, fSize);
		debugger(buffer);
#endif
		return false;
	}
	if (count == 0)
		return true;

	if (index + count < fSize) {
		memmove(fElements + index, fElements + index + count,
			sizeof(Element) * (fSize - index - count));
	}

	_Resize(index, -count);

	fSize -= count;
	return true;
}


template<typename Element>
void
Array<Element>::Clear()
{
	if (fSize == 0)
		return;

	free(fElements);

	fElements = NULL;
	fSize = 0;
	fCapacity = 0;
}


template<typename Element>
void
Array<Element>::MakeEmpty()
{
	Clear();
}


template<typename Element>
Element&
Array<Element>::ElementAt(int32 index)
{
	return fElements[index];
}


template<typename Element>
const Element&
Array<Element>::ElementAt(int32 index) const
{
	return fElements[index];
}


template<typename Element>
Element&
Array<Element>::operator[](int32 index)
{
	return fElements[index];
}


template<typename Element>
const Element&
Array<Element>::operator[](int32 index) const
{
	return fElements[index];
}


template<typename Element>
Array<Element>&
Array<Element>::operator=(const Array<Element>& other)
{
	Clear();

	if (other.fSize > 0 && _Resize(0, other.fSize)) {
		fSize = other.fSize;
		memcpy(fElements, other.fElements, fSize * sizeof(Element));
	}

	return *this;
}


template<typename Element>
bool
Array<Element>::_Resize(int32 index, int32 delta)
{
	// determine new capacity
	int32 newSize = fSize + delta;
	int32 newCapacity = kMinCapacity;
	while (newCapacity < newSize)
		newCapacity *= 2;

	if (newCapacity == fCapacity) {
		// the capacity doesn't change -- still make room for/remove elements
		if (index < fSize) {
			if (delta > 0) {
				// leave a gap of delta elements
				memmove(fElements + index + delta, fElements + index,
					(fSize - index) * sizeof(Element));
			} else if (index < fSize + delta) {
				// drop -delta elements
				memcpy(fElements + index, fElements + index - delta,
					(fSize - index + delta) * sizeof(Element));
			}
		}

		return true;
	}

	// allocate new array
	Element* elements = (Element*)malloc(newCapacity * sizeof(Element));
	if (elements == NULL)
		return false;

	if (index > 0)
		memcpy(elements, fElements, index * sizeof(Element));
	if (index < fSize) {
		if (delta > 0) {
			// leave a gap of delta elements
			memcpy(elements + index + delta, fElements + index,
				(fSize - index) * sizeof(Element));
		} else if (index < fSize + delta) {
			// drop -delta elements
			memcpy(elements + index, fElements + index - delta,
				(fSize - index + delta) * sizeof(Element));
		}
	}

	free(fElements);
	fElements = elements;
	fCapacity = newCapacity;
	return true;
}


}	// namespace BPrivate


using BPrivate::Array;


#endif	// _ARRAY_H
