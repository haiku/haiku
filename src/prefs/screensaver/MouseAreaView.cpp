#include "MouseAreaView.h"
#include "Constants.h"
#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <stdio.h>

void 
MouseAreaView::Draw(BRect update) 
{
	SetViewColor(216,216,216);
	// Top of monitor
	SetHighColor(kGrey);
	FillRoundRect(scale(0,7,0,3,Bounds()),4,4);
	SetHighColor(kBlack);
	StrokeRoundRect(scale(0,7,0,3,Bounds()),4,4);
	SetHighColor(kLightBlue);
	fScreenArea=scale(1,6,1,2,Bounds());
	FillRect(fScreenArea);
	SetHighColor(kDarkGrey);
	StrokeRect(fScreenArea);
	// Base of monitor
	SetHighColor(kGrey);
	FillTriangle(scale(1,5,Bounds()),scale(3,4,Bounds()),scale(3,5,Bounds()));
	FillTriangle(scale(4,5,Bounds()),scale(4,4,Bounds()),scale(6,5,Bounds()));
	FillRect(scale(3,4,4,5,Bounds()));
	FillRect(scale(3,4,3,4,Bounds()));
	SetHighColor(kBlack);
	StrokeRect(scale(3,4,3,4,Bounds()));
	StrokeLine(scale(2,4,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(1,5,Bounds()));
	StrokeLine(scale(6,5,Bounds()),scale(5,4,Bounds()));
	DrawArrow();
}


BRect 
getArrowSize(BRect area,bool isCentered)
{
	int areaWidth=area.IntegerWidth();
	int areaHeight=area.IntegerHeight();
	if (areaHeight<areaWidth)
		areaWidth=areaHeight;
	areaWidth/=3;
	BRect foo(0,0,areaWidth,areaWidth);
	if (isCentered)
		foo.OffsetBy(area.left+area.Width()/2-(areaWidth/2),area.top+area.Height()/2-(areaWidth/2));
	return (foo);
}


void 
MouseAreaView::DrawArrow(void) 
{
	if (fCurrentDirection!=NONE) {
		BRect area(getArrowSize(fScreenArea,false));
		bool invertX=(fCurrentDirection==UPRIGHT||fCurrentDirection==DOWNRIGHT);
		bool invertY=(fCurrentDirection==UPRIGHT||fCurrentDirection==UPLEFT);
		
		float size=area.Width();
		MovePenTo(((invertX)?fScreenArea.right-size-2:2+fScreenArea.left),((!invertY)?fScreenArea.bottom-size-2:2+fScreenArea.top));
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
		SetHighColor(kBlack);
		FillShape(&arrow,B_SOLID_HIGH);
	} else {
		BRect area(getArrowSize(fScreenArea,true));
		SetHighColor(kRed);
		SetPenSize(2);
		StrokeEllipse(area);
		StrokeLine(BPoint(area.right,area.top),BPoint(area.left,area.bottom));
	}
}


void MouseAreaView::MouseUp(BPoint point) 
{
	if (fScreenArea.Contains(point)) {
		if (getArrowSize(fScreenArea,true).Contains(point))
			fCurrentDirection=NONE;
		else {
			float centerX=fScreenArea.left+(fScreenArea.Width()/2);
			float centerY=fScreenArea.top+(fScreenArea.Height()/2);
			if (point.x<centerX)
				fCurrentDirection=((point.y<centerY)?UPLEFT:DOWNLEFT);
			else
				fCurrentDirection=((point.y<centerY)?UPRIGHT:DOWNRIGHT);
			}
		Draw(fScreenArea);
	}
}
