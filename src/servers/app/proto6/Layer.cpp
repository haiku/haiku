/*
	Layer.cpp
		Class used for rendering to the frame buffer. One layer per view on screen and
		also for window decorators
*/
#include "Layer.h"
#include "ServerWindow.h"
#include "Desktop.h"
#include "PortLink.h"
#include "DisplayDriver.h"
#include <iostream.h>
#include <Debug.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>

//#define DEBUG_LAYERS

Layer::Layer(BRect rect, const char *layername, ServerWindow *srvwin,
	int32 viewflags, int32 token)
{
	if(rect.IsValid())
		// frame is in parent layer's coordinates
		frame=rect;
	else
	{
#ifdef DEBUG_LAYERS
printf("Invalid BRect: "); rect.PrintToStream();
#endif
		frame.Set(0,0,1,1);
	}

	name=new BString(layername);

	// Layer does not start out as a part of the tree
	parent=NULL;
	uppersibling=NULL;
	lowersibling=NULL;
	topchild=NULL;
	bottomchild=NULL;

	visible=new BRegion(Bounds());
	full=new BRegion(Bounds());
	invalid=NULL;

	serverwin=srvwin;
	
	view_token=token;

	flags=viewflags;

	hidecount=0;
	is_dirty=false;
	is_updating=false;

	// Because this is not part of the tree, level is negative (thus, irrelevant)
	level=0;
#ifdef DEBUG_LAYERS
	PrintToStream();
#endif
}

Layer::Layer(BRect rect, const char *layername)
{
	// This constructor is used for the top layer in each workspace
	if(rect.IsValid())
		frame=rect;
	else
		frame.Set(0,0,1,1);

	name=new BString(layername);

	// Layer is the root for the workspace
	parent=NULL;
	uppersibling=NULL;
	lowersibling=NULL;
	topchild=NULL;
	bottomchild=NULL;

	visible=new BRegion(Bounds());
	full=new BRegion(Bounds());
	invalid=NULL;

	serverwin=NULL;
	flags=0;
	hidecount=0;
	is_dirty=false;

	level=0;

	view_token=-1;
#ifdef DEBUG_LAYERS
	PrintToStream();
#endif
}

Layer::~Layer(void)
{
#ifdef DEBUG_LAYERS
printf("Layer Destructor for %s\n",name->String());
#endif
	if(visible)
	{
		delete visible;
		visible=NULL;
	}
	if(full)
	{
		delete full;
		full=NULL;
	}
	if(invalid)
	{
		delete invalid;
		invalid=NULL;
	}
	if(name)
	{
		delete name;
		name=NULL;
	}
}

// Tested for adding children with 1 and 2 child cases, but not more
void Layer::AddChild(Layer *layer)
{
	// Adds a layer to the top of the layer's children
#ifdef DEBUG_LAYERS
printf("AddChild %s\n",layer->name->String());
printf("Before AddChild: \n");
PrintNode();
if(parent)
	parent->PrintNode();
if(uppersibling)
	uppersibling->PrintNode();
if(lowersibling)
	lowersibling->PrintNode();
if(topchild)
	topchild->PrintNode();
if(bottomchild)
	bottomchild->PrintNode();
#endif


	if(layer->parent!=NULL)
	{
		printf("ERROR: AddChild(): View already has a parent\n");
		return;
	}
	layer->parent=this;
	if(layer->visible && layer->hidecount==0 && visible)
	{
		// Technically, we could safely take the address of ConvertToParent(BRegion)
		// but we don't just to avoid a compiler nag
		
		BRegion *reg=new BRegion(layer->ConvertToParent(layer->visible));
		visible->Exclude(reg);
		delete reg;
	}

	if(topchild!=NULL)
	{
		layer->lowersibling=topchild;
		topchild->uppersibling=layer;
		if(layer->frame.Intersects(layer->lowersibling->frame))
		{
			if(layer->lowersibling->visible && layer->lowersibling->hidecount==0)
			{
				BRegion *reg=new BRegion(ConvertToParent(layer->visible));
				BRegion *reg2=new BRegion(layer->lowersibling->ConvertFromParent(reg));
				delete reg;
				layer->lowersibling->visible->Exclude(reg2);
				delete reg2;
			}
		}
	}
	else
		bottomchild=layer;
	topchild=layer;
	layer->level=level+1;

#ifdef DEBUG_LAYERS
printf("After AddChild: \n");
PrintNode();
if(parent)
	parent->PrintNode();
if(uppersibling)
	uppersibling->PrintNode();
if(lowersibling)
	lowersibling->PrintNode();
if(topchild)
	topchild->PrintNode();
if(bottomchild)
	bottomchild->PrintNode();
#endif
}

