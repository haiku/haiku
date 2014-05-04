/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H

#include <OS.h>

#include <string.h>

#include <algorithm>


#define	PAGE_MASK (B_PAGE_SIZE - 1)

#define	PAGE_OFFSET(x) ((x) & (PAGE_MASK))
#define	PAGE_BASE(x) ((x) & ~(PAGE_MASK))
#define TO_PAGE_SIZE(x) ((x + (PAGE_MASK)) & ~(PAGE_MASK))


extern "C" void dprintf(const char *format, ...);


namespace utility {


template<typename T>
class vector {
public:
	inline			vector();

			void	push_back(const T& value);
			void	pop_back()	{ fSize--; }

			T&		back()	{ return fData[fSize - 1]; }

			T&		operator[](size_t idx)	{ return fData[idx]; }
			const T&	operator[](size_t idx) const	{ return fData[idx]; }

			size_t	size() const	{ return fSize; }
			bool	empty() const	{ return size() == 0; }

private:
			void	_Grow();

			size_t	fMaxSize;
			size_t	fSize;
			T*		fData;
};


template<typename T>
vector<T>::vector()
	:
	fMaxSize(0),
	fSize(0),
	fData(NULL)
{
}


template<typename T>
void
vector<T>::push_back(const T& value)
{
	if (fSize + 1 > fMaxSize)
		_Grow();
	fData[fSize++] = value;
}


template<typename T>
void
vector<T>::_Grow()
{
	size_t newSize = std::max(fMaxSize * 2, size_t(4));
	T* newBuffer = new T[newSize];
	if (fSize > 0) {
		memcpy(newBuffer, fData, fSize * sizeof(T));
		delete[] fData;
	}	
	fData = newBuffer;
	fMaxSize = newSize;
}


}	// namespace utility


#endif	// UTILITY_H
