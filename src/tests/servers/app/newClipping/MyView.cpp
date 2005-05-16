#include <Message.h>
#include <Messenger.h>

#include <stdio.h>

#include "MyView.h"
#include "Layer.h"

MyView::MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	rgb_color	col;
	col.red		= 49;
	col.green	= 101;
	col.blue	= 156;
	topLayer = new Layer(Bounds(), "topLayer", B_FOLLOW_ALL, col);
	topLayer->SetRootLayer(this);

//	topLayer->RebuildVisibleRegions(BRegion(Bounds()), NULL);
	topLayer->rebuild_visible_regions(BRegion(Bounds()), BRegion(Bounds()), NULL);
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

void MyView::Draw(BRect area)
{
	DrawSubTree(topLayer);
}

void MyView::DrawSubTree(Layer* lay)
{
printf("======== %s =======\n", lay->Name());
	lay->Visible()->PrintToStream();
	lay->FullVisible()->PrintToStream();
	Layer *child = lay->VirtualBottomChild();
	while(child)
	{
		DrawSubTree(child);
		child = child->VirtualUpperSibling();
	}
	ConstrainClippingRegion(lay->Visible());
	SetHighColor(lay->HighColor());
	BRect	temp = lay->Bounds();
	lay->ConvertToScreen2(&temp);
	FillRect(temp);
	ConstrainClippingRegion(NULL);
}