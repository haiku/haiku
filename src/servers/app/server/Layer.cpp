//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Layer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#include "Layer.h"
#include "RectUtils.h"
//#include "ServerWindow.h"
#include "PortLink.h"
#include <string.h>
#include <stdio.h>

Layer::Layer(BRect frame, const char *name, int32 resize, int32 flags,ServerWindow *win)
{
/*	// frame is in _parent layer's coordinates
	if(frame.IsValid())
		_frame=frame;
	else
		_frame.Set(0,0,1,1);

	_name=new BString(name);

	// Layer does not start out as a part of the tree
	_parent=NULL;
	_uppersibling=NULL;
	_lowersibling=NULL;
	_topchild=NULL;
	_bottomchild=NULL;

	_visible=new BRegion(Bounds());
	_full=new BRegion(Bounds());
	_invalid=NULL;

	_serverwin=win;
	
	// TODO: Can't remember why we have these... Find out why
	_view_token=0;

	_flags=flags;

	_hidecount=0;
	_is_dirty=false;
	_is_updating=false;

	_level=0;
	_layerdata=new LayerData;
*/
}

Layer::~Layer(void)
{
/*	if(_visible)
	{
		delete _visible;
		_visible=NULL;
	}
	if(_full)
	{
		delete _full;
		_full=NULL;
	}
	if(_invalid)
	{
		delete _invalid;
		_invalid=NULL;
	}
	if(_name)
	{
		delete _name;
		_name=NULL;
	}
	if(_layerdata)
	{
		delete _layerdata;
		_layerdata=NULL;
	}
*/
}

// Tested for adding children with 1 and 2 child cases, but not more
void Layer::AddChild(Layer *layer, Layer *before=NULL, bool rebuild)
{
/*	// Adds a layer to the top of the layer's children
	if(layer->_parent!=NULL)
	{
		printf("ERROR: AddChild(): View already has a _parent\n");
		return;
	}
	layer->_parent=this;
	if(layer->_visible && layer->_hidecount==0 && _visible)
	{
		// Technically, we could safely take the address of ConvertToParent(BRegion)
		// but we don't just to avoid a compiler nag
		BRegion *reg=new BRegion(layer->ConvertToParent(layer->_visible));
		_visible->Exclude(reg);
		delete reg;
	}

	// we need to change this to a loop for each _lowersibling of the layer
	if(_topchild!=NULL)
	{
		// we're adding to the back side of the stack??
		layer->_lowersibling=_topchild;
		
		// added layer will be at the bottom of the stack
		_topchild->_uppersibling=layer;
		for(Layer *lay=layer->_lowersibling; lay!=NULL; lay=lay->_lowersibling)
		{
			if(layer->_frame.Intersects(lay->_frame))
			{
				if(lay->_visible && lay->_hidecount==0)
				{
					// reg is what is _visible in the layer's _parent's coordinate system
					BRegion *reg=new BRegion(ConvertToParent(layer->_visible));
					// reg2 is the layer's _visible region in the sibling's coordinate system
					BRegion *reg2=new BRegion(lay->ConvertFromParent(reg));
					delete reg;
	
	//				layer->_lowersibling->_visible->Exclude(reg2);
					// lowersiblings occupy screen space _above_ a layer, so the layer itself
					// must remove from its _visible region 
					layer->_visible->Exclude(reg2);
					delete reg2;
				}
			}
		}
	}
	else
		_bottomchild=layer;
	_topchild=layer;
	layer->_level=_level+1;
*/
}

// Tested for cases with 1 and 2 children, but no more than that
void Layer::RemoveChild(Layer *layer, bool rebuild)
{
/*	// Remove a layer from the tree
	if(layer->_parent==NULL)
	{
		printf("ERROR: RemoveChild(): View doesn't have a _parent\n");
		return;
	}
	if(layer->_parent!=this)
	{
		printf("ERROR: RemoveChild(): View is not a child of this layer\n");
		return;
	}

	if(_hidecount==0 && layer->_visible && layer->_parent->_visible)
	{
		BRegion *reg=new BRegion(ConvertToParent(_visible));
		layer->_parent->_visible->Include(reg);
		delete reg;
	}

	// Take care of _parent
	layer->_parent=NULL;
	if(_topchild==layer)
		_topchild=layer->_lowersibling;
	if(_bottomchild==layer)
		_bottomchild=layer->_uppersibling;

	// Take care of siblings
	if(layer->_uppersibling!=NULL)
		layer->_uppersibling->_lowersibling=layer->_lowersibling;
	if(layer->_lowersibling!=NULL)
		layer->_lowersibling->_uppersibling=layer->_uppersibling;
	layer->_uppersibling=NULL;
	layer->_lowersibling=NULL;

	RebuildRegions();
*/
}

