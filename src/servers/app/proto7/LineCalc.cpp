#include "LineCalc.h"

LineCalc::LineCalc(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
}

float LineCalc::GetX(float y)
{
	return ( (y-offset)/slope );
}

float LineCalc::GetY(float x)
{
	return ( (slope * x) + offset );
}