// Tested for cases with 1 and 2 children, but no more than that
void Layer::RemoveChild(Layer *layer)
{
	// Remove a layer from the tree
#ifdef DEBUG_LAYERS
printf("RemoveChild %s\n",layer->name->String());
printf("Before RemoveChild: \n");
PrintNode();
if(parent)
	parent->PrintNode();
if(uppersibling)
	uppersibling->PrintNode();
if(lowersibling)
	lowersibling->PrintNode();
if(topchild)
	topchild->PrintNode();
if(bottomchild)
	bottomchild->PrintNode();
#endif
	
	if(layer->parent==NULL)
	{
		printf("ERROR: RemoveChild(): View doesn't have a parent\n");
		return;
	}
	if(layer->parent!=this)
	{
		printf("ERROR: RemoveChild(): View is not a child of this layer\n");
		return;
	}

	if(hidecount==0 && layer->visible && layer->parent->visible)
	{
		BRegion *reg=new BRegion(ConvertToParent(visible));
		layer->parent->visible->Include(reg);
		delete reg;
	}

	// Take care of parent
	layer->parent=NULL;
	if(topchild==layer)
		topchild=layer->lowersibling;
	if(bottomchild==layer)
		bottomchild=layer->uppersibling;

	// Take care of siblings
	if(layer->uppersibling!=NULL)
		layer->uppersibling->lowersibling=layer->lowersibling;
	if(layer->lowersibling!=NULL)
		layer->lowersibling->uppersibling=layer->uppersibling;
	layer->SetLevel(0);
	layer->uppersibling=NULL;
	layer->lowersibling=NULL;

#ifdef DEBUG_LAYERS
printf("After RemoveChild: \n");
PrintNode();
if(parent)
	parent->PrintNode();
if(uppersibling)
	uppersibling->PrintNode();
if(lowersibling)
	lowersibling->PrintNode();
if(topchild)
	topchild->PrintNode();
if(bottomchild)
	bottomchild->PrintNode();
#endif
}

void Layer::RemoveSelf(void)
{
	// A Layer removes itself from the tree (duh)
#ifdef DEBUG_LAYERS
	cout << "RemoveSelf " << name->String() << endl;
#endif
	if(parent==NULL)
	{
		cout << "ERROR: RemoveSelf(): View doesn't have a parent\n" << flush;
		return;
	}
	parent->RemoveChild(this);
}

Layer *Layer::GetChildAt(BPoint pt, bool recursive=false)
{
	// Find out which child gets hit if we click at a certain spot. Returns NULL
	// if there are no visible children or if the click does not hit a child layer
	// If recursive==true, then it will continue to call until it reaches a layer
	// which has no children, i.e. a layer that is at the top of its 'branch' in
	// the layer tree
	
	Layer *child;
	if(recursive)
	{
		for(child=bottomchild; child!=NULL; child=child->uppersibling)
		{
			if(child->bottomchild!=NULL)
				child->GetChildAt(pt,true);
			
			if(child->hidecount>0)
				continue;
			
			if(child->frame.Contains(pt))
				return child;
		}
	}
	else
	{
		for(child=bottomchild; child!=NULL; child=child->uppersibling)
		{
			if(child->hidecount>0)
				continue;
//			if(child->visible && child->visible->Contains(ConvertFromTop(pt)))
//				printf("child hit by mouse. News at 11\n");
			if(child->frame.Contains(pt))
				return child;
		}
	}
	return NULL;
}

BRect Layer::Bounds(void)
{
	return frame.OffsetToCopy(0,0);
}

BRect Layer::Frame(void)
{
	return frame;
}

