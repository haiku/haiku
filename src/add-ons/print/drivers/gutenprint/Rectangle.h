/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef RECTANGLE_H
#define RECTANGLE_H


#include <SupportDefs.h>


template<typename T>
class Rectangle
{
public:
	Rectangle()
	:
	left(0),
	top(0),
	right(0),
	bottom(0)
	{

	}


	Rectangle(const BRect& rect)
	:
	left(static_cast<T>(rect.left)),
	top(static_cast<T>(rect.top)),
	right(static_cast<T>(rect.right)),
	bottom(static_cast<T>(rect.bottom))
	{
	}


	Rectangle(T left, T top, T right, T bottom)
	:
	left(left),
	top(top),
	right(right),
	bottom(bottom)
	{
	}


	Rectangle<T>& operator=(const BRect& rect) {
		left = static_cast<T>(rect.left);
		top = static_cast<T>(rect.top);
		right = static_cast<T>(rect.right);
		bottom = static_cast<T>(rect.bottom);
		return *this;
	}


	T Width() const {
		return right - left;
	}


	T Height() const {
		return bottom - top;
	}


	T left;
	T top;
	T right;
	T bottom;
};


typedef Rectangle<int32> RectInt32;


#endif
