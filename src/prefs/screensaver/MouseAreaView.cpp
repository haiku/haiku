#include "MouseAreaView.h"
#include "Constants.h"
#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <stdio.h>

inline BPoint scaleDirect(float x, float y,BRect area)
{
	return BPoint(area.Width()*x+area.left,area.Height()*y+area.top);
}

inline BRect scaleDirect (float x1,float x2,float y1,float y2,BRect area)
{
	return BRect(area.Width()*x1+area.left,area.Height()*y1+area.top, area.Width()*x2+area.left,area.Height()*y2+area.top);
}

float positionalX[]= {0,.1,.25,.3,.7,.75,.9,1.0};
float positionalY[]= {0,.1,.7,.8,.9,1.0};

inline BPoint scale(int x, int y,BRect area) { return scaleDirect(positionalX[x],positionalY[y],area); }
inline BRect scale(int x1, int x2, int y1, int y2,BRect area) { return scaleDirect(positionalX[x1],positionalX[x2],positionalY[y1],positionalY[y2],area); }

int secondsToSlider(int val)
{
	int t;
	for (t=0;t<sizeof(timeInSeconds)/sizeof(int);t++)
		if (timeInSeconds[t]==val)
				return t;
	return -1;
}

void MouseAreaView::Draw(BRect update)
{
	SetViewColor(216,216,216);
	// Top of monitor
	SetHighColor(grey);
	FillRoundRect(scale(0,7,0,3,Bounds()),4,4);
	SetHighColor(black);
	StrokeRoundRect(scale(0,7,0,3,Bounds()),4,4);
	SetHighColor(lightBlue);
	screenArea=scale(1,6,1,2,Bounds());
	FillRect(screenArea);
	SetHighColor(darkGrey);
	StrokeRect(screenArea);
	// Base of monitor
	SetHighColor(grey);
	FillTriangle(scale(1,5,Bounds()),scale(3,4,Bounds()),scale(3,5,Bounds()));
	FillTriangle(scale(4,5,Bounds()),scale(4,4,Bounds()),scale(6,5,Bounds()));
	FillRect(scale(3,4,4,5,Bounds()));
	FillRect(scale(3,4,3,4,Bounds()));
	SetHighColor(black);
	StrokeRect(scale(3,4,3,4,Bounds()));
	StrokeLine(scale(2,4,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(5,4,Bounds()));
	DrawArrow();
}

float arrowX[]= {0,.25,.5,.66667,.90,.9};
float arrowY[]= {0,.15,.25,.3333333,.6666667,1.0};
inline BPoint scale3(int x, int y,BRect area,bool invertX,bool invertY) 
	{ return scaleDirect(((invertX)?1-arrowX[x]:arrowX[x]),((invertY)?1-arrowY[y]:arrowY[y]),area); }

BRect getArrowSize(BRect area,bool isCentered)
{
	int size=area.IntegerWidth();
	int temp=area.IntegerHeight();
	if (temp<size)
		size=temp;
	size/=3;
	BRect foo(0,0,size,size);
	if (isCentered)
		foo.OffsetBy(area.left+area.Width()/2-(size/2),area.top+area.Height()/2-(size/2));
	return (foo);
}

void MouseAreaView::DrawArrow(void)
{
	if (currentDirection!=NONE)
		{
		BRect area(getArrowSize(screenArea,false));
		bool invertX=(currentDirection==UPRIGHT||currentDirection==DOWNRIGHT);
		bool invertY=(currentDirection==UPRIGHT||currentDirection==UPLEFT);
		
		float size=area.Width();
		MovePenTo(((invertX)?screenArea.right-size-2:2+screenArea.left),((!invertY)?screenArea.bottom-size-2:2+screenArea.top));
		BShape arrow;
		arrow.MoveTo(scale3(0,1,area,invertX,invertY));
		arrow.LineTo(scale3(0,5,area,invertX,invertY));
		arrow.LineTo(scale3(4,5,area,invertX,invertY));
		arrow.LineTo(scale3(3,4,area,invertX,invertY));
		arrow.LineTo(scale3(5,3,area,invertX,invertY));
		arrow.LineTo(scale3(2,0,area,invertX,invertY));
		arrow.LineTo(scale3(1,2,area,invertX,invertY));
		arrow.LineTo(scale3(0,1,area,invertX,invertY));
		arrow.Close();
		SetHighColor(black);
		FillShape(&arrow,B_SOLID_HIGH);
		}
	else
		{
		PushState();
		BRect area(getArrowSize(screenArea,true));
		SetHighColor(red);
		SetPenSize(2);
		StrokeEllipse(area);
		StrokeLine(BPoint(area.right,area.top),BPoint(area.left,area.bottom));
		PopState();
		}
}


void MouseAreaView::MouseUp(BPoint point)
{
	if (screenArea.Contains(point))
		{
		if (getArrowSize(screenArea,true).Contains(point))
			currentDirection=NONE;
		else
			{
			float centerX=screenArea.left+(screenArea.Width()/2);
			float centerY=screenArea.top+(screenArea.Height()/2);
			if (point.x<centerX)
				currentDirection=((point.y<centerY)?UPLEFT:DOWNLEFT);
			else
				currentDirection=((point.y<centerY)?UPRIGHT:DOWNRIGHT);
			}
		Draw(screenArea);
		}
}

