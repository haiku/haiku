/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "IconTarPtr.h"


IconTarPtr::IconTarPtr(const BString& name)
	:
	fName(name),
	fOffsetsMask(0)
{
}


IconTarPtr::~IconTarPtr()
{
}


const BString&
IconTarPtr::Name() const
{
	return fName;
}


off_t
IconTarPtr::Offset(BitmapSize size) const
{
	return fOffsets[size];
}


bool
IconTarPtr::HasOffset(BitmapSize size) const
{
	return 0 != (fOffsetsMask & (1 << size));
}


void
IconTarPtr::SetOffset(BitmapSize size, off_t value)
{
	fOffsets[size] = value;
	fOffsetsMask |= (1 << size);
}
