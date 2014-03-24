/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BulletData.h"

#include <new>


BulletData::BulletData()
	:
	fString(""),
	fSpacing(0.0f)
{
}


BulletData::BulletData(const BString& string, float spacing)
	:
	fString(string),
	fSpacing(spacing)
{
}


BulletData::BulletData(const BulletData& other)
	:
	fString(other.fString),
	fSpacing(other.fSpacing)
{
}


bool
BulletData::operator==(const BulletData& other) const
{
	if (this == &other)
		return true;

	return fString == other.fString
		&& fSpacing == other.fSpacing;
}


bool
BulletData::operator!=(const BulletData& other) const
{
	return !(*this == other);
}


BulletDataRef
BulletData::SetString(const BString& string)
{
	if (fString == string)
		return BulletDataRef(this);

	BulletData* ret = new(std::nothrow) BulletData(*this);
	if (ret == NULL)
		return BulletDataRef(this);

	ret->fString = string;
	return BulletDataRef(ret, true);
}


BulletDataRef
BulletData::SetSpacing(float spacing)
{
	if (fSpacing == spacing)
		return BulletDataRef(this);

	BulletData* ret = new(std::nothrow) BulletData(*this);
	if (ret == NULL)
		return BulletDataRef(this);

	ret->fSpacing = spacing;
	return BulletDataRef(ret, true);
}


// #pragma mark - private


BulletData&
BulletData::operator=(const BulletData& other)
{
	return *this;
}
