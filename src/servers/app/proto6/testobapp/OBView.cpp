#include <stdio.h>
#include <Region.h>
#include "PortLink.h"
#include "ServerProtocol.h"
#include "OBView.h"
#include "OBWindow.h"

//#define DEBUG_VIEW

#ifndef SET_RGB_COLOR
	#define SET_RGB_COLOR(a,b,c,d,e) a.red=b; a.green=c; a.blue=d; a.alpha=e;
#endif

OBView::OBView(BRect frame,const char *name,uint32 resizeMask,uint32 flags)
	: BHandler(name)
{
	viewflags=flags;
	resizeflags=resizeMask;
	origin_h=0.0;
	origin_v=0.0;

	owner=NULL;
	parent=NULL;
	uppersibling=NULL;
	lowersibling=NULL;
	topchild=NULL;
	bottomchild=NULL;
	hidelevel=0;

	// This will be changed when added to a BWindow as the top view
	topview=false;

	SET_RGB_COLOR(highcol,0,0,0,255);
	SET_RGB_COLOR(lowcol,255,255,255,255);
	SET_RGB_COLOR(viewcol,255,255,255,255);

	// We need to align to the nearest pixel, rounded down
	// There's probably a better way to do this, but it'll work for now...
	vframe.left=(int32)frame.left;
	vframe.top=(int32)frame.top;
	vframe.right=(int32)frame.right;
	vframe.bottom=(int32)frame.bottom;
}

OBView::~OBView()
{
	// Be Book says it's an error to delete while attached to a window.
	// We'll just prevent that problem...
	if(owner)
	{
		// No, this isn't an error to utilize parent and not owner. We 
		// aren't actually attached to the window class itself. We're
		// attached to a view which is a member of the window.
		parent->RemoveChild(this);
	}
	

	OBView *v;
	for(v=topchild; v!=NULL; v=v->lowersibling)
		delete v;
}

void OBView::AttachedToWindow()
{
	// Hook function
}

void OBView::AllAttached()
{
	// Hook function
}

void OBView::DetachedFromWindow()
{
	// Hook function
}

void OBView::AllDetached()
{
	// Hook function
}

void OBView::MessageReceived(BMessage *msg)
{
	// Adds scripting support to the BHandler version. This is
	// only a skeleton implementation, so it has been skipped
	BHandler::MessageReceived(msg);
}

void OBView::AddChild(OBView *child, OBView *before = NULL)
{
	if(!child)
		return;
	if(child->parent || child->owner)
	{
		printf("%s::AddChild(): child already has a parent\n",Name());
		return;
	}
	if(child->parent==this)
	{
		printf("%s::AddChild(): child already belongs to view\n",Name());
		return;
	}
	
	// Adds a layer to the top of the layer's children
#ifdef DEBUG_VIEW
	printf("%s::AddChild(%s)\n", Name(), child->Name());
#endif
	
	child->parent=this;
	child->owner=owner;

	// View specified. Add new child above the specified view
	if(before)
	{
		if(before->parent==this)
		{
			child->lowersibling=before;
			child->uppersibling=before->uppersibling;
			before->uppersibling->lowersibling=child;
			before->uppersibling=child;

			// If we're attached to a window, notify all child views of
			// their attachment
			if(owner)
			{
				CallAttached(child);
				child->AllAttached();
			}
				
		}
		else
		{
			printf("%s::AddChild(): Can't add %s before %s - different parents\n", 
				Name(), child->Name(), before->Name());
		}
		return;
	}

	// If we haven't specified, child is added to bottom of list
	if(bottomchild!=NULL)
	{
		child->uppersibling=bottomchild;
		bottomchild->lowersibling=child;
	}
	bottomchild=child;

}

