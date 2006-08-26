/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SIZE_H
#define	_SIZE_H

#include <limits.h>

#include <Rect.h>
#include <SupportDefs.h>

enum {
	B_SIZE_UNSET		= -1,
	B_SIZE_UNLIMITED	= 1024 * 1024 * 1024,
};

class BSize {
public:
			float				width;
			float				height;
	
	inline						BSize();
	inline						BSize(const BSize& other);
	inline						BSize(float width, float height);
	inline						BSize(const BRect& rect);

	inline	float				Width() const;
	inline	float				Height() const;

	inline	void				SetWidth(float width);
	inline	void				SetHeight(float height);

	inline	int32				IntegerWidth() const;
	inline	int32				IntegerHeight() const;

	inline	bool				IsWidthSet() const;
	inline	bool				IsHeightSet() const;

	inline	bool				operator==(const BSize& other) const;
	inline	bool				operator!=(const BSize& other) const;

	inline	BSize&				operator=(const BSize& other);
};


// constructor
inline
BSize::BSize()
	: width(B_SIZE_UNSET),
	  height(B_SIZE_UNSET)
{
}

// copy constructor
inline
BSize::BSize(const BSize& other)
	: width(other.width),
	  height(other.height)
{
}

// constructor
inline
BSize::BSize(float width, float height)
	: width(width),
	  height(height)
{
}

// constructor
inline
BSize::BSize(const BRect& rect)
	: width(rect.Width()),
	  height(rect.Height())
{
}

// Width
inline float
BSize::Width() const
{
	return width;
}

// Height
inline float
BSize::Height() const
{
	return height;
}

// SetWidth
inline void
BSize::SetWidth(float width)
{
	this->width = width;
}

// SetHeight
inline void
BSize::SetHeight(float height)
{
	this->height = height;
}

// IntegerWidth
inline int32
BSize::IntegerWidth() const
{
	return (int32)width;
}

// IntegerHeight
inline int32
BSize::IntegerHeight() const
{
	return (int32)height;
}

// IsWidthSet
inline bool
BSize::IsWidthSet() const
{
	return width != B_SIZE_UNSET;
}

// IsHeightSet
inline bool
BSize::IsHeightSet() const
{
	return height != B_SIZE_UNSET;
}

// ==
inline bool
BSize::operator==(const BSize& other) const
{
	return (width == other.width && height == other.height);
}

// !=
inline bool
BSize::operator!=(const BSize& other) const
{
	return !(*this == other);
}

// =
inline BSize&
BSize::operator=(const BSize& other)
{
	width = other.width;
	height = other.height;
	return *this;
}

#endif	// _SIZE_H
