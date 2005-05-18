#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <stdio.h>
#include <strings.h>

#include <Window.h>

#include "Layer.h"
#include "MyView.h"

extern BWindow* wind;

Layer::Layer(BRect frame, const char* name, uint32 rm, rgb_color c)
{
	fFrame = frame;
	fOrigin.Set(0.0f, 0.0f);
	fResizeMode = rm;
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
	MyView *view = GetRootLayer();
	if (view)
		if (fParent)
		{
			rect->Set(	rect->left - fOrigin.x,
						rect->top - fOrigin.y,
						rect->right - fOrigin.x,
						rect->bottom - fOrigin.y);

			rect->Set(	rect->left + fFrame.left,
						rect->top + fFrame.top,
						rect->right + fFrame.left,
						rect->bottom + fFrame.top);

			fParent->ConvertToScreen2(rect);
		}
}

MyView* Layer::GetRootLayer() const
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

bool Layer::IsVisuallyHidden() const
{
	// TODO: implement
	return false;
}

void Layer::resize_layer_frame_by(float x, float y)
{
	uint16		rm = fResizeMode & 0x0000FFFF;
	BRect		newFrame = fFrame;
	float		dx, dy;

	if ((rm & 0x000FU) == _VIEW_LEFT_)
	{
		newFrame.left += 0.0f;
		newFrame.right += x;
	}
	else if ((rm & 0x000FU) == _VIEW_RIGHT_)
	{
		newFrame.left += x;
		newFrame.right += x;
	}
	else if ((rm & 0x000FU) == _VIEW_CENTER_)
	{
		newFrame.left += x/2;
		newFrame.right += x/2;
	}

	if ((rm & 0x00F0U) == _VIEW_TOP_)
	{
		newFrame.top += 0.0f;
		newFrame.bottom += y;
	}
	else if ((rm & 0x00F0U) == _VIEW_BOTTOM_)
	{
		newFrame.top += y;
		newFrame.bottom += y;
	}
	else if ((rm & 0x00F0U) == _VIEW_CENTER_)
	{
		newFrame.top += y/2;
		newFrame.bottom += y/2;
	}

	dx	= newFrame.Width() - fFrame.Width();
	dy	= newFrame.Height() - fFrame.Height();

	fFrame	= newFrame;

	for (Layer *lay = VirtualBottomChild(); lay ; lay = VirtualUpperSibling())
	{
		lay->resize_layer_frame_by(dx, dy);
	}
}

void Layer::ResizeBy(float dx, float dy)
{
// TODO: add support for B_FULL_UPDATE_ON_RESIZE
// TODO: center and right alligned view must be full redrawn

	fFrame.Set(fFrame.left, fFrame.top, fFrame.right+dx, fFrame.bottom+dy);

	// resize children using their resize_mask.
	for (Layer *lay = fParent->VirtualBottomChild();
				lay; lay = fParent->VirtualUpperSibling())
			lay->resize_layer_frame_by(dx, dy);

	if (!IsVisuallyHidden() && GetRootLayer())
	{
		BRegion oldFullVisible(fFullVisible);

		// we'll invalidate the old position and the new, maxmial, one (bounds in screen coords).
		BRegion invalid(fFullVisible);
		BRect	r(Bounds());
		ConvertToScreen2(&r);
		invalid.Include(r);

		clear_visible_regions();

		fParent->RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now redraw regions that became visible

		// what's invalid, are the differences between to old and the new fullVisible region
		// 1) in case we grow.
		BRegion		redrawReg(fFullVisible);
		redrawReg.Exclude(&oldFullVisible);
		// 2) in case we shrink
		BRegion		redrawReg2(oldFullVisible);
		redrawReg2.Exclude(&fFullVisible);
		// 3) combine.
		redrawReg.Include(&redrawReg2);

		// add redrawReg to our RootLayer's redraw region.
		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	// call hook function
	if (dx != 0.0f || dy != 0.0f)
		ResizedByHook(dx, dy);
}

void Layer::MoveBy(float dx, float dy)
{
	fFrame.Set(fFrame.left+dx, fFrame.top+dy, fFrame.right+dx, fFrame.bottom+dy);

	if (!IsVisuallyHidden() && GetRootLayer())
	{
		BRegion oldFullVisible(fFullVisible);

		// we'll invalidate the old position and the new, maxmial, one (bounds in screen coords).
		BRegion invalid(fFullVisible);
		BRect	r(Bounds());
		ConvertToScreen2(&r);
		invalid.Include(r);

		clear_visible_regions();

		fParent->RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now copy common parts and redraw regions that became visible

		// include the actual and the old fullVisible regions. later, we'll exclude the common parts.
		BRegion		redrawReg(fFullVisible);
		redrawReg.Include(&oldFullVisible);

		// offset to layer's new location so that we can calculate the common region.
		oldFullVisible.OffsetBy(dx, dy);

		// finally we have the region that needs to be redrawn.
		redrawReg.Exclude(&oldFullVisible);

		// by intersecting the old fullVisible offseted to layer's new location, with the current
		// fullVisible, we'll have the common region which can be copied using HW acceleration.
		oldFullVisible.IntersectWith(&fFullVisible);

		// offset back and instruct the HW to do the actual copying.
		oldFullVisible.OffsetBy(-dx, -dy);
// TODO: HACK this!
//		GetDisplayDriver()->CopyRegion(&oldFullVisible, dx, dy);

		// add redrawReg to our RootLayer's redraw region.
		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	// call hook function
	if (dx != 0.0f || dy != 0.0f)
		MovedByHook(dx, dy);
}

void Layer::ScrollBy(float dx, float dy)
{
	fOrigin.Set(fOrigin.x + dx, fOrigin.y + dy);

	if (!IsVisuallyHidden() && GetRootLayer())
	{
		// set the region to be invalidated.
		BRegion		invalid(fFullVisible);

		clear_visible_regions();

		rebuild_visible_regions(invalid, invalid, VirtualBottomChild());

		// for the moment we say that the whole surface needs to be redraw.
		BRegion		redrawReg(fFullVisible);

		// offset old region so that we can start comparing.
		invalid.OffsetBy(dx, dy);

		// no need to redraw common regions. redraw only what's needed.
		redrawReg.Exclude(&invalid);

		// compute the common region. we'll use HW acc to copy this to the new location.
		invalid.IntersectWith(&fFullVisible);

		// offset back and instruct the driver to do the actual copying.
		invalid.OffsetBy(-dx, -dy);
// TODO: HACK this!
//		GetDisplayDriver()->CopyRegion(&invalid, dx, dy);

		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	if (dx != 0.0f || dy != 0.0f)
		ScrolledByHook(dx, dy);
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
	Layer	*child = VirtualBottomChild();
	while (child)
	{
		child->clear_visible_regions();
		child = child->VirtualUpperSibling();
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
