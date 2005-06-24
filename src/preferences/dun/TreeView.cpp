/*

The Amazing TreeView !

Author: Misza (misza@ihug.com.au)

(C) 2002 OpenBeOS under MIT license

*/
#include "TreeView.h"
#include "DUNWindow.h"
TreeView::TreeView(BRect r) : BView(r,"miszaView",B_FOLLOW_TOP,B_WILL_DRAW)
{	
	down = false;
	status = false;
}
void TreeView::MouseUp(BPoint point)
{	//controls what get resized and how
	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons);
	
	if(where != point)
	{	Invalidate();
		down = false;
	}
	
	if(!status)//if view is hidden
	{
	Invalidate();

	status = true;
	((DUNWindow*)Window())->DoResizeMagic();
	}
	else if(status)
	{
	Invalidate();

	status = false;
	((DUNWindow*)Window())->DoResizeMagic();
	}
}

void TreeView::MouseDown(BPoint point)
{
	SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
	down = true;
	Invalidate();
}

void TreeView::Draw(BRect updateRect)
{	//colours for drawing the triangle
	
	SetHighColor(0,0,0);
	SetLowColor(150,150,150);
	if(status && !down)//if view is showing
	{
		//horiz triangle
		FillTriangle(BPoint(0,2),BPoint(8,2),BPoint(4,6),B_SOLID_LOW);
		StrokeTriangle(BPoint(0,2),BPoint(8,2),BPoint(4,6),B_SOLID_HIGH);
	}
	else if (!status && !down){
	//vert triangle
		FillTriangle(BPoint(2,0),BPoint(2,8),BPoint(6,4),B_SOLID_LOW);
		StrokeTriangle(BPoint(2,0),BPoint(2,8),BPoint(6,4),B_SOLID_HIGH);
	}
	else
	{//midway
		SetHighColor(0,0,0);
		SetLowColor(100,100,0);
		FillTriangle(BPoint(0,6),BPoint(6,0),BPoint(6,6),B_SOLID_LOW);
		StrokeTriangle(BPoint(0,6),BPoint(6,0),BPoint(6,6),B_SOLID_HIGH);
	}
	
	down = false;
	SetHighColor(0,0,0);
	SetLowColor(150,150,150);

}

void TreeView::SetTreeViewStatus(bool condition)
{
	status = condition;
}	