void Layer::RemoveSelf(bool rebuild)
{
/*	// A Layer removes itself from the tree (duh)
	if(_parent==NULL)
	{
		printf("ERROR: RemoveSelf(): View doesn't have a _parent\n");
		return;
	}
	_parent->RemoveChild(this);
*/
}

Layer *Layer::GetChildAt(BPoint pt, bool recursive=false)
{
/*	// Find out which child gets hit if we click at a certain spot. Returns NULL
	// if there are no _visible children or if the click does not hit a child layer
	// If recursive==true, then it will continue to call until it reaches a layer
	// which has no children, i.e. a layer that is at the top of its 'branch' in
	// the layer tree
	
	Layer *child;
	if(recursive)
	{
		for(child=_bottomchild; child!=NULL; child=child->_uppersibling)
		{
			if(child->_bottomchild!=NULL)
				child->GetChildAt(pt,true);
			
			if(child->_hidecount>0)
				continue;
			
			if(child->_frame.Contains(pt))
				return child;
		}
	}
	else
	{
		for(child=_bottomchild; child!=NULL; child=child->_uppersibling)
		{
			if(child->_hidecount>0)
				continue;
//			if(child->_visible && child->_visible->Contains(ConvertFromTop(pt)))
//				printf("child hit by mouse. News at 11\n");
			if(child->_frame.Contains(pt))
				return child;
		}
	}
*/	return NULL;
}

BRect Layer::Bounds(void)
{
	return _frame.OffsetToCopy(0,0);
}

BRect Layer::Frame(void)
{
	return _frame;
}

void Layer::PruneTree(void)
{
/*	// recursively deletes all children (and grandchildren, etc) of the passed layer
	// This is mostly used for server shutdown or deleting a workspace
	Layer *lay,*nextlay;

	lay=_topchild;
	_topchild=NULL;
	
	while(lay!=NULL)
	{
		if(lay->_topchild!=NULL)
		{
//			lay->_topchild->PruneTree();
			lay->PruneTree();
		}
		nextlay=lay->_lowersibling;
		lay->_lowersibling=NULL;
		delete lay;
		lay=nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
*/
}

Layer *Layer::FindLayer(int32 token)
{
/*	// recursive search for a layer based on its view token
	Layer *lay, *trylay;

	// Search child layers first
	for(lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->_view_token==token)
			return lay;
	}
	
	// Hmmm... not in this layer's children. Try lower descendants
	for(lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		trylay=lay->FindLayer(token);
		if(trylay)
			return trylay;
	}
	
	// Well, we got this far in the function, so apparently there is no match to be found
*/	return NULL;
}

// Tested
void Layer::Invalidate(BRegion region)
{
/*	int32 i;
	BRect r;

	// See if the region intersects with our current area
	if(region.Intersects(Bounds()) && _hidecount==0)
	{
		BRegion clippedreg(region);
		clippedreg.IntersectWith(_visible);
		if(clippedreg.CountRects()>0)
		{
			_is_dirty=true;
			if(_invalid)
				_invalid->Include(&clippedreg);
			else
				_invalid=new BRegion(clippedreg);
		}		
	}
	
	BRegion *reg;
	for(Layer *lay=_topchild;lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->_hidecount==0)
		{	
			reg=new BRegion(lay->ConvertFromParent(&region));
	
			for(i=0;i<reg->CountRects();i++)
			{
				r=reg->RectAt(i);
				if(_frame.Intersects(r))
					lay->Invalidate(r);
			}
	
			delete reg;
		}
	}
*/
}

