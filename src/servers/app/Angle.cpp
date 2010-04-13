//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Angle.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Angle class for speeding up trig functions
//
//------------------------------------------------------------------------------
#include "Angle.h"
#include <math.h>

#ifndef ANGLE_PI
	#define ANGLE_PI 3.14159265358979323846
#endif

static bool sTablesInitialized = false;
static float sSinTable[360];
static float sCosTable[360];
static float sTanTable[360];

/*!
	\brief Constructor
	\param angle Value in degrees
*/
Angle::Angle(float angle)
	: fAngleValue(angle)
{
	_InitTrigTables();
}

//! Constructor
Angle::Angle()
	: fAngleValue(0)
{
	_InitTrigTables();
}

//! Empty destructor
Angle::~Angle()
{
}

//! Constrains angle to 0 <= angle <= 360
void
Angle::Normalize()
{
	// if the value of the angle is >=360 or <0, make it so that it is
	// within those bounds
    fAngleValue = fmodf(fAngleValue, 360);
    if (fAngleValue < 0)
        fAngleValue += 360;
}

/*!
	\brief Obtains the sine of the angle
	\return The sine of the angle
*/
float
Angle::Sine()
{
	return sSinTable[(int)fAngleValue];
}

/*!
	\brief Calculates an angle given a float value
	\param value Number between 0 and 1 inclusive
	\return The angle obtained or 0 if value passed was invalid
*/
Angle
Angle::InvSine(float value)
{
	// Returns the inverse sine of a value in the range 0 <= value <= 1 via
	//	reverse-lookup any value out of range causes the function to return 0

	// Filter out bad values
	value = fabs(value);

	if (value > 1)
		return Angle(0);

	uint16 i = 90;
	while (value < sSinTable[i])
		i--;

	// current sSinTable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if ((value - sSinTable[i]) > (sSinTable[i + 1] - value))
		return Angle(i + 1);

	return Angle(i);		// value is closer to previous
}


/*!
	\brief Obtains the cosine of the angle
	\return The cosine of the angle
*/
float
Angle::Cosine(void)
{
	return sCosTable[(int)fAngleValue];
}

/*!
	\brief Calculates an angle given a float value
	\param value Number between 0 and 1 inclusive
	\return The angle obtained or 0 if value passed was invalid
*/
Angle
Angle::InvCosine(float value)
{
	// Returns the inverse cosine of a value in the range 0 <= value <= 1 via
	//	reverse-lookup any value out of range causes the function to return 0

	// Filter out bad values
	value = fabs(value);

	if (value > 1)
		return 0;

	uint16 i = 90;
	while (value > sCosTable[i])
		i--;

	// current sCosTable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if ((value - sCosTable[i]) < (sCosTable[i + 1] - value))
		return Angle(i + 1);

	return Angle(i);		// value is closer to previous
}

/*!
	\brief Obtains the tangent of the angle
	\return The tangent of the angle
*/
float
Angle::Tangent(int *status)
{
	if (fAngleValue == 90 || fAngleValue == 270) {
		if (status)
			*status = 0;
		return 0.0;
	}

	return sTanTable[(int)fAngleValue];
}

/*!
	\brief Returns the inverse tangent of a value given
	\param value Number between 0 and 1 inclusive
	\return The angle found or 0 if value was invalid
*/
Angle
Angle::InvTangent(float value)
{
	// Filter out bad values
	value = fabs(value);

	if (value > 1)
		return Angle(0);

	uint16 i = 90;
	while (value > sTanTable[i])
		i--;

	if ((value - sTanTable[i]) < (sTanTable[i+1] - value))
		return Angle(i+1);

	return Angle(i);		// value is closer to previous
}

/*!
	\brief Returns a value based on what quadrant the angle is in
	\return
	- \c 1: 0 <= angle <90
	- \c 2: 90 <= angle < 180
	- \c 3: 180 <= angle < 270
	- \c 4: 270 <= angle < 360
*/
uint8
Angle::Quadrant()
{
	// We can get away with not doing extra value checks because of the order in
	// which the checks are done.
	if (fAngleValue < 90)
		return 1;

	if (fAngleValue < 180)
		return 2;

	if (fAngleValue < 270)
		return 3;

	return 4;
}

/*!
	\brief Obtains the angle constrained to between 0 and 180 inclusive
	\return The constrained value
*/
Angle
Angle::Constrain180()
{
	// Constrains angle to 0 <= angle < 180
	if (fAngleValue < 180)
		return Angle(fAngleValue);

	float value = fmodf(fAngleValue, 180);;
    if (value < 0)
        value += 180;
	return Angle(value);
}

/*!
	\brief Obtains the angle constrained to between 0 and 90 inclusive
	\return The constrained value
*/
Angle
Angle::Constrain90()
{
	// Constrains angle to 0 <= angle < 90
	if (fAngleValue < 90)
		return Angle(fAngleValue);

	float value = fmodf(fAngleValue, 90);;
    if (value < 0)
        value += 90;
	return Angle(value);
}

/*!
	\brief Sets the angle's value and normalizes the value
	\param angle Value in degrees
*/
void
Angle::SetValue(float angle)
{
	fAngleValue = angle;
	Normalize();
}


float
Angle::Value() const
{
	return fAngleValue;
}

//! Initializes the global trig tables
void
Angle::_InitTrigTables()
{
	if (sTablesInitialized)
		return;
	sTablesInitialized = true;

	for(int32 i = 0; i < 90; i++) {
		double currentRadian = (i * ANGLE_PI) / 180.0;

		// Get these so that we can do some superfast assignments
		double sinValue = sin(currentRadian);
		double cosValue = cos(currentRadian);

		// Do 4 assignments, taking advantage of sin/cos symmetry
		sSinTable[i] = sinValue;
		sSinTable[i + 90] = cosValue;
		sSinTable[i + 180] = sinValue * -1;
		sSinTable[i + 270] = cosValue * -1;

		sCosTable[i] = cosValue;
		sCosTable[i + 90] = sinValue * -1;
		sCosTable[i + 180] = cosValue * -1;
		sCosTable[i + 270] = sinValue;

		double tanValue = sinValue / cosValue;

		sTanTable[i] = tanValue;
		sTanTable[i + 90] = tanValue;
		sTanTable[i + 180] = tanValue;
		sTanTable[i + 270] = tanValue;
	}
}


Angle&
Angle::operator=(const Angle &from)
{
	fAngleValue = from.fAngleValue;
	return *this;
}


Angle&
Angle::operator=(const float &from)
{
	fAngleValue = from;
	return *this;
}


Angle&
Angle::operator=(const long &from)
{
	fAngleValue = (float)from;
	return *this;
}


Angle&
Angle::operator=(const int &from)
{
	fAngleValue = (float)from;
	return *this;
}


bool
Angle::operator==(const Angle &from)
{
	return (fAngleValue == from.fAngleValue);
}


bool
Angle::operator!=(const Angle &from)
{
	return (fAngleValue != from.fAngleValue);
}


bool
Angle::operator>(const Angle &from)
{
	return (fAngleValue > from.fAngleValue);
}


bool
Angle::operator<(const Angle &from)
{
	return (fAngleValue < from.fAngleValue);
}


bool
Angle::operator>=(const Angle &from)
{
	return (fAngleValue >= from.fAngleValue);
}


bool
Angle::operator<=(const Angle &from)
{
	return (fAngleValue <= from.fAngleValue);
}