void Layer::SetLevel(int32 value)
{
	// Sets hierarchy level of layer and all children
#ifdef DEBUG_LAYERS
	printf("SetLevel %ld\n",value);
#endif
	level=value;

	Layer *lay;

	lay=topchild;

	while(lay!=NULL)
	{
		if(lay->topchild!=NULL)
			lay->topchild->SetLevel(value+1);
		lay=lay->lowersibling;
	}
}

void Layer::PruneTree(void)
{
	// recursively deletes all children (and grandchildren, etc) of the passed layer
	// This is mostly used for server shutdown or deleting a workspace
#ifdef DEBUG_LAYERS
	printf("PruneTree() at level %ld\n", level);
#endif
	Layer *lay,*nextlay;

	lay=topchild;
	topchild=NULL;
	
	while(lay!=NULL)
	{
		if(lay->topchild!=NULL)
		{
//			lay->topchild->PruneTree();
			lay->PruneTree();
		}
		nextlay=lay->lowersibling;
		lay->lowersibling=NULL;
		delete lay;
		lay=nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
}

Layer *Layer::FindLayer(int32 token)
{
	// recursive search for a layer based on its view token
	Layer *lay, *trylay;

	// Search child layers first
	for(lay=topchild; lay!=NULL; lay=lay->lowersibling)
	{
		if(lay->view_token==token)
			return lay;
	}
	
	// Hmmm... not in this layer's children. Try lower descendants
	for(lay=topchild; lay!=NULL; lay=lay->lowersibling)
	{
		trylay=lay->FindLayer(token);
		if(trylay)
			return trylay;
	}
	
	// Well, we got this far in the function, so apparently there is no match to be found
	return NULL;
}

// Tested
void Layer::Invalidate(BRegion region)
{
	int32 i;
	BRect r;

	// See if the region intersects with our current area
	if(region.Intersects(Bounds()) && hidecount==0)
	{
		BRegion clippedreg(region);
		clippedreg.IntersectWith(visible);
		if(clippedreg.CountRects()>0)
		{
			is_dirty=true;
			if(invalid)
				invalid->Include(&clippedreg);
			else
				invalid=new BRegion(clippedreg);
		}		
	}
	
	BRegion *reg;
	for(Layer *lay=topchild;lay!=NULL; lay=lay->lowersibling)
	{
		if(lay->hidecount==0)
		{	
			reg=new BRegion(lay->ConvertFromParent(&region));
	
			for(i=0;i<reg->CountRects();i++)
			{
				r=reg->RectAt(i);
				if(frame.Intersects(r))
					lay->Invalidate(r);
			}
	
			delete reg;
		}
	}
}

// Tested
void Layer::Invalidate(BRect rect)
{
	// Make our own section dirty and pass it on to any children, if necessary....
	// YES, WE ARE SHARING DIRT! Mudpies anyone? :D

	if(Frame().Intersects(rect))
	{
		
		// Clip the rectangle to the visible region of the layer
		if(visible->Intersects(rect))
		{
			BRegion reg(rect);
			reg.IntersectWith(visible);
			if(reg.CountRects()>0)
			{
				is_dirty=true;
				if(invalid)
					invalid->Include(&reg);
				else
					invalid=new BRegion(reg);
			}
		}
	}	
	for(Layer *lay=topchild;lay!=NULL; lay=lay->lowersibling)
		lay->Invalidate(lay->ConvertFromParent(rect));
}

void Layer::RequestDraw(void)
{
	if(visible==NULL || hidecount>0)
		return;

	if(serverwin)
	{
		if(invalid==NULL)
			invalid=new BRegion(*visible);
		serverwin->RequestDraw(invalid->Frame());
		delete invalid;
		invalid=NULL;
	}

	is_dirty=false;
	for(Layer *lay=topchild; lay!=NULL; lay=lay->lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw();
	}

}

bool Layer::IsDirty(void) const
{
//	return is_dirty;
	return (!invalid)?true:false;
}

void Layer::ShowLayer(void)
{
	if(hidecount==0)
		return;

	hidecount--;
	if(hidecount==0)
	{
		BRegion *reg=new BRegion(ConvertToParent(visible));
		parent->visible->Exclude(reg);
		delete reg;
		is_dirty=true;
	}
	
	Layer *child;
	for(child=topchild; child!=NULL; child=child->lowersibling)
		child->ShowLayer();
}

void Layer::HideLayer(void)
{
	if(hidecount==0)
	{
		BRegion *reg=new BRegion(ConvertToParent(visible));
		parent->visible->Include(reg);
		delete reg;
		parent->is_dirty=true;
		is_dirty=true;
	}
	hidecount++;
	
	Layer *child;
	for(child=topchild; child!=NULL; child=child->lowersibling)
		child->HideLayer();
}

// Tested
uint32 Layer::CountChildren(void)
{
	uint32 i=0;
	Layer *lay=topchild;
	while(lay!=NULL)
	{
		lay=lay->lowersibling;
		i++;
	}
	return i;
}

void Layer::MoveBy(float x, float y)
{
	BRect oldframe(frame);
	frame.OffsetBy(x,y);

	if(parent)
	{
		if(parent->invalid==NULL)
			parent->invalid=new BRegion(oldframe);
		else
			parent->invalid->Include(oldframe);
	}

//	for(Layer *lay=topchild; lay!=NULL; lay=lay->lowersibling)
//		lay->MoveBy(x,y);
	Invalidate(Frame());
}

void Layer::ResizeBy(float x, float y)
{
	BRect oldframe=frame;
	frame.right+=x;
	frame.bottom+=y;

	if(parent)
		parent->RebuildRegions();
	else
		RebuildRegions();

//	for(Layer *lay=topchild; lay!=NULL; lay=lay->lowersibling)
//		lay->ResizeBy(x,y);
	if(x<0 || y<0)
		parent->Invalidate(oldframe);
	Invalidate(Bounds());
}

void Layer::RebuildRegions(bool include_children=true)
{
	if(full)
		full->Include(Bounds());
	else
		full=new BRegion(Bounds());

	if(visible)
		visible->Include(Bounds());
	else
		visible=new BRegion(Bounds());

	// Remove child footprints from visible region here

	if(include_children)
	{
		for(Layer *lay=topchild; lay!=NULL; lay=lay->lowersibling)
		{
			if(lay->topchild)
				lay->RebuildRegions(true);
		}
	}
}

void Layer::PrintToStream(void)
{
	printf("-----------\nLayer %s\n",name->String());
	if(parent)
		printf("Parent: %s (%p)\n",parent->name->String(), parent);
	else
		printf("Parent: NULL\n");
	if(uppersibling)
		printf("Upper sibling: %s (%p)\n",uppersibling->name->String(), uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(lowersibling)
		printf("Lower sibling: %s (%p)\n",lowersibling->name->String(), lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(topchild)
		printf("Top child: %s (%p)\n",topchild->name->String(), topchild);
	else
		printf("Top child: NULL\n");
	if(bottomchild)
		printf("Bottom child: %s (%p)\n",bottomchild->name->String(), bottomchild);
	else
		printf("Bottom child: NULL\n");
	printf("Frame: "); frame.PrintToStream();
	printf("Token: %ld\nLevel: %ld\n",view_token, level);
	printf("Hide count: %u\n",hidecount);
	if(invalid)
	{
		printf("Invalid Areas: "); invalid->PrintToStream();
	}
	else
		printf("Invalid Areas: NULL\n");
	if(visible)
	{
		printf("Visible Areas: "); visible->PrintToStream();
	}
	else
		printf("Visible Areas: NULL\n");
	printf("Is updating = %s\n",(is_updating)?"yes":"no");
}

void Layer::PrintNode(void)
{
	printf("-----------\nLayer %s\n",name->String());
	if(parent)
		printf("Parent: %s (%p)\n",parent->name->String(), parent);
	else
		printf("Parent: NULL\n");
	if(uppersibling)
		printf("Upper sibling: %s (%p)\n",uppersibling->name->String(), uppersibling);
	else
		printf("Upper sibling: NULL\n");
	if(lowersibling)
		printf("Lower sibling: %s (%p)\n",lowersibling->name->String(), lowersibling);
	else
		printf("Lower sibling: NULL\n");
	if(topchild)
		printf("Top child: %s (%p)\n",topchild->name->String(), topchild);
	else
		printf("Top child: NULL\n");
	if(bottomchild)
		printf("Bottom child: %s (%p)\n",bottomchild->name->String(), bottomchild);
	else
		printf("Bottom child: NULL\n");
}

// Tested
BRect Layer::ConvertToParent(BRect rect)
{
	return (rect.OffsetByCopy(frame.LeftTop()));
}

// Tested
BPoint Layer::ConvertToParent(BPoint point)
{
	float x=point.x + frame.left,
		y=point.y+frame.top;
	return (BPoint(x,y));
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
	return (rect.OffsetByCopy(frame.left*-1,frame.top*-1));
}

// Tested
BPoint Layer::ConvertFromParent(BPoint point)
{
	return ( point-frame.LeftTop());
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
	if (parent!=NULL)
		return(parent->ConvertToTop(rect.OffsetByCopy(frame.LeftTop())) );
	else
		return(rect);
}

// Tested
BPoint Layer::ConvertToTop(BPoint point)
{
	if (parent!=NULL)
		return(parent->ConvertToTop(point + frame.LeftTop()) );
	else
		return(point);
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
	if (parent!=NULL)
		return(parent->ConvertFromTop(rect.OffsetByCopy(frame.LeftTop().x*-1,
			frame.LeftTop().y*-1)) );
	else
		return(rect);
}

// Tested
BPoint Layer::ConvertFromTop(BPoint point)
{
	if (parent!=NULL)
		return(parent->ConvertFromTop(point - frame.LeftTop()) );
	else
		return(point);
}

RootLayer::RootLayer(BRect rect, const char *layername, ServerWindow *srvwin, 
	int32 viewflags, int32 token)
	: Layer(rect,layername,srvwin,viewflags,token)
{
	updater_id=-1;
}

RootLayer::RootLayer(BRect rect, const char *layername)
	: Layer(rect,layername)
{
	updater_id=-1;
	driver=get_gfxdriver();
}

RootLayer::~RootLayer(void)
{
}

void RootLayer::SetVisible(bool fVisible)
{
	if(is_visible!=fVisible)
	{
		is_visible=fVisible;
/*		if(visible)
		{
			updater_id=spawn_thread(UpdaterThread,name->String(),B_NORMAL_PRIORITY,this);
			if(updater_id!=B_NO_MORE_THREADS && updater_id!=B_NO_MEMORY)
				resume_thread(updater_id);
		}
*/
	}
}

bool RootLayer::IsVisible(void) const
{
	return is_visible;
}

int32 RootLayer::UpdaterThread(void *data)
{
	// Updater thread which checks to see if its layer needs updating. If so, then
	// call the recursive function RequestDraw.
	
	RootLayer *root=(RootLayer*)data;

	while(1)
	{
		if(!root->is_visible)
		{
			return 0;
			exit_thread(1);
		}

		if(root->IsDirty())
		{
			layerlock->Lock();
			root->RequestDraw();
			layerlock->Unlock();
		}
	}
}

void RootLayer::RequestDraw(void)
{
	if(!invalid)
		return;
	
	ASSERT(driver!=NULL);

#ifdef DEBUG_LAYERS
printf("Root::RequestDraw: invalid rects: %ld\n",invalid->CountRects());
#endif

	// Redraw the base
	for(int32 i=0; invalid->CountRects();i++)
	{
		if(invalid->RectAt(i).IsValid())
		{
#ifdef DEBUG_LAYERS
printf("Root::RequestDraw:FillRect, color %u,%u,%u,%u\n",bgcolor.red,bgcolor.green,
	bgcolor.blue,bgcolor.alpha);
#endif
			driver->FillRect(invalid->RectAt(i),bgcolor);
		}
		else
			break;
	}

	delete invalid;
	invalid=NULL;
	is_dirty=false;

	// force redraw of all dirty windows	
	for(Layer *lay=topchild; lay!=NULL; lay=lay->lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw();
	}

}

void RootLayer::SetColor(rgb_color col)
{
	bgcolor=col;
#ifdef DEBUG_LAYERS
printf("Root::SetColor(%u,%u,%u,%u)\n",bgcolor.red,bgcolor.green,
	bgcolor.blue,bgcolor.alpha);
#endif
}

rgb_color RootLayer::GetColor(void) const
{	
	return bgcolor;
}