// Tested
void Layer::Invalidate(BRect rect)
{
/*	// Make our own section dirty and pass it on to any children, if necessary....
	// YES, WE ARE SHARING DIRT! Mudpies anyone? :D
//	if(Frame().Intersects(rect) || Frame().Contains(rect))
	if(TestRectIntersection(Frame(),rect))
	{
		// Clip the rectangle to the _visible region of the layer
//		if(_visible->Intersects(rect))
		if(TestRegionIntersection(_visible,rect))
		{
//			BRegion reg(rect);
//			reg.IntersectWith(_visible);
			BRegion reg(*_visible);
			IntersectRegionWith(&reg,rect);
			if(reg.CountRects()>0)
			{
				_is_dirty=true;
				if(_invalid)
					_invalid->Include(&reg);
				else
					_invalid=new BRegion(reg);
			}
			else
			{
			}
		}
		else
		{
		}
	}	
	for(Layer *lay=_topchild;lay!=NULL; lay=lay->_lowersibling)
		lay->Invalidate(lay->ConvertFromParent(rect));
*/
}

void Layer::RequestDraw(const BRect &r)
{
	// TODO: Implement and fix
/*	if(_visible==NULL || _hidecount>0)
		return;

	if(_serverwin)
	{
		if(_invalid==NULL)
			_invalid=new BRegion(*_visible);
		_serverwin->RequestDraw(_invalid->Frame());
		delete _invalid;
		_invalid=NULL;
	}

	_is_dirty=false;
	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw();
	}
*/
}

bool Layer::IsDirty(void) const
{
//	return _is_dirty;
	return (!_invalid)?true:false;
}

void Layer::Show(void)
{
/*	if(_hidecount==0)
		return;

	_hidecount--;
	if(_hidecount==0)
	{
		BRegion *reg=new BRegion(ConvertToParent(_visible));
		_parent->_visible->Exclude(reg);
		delete reg;
		_is_dirty=true;
	}
	
	Layer *child;
	for(child=_topchild; child!=NULL; child=child->_lowersibling)
		child->Show();
*/
}

void Layer::Hide(void)
{
/*	if(_hidecount==0)
	{
		BRegion *reg=new BRegion(ConvertToParent(_visible));
		_parent->_visible->Include(reg);
		delete reg;
		_parent->_is_dirty=true;
		_is_dirty=true;
	}
	_hidecount++;
	
	Layer *child;
	for(child=_topchild; child!=NULL; child=child->_lowersibling)
		child->Hide();
*/
}

// Tested
uint32 Layer::CountChildren(void)
{
/*	uint32 i=0;
	Layer *lay=_topchild;
	while(lay!=NULL)
	{
		lay=lay->_lowersibling;
		i++;
	}
	return i;
*/	return 0;
}

void Layer::MoveBy(float x, float y)
{
/*	BRect oldframe(_frame);
	_frame.OffsetBy(x,y);

	if(_parent)
	{
		if(_parent->_invalid==NULL)
			_parent->_invalid=new BRegion(oldframe);
		else
			_parent->_invalid->Include(oldframe);
	}

//	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
//		lay->MoveBy(x,y);
//	Invalidate(Frame());
*/
}

void Layer::ResizeBy(float x, float y)
{
/*	BRect oldframe=_frame;
	_frame.right+=x;
	_frame.bottom+=y;

	if(_parent)
		_parent->RebuildRegions();
	else
		RebuildRegions();

//	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
//		lay->ResizeBy(x,y);
	if(x<0 || y<0)
		_parent->Invalidate(oldframe);
//	Invalidate(Frame());
*/
}

void Layer::RebuildRegions(bool include_children=true)
{
/*	BRegion *reg,*reg2;
	if(_full)
		_full->Include(Bounds());
	else
		_full=new BRegion(Bounds());

	if(_visible)
		_visible->Include(Bounds());
	else
		_visible=new BRegion(Bounds());

	// Remove child footprints from _visible region
	for(Layer *childlay=_topchild; childlay!=NULL; childlay=childlay->_lowersibling)
	{
		if(childlay->_visible && childlay->_hidecount==0)
		{
			// reg is what is _visible in the layer's _parent's coordinate system
			reg=new BRegion(ConvertToParent(_visible));
			// reg2 is the layer's _visible region in the sibling's coordinate system
			reg2=new BRegion(childlay->ConvertFromParent(reg));
			delete reg;

			// lowersiblings occupy screen space _above_ a layer, so the layer itself
			// must remove from its _visible region 
			_visible->Exclude(reg2);
			delete reg2;
		}
	}

	// Remove _lowersibling footprints, which are on top of the layer on screen
	for(Layer *siblay=_lowersibling; siblay!=NULL; siblay=siblay->_lowersibling)
	{
		if(_frame.Intersects(siblay->_frame))
		{
			if(siblay->_visible && siblay->_hidecount==0)
			{
				// reg is what is _visible in the layer's _parent's coordinate system
				reg=new BRegion(ConvertToParent(_visible));
				// reg2 is the layer's _visible region in the sibling's coordinate system
				reg2=new BRegion(siblay->ConvertFromParent(reg));
				delete reg;

				// lowersiblings occupy screen space _above_ a layer, so the layer itself
				// must remove from its _visible region 
				_visible->Exclude(reg2);
				delete reg2;
			}
		}
	}

	if(include_children)
	{
		for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
		{
			if(lay->_topchild)
				lay->RebuildRegions(true);
		}
	}
*/
}

