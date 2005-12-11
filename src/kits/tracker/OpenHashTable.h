/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// Hash table with open addresssing

#ifndef __OPEN_HASH_TABLE__
#define __OPEN_HASH_TABLE__

#include <new>
#include <stdlib.h>

namespace BPrivate {

template <class Element>
class ElementVector {
	// element vector for OpenHashTable needs to implement this
	// interface
	public:
		Element &At(int32 index);
		Element *Add();
		int32 IndexOf(const Element &) const;
		void Remove(int32 index);
};

class OpenHashElement {
	public:
		uint32 Hash() const;
		bool operator==(const OpenHashElement &) const;
		void Adopt(OpenHashElement &);
			// low overhead copy, original element is in undefined state
			// after call (calls Adopt on BString members, etc.)
		int32 fNext;
};

const uint32 kPrimes [] = {
	509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139,
	524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859,
	134217689, 268435399, 536870909, 1073741789, 2147483647, 0
};

template <class Element, class ElementVec = ElementVector<Element> >
class OpenHashTable {
	public:
		OpenHashTable(int32 minSize, ElementVec *elementVector = 0);
			// it is up to the subclass of OpenHashTable to supply
			// elementVector
		~OpenHashTable();

		void SetElementVector(ElementVec *elementVector);

		Element *FindFirst(uint32 elementHash) const;
		Element &Add(uint32 elementHash);

		void Remove(Element *);

		// when calling Add, any outstanding element pointer may become
		// invalid; to deal with this, get the element index and restore
		// it after the add
		int32 ElementIndex(const Element *) const;
		Element *ElementAt(int32 index) const;

		int32 VectorSize() const;

	protected:
		static int32 OptimalSize(int32 minSize);

		int32 fArraySize;
		int32 *fHashArray;
		ElementVec *fElementVector;
};


template <class Element>
class OpenHashElementArray : public ElementVector<Element> {
	// this is a straightforward implementation of an element vector
	// deleting is handled by linking deleted elements into a free list
	// the vector never shrinks
	public:
		OpenHashElementArray(int32 initialSize);
		~OpenHashElementArray();

		Element &At(int32 index);
		const Element &At(int32 index) const;
		int32 Add(const Element &);
		int32 Add();
		void Remove(int32 index);
		int32 IndexOf(const Element &) const;
		int32 Size() const;

