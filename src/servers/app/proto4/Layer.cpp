/*
	Layer.cpp:

*/
#include <string.h>
#include <iostream.h>
#include "ServerWindow.h"
#include "Layer.h"

Layer::Layer(ServerWindow *Win, const char *Name, Layer *Parent,
	BRect &Frame, int32 Flags)
{
	Initialize();
	flags=Flags;
	frame=Frame;
	if(Name!=NULL)
	{
		name=new char[strlen(Name)];
		cout << "Layer::Layer name=" << strcpy((char *)name, Name) << endl;
	}
	else
		name=NULL;
	if(Parent!=NULL)
		Parent->AddChild(this);
}

Layer::~Layer(void)
{
}

void Layer::Initialize(void)
{
	visible=NULL;
	full=NULL;
	update=NULL;
	high.red=255; high.green=255; high.blue=255; high.alpha=255;
	low.red=255; low.green=255; low.blue=255; low.alpha=255;
	erase.red=255; erase.green=255; erase.blue=255; erase.alpha=255;
	drawingmode=B_OP_COPY;
	parent=NULL;
	uppersibling=NULL;
	lowersibling=NULL;
	topchild=NULL;
	bottomchild=NULL;
	hidecount=0;
}

void Layer::AddChild(Layer *child, bool topmost=true)
{
	// If we don't have any children, simply add the thing.
	if (bottomchild==NULL && topchild==NULL)
	{
		bottomchild=child;
		topchild=child;
		child->lowersibling=NULL;
		child->uppersibling=NULL;
	}
	else
	{
		if (topmost)
		{
			// "Move" the top child if there is one already and add to the
			// top of the tree
			if (topchild!=NULL)
				topchild->uppersibling = child;

			topchild=child;
			child->uppersibling= NULL;
			child->lowersibling = topchild;
		}
		else
		{
			// Add layer to bottom of tree unless there are no other
			// children
			Layer *lay=bottomchild;
			if (lay!=NULL)
			{
				child->uppersibling=lay;
				child->lowersibling=lay->lowersibling;
				lay->lowersibling=child;

				if(child->lowersibling!=NULL)
					child->lowersibling->uppersibling=child;
				else
					bottomchild=child;
			}
			else
			{
				topchild=child;
				topchild->uppersibling=child;
				child->lowersibling=topchild;
				child->uppersibling=NULL;
			}
		}
	}
	child->parent = this;
	child->SetAddedData(hidecount);
}

void Layer::RemoveChild(Layer *child)
{
	if(child->parent!=this)
	{
		cout << "DEBUG: attempt to remove a non-child layer\n" << endl;
		return;
	}

    if (child==bottomchild)
		bottomchild=child->uppersibling;

    if (child==bottomchild)
		bottomchild=child->uppersibling;

    if (child->lowersibling!=NULL)
		child->lowersibling->uppersibling=child->uppersibling;
    
    if (child->uppersibling!=NULL)
		child->uppersibling->lowersibling=child->lowersibling;

    child->parent=NULL;
    child->lowersibling=NULL;
    child->uppersibling=NULL;
}

void Layer::RemoveSelf(void)
{
	if (parent!=NULL)
		parent->RemoveChild(this);
}

void Layer::SetAddedData(uint8 count)
{
	hidecount+=count;
	Layer *lay;
    for (lay=topchild; lay!=NULL; lay=lay->lowersibling)
		lay->SetAddedData(count);
}

void Layer::Show(void)
{
	hidecount--;
	if(hidecount==0)
		SetDirty(true);
}

void Layer::Hide(void)
{
	if(parent==NULL)
	{
		cout << "DEBUG: attempt to hide an orphan layer" << endl;
		return;
	}
	if(hidecount==0)
	{
		// Invalidate any sibling layers when this gets hidden so that
		// we see what's left. This is a slower way to do things, but it
		// works
		Layer *sibling;
		for (sibling=parent->bottomchild; sibling!=NULL; sibling=sibling->uppersibling)
		{
			if (sibling->frame.Intersects(frame))
			{
				sibling->MarkModified(frame & sibling->frame);
			}
		}
		
		// Hide all child layers	
		Layer *child;
		for (child=topchild; child!=NULL; child=child->lowersibling)
		{
			child->Hide();
		}
	}
	hidecount++;
}

bool Layer::IsVisible(void)
{
	return (hidecount==0)?true:false;
}

void Layer::Update(bool ForceUpdate)
{
}

void Layer::SetFrame(const BRect &Rect)
{
}

BRect Layer::Frame(void)
{
	return frame;
}

BRect Layer::Bounds(void)
{
	BRect r=frame;
	r.OffsetTo(0,0);
	return r;
}

void Layer::Invalidate(const BRect &rect)
{
	if (hidecount==0)
	{
		if (update==NULL)
			update=new BRegion(rect);
		else
			update->Include(rect);
	}
}

// Invalidates the entire layer, possibly including the child layers
void Layer::Invalidate(bool recursive)
{
	if (hidecount==0)
	{
		delete update;
		update = new BRegion(frame);

		if (recursive)
		{
			for (Layer *lay=bottomchild; lay!=NULL; lay=lay->uppersibling)
				lay->Invalidate(true);
		}
	}
}

// This invalidates the layer and all its child layers
void Layer::SetDirty(bool flag)
{
	isdirty=flag;
	
	Layer *lay;
	for (lay=bottomchild; lay!=NULL; lay=lay->uppersibling )
	{
		lay->SetDirty(flag);
	}
}

void Layer::AddRegion(BRegion *Region)
{
}

void Layer::RemoveRegions(void)
{
/*	delete visible;
	delete full;
	delete update;
	
	visible=NULL;
	full=NULL;
	update=NULL;
	
	Layer *lay;
	for (lay=topchild; lay!=NULL; lay=lay->lowersibling )
		lay->DeleteRegions();
*/
}

void Layer::UpdateRegions(bool ForceUpdate=true, bool Root=true)
{
}

void Layer::MarkModified(BRect rect)
{
    if (frame.Intersects(rect))
    {
    	BRect r=rect & frame;
		isdirty=true;
    
		Layer* lay;
		for (lay=bottomchild; lay!=NULL; lay=lay->uppersibling )
		    lay->MarkModified(r);
    }
}

// virtual function to be defined by child classes
void Layer::Draw(const BRect &Rect)
{
}