bool OBView::RemoveChild(OBView *child)
{
	if(!child)
		return false;

	if(child->parent!=this)
	{
		printf("%s::RemoveChild(): %s not child of this view\n",Name(),child->Name());
		return false;
	}
	
	// Adds a layer to the top of the layer's children
#ifdef DEBUG_VIEW
	printf("%s::RemoveChild(%s)\n", Name(), child->Name());
#endif

	if(owner)
	{
		CallDetached(child);
		
		// We only need to tell the server to prune the layer tree
		// at the child's level
		PortLink *link=new PortLink(owner->outport);
		link->SetOpCode(LAYER_DELETE);
		link->Attach(child->id);
		link->Flush();
		delete link;
	}

	// Note that we do the pointer reassignments last because the other
	// function calls made in RemoveChild() depend on these pointers	
	if(topchild==child)
		topchild=child->lowersibling;
	if(bottomchild==child)
		bottomchild=child->lowersibling;

	if(child->uppersibling!=NULL)
		child->uppersibling->lowersibling=child->lowersibling;
	if(child->lowersibling!=NULL)
		child->lowersibling->uppersibling=child->uppersibling;

	child->parent=NULL;
	child->owner=NULL;
	child->lowersibling=NULL;
	child->uppersibling=NULL;
	return true;
}

bool OBView::RemoveSelf()
{
	return false;
}

int32 OBView::CountChildren() const
{
	OBView *v=topchild;
	int32 children=0;
	
	while(v!=NULL)
	{
		v=v->lowersibling;
		children++;
	}
	
	return children;
}

OBView *OBView::ChildAt(int32 index) const
{
	// We're using a tree, so traverse the tree
	OBView *v=topchild;
	
	for(int32 i=0;i<index;i++)
	{
		if(v==NULL)
			return NULL;
		v=v->lowersibling;
	}
	
	return v;
}

OBView *OBView::NextSibling() const
{
	return lowersibling;
}

OBView *OBView::PreviousSibling() const
{
	return uppersibling;
}

OBWindow *OBView::Window() const
{
	return owner;
}

void OBView::Draw(BRect updateRect)
{
}

void OBView::MouseDown(BPoint where)
{
	// Hook function
}

void OBView::MouseUp(BPoint where)
{
	// Hook function
}

void OBView::MouseMoved(BPoint where,uint32 code,const BMessage *a_message)
{
	// Hook function
}

void OBView::WindowActivated(bool state)
{
	// Hook function
}

void OBView::KeyDown(const char *bytes, int32 numBytes)
{
	// Hook function
}

void OBView::KeyUp(const char *bytes, int32 numBytes)
{
	// Hook function
}

void OBView::Pulse()
{
	// Hook function
}

void OBView::FrameMoved(BPoint new_position)
{
	// Hook function
}

void OBView::FrameResized(float new_width, float new_height)
{
	// Hook function
}

void OBView::TargetedByScrollView(BScrollView *scroll_view)
{
	// Hook function
}

void OBView::BeginRectTracking(BRect startRect,uint32 style = B_TRACK_WHOLE_RECT)
{
	if(!owner)
	{
		printf("BeginRectTracking(): view must be attached to a window\n");
		return;
	}
	owner->serverlink->SetOpCode(BEGIN_RECT_TRACKING);
	owner->serverlink->Attach(&startRect, sizeof(BRect));
	owner->serverlink->Attach(&style, sizeof(uint32));
	owner->serverlink->Flush();
}

void OBView::EndRectTracking(void)
{
	if(!owner)
	{
		printf("EndRectTracking(): view must be attached to a window\n");
		return;
	}
	owner->serverlink->SetOpCode(END_RECT_TRACKING);
	owner->serverlink->Flush();
}

void OBView::GetMouse(BPoint *location, uint32 *buttons,
	bool checkMessageQueue=true)
{
}

OBView *OBView::FindView(const char *name)
{
	OBView *v;
	v=FindViewInChildren(name);

	return v;
}

OBView *OBView::Parent() const
{
	return parent;
}

BRect OBView::Bounds() const
{
	BRect rframe=vframe;
	rframe.OffsetTo(0,0);
	return rframe;
}

BRect OBView::Frame() const
{
	return vframe;
}

void OBView::ConvertToScreen(BPoint* pt) const
{
}