	private:
		Element *fData;
		int32 fSize;
		int32 fNextFree;
		int32 fNextDeleted;
};


//--- inline implementation --------------------------------


template<class Element, class ElementVec>
OpenHashTable<Element, ElementVec>::OpenHashTable(int32 minSize,ElementVec *elementVector)
	:
	fArraySize(OptimalSize(minSize)),
	fElementVector(elementVector)
{
	fHashArray = new int32[fArraySize];
	for (int32 index = 0; index < fArraySize; index++)
		fHashArray[index] = -1;
}


template<class Element, class ElementVec>
OpenHashTable<Element, ElementVec>::~OpenHashTable()
{
	delete fHashArray;
}


template<class Element, class ElementVec>
int32 
OpenHashTable<Element, ElementVec>::OptimalSize(int32 minSize)
{
	for (int32 index = 0; ; index++)
		if (!kPrimes[index] || kPrimes[index] >= (uint32)minSize)
			return (int32)kPrimes[index];

	return 0;
}


template<class Element, class ElementVec>
Element *
OpenHashTable<Element, ElementVec>::FindFirst(uint32 hash) const
{
	ASSERT(fElementVector);
	hash %= fArraySize;
	if (fHashArray[hash] < 0)
		return 0;
	
	return &fElementVector->At(fHashArray[hash]);
}


template<class Element, class ElementVec>
int32 
OpenHashTable<Element, ElementVec>::ElementIndex(const Element *element) const
{
	return fElementVector->IndexOf(*element);
}


template<class Element, class ElementVec>
Element *
OpenHashTable<Element, ElementVec>::ElementAt(int32 index) const
{
	return &fElementVector->At(index);
}


template<class Element, class ElementVec>
int32 
OpenHashTable<Element, ElementVec>::VectorSize() const
{
	 return fElementVector->Size();
}


template<class Element, class ElementVec>
Element &
OpenHashTable<Element, ElementVec>::Add(uint32 hash)
{
	ASSERT(fElementVector);
	hash %= fArraySize;
	Element &result = *fElementVector->Add();
	result.fNext = fHashArray[hash];
	fHashArray[hash] = fElementVector->IndexOf(result);
	return result;
}


template<class Element, class ElementVec>
void 
OpenHashTable<Element, ElementVec>::Remove(Element *element)
{
	uint32 hash = element->Hash() % fArraySize;
	int32 next = fHashArray[hash];
	ASSERT(next >= 0);

	if (&fElementVector->At(next) == element) {
		fHashArray[hash] = element->fNext;
		fElementVector->Remove(next);
		return;
	}

	for (int32 index = next; index >= 0; ) {
		// look for an existing match in table
		int32 next = fElementVector->At(index).fNext;
		if (next < 0) {
			TRESPASS();
			return;
		}

		if (&fElementVector->At(next) == element) {
			fElementVector->At(index).fNext = element->fNext;
			fElementVector->Remove(next);
			return;
		}
		index = next;
	}
}


template<class Element, class ElementVec>
void 
OpenHashTable<Element, ElementVec>::SetElementVector(ElementVec *elementVector)
{
	fElementVector = elementVector;
}


template<class Element>
OpenHashElementArray<Element>::OpenHashElementArray(int32 initialSize)
	:
	fSize(initialSize),
	fNextFree(0),
	fNextDeleted(-1)
{
	fData = (Element *)calloc((size_t)initialSize , sizeof(Element));
	if (!fData)
		throw std::bad_alloc();
}


template<class Element>
OpenHashElementArray<Element>::~OpenHashElementArray()
{
	free(fData);
}


template<class Element>
Element &
OpenHashElementArray<Element>::At(int32 index)
{
	ASSERT(index < fSize);
	return fData[index];
}


template<class Element>
const Element &
OpenHashElementArray<Element>::At(int32 index) const
{
	ASSERT(index < fSize);
	return fData[index];
}


template<class Element>
int32 
OpenHashElementArray<Element>::IndexOf(const Element &element) const
{
	int32 result = &element - fData;
	if (result < 0 || result > fSize)
		return -1;
	
	return result;
}


template<class Element>
int32 
OpenHashElementArray<Element>::Size() const
{
	return fSize;
}


template<class Element>
int32 
OpenHashElementArray<Element>::Add(const Element &newElement)
{
	int32 index = Add();
	At(index).Adopt(newElement);
	return index;
}


#if DEBUG
const int32 kGrowChunk = 10;
#else
const int32 kGrowChunk = 1024;
#endif


template<class Element>
int32 
OpenHashElementArray<Element>::Add()
{
	int32 index = fNextFree;
	if (fNextDeleted >= 0) {
		index = fNextDeleted;
		fNextDeleted = At(index).fNext;		
	} else if (fNextFree >= fSize - 1) {
		int32 newSize = fSize + kGrowChunk;
		Element *newData = (Element *)calloc((size_t)newSize , sizeof(Element));
		if (!newData)
			throw std::bad_alloc();
		memcpy(newData, fData, fSize * sizeof(Element));
		free(fData);
		fData = newData;
		fSize = newSize;
		index = fNextFree;
		fNextFree++;
	} else
		fNextFree++;

	new (&At(index)) Element;
		// call placement new to initialize the element properly
	ASSERT(At(index).fNext == -1);

	return index;		
}


template<class Element>
void 
OpenHashElementArray<Element>::Remove(int32 index)
{
	// delete by chaining empty elements in a single linked
	// list, reusing the next field
	ASSERT(index < fSize);
	At(index).~Element();
		// call the destructor explicitly to destroy the element
		// properly
	At(index).fNext = fNextDeleted;
	fNextDeleted = index;
}

} // namespace BPrivate

using namespace BPrivate;

#endif
