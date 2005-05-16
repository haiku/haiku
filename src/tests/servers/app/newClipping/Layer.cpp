#include <OS.h>
#include <View.h>
#include <Window.h>
#include <Looper.h>
#include <Region.h>
#include <Rect.h>
#include <stdio.h>
#include <strings.h>

#include "Layer.h"

extern BWindow* wind;

Layer::Layer(BRect frame, const char* name, uint32 rm, rgb_color c)
{
	fFrame = frame;
	fRM = rm;
	fColor = c;

	fBottom = NULL;
	fUpper = NULL;
	fLower = NULL;
	fTop = NULL;
	fParent = NULL;
	fView = NULL;

	strcpy(fName, name);
}

Layer::~Layer()
{
	Layer	*c = fBottom;
	Layer	*toast;
	while (c)
	{
		toast = c;
		c = c->fUpper;
		delete toast;
	}
}

void Layer::ConvertToScreen2(BRect* rect)
{
	BView *view = GetRootLayer();
	if (view)
		if (fParent)
		{
			rect->Set(	rect->left + fFrame.left,
						rect->top + fFrame. top,
						rect->right + fFrame.left,
						rect->bottom + fFrame.top);

			fParent->ConvertToScreen2(rect);
		}
}

BView* Layer::GetRootLayer() const
{
	if (fView)
		return fView;
	else
		if (fParent)
			return fParent->GetRootLayer();
		else
			return NULL;
}

Layer* Layer::VirtualBottomChild() const
{
	return fBottom;
}

Layer* Layer::VirtualTopChild() const
{
	return fTop;
}

Layer* Layer::VirtualUpperSibling() const
{
	return fUpper;
}

Layer* Layer::VirtualLowerSibling() const
{
	return fLower;
}

void Layer::AddLayer(Layer* layer)
{
	if( layer->fParent != NULL ) 
	{
		printf("ERROR: Layer already has a parent\n");
		return;
	}
	
	layer->fParent = this;
	
	if (!fBottom)
	{
		fBottom = layer;
		fTop = layer;
		return;
	}
	fBottom->fLower = layer;
	layer->fUpper = fBottom;
	fBottom = layer;
}

bool Layer::RemLayer(Layer* layer)
{
	if(!layer->fParent || layer->fParent != this)
	{
		printf("ERROR: Rem: Layer doesn't have a fParent or !=this\n");
		return false;
	}
	
	layer->fParent = NULL;
	
	if(fTop == layer)
		fTop = layer->fLower;
	
	if(fBottom == layer )
		fBottom = layer->fUpper;
	
	if(layer->fUpper != NULL)
		layer->fUpper->fLower = layer->fLower;
	
	if(layer->fLower != NULL)
		layer->fLower->fUpper = layer->fUpper;
	
	layer->fUpper = NULL;
	layer->fLower = NULL;

	layer->clear_visible_regions();

	return true;
}

void Layer::set_user_regions(BRegion &reg)
{
// OPT: maybe we should have all these cached in a 'fFull' member

	// 1) set to frame in screen coords
//	BRect			screenFrame(fFrame);
	BRect			screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

	// 2) intersect with screen region
// TODO: remove locking when for real
wind->Lock();
printf("Adi: RL: %p\n", wind);
printf("Adi: RL: %s\n", wind->Name());
	BRegion			screenReg(GetRootLayer()->Bounds());
wind->Unlock();
	reg.IntersectWith(&screenReg);

// TODO: you MUST at some point uncomment this block!
/*
	// 3) impose user constrained regions
	LayerData		*stackData = fLayerData;
	while (stackData)
	{
		// transform in screen coords
		BRegion		screenReg(stackData->ClippingRegion());
		ConvertToScreen2(&screenReg);
		reg.IntersectWith(&screenReg);
		stackData	= stackData->prevState;
	}
*/
}

void Layer::RebuildVisibleRegions(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible);
	localVisible.IntersectWith(&invalid);
	rebuild_visible_regions(invalid, localVisible, startFrom);
}

void Layer::rebuild_visible_regions(const BRegion &invalid,
									const BRegion &parentLocalVisible,
									const Layer *startFrom)
{
	bool fullRebuild = false;
	// intersect maximum wanted region with the invalid region
	BRegion common;
	set_user_regions(common);
	common.IntersectWith(&invalid);
	// if the resulted region is not valid, this layer is not in the catchment area
	// of the region being invalidated
	if (!common.Frame().IsValid())
		return;

	// now intersect with parent's visible part of the region that was/is invalidated
	common.IntersectWith(&parentLocalVisible);
//printf("\t%s: For this layer we invalidate:\n", fName);
//common.PrintToStream();
	if (common.Frame().IsValid())
	{
		// we have something to include to our fullVisible. It may already be in
		// there, but we'll never know.
		fFullVisible.Include(&common);
	}
	else
	{
		// this layer is in the catchment area of the region being invalidated,
		// yet it will have no new visible area attached to it. It means
		// this layer was overshaddowed by those above it and any visible area
		// that it may have common with the region being invalidated, must be
		// excluded from it's fFullVisible and fVisible regions.
		fFullVisible.Exclude(&invalid);
		fVisible.Exclude(&invalid);
		// we don't return here becase we want the same thing to happen to all
		// our children.

		// Don't worry about the last line from this method, it will do nothing -
		// common is invalid. Same goes for the last one in the 'for' statement below.
	}

	for (Layer *lay = VirtualBottomChild(); lay ; lay = VirtualUpperSibling())
	{
		if (lay == startFrom)
			fullRebuild = true;

		if (fullRebuild)
			lay->rebuild_visible_regions(invalid, common, lay->VirtualBottomChild());

		common.Exclude(&lay->fFullVisible);
fVisible.Exclude(&lay->fFullVisible);
	}

	// the visible region of this layer is what left after all its children took
	// what they could.
	fVisible.Include(&common);
}

void Layer::clear_visible_regions()
{
	fVisible.MakeEmpty();
	fFullVisible.MakeEmpty();
	Layer	*child = fBottom;
	while (child)
	{
		child->clear_visible_regions();
		child = child->fUpper;
	}
}

void Layer::PrintToStream() const
{
	printf("-> %s\n", fName);
	Layer *child = VirtualBottomChild();
	while(child)
	{
		child->PrintToStream();
		child = child->VirtualUpperSibling();
	}
}
