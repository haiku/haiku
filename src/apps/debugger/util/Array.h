/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARRAY_H
#define ARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


template<typename Element>
class Array {
public:
	inline						Array();
								Array(const Array<Element>& other);
								~Array();

	inline	int					Size() const		{ return fSize; }
	inline	int					Count() const		{ return fSize; }
	inline	bool				IsEmpty() const		{ return fSize == 0; }
	inline	Element*			Elements() const	{ return fElements; }

	inline	bool				Add(const Element& element);
	inline	bool				AddUninitialized(int elementCount);
	inline	bool				Insert(const Element& element, int index);
	inline	bool				Remove(int index);

			void				Clear();
	inline	void				MakeEmpty();

	inline	Element&			ElementAt(int index);
	inline	const Element&		ElementAt(int index) const;

	inline	Element&			operator[](int index);
	inline	const Element&		operator[](int index) const;

			Array<Element>&		operator=(const Array<Element>& other);

private:
	static	const int			kMinCapacity = 8;

			bool				_Resize(int index, int delta);

private:
			Element*			fElements;
			int					fSize;
			int					fCapacity;
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
	if (fSize == fCapacity) {
		if (!_Resize(fSize, 1))
			return false;
	}

	fElements[fSize] = element;
	fSize++;
	return true;
}


template<typename Element>
bool
Array<Element>::AddUninitialized(int elementCount)
{
	if (elementCount < 0)
		return false;

	if (fSize + elementCount > fCapacity) {
		if (!_Resize(fSize, elementCount))
			return false;
	}

	fSize += elementCount;
	return true;
}


template<typename Element>
bool
Array<Element>::Insert(const Element& element, int index)
{
	if (index < 0 || index > fSize)
		index = fSize;

	if (fSize == fCapacity) {
		if (!_Resize(index, 1))
			return false;
	} else if (index < fSize) {
		memmove(fElements + index + 1, fElements + index,
			sizeof(Element) * (fSize - index));
	}

	fElements[index] = element;
	fSize++;
	return true;
}


template<typename Element>
bool
Array<Element>::Remove(int index)
{
	if (index < 0 || index >= fSize) {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "Array::Remove(): index: %d, size: %d",
			index, fSize);
		return false;
	}

	if (fSize <= fCapacity / 2 && fCapacity > kMinCapacity) {
		_Resize(index, -1);
	} else if (index < fSize) {
		memmove(fElements + index, fElements + index + 1,
			sizeof(Element) * (fSize - index - 1));
	}

	fSize--;
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
Array<Element>::ElementAt(int index)
{
	return fElements[index];
}


template<typename Element>
const Element&
Array<Element>::ElementAt(int index) const
{
	return fElements[index];
}


template<typename Element>
Element&
Array<Element>::operator[](int index)
{
	return fElements[index];
}


template<typename Element>
const Element&
Array<Element>::operator[](int index) const
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
Array<Element>::_Resize(int index, int delta)
{
	// determine new capacity
	int newSize = fSize + delta;
	int newCapacity = kMinCapacity;
	while (newCapacity < newSize)
		newCapacity *= 2;

	if (newCapacity == fCapacity)
		return true;

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


#endif	// ARRAY_H
