#include "DisplaySupport.h"

BezierCurve::BezierCurve(BPoint* pts)
{
	int i;

	pointArray = NULL;
	for (i=0; i<4; i++)
		points.AddItem(new BPoint(pts[i]));
	GeneratePoints(0);
}

BezierCurve::~BezierCurve()
{
	int i, numPoints;

	numPoints = points.CountItems();
	for (i=0; i<numPoints; i++)
		delete (BPoint*)points.ItemAt(i);
	if ( pointArray )
		delete[] pointArray;
}

BPoint* BezierCurve::GetPointArray()
{
	int i, numPoints;

	if ( !pointArray )
	{
		numPoints = points.CountItems();
		pointArray = new BPoint[numPoints];
		for (i=0; i<numPoints; i++)
			pointArray[i] = *((BPoint*)points.ItemAt(i));
	}
	return pointArray;
}

int BezierCurve::GeneratePoints(int startPos)
{
	double slopeAB, slopeBC, slopeCD;
	BPoint** pointList = (BPoint **)points.Items();

	// TODO Check for more conditions where we can fudge the curve to a line

	if ( (fabs(pointList[startPos]->x-pointList[startPos+3]->x) <= 1) &&
		(fabs(pointList[startPos]->y-pointList[startPos+3]->y) <= 1) )
		return 0;

	if ( (pointList[startPos]->x == pointList[startPos+1]->x) &&
		(pointList[startPos+1]->x == pointList[startPos+2]->x) &&
		(pointList[startPos+2]->x == pointList[startPos+3]->x) )
		return 0;

	slopeAB = (pointList[startPos]->y-pointList[startPos+1]->y)/(pointList[startPos]->x-pointList[startPos+1]->x);
	slopeBC = (pointList[startPos+1]->y-pointList[startPos+2]->y)/(pointList[startPos+1]->x-pointList[startPos+2]->x);
	slopeCD = (pointList[startPos+2]->y-pointList[startPos+3]->y)/(pointList[startPos+2]->x-pointList[startPos+3]->x);

	if ( (slopeAB == slopeBC) && (slopeBC == slopeCD) )
		return 0;

	BPoint *pointAB = new BPoint();
	BPoint pointBC;
	BPoint *pointCD = new BPoint();
	BPoint *pointABC = new BPoint();
	BPoint *pointBCD = new BPoint();
	BPoint *curvePoint = new BPoint();
	int lengthIncrement = 3;

	pointAB->x = (pointList[startPos]->x + pointList[startPos+1]->x)/2;
	pointAB->y = (pointList[startPos]->y + pointList[startPos+1]->y)/2;
	pointBC.x = (pointList[startPos+1]->x + pointList[startPos+2]->x)/2;
	pointBC.y = (pointList[startPos+1]->y + pointList[startPos+2]->y)/2;
	pointCD->x = (pointList[startPos+2]->x + pointList[startPos+3]->x)/2;
	pointCD->y = (pointList[startPos+2]->y + pointList[startPos+3]->y)/2;
	pointABC->x = (pointAB->x + pointBC.x)/2;
	pointABC->y = (pointAB->y + pointBC.y)/2;
	pointBCD->x = (pointCD->x + pointBC.x)/2;
	pointBCD->y = (pointCD->y + pointBC.y)/2;
	curvePoint->x = (pointABC->x + pointBCD->x)/2;
	curvePoint->y = (pointABC->y + pointBCD->y)/2;
	delete pointList[startPos+1];
	delete pointList[startPos+2];
	pointList[startPos+1] = pointAB;
	pointList[startPos+2] = pointCD;
	points.AddItem(pointABC,startPos+2);
	points.AddItem(curvePoint,startPos+3);
	points.AddItem(pointBCD,startPos+4);
	lengthIncrement += GeneratePoints(startPos);
	lengthIncrement += GeneratePoints(startPos + lengthIncrement);
	return lengthIncrement;
}

Blitter::Blitter()
{
	DrawFunc = &Blitter::draw_none;
}

