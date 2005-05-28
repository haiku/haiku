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

Layer* MyView::FindLayer(Layer *lay, const char *bytes) const
{
	if (strcmp(lay->Name(), bytes) == 0)
		return lay;
	else
	{
		Layer	*ret = NULL;
		Layer	*cursor = lay->VirtualBottomChild();
		while(cursor)
		{
			ret = FindLayer(cursor, bytes);
			if (ret)
				return ret;
			cursor = cursor->VirtualUpperSibling();
		}
	}
	return NULL;
}

void MyView::CopyRegion(BRegion *reg, float dx, float dy)
{
	// Yes... in this sandbox app, do a redraw.
wind->Lock();
	ConstrainClippingRegion(reg);
	DrawSubTree(topLayer);
	Flush();
wind->Unlock();
}

void MyView::RequestRedraw()
{
	wind->Lock();
	Invalidate();
	wind->Unlock();
}

void MyView::Draw(BRect area)
{
	ConstrainClippingRegion(&fRedrawReg);
FillRect(Bounds());
Flush();
snooze(1000000);
	PushState();
	DrawSubTree(topLayer);
	PopState();
	ConstrainClippingRegion(NULL);
	fRedrawReg.MakeEmpty();
}

void MyView::DrawSubTree(Layer* lay)
{
//printf("======== %s =======\n", lay->Name());
//	lay->Visible()->PrintToStream();
//	lay->FullVisible()->PrintToStream();
	Layer *child = lay->VirtualBottomChild();
	while(child)
	{
		DrawSubTree(child);
		child = child->VirtualUpperSibling();
	}
	ConstrainClippingRegion(lay->Visible());
	SetHighColor(lay->HighColor());
	BRegion	reg;
	lay->GetWantedRegion(reg);
	FillRect(reg.Frame());
	Flush();
	ConstrainClippingRegion(NULL);
}