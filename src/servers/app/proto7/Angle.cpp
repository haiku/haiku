#include "Angle.h"

#ifndef ANGLE_PI
	#define ANGLE_PI 3.14159265358979323846
#endif

bool tables_initialized=false;
float sintable[360], costable[360], tantable[360];

Angle::Angle(float angle)
{
	angle_value=angle;
	if(tables_initialized==false)
	{
		InitTrigTables();
		tables_initialized=true;
	}
}

Angle::Angle(void)
{
	angle_value=0;
	if(tables_initialized==false)
	{
		InitTrigTables();
		tables_initialized=true;
	}
}

Angle::~Angle(void)
{
}

void Angle::Normalize(void)
{
	// if the value of the angle is >=360 or <0, make it so that it is 
	// within those bounds
	
	if(angle_value>359)
	{
		while(angle_value>359)
			angle_value-=360;
		return;
	}
	if(angle_value<0)
	{
		while(angle_value<0)
			angle_value+=360;
	}
}

float Angle::Sine(void)
{
	return sintable[(int)angle_value];
}

Angle Angle::InvSine(float value)
{
	// Returns the inverse sine of a value in the range 0 <= value <= 1 via 
	//	reverse-lookup any value out of range causes the function to return 0
	uint16 i=90;
	
	// Filter out bad values
	if(value<0)
		value*=-1;

	if(value > 1)
		return 0;
		
	while(value < sintable[i])
		i--;

	// current sintable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if( (value - sintable[i]) > (sintable[i+1] - value) )
		return (i+1);
	
	return Angle(i);		// value is closer to previous
}


float Angle::Cosine(void)
{
	return costable[(int)angle_value];
}

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

float Angle::Tangent(int *status=NULL)
{
	if(angle_value==90 || angle_value==270)
	{
		if(status)
			*status=0;
		return 0.0;
	}
	
	return tantable[(int)angle_value];
}

Angle Angle::InvTangent(float value)
{
	// Returns the inverse cosine of a value in the range 0 <= value <= 1 via 
	//	reverse-lookup any value out of range causes the function to return 0
	uint16 i=90;

	// Filter out bad values
	if(value<0)
		value*=-1;

	if(value > 1)
		return 0;
		
	while(value > tantable[i])
		i--;

	// current costable[i] is less than value. Pick the degree value which is closer
	// to the passed value
	if( (value - tantable[i]) < (tantable[i+1] - value) )
		return (i+1);

	return Angle(i);		// value is closer to previous
}

uint8 Angle::Quadrant(void)
{
	// Returns a value based on what quadrant the angle is in.
	// 1: 0 <= angle <90
	// 2: 90 <= angle < 180
	// 3: 180 <= angle < 270
	// 4: 270 <= angle < 360
	
	
	// We can get away with not doing extra value checks because of the order in
	// which the checks are done.
	if(angle_value < 90)
		return 1;

	if(angle_value < 180)
		return 2;

	if(angle_value < 270)
		return 3;
	
	return 4;
}

Angle Angle::Constrain180(void)
{
	// Constrains angle to 0 <= angle < 180
	float val=angle_value;

	if(angle_value<180)
		return Angle(angle_value);

	while(!(val<180))
		val-=90;
	return Angle(val);
}

Angle Angle::Constrain90(void)
{
	// Constrains angle to 0 <= angle < 90
	float val=angle_value;

	if(angle_value<90)
		return Angle(angle_value);

	while(!(val<90))
		val-=90;
	return Angle(val);
}

void Angle::SetValue(float angle)
{
	angle_value=angle;
	Normalize();
}

float Angle::Value(void) const
{
	return angle_value;
}

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

Angle & Angle::operator=(const Angle &from)
{
	angle_value=from.angle_value;
	return *this;
}

Angle & Angle::operator=(const float &from)
{
	angle_value=from;
	return *this;
}

Angle & Angle::operator=(const long &from)
{
	angle_value=(float)from;
	return *this;
}

Angle & Angle::operator=(const int &from)
{
	angle_value=(float)from;
	return *this;
}

bool Angle::operator==(const Angle &from)
{
	return (angle_value==from.angle_value)?true:false;
}

bool Angle::operator!=(const Angle &from)
{
	return (angle_value!=from.angle_value)?true:false;
}

bool Angle::operator>(const Angle &from)
{
	return (angle_value>from.angle_value)?true:false;
}

bool Angle::operator<(const Angle &from)
{
	return (angle_value<from.angle_value)?true:false;
}

bool Angle::operator>=(const Angle &from)
{
	return (angle_value>=from.angle_value)?true:false;
}

bool Angle::operator<=(const Angle &from)
{
	return (angle_value<=from.angle_value)?true:false;
}