void Blitter::draw_8_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint8 *s = (uint8 *)src;
	uint8 *d = (uint8 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_8_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint8 *s = (uint8 *)src;
	uint16 *d = (uint16 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_8_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint8 *s = (uint8 *)src;
	uint32 *d = (uint32 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}
	
void Blitter::draw_16_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint16 *s = (uint16 *)src;
	uint8 *d = (uint8 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_16_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint16 *s = (uint16 *)src;
	uint16 *d = (uint16 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_16_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint16 *s = (uint16 *)src;
	uint32 *d = (uint32 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}
	
void Blitter::draw_32_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint32 *s = (uint32 *)src;
	uint8 *d = (uint8 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_32_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint32 *s = (uint32 *)src;
	uint16 *d = (uint16 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}

void Blitter::draw_32_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	uint32 *s = (uint32 *)src;
	uint32 *d = (uint32 *)dst;
	
	while(width--)
	{
		*d++ = s[xscale_position >> 16];
		xscale_position += xscale_factor;
	}
}
	
void Blitter::Draw(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor)
{
	(this->*DrawFunc)(src, dst, width, xscale_position, xscale_factor);
}

void Blitter::Select(int32 source_bits_per_pixel, int32 dest_bits_per_pixel)
{
	if(source_bits_per_pixel == 8 && dest_bits_per_pixel == 8)
		DrawFunc = &Blitter::draw_8_to_8;
	else if(source_bits_per_pixel == 8 && dest_bits_per_pixel == 16)
		DrawFunc = &Blitter::draw_8_to_16;
	else if(source_bits_per_pixel == 8 && dest_bits_per_pixel == 32)
		DrawFunc = &Blitter::draw_8_to_32;
	else if(source_bits_per_pixel == 16 && dest_bits_per_pixel == 16)
		DrawFunc = &Blitter::draw_16_to_16;
	else if(source_bits_per_pixel == 16 && dest_bits_per_pixel == 8)
		DrawFunc = &Blitter::draw_16_to_8;
	else if(source_bits_per_pixel == 16 && dest_bits_per_pixel == 32)
		DrawFunc = &Blitter::draw_16_to_32;
	else if(source_bits_per_pixel == 32 && dest_bits_per_pixel == 32)
		DrawFunc = &Blitter::draw_32_to_32;
	else if(source_bits_per_pixel == 32 && dest_bits_per_pixel == 16)
		DrawFunc = &Blitter::draw_32_to_16;
	else if(source_bits_per_pixel == 32 && dest_bits_per_pixel == 8)
		DrawFunc = &Blitter::draw_32_to_8;
	else
		DrawFunc = &Blitter::draw_none;
}

LineCalc::LineCalc()
{
}

LineCalc::LineCalc(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
	minx = MIN(start.x,end.x);
	maxx = MAX(start.x,end.x);
	miny = MIN(start.y,end.y);
	maxy = MAX(start.y,end.y);
}

void LineCalc::SetPoints(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
	minx = MIN(start.x,end.x);
	maxx = MAX(start.x,end.x);
	miny = MIN(start.y,end.y);
	maxy = MAX(start.y,end.y);
}

bool LineCalc::ClipToRect(const BRect& rect)
{
	if ( (maxx < rect.left) || (minx > rect.right) || (miny < rect.top) || (maxy > rect.bottom) )
		return false;
	BPoint newStart(-1,-1);
	BPoint newEnd(-1,-1);
	if ( maxx == minx )
	{
		newStart.x = newEnd.x = minx;
		if ( miny < rect.top )
			newStart.y = rect.top;
		else
			newStart.y = miny;
		if ( maxy > rect.bottom )
			newEnd.y = rect.bottom;
		else
			newEnd.y = maxy;
	}
	else if ( maxy == miny )
	{
		newStart.y = newEnd.y = miny;
		if ( minx < rect.left )
			newStart.x = rect.left;
		else
			newStart.x = minx;
		if ( maxx > rect.right )
			newEnd.x = rect.right;
		else
			newEnd.x = maxx;
	}
	else
	{
		float leftInt, rightInt, topInt, bottomInt;
		BPoint tempPoint;
		leftInt = GetY(rect.left);
		rightInt = GetY(rect.right);
		topInt = GetX(rect.top);
		bottomInt = GetX(rect.bottom);
		if ( end.x < start.x )
		{
			tempPoint = start;
			start = end;
			end = tempPoint;
		}
		if ( start.x < rect.left )
		{
			if ( (leftInt >= rect.top) && (leftInt <= rect.bottom) )
			{
				newStart.x = rect.left;
				newStart.y = leftInt;
			}
			if ( start.y < end.y )
			{
				if ( (topInt >= rect.left) && (topInt <= rect.right) )
				{
					newStart.x = topInt;
					newStart.y = rect.top;
				}
			}
			else
			{
				if ( (bottomInt >= rect.left) && (bottomInt <= rect.right) )
				{
					newStart.x = bottomInt;
					newStart.y = rect.bottom;
				}
			}
		}
		else
		{
			if ( start.y < rect.top )
			{
				if ( (topInt >= rect.left) && (topInt <= rect.right) )
				{
					newStart.x = topInt;
					newStart.y = rect.top;
				}
			}
			else if ( start.y > rect.bottom )
			{
				if ( (bottomInt >= rect.left) && (bottomInt <= rect.right) )
				{
					newStart.x = bottomInt;
					newStart.y = rect.bottom;
				}
			}
			else
				newStart = start;
		}
		if ( end.x > rect.right )
		{
			if ( (rightInt >= rect.top) && (rightInt <= rect.bottom) )
			{
				newEnd.x = rect.right;
				newEnd.y = rightInt;
			}
			if ( start.y < end.y )
			{
				if ( (bottomInt >= rect.left) && (bottomInt <= rect.right) )
				{
					newEnd.x = bottomInt;
					newEnd.y = rect.bottom;
				}
			}
			else
			{
				if ( (topInt >= rect.left) && (topInt <= rect.right) )
				{
					newEnd.x = topInt;
					newEnd.y = rect.top;
				}
			}
		}
		else
		{
			if ( end.y < rect.top )
			{
				if ( (topInt >= rect.left) && (topInt <= rect.right) )
				{
					newEnd.x = topInt;
					newEnd.y = rect.top;
				}
			}
			else if ( end.y > rect.bottom )
			{
				if ( (bottomInt >= rect.left) && (bottomInt <= rect.right) )
				{
					newEnd.x = bottomInt;
					newEnd.y = rect.bottom;
				}
			}
			else
				newEnd = end;
		}
	}
	if ( (newStart.x == -1) || (newStart.y == -1) || (newEnd.x == -1) || (newEnd.y == -1) )
		return false;
	SetPoints(newStart,newEnd);

	return true;
}

void LineCalc::Swap(LineCalc &from)
{
	BPoint pta, ptb;
	pta = start;
	ptb = end;
	SetPoints(from.start,from.end);
	from.SetPoints(pta,ptb);
}

float LineCalc::GetX(float y)
{
	if (start.x == end.x)
		return start.x;
	return ( (y-offset)/slope );
}

float LineCalc::GetY(float x)
{
	if ( start.x == end.x )
		return start.y;
	return ( (slope * x) + offset );
}
