/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Bullet.h"


static BulletData sEmptyBullet;


Bullet::Bullet()
	:
	fBulletData(&sEmptyBullet)
{
}


Bullet::Bullet(const BString& string, float spacing)
	:
	fBulletData(new BulletData(string, spacing), true)
{
}


Bullet::Bullet(const Bullet& other)
	:
	fBulletData(other.fBulletData)
{
}


Bullet&
Bullet::operator=(const Bullet& other)
{
	if (this == &other)
		return *this;

	fBulletData = other.fBulletData;
	return *this;
}


bool
Bullet::operator==(const Bullet& other) const
{
	if (this == &other)
		return true;

	if (fBulletData == other.fBulletData)
		return true;

	if (fBulletData.Get() != NULL && other.fBulletData.Get() != NULL)
		return *fBulletData.Get() == *other.fBulletData.Get();

	return false;
}


bool
Bullet::operator!=(const Bullet& other) const
{
	return !(*this == other);
}


bool
Bullet::SetString(const BString& string)
{
	BulletDataRef data = fBulletData->SetString(string);
	if (data == fBulletData)
		return data->String() == string;

	fBulletData = data;
	return true;
}


const BString&
Bullet::String() const
{
	return fBulletData->String();
}


bool
Bullet::SetSpacing(float spacing)
{
	BulletDataRef data = fBulletData->SetSpacing(spacing);
	if (data == fBulletData)
		return data->Spacing() == spacing;

	fBulletData = data;
	return true;
}


float
Bullet::Spacing() const
{
	return fBulletData->Spacing();
}
