//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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

bool tables_initialized=false;
float sintable[360], costable[360], tantable[360];

/*!
	\brief Constructor
	\param angle Value in degrees
*/
Angle::Angle(float angle)
{
	fAngleValue=angle;
	if(tables_initialized==false)
	{
		InitTrigTables();
		tables_initialized=true;
	}
}

//! Constructor
Angle::Angle(void)
{
	fAngleValue=0;
	if(tables_initialized==false)
	{
		InitTrigTables();
		tables_initialized=true;
	}
}

//! Empty destructor
Angle::~Angle(void)
{
}

//! Constrains angle to 0 <= angle <= 360
void Angle::Normalize(void)
{
	// if the value of the angle is >=360 or <0, make it so that it is 
	// within those bounds
	
	if(fAngleValue>359)
	{
		while(fAngleValue>359)
			fAngleValue-=360;
		return;
	}
	if(fAngleValue<0)
	{
		while(fAngleValue<0)
			fAngleValue+=360;
	}
}

/*!
	\brief Obtains the sine of the angle
	\return The sine of the angle
*/
float Angle::Sine(void)
{
	return sintable[(int)fAngleValue];
}

/*!
	\brief Calculates an angle given a float value
	\param value Number between 0 and 1 inclusive
	\return The angle obtained or 0 if value passed was invalid
*/
Angle Angle::InvSine(float value)
{
	// Returns the inverse sine of a value in the range 0 <= value <= 1 via 
	//	reverse-lookup any value out of range causes the function to return 0
	uint16 i=90;
	
	// Filter out bad values
	if(value<0)
		value*=-1;

	if(value > 1)
		return Angle(0);
		
	while(value < sintable[i])
		i--;

	// current sintable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if( (value - sintable[i]) > (sintable[i+1] - value) )
		return (i+1);
	
	return Angle(i);		// value is closer to previous
}


/*!
	\brief Obtains the cosine of the angle
	\return The cosine of the angle
*/
float Angle::Cosine(void)
{
	return costable[(int)fAngleValue];
}

/*!
	\brief Calculates an angle given a float value
	\param value Number between 0 and 1 inclusive
	\return The angle obtained or 0 if value passed was invalid
*/
Angle Angle::InvCosine(float value)
{
	// Returns the inverse cosine of a value in the range 0 <= value <= 1 via 
	//	reverse-lookup any value out of range causes the function to return 0
	uint16 i=90;

	// Filter out bad values
	if(value<0)
		value*=-1;

	if(value > 1)
		return 0;
		
	while(value > costable[i])
		i--;

	// current costable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if( (value - costable[i]) < (costable[i+1] - value) )
		return (i+1);

	return Angle(i);		// value is closer to previous
}

/*!
	\brief Obtains the tangent of the angle
	\return The tangent of the angle
*/
float Angle::Tangent(int *status)
{
	if(fAngleValue==90 || fAngleValue==270)
	{
		if(status)
			*status=0;
		return 0.0;
	}
	
	return tantable[(int)fAngleValue];
}