void Layer::PrintToStream(void)
{
	printf("-----------\nLayer %s\n",_name->String());
	if(_parent)
		printf("Parent: %s (%p)\n",_parent->_name->String(), _parent);
	else
		printf("Parent: NULL\n");
	if(_uppersibling)
		printf("Upper sibling: %s (%p)\n",_uppersibling->_name->String(), _uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(_lowersibling)
		printf("Lower sibling: %s (%p)\n",_lowersibling->_name->String(), _lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(_topchild)
		printf("Top child: %s (%p)\n",_topchild->_name->String(), _topchild);
	else
		printf("Top child: NULL\n");
	if(_bottomchild)
		printf("Bottom child: %s (%p)\n",_bottomchild->_name->String(), _bottomchild);
	else
		printf("Bottom child: NULL\n");
	printf("Frame: "); _frame.PrintToStream();
	printf("Token: %ld\nLevel: %ld\n",_view_token, _level);
	printf("Hide count: %u\n",_hidecount);
	if(_invalid)
	{
		printf("Invalid Areas: "); _invalid->PrintToStream();
	}
	else
		printf("Invalid Areas: NULL\n");
	if(_visible)
	{
		printf("Visible Areas: "); _visible->PrintToStream();
	}
	else
		printf("Visible Areas: NULL\n");
	printf("Is updating = %s\n",(_is_updating)?"yes":"no");
}

void Layer::PrintNode(void)
{
	printf("-----------\nLayer %s\n",_name->String());
	if(_parent)
		printf("Parent: %s (%p)\n",_parent->_name->String(), _parent);
	else
		printf("Parent: NULL\n");
	if(_uppersibling)
		printf("Upper sibling: %s (%p)\n",_uppersibling->_name->String(), _uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(_lowersibling)
		printf("Lower sibling: %s (%p)\n",_lowersibling->_name->String(), _lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(_topchild)
		printf("Top child: %s (%p)\n",_topchild->_name->String(), _topchild);
	else
		printf("Top child: NULL\n");
	if(_bottomchild)
		printf("Bottom child: %s (%p)\n",_bottomchild->_name->String(), _bottomchild);
	else
		printf("Bottom child: NULL\n");
	if(_visible)
	{
		printf("Visible Areas: "); _visible->PrintToStream();
	}
	else
		printf("Visible Areas: NULL\n");
}

// Tested
BRect Layer::ConvertToParent(BRect rect)
{
	return (rect.OffsetByCopy(_frame.LeftTop()));
}

// Tested
BRegion Layer::ConvertToParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertToParent(reg->RectAt(i)));
	return BRegion(newreg);
}

// Tested
BRect Layer::ConvertFromParent(BRect rect)
{
	return (rect.OffsetByCopy(_frame.left*-1,_frame.top*-1));
}

// Tested
BRegion Layer::ConvertFromParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromParent(reg->RectAt(i)));
	return BRegion(newreg);
}

// Tested
BRegion Layer::ConvertToTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertToTop(reg->RectAt(i)));
	return BRegion(newreg);
}

// Tested
BRect Layer::ConvertToTop(BRect rect)
{
	if (_parent!=NULL)
		return(_parent->ConvertToTop(rect.OffsetByCopy(_frame.LeftTop())) );
	else
		return(rect);
}

// Tested
BRegion Layer::ConvertFromTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromTop(reg->RectAt(i)));
	return BRegion(newreg);
}

// Tested
BRect Layer::ConvertFromTop(BRect rect)
{
	if (_parent!=NULL)
		return(_parent->ConvertFromTop(rect.OffsetByCopy(_frame.LeftTop().x*-1,
			_frame.LeftTop().y*-1)) );
	else
		return(rect);
}
