#ifndef LINECALC_H_
#define LINECALC_H_

#include <Point.h>

class LineCalc
{
public:
	LineCalc(const BPoint &pta, const BPoint &ptb);
	float GetX(float y);
	float GetY(float x);
	float Slope(void) { return slope; }
	float Offset(void) { return offset; }
private:
	float slope;
	float offset;
	BPoint start, end;
};

#endif