/*!
	\brief Returns the inverse tangent of a value given
	\param value Number between 0 and 1 inclusive
	\return The angle found or 0 if value was invalid
*/
Angle Angle::InvTangent(float value)
{
	uint16 i=90;

	// Filter out bad values
	if(value<0)
		value*=-1;

	if(value > 1)
		return Angle(0);
		
	while(value > tantable[i])
		i--;

	if( (value - tantable[i]) < (tantable[i+1] - value) )
		return (i+1);

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
uint8 Angle::Quadrant(void)
{
	// We can get away with not doing extra value checks because of the order in
	// which the checks are done.
	if(fAngleValue < 90)
		return 1;

	if(fAngleValue < 180)
		return 2;

	if(fAngleValue < 270)
		return 3;
	
	return 4;
}

/*!
	\brief Obtains the angle constrained to between 0 and 180 inclusive
	\return The constrained value
*/
Angle Angle::Constrain180(void)
{
	// Constrains angle to 0 <= angle < 180
	float val=fAngleValue;

	if(fAngleValue<180)
		return Angle(fAngleValue);

	while(!(val<180))
		val-=90;
	return Angle(val);
}

/*!
	\brief Obtains the angle constrained to between 0 and 90 inclusive
	\return The constrained value
*/
Angle Angle::Constrain90(void)
{
	// Constrains angle to 0 <= angle < 90
	float val=fAngleValue;

	if(fAngleValue<90)
		return Angle(fAngleValue);

	while(!(val<90))
		val-=90;
	return Angle(val);
}

/*!
	\brief Sets the angle's value and normalizes the value
	\param angle Value in degrees
*/
void Angle::SetValue(float angle)
{
	fAngleValue=angle;
	Normalize();
}

/*!
	\brief Returns the value of the angle
	\return The angle's value in degrees
*/
float Angle::Value(void) const
{
	return fAngleValue;
}

//! Initializes the global trig tables
void Angle::InitTrigTables(void)
{
	int8 i;
	double sval,cval,tval,current_radian;
	
	for(i=0;i<90; i++)
	{
		current_radian=(i * ANGLE_PI)/180.0;

		// Get these so that we can do some superfast assignments
		sval=(float)sin(current_radian);
		cval=(float)cos(current_radian);
		
		// Do 4 assignments, taking advantage of sin/cos symmetry
		sintable[i]=sval;
		sintable[i+90]=cval;
		sintable[i+180]=sval * -1;
		sintable[i+270]=cval * -1;
		
		costable[i]=cval;
		costable[i+90]=sval * -1;
		costable[i+180]=cval * -1;
		costable[i+270]=sval;

		tval=sval/cval;

		tantable[i]=tval;
		tantable[i+90]=tval;
		tantable[i+180]=tval;
		tantable[i+270]=tval;
	}
}

/*!
	\brief Overloaded assignment operator
	\param from Angle to copy the value from
	\return The angle's new value
*/
Angle & Angle::operator=(const Angle &from)
{
	fAngleValue=from.fAngleValue;
	return *this;
}

/*!
	\brief Overloaded assignment operator
	\param from New value of the angle
	\return The angle's new value
*/
Angle & Angle::operator=(const float &from)
{
	fAngleValue=from;
	return *this;
}

/*!
	\brief Overloaded assignment operator
	\param from New value of the angle
	\return The angle's new value
*/
Angle & Angle::operator=(const long &from)
{
	fAngleValue=(float)from;
	return *this;
}

/*!
	\brief Overloaded assignment operator
	\param from New value of the angle
	\return The angle's new value
*/
Angle & Angle::operator=(const int &from)
{
	fAngleValue=(float)from;
	return *this;
}

/*!
	\brief Overloaded equivalence operator
	\param The angle to compare to
	\return True if equal, false if not
*/
bool Angle::operator==(const Angle &from)
{
	return (fAngleValue==from.fAngleValue)?true:false;
}

/*!
	\brief Overloaded inequality operator
	\param The angle to compare to
	\return True if not equal, false if not
*/
bool Angle::operator!=(const Angle &from)
{
	return (fAngleValue!=from.fAngleValue)?true:false;
}

/*!
	\brief Overloaded greater than operator
	\param The angle to compare to
	\return True if greater, false if not
*/
bool Angle::operator>(const Angle &from)
{
	return (fAngleValue>from.fAngleValue)?true:false;
}

/*!
	\brief Overloaded less than operator
	\param The angle to compare to
	\return True if less than, false if not
*/
bool Angle::operator<(const Angle &from)
{
	return (fAngleValue<from.fAngleValue)?true:false;
}

/*!
	\brief Overloaded greater than or equal to operator
	\param The angle to compare to
	\return True if greater than or equal to, false if not
*/
bool Angle::operator>=(const Angle &from)
{
	return (fAngleValue>=from.fAngleValue)?true:false;
}

/*!
	\brief Overloaded less than or equal to operator
	\param The angle to compare to
	\return True if less than or equal to, false if not
*/
bool Angle::operator<=(const Angle &from)
{
	return (fAngleValue<=from.fAngleValue)?true:false;
}
