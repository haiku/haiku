/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "OneElementLayouter.h"

#include <Size.h>


class OneElementLayouter::MyLayoutInfo : public LayoutInfo {
public:
	float	fSize;

	MyLayoutInfo()
		: fSize(0)
	{
	}
	
	virtual float ElementLocation(int32 element)
	{
		return 0;
	}

	virtual float ElementSize(int32 element)
	{
		return fSize;
	}
};


// constructor
OneElementLayouter::OneElementLayouter()
	: fMin(-1),
	  fMax(B_SIZE_UNLIMITED),
	  fPreferred(-1)
{
}

// destructor
OneElementLayouter::~OneElementLayouter()
{
}

// AddConstraints
void
OneElementLayouter::AddConstraints(int32 element, int32 length,
	float min, float max, float preferred)
{
	fMin = max_c(fMin, min);
	fMax = min_c(fMax, max);
	fMax = max_c(fMax, fMin);
	fPreferred = max_c(fPreferred, preferred);
	fPreferred = max_c(fPreferred, fMin);
	fPreferred = min_c(fPreferred, fMax);
}

// SetWeight
void
OneElementLayouter::SetWeight(int32 element, float weight)
{
	// not needed
}

// MinSize
float
OneElementLayouter::MinSize()
{
	return fMin;
}

// MaxSize
float
OneElementLayouter::MaxSize()
{
	return fMax;
}

// PreferredSize
float
OneElementLayouter::PreferredSize()
{
	return fPreferred;
}

// CreateLayoutInfo
LayoutInfo*
OneElementLayouter::CreateLayoutInfo()
{
	return new MyLayoutInfo;
}

// Layout
void
OneElementLayouter::Layout(LayoutInfo* layoutInfo, float size)
{
	((MyLayoutInfo*)layoutInfo)->fSize = max_c(size, fMin);
}

// CloneLayouter
Layouter*
OneElementLayouter::CloneLayouter()
{
	OneElementLayouter* layouter = new OneElementLayouter;
	layouter->fMin = fMin;
	layouter->fMax = fMax;
	layouter->fPreferred = fPreferred;

	return layouter;
}
