#include <Message.h>
#include <Messenger.h>
#include <Window.h>

#include <stdio.h>

#include "MyView.h"
#include "Layer.h"

extern BWindow *wind;

MyView::MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	fTracking	= false;
	fIsResize	= false;
	fIs2ndButton= false;
	fMovingLayer = NULL;

	rgb_color	col;
	col.red		= 49;
	col.green	= 101;
	col.blue	= 156;
	topLayer = new Layer(Bounds(), "topLayer", B_FOLLOW_ALL, 0, col);
	topLayer->SetRootLayer(this);

	topLayer->rebuild_visible_regions(BRegion(Bounds()), BRegion(Bounds()), NULL);
	fRedrawReg.Set(Bounds());
}

MyView::~MyView()
{
	delete topLayer;
}

Layer* MyView::FindLayer(Layer *lay, BPoint &where) const
{
	if (lay->Visible()->Contains(where))
		return lay;
	else
		for (Layer *child = lay->VirtualBottomChild(); child; child = lay->VirtualUpperSibling())
		{
			Layer	*found = FindLayer(child, where);
			if (found)
				return found;
		}
	return NULL;
}

void MyView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	int32		buttons;
	Looper()->CurrentMessage()->FindInt32("buttons", &buttons);
	fLastPos = where;
	if (buttons == B_PRIMARY_MOUSE_BUTTON)
	{
		fTracking = true;
		fMovingLayer = FindLayer(topLayer, where);
		if (fMovingLayer == topLayer)
			fMovingLayer = NULL;
		if (fMovingLayer)
		{
			BRect	bounds(fMovingLayer->Bounds());
			fMovingLayer->ConvertToScreen2(&bounds);
			BRect	resizeRect(bounds.right-10, bounds.bottom-10, bounds.right, bounds.bottom);
			if (resizeRect.Contains(where))
				fIsResize = true;
			else
				fIsResize = false;
		}
	}
	else if (buttons == B_SECONDARY_MOUSE_BUTTON)
	{
		fIs2ndButton = true;
	}
	else if (buttons == B_TERTIARY_MOUSE_BUTTON)
	{
		DrawSubTree(topLayer);
	}
}

void MyView::MouseUp(BPoint where)
{
	fTracking = false;
	fIs2ndButton = false;
	fMovingLayer = NULL;
}

void MyView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	if (fTracking)
	{
		float dx, dy;
		dx = where.x - fLastPos.x;
		dy = where.y - fLastPos.y;
		fLastPos = where;

		if (dx != 0 || dy != 0)
		{
			if (fMovingLayer)
			{
				if (fIsResize)
					fMovingLayer->ResizeBy(dx, dy);
				else
					fMovingLayer->MoveBy(dx, dy);
			}
		}
	}
	else if (fIs2ndButton)
	{
		SetHighColor(0,0,0);
		StrokeLine(fLastPos, where);
		Flush();
		fLastPos = where;
	}
}

void MyView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_MOUSE_WHEEL_CHANGED:
		{
			float	dy;
			msg->FindFloat("be:wheel_delta_y", &dy);

			BPoint	pt;
			uint32	buttons;
			Layer	*lay;
			GetMouse(&pt, &buttons, false);
			if ((lay = FindLayer(topLayer, pt)))
				lay->ScrollBy(0, dy*5);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void MyView::CopyRegion(BRegion *reg, float dx, float dy)
{
	// Yes... in this sandbox app, do a redraw.
	reg->OffsetBy(dx, dy);
wind->Lock();
	ConstrainClippingRegion(reg);
	PushState();
	DrawSubTree(topLayer);
	PopState();
	ConstrainClippingRegion(NULL);
wind->Unlock();
}

void MyView::RequestRedraw()
{
	wind->Lock();
	ConstrainClippingRegion(&fRedrawReg);
	PushState();
	DrawSubTree(topLayer);
	PopState();
	ConstrainClippingRegion(NULL);

	fRedrawReg.MakeEmpty();

	wind->Unlock();
}

void MyView::Draw(BRect area)
{
/*
	ConstrainClippingRegion(&fRedrawReg);
//FillRect(Bounds());
//Flush();
//snooze(1000000);
	PushState();
	DrawSubTree(topLayer);
	PopState();
	ConstrainClippingRegion(NULL);
	fRedrawReg.MakeEmpty();
*/
}

void MyView::DrawSubTree(Layer* lay)
{
//printf("======== %s =======\n", lay->Name());
//	lay->Visible()->PrintToStream();
//	lay->FullVisible()->PrintToStream();
	for (Layer *child = lay->VirtualBottomChild(); child; child = lay->VirtualUpperSibling())
		DrawSubTree(child);

	ConstrainClippingRegion(lay->Visible());
	SetHighColor(lay->HighColor());
	BRegion	reg;
	lay->GetWantedRegion(reg);
	FillRect(reg.Frame());
	Flush();
	ConstrainClippingRegion(NULL);
}