BPoint OBView::ConvertToScreen(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBView::ConvertFromScreen(BPoint* pt) const
{
}

BPoint OBView::ConvertFromScreen(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBView::ConvertToScreen(BRect *r) const
{
}

BRect OBView::ConvertToScreen(BRect r) const
{
	return BRect(0,0,0,0);
}

void OBView::ConvertFromScreen(BRect *r) const
{
}

BRect OBView::ConvertFromScreen(BRect r) const
{
	return BRect(0,0,0,0);
}

void OBView::ConvertToParent(BPoint *pt) const
{
}

BPoint OBView::ConvertToParent(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBView::ConvertFromParent(BPoint *pt) const
{
}

BPoint OBView::ConvertFromParent(BPoint pt) const
{
	return BPoint(-1,-1);
}

void OBView::ConvertToParent(BRect *r) const
{
}

BRect OBView::ConvertToParent(BRect r) const
{
	return BRect(0,0,0,0);
}

void OBView::ConvertFromParent(BRect *r) const
{
}

BRect OBView::ConvertFromParent(BRect r) const
{
	return BRect(0,0,0,0);
}

BPoint OBView::LeftTop() const
{
	return BPoint(-1,-1);
}

void OBView::GetClippingRegion(BRegion *region) const
{
}

void OBView::ConstrainClippingRegion(BRegion *region)
{
}

void OBView::SetDrawingMode(drawing_mode mode)
{
}

drawing_mode OBView::DrawingMode() const
{
	return B_OP_COPY;
}

void OBView::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
}

void OBView::GetBlendingMode(source_alpha *srcAlpha, alpha_function *alphaFunc) const
{
}

void OBView::SetPenSize(float size)
{
}

float OBView::PenSize() const
{
	return 0.0;
}

void OBView::SetViewColor(rgb_color c)
{
	SET_RGB_COLOR(viewcol,c.red,c.green,c.blue,c.alpha);
	owner->drawmsglink->Attach((int32)GFX_SET_VIEW_COLOR);
	owner->drawmsglink->Attach((int8)c.red);
	owner->drawmsglink->Attach((int8)c.green);
	owner->drawmsglink->Attach((int8)c.blue);
	owner->drawmsglink->Attach((int8)c.alpha);
}

void OBView::SetViewColor(uchar r, uchar g, uchar b, uchar a=255)
{
	SET_RGB_COLOR(viewcol,r,g,b,a);
	owner->drawmsglink->Attach((int32)GFX_SET_VIEW_COLOR);
	owner->drawmsglink->Attach((int8)r);
	owner->drawmsglink->Attach((int8)g);
	owner->drawmsglink->Attach((int8)b);
	owner->drawmsglink->Attach((int8)a);
}

rgb_color OBView::ViewColor() const
{
	return viewcol;
}

void OBView::SetHighColor(rgb_color c)
{
	SET_RGB_COLOR(highcol,c.red,c.green,c.blue,c.alpha);
	owner->drawmsglink->Attach((int32)GFX_SET_HIGH_COLOR);
	owner->drawmsglink->Attach((int8)c.red);
	owner->drawmsglink->Attach((int8)c.green);
	owner->drawmsglink->Attach((int8)c.blue);
	owner->drawmsglink->Attach((int8)c.alpha);
}

void OBView::SetHighColor(uchar r, uchar g, uchar b, uchar a=255)
{
	SET_RGB_COLOR(highcol,r,g,b,a);
	owner->drawmsglink->Attach((int32)GFX_SET_HIGH_COLOR);
	owner->drawmsglink->Attach((int8)r);
	owner->drawmsglink->Attach((int8)g);
	owner->drawmsglink->Attach((int8)b);
	owner->drawmsglink->Attach((int8)a);
}

rgb_color OBView::HighColor() const
{
	return highcol;
}

void OBView::SetLowColor(rgb_color c)
{
	SET_RGB_COLOR(lowcol,c.red,c.green,c.blue,c.alpha);
	owner->drawmsglink->Attach((int32)GFX_SET_LOW_COLOR);
	owner->drawmsglink->Attach((int8)c.red);
	owner->drawmsglink->Attach((int8)c.green);
	owner->drawmsglink->Attach((int8)c.blue);
	owner->drawmsglink->Attach((int8)c.alpha);
}

void OBView::SetLowColor(uchar r, uchar g, uchar b, uchar a=255)
{
	SET_RGB_COLOR(lowcol,r,g,b,a);
	owner->drawmsglink->Attach((int32)GFX_SET_LOW_COLOR);
	owner->drawmsglink->Attach((int8)r);
	owner->drawmsglink->Attach((int8)g);
	owner->drawmsglink->Attach((int8)b);
	owner->drawmsglink->Attach((int8)a);
}

rgb_color OBView::LowColor() const
{
	return lowcol;
}

void OBView::Invalidate(BRect invalRect)
{
	if(!owner)
	{
		printf("Invalidate(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach(invalRect);
}

void OBView::Invalidate(const BRegion *invalRegion)
{
	// undocumented function
}

void OBView::Invalidate()
{
	if(!owner)
	{
		printf("Invalidate(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach(Bounds());
}

void OBView::SetFlags(uint32 flags)
{
}

uint32 OBView::Flags() const
{
	return viewflags;
}

void OBView::SetResizingMode(uint32 mode)
{
}

uint32 OBView::ResizingMode() const
{
	return resizeflags;
}

void OBView::MoveBy(float dh, float dv)
{
}

void OBView::MoveTo(BPoint where)
{
}

void OBView::MoveTo(float x, float y)
{
}

void OBView::ResizeBy(float dh, float dv)
{
}

void OBView::ResizeTo(float width, float height)
{
}

void OBView::MakeFocus(bool focusState = true)
{
}

bool OBView::IsFocus() const
{
	return has_focus;
}

void OBView::Show()
{
	hidelevel--;
	
	for(OBView *v=topchild; v!=NULL; v=v->lowersibling )
		v->Show();
}

void OBView::Hide()
{
	hidelevel++;
	
	for(OBView *v=topchild; v!=NULL; v=v->lowersibling )
		v->Show();
}

bool OBView::IsHidden() const
{
	return (hidelevel>0)?true:false;
}

bool OBView::IsHidden(const OBView* looking_from) const
{
	// undocumented function
	return false;
}

void OBView::Flush() const
{
	// When attachment count inclusion is implemented, it needs to go
	// in this function

	owner->drawmsglink->SetOpCode(GFX_FLUSH);
	owner->drawmsglink->Flush();
}

void OBView::Sync() const
{
	// When attachment count inclusion is implemented, it needs to go
	// in this function

	int32 code;
	status_t status;
	ssize_t buffersize;

	owner->drawmsglink->SetOpCode(GFX_SYNC);

	// Reply received:

	// Code: SYNC
	// Attached data: none
	// Buffersize: 0
	owner->drawmsglink->FlushWithReply(&code,&status,&buffersize);
}

void OBView::StrokeRect(BRect r, pattern p=B_SOLID_HIGH)
{
	if(!owner)
	{
		printf("StrokeRect(): view must be attached to a window\n");
		return;
	}
	
	owner->drawmsglink->Attach((int32)GFX_STROKE_RECT);
	owner->drawmsglink->Attach(r);
	owner->drawmsglink->Attach(&p, sizeof(pattern));
}

void OBView::FillRect(BRect r, pattern p=B_SOLID_HIGH)
{
	if(!owner)
	{
		printf("FillRect(): view must be attached to a window\n");
		return;
	}
	
	owner->drawmsglink->Attach((int32)GFX_FILL_RECT);
	owner->drawmsglink->Attach(r);
	owner->drawmsglink->Attach(&p, sizeof(pattern));
}

void OBView::MovePenTo(BPoint pt)
{
	if(!owner)
	{
		printf("MovePenTo(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach((int32)GFX_MOVEPENTO);
	owner->drawmsglink->Attach(pt);
}

void OBView::MovePenTo(float x, float y)
{
	if(!owner)
	{
		printf("MovePenTo(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach((int32)GFX_MOVEPENTO);
	owner->drawmsglink->Attach(BPoint(x,y));
}

void OBView::MovePenBy(float x, float y)
{
	if(!owner)
	{
		printf("MovePenBy(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach((int32)GFX_MOVEPENBY);
	owner->drawmsglink->Attach(x);
	owner->drawmsglink->Attach(y);
}

BPoint OBView::PenLocation() const
{
	return BPoint(0,0);
}

void OBView::StrokeLine(BPoint toPt,pattern p=B_SOLID_HIGH)
{
	if(!owner)
	{
		printf("StrokeLine(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach((int32)GFX_STROKE_LINE);
	owner->drawmsglink->Attach(toPt);
	owner->drawmsglink->Attach((pattern *)&p, sizeof(pattern));
}

void OBView::StrokeLine(BPoint pt0,BPoint pt1,pattern p=B_SOLID_HIGH)
{
	if(!owner)
	{
		printf("StrokeLine(): view must be attached to a window\n");
		return;
	}
	owner->drawmsglink->Attach((int32)GFX_MOVEPENTO);
	owner->drawmsglink->Attach(pt0);
	
	owner->drawmsglink->Attach((int32)GFX_STROKE_LINE);
	owner->drawmsglink->Attach(pt1);
	owner->drawmsglink->Attach((pattern *)&p, sizeof(pattern));
}

// Protected members

int32 OBView::GetServerToken(void)
{
	// Function which asks the app_server for an ID value
	// NOTE that it requires an owner

	if(!owner)
	{
		printf("GetServerToken(): view must be attached to a window\n");
		return -1;
	}

	int32 code;
	status_t status;
	ssize_t buffersize;
	int32 *buffer;

	owner->serverlink->SetOpCode(VIEW_GET_TOKEN);

	// Reply received:

	// Code: GET_VIEW_TOKEN
	// Attached data:
	// 1) int32 view ID
	buffer=(int32*)owner->drawmsglink->FlushWithReply(&code,&status,&buffersize);
	id=*buffer;

	// necessary because each reply allocates memory from the heap for
	// the returned buffer
	delete buffer;

	return id;
}

void OBView::CallAttached(OBView *view)
{
	// Recursively calls AttachedToWindow() for all children of the view
	AddedToWindow();
	AttachedToWindow();
		
	for(OBView *v=topchild; v!=NULL; v=v->lowersibling)
	{
		if(v->topchild)
			CallAttached(v->topchild);
	}
}

void OBView::CallDetached(OBView *view)
{
	// Recursively calls DetachedFromWindow() for all children of the view
	DetachedFromWindow();
		
	for(OBView *v=topchild; v!=NULL; v=v->lowersibling)
	{
		if(v->topchild)
			CallDetached(v->topchild);
	}
}

void OBView::AddedToWindow(void)
{
	// Hook function called during AddChild to take care of setup in server
	// of adding a bunch of views at once, not just one

	// Theoretically, shouldn't be needed, but this is here in case I do
	// something really stupid. ;)
	if(!owner)
		return;

	// If we're finally attached to a window, then the child needs to become
	// known to the server

	// Create View message. Layers are app_server's view counterparts
	
	// Attach Data:
	// 1) (int32) id of the parent view
	// 2) (int32) id of the child view
	// 3) (BRect) frame in parent's coordinates
	// 4) (int32) resize flags
	// 5) (int32) view flags
	// 6) (uint16) view's hide level
	id=GetServerToken();

	PortLink *link=new PortLink(owner->outport);
	link->SetOpCode(LAYER_CREATE);
	link->Attach(parent->id);
	link->Attach(id);
	link->Attach(vframe);
	link->Attach((int32)resizeflags);
	link->Attach((int32)viewflags);
	link->Attach(&hidelevel,sizeof(uint16));
	link->Flush();
	delete link;
}

OBView *OBView::FindViewInChildren(const char *name)
{
	// Looks in all children of a view and returns the first view
	// with a matching name.
	
	// Process:
	// 1) Search this view's children starting with topchild
	// 2) If not found, cycle through them again, this second time
	//		recursively searching each child.
	// 3) If the view is found in the first child search, assign the **
	//		and return the *.
	// 4) If the view is found in the second child search, break out of the
	//		search loop by assigning and returning the *
	// 5) If we complete the loop, it means that the view was not found.
	//		We should, thus, return NULL
	return NULL;
}
