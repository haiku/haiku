#include <Region.h>
#include <String.h>
#include <Locker.h>
#include <Region.h>
#include <Debug.h>
#include "View.h"	// for mouse button defines
#include "ServerWindow.h"
#include "Decorator.h"
#include "DisplayDriver.h"
#include "Desktop.h"
#include "WindowBorder.h"

#define DEBUG_WINBORDER

#ifdef DEBUG_WINBORDER
#include <stdio.h>
#endif

bool is_moving_window=false;
bool is_resizing_window=false;
bool is_sliding_tab=false;
extern ServerWindow *active_serverwindow;

WindowBorder::WindowBorder(ServerWindow *win, const char *bordertitle)
 : Layer((win==NULL)?BRect(0,0,1,1):win->frame,
   (win==NULL)?NULL:win->title->String())
{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(%p, %s)\n",win,bordertitle);
#endif
	mbuttons=0;
	swin=win;
	if(swin)
		frame=swin->frame;

	mousepos.Set(0,0);
	update=false;

	title=new BString(bordertitle);
	hresizewin=false;
	vresizewin=false;
}

WindowBorder::~WindowBorder(void)
{
#ifdef DEBUG_WINBORDER
printf("~WindowBorder()\n");
#endif
	delete title;
}

void WindowBorder::MouseDown(BPoint pt, int32 buttons, int32 modifiers)
{
	mbuttons=buttons;
	kmodifiers=modifiers;
	click_type click=decor->Clicked(pt, mbuttons, kmodifiers);
	mousepos=pt;

	switch(click)
	{
		case CLICK_MOVETOBACK:
		{
			MoveToBack();
			break;
		}
		case CLICK_MOVETOFRONT:
		{
			MoveToFront();
			break;
		}
		case CLICK_CLOSE:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): SetClose\n");
#endif
			decor->SetClose(true);
			decor->Draw();
			break;
		}
		case CLICK_ZOOM:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): SetZoom\n");
#endif
			decor->SetZoom(true);
			decor->Draw();
			break;
		}
		case CLICK_MINIMIZE:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): SetMinimize\n");
#endif
			decor->SetMinimize(true);
			decor->Draw();
			break;
		}
		case CLICK_DRAG:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): Click Tab\n");
#endif
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
				is_moving_window=true;
			}
			if(buttons==B_SECONDARY_MOUSE_BUTTON)
				MoveToBack();
			break;
		}
		case CLICK_SLIDETAB:
		{
			is_sliding_tab=true;
			break;
		}
		case CLICK_RESIZE:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): Click Resize\n");
#endif
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
				is_resizing_window=true;
			}
			break;
		}
		case CLICK_NONE:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): Click None\n");
#endif
			break;
		}
		default:
		{
#ifdef DEBUG_WINBORDER
printf("WindowBorder()::MouseDown: Undefined click type\n");
#endif
			break;
		}
	}
	if(click!=CLICK_NONE)
		ActivateWindow(swin);
}

void WindowBorder::MouseMoved(BPoint pt, int32 buttons, int32 modifiers)
{
	mbuttons=buttons;
	kmodifiers=modifiers;
	click_type click=decor->Clicked(pt, mbuttons, kmodifiers);
	if(click!=CLICK_CLOSE && decor->GetClose())
	{
		decor->SetClose(false);
		decor->Draw();
	}	

	if(click!=CLICK_ZOOM && decor->GetZoom())
	{
		decor->SetZoom(false);
		decor->Draw();
	}	

	if(click!=CLICK_MINIMIZE && decor->GetMinimize())
	{
		decor->SetMinimize(false);
		decor->Draw();
	}	

	if(is_sliding_tab)
	{
		float dx=pt.x-mousepos.x;

		if(dx!=0)
		{
			// SlideTab returns how much things were moved, and currently
			// supports just the x direction, so get the value so
			// we can invalidate the proper area.
			decor->SlideTab(dx,0);
			BRegion r(frame),*footprint=decor->GetFootprint();
			parent->Invalidate(footprint->Frame());
			parent->RequestDraw();
			decor->DrawTab();
			delete footprint;
		}
	}
	if(is_moving_window)
	{
		float dx=pt.x-mousepos.x,
			dy=pt.y-mousepos.y;
		if(dx!=0 || dy!=0)
		{
			BRect oldmoveframe=swin->frame;
			clientframe.OffsetBy(pt);
	
			swin->Lock();
			swin->frame.OffsetBy(dx,dy);
			swin->Unlock();
			
			layerlock->Lock();
//			InvalidateLowerSiblings(oldmoveframe);
			parent->Invalidate(oldmoveframe);
			MoveBy(dx,dy);
			parent->RequestDraw();
			decor->MoveBy(BPoint(dx, dy));
			decor->Draw();
			layerlock->Unlock();
		}
	}
	if(is_resizing_window)
	{
		float dx=pt.x-mousepos.x,
			dy=pt.y-mousepos.y;
		if(dx!=0 || dy!=0)
		{
			clientframe.right+=dx;
			clientframe.bottom+=dy;
	
			swin->Lock();
			swin->frame.right+=dx;
			swin->frame.bottom+=dy;
			swin->Unlock();
	
			layerlock->Lock();
			ResizeBy(dx,dy);
			parent->RequestDraw();
			layerlock->Unlock();
			decor->ResizeBy(dx,dy);
			decor->Draw();
		}
	}
	mousepos=pt;
}

void WindowBorder::MouseUp(BPoint pt, int32 buttons, int32 modifiers)
{
//	mbuttons&= ~buttons;
	mbuttons=buttons;
	kmodifiers=modifiers;
	is_moving_window=false;
	is_resizing_window=false;
	is_sliding_tab=false;

	click_type click=decor->Clicked(pt, mbuttons, kmodifiers);

	switch(click)
	{
		case CLICK_CLOSE:
		{
			decor->SetClose(false);
			decor->Draw();
			
			// call close window stuff here
			
			break;
		}
		case CLICK_ZOOM:
		{
			decor->SetZoom(false);
			decor->Draw();
			
			// call zoom stuff here
			
			break;
		}
		case CLICK_MINIMIZE:
		{
			decor->SetMinimize(false);
			decor->Draw();
			
			// call minimize stuff here
			
		}
		default:
		{
			break;
		}
	}
}

void WindowBorder::Draw(BRect update_rect)
{
#ifdef DEBUG_WINBORDER
printf("WindowBorder()::Draw():"); update_rect.PrintToStream();
#endif

	if(update && visible!=NULL)
		is_updating=true;

	decor->Draw(update_rect);

	if(update && visible!=NULL)
		is_updating=false;
}

void WindowBorder::SetDecorator(Decorator *newdecor)
{
	// AtheOS kills the decorator here. However, the decor
	// under OBOS doesn't belong to the border - it belongs to the
	// ServerWindow, so the ServerWindow will handle all (de)allocation
	// tasks. We just need to update the pointer.
#ifdef DEBUG_WINBORDER
printf("WindowBorder::SetDecorator(%p)\n",newdecor);
#endif
	if(newdecor)
	{
		decor=newdecor;
		decor->SetTitle(title->String());
		decor->ResizeBy(0,0);
//		if(visible)
//			delete visible;
//		visible=decor->GetFootprint();
	}
}

ServerWindow *WindowBorder::Window(void) const
{
	return swin;
}
void WindowBorder::RequestDraw(void)
{
//printf("Layer %s::RequestDraw\n",name->String());
	if(invalid)
	{
//printf("drew something\n");
//		for(int32 i=0; i<invalid->CountRects();i++)
//			decor->Draw(ConvertToTop(invalid->RectAt(i)));
			decor->Draw();
		delete invalid;
		invalid=NULL;
		is_dirty=false;
	}

}

void WindowBorder::InvalidateLowerSiblings(BRect r)
{
	// this is used to update the other windows when a window border
	// is moved or resized.
	Layer *lay;
	BRect toprect=ConvertToTop(r);	
	
	for(lay=uppersibling;lay!=NULL;lay=lay->uppersibling)
	{
		lay->Invalidate(toprect);
		lay->RequestDraw();
	}

}

void WindowBorder::MoveToBack(void)
{
#ifdef DEBUG_WINBORDER
printf("WindowBorder(): MoveToBack\n");
#endif
	Layer *temp=NULL;
	if(uppersibling)
		temp=uppersibling;

	// Move the window to the back (the top of the tree)
	Layer *top=GetRootLayer();
	ASSERT(top!=NULL);
	layerlock->Lock();
	top->RemoveChild(this);
	top->AddChild(this);
	level=1;
	swin->SetFocus(false);
	decor->SetFocus(false);
	decor->Draw();

	if(temp)
		ActivateWindow(uppersibling->serverwin);
	layerlock->Unlock();
	parent->RebuildRegions(true);
}

void WindowBorder::MoveToFront(void)
{
#ifdef DEBUG_WINBORDER
printf("MoveToFront: \n");
#endif
	// Move the window to the front by making it the bottom
	// child of the root layer
	Layer *top=GetRootLayer();
	layerlock->Lock();

	top->RemoveChild(this);

	// Make this the bottom child of the parent layer. Can't use
	// AddChild(), so we'll manipulate the pointers directly. Remember,
	// we need to change pointers for 4 layers - the parent, the 
	// uppersibling (if any), the lowersibling, and the layer in question

	// temporary placeholder while we exchange the data
	Layer *templayer;

	// tweak parent	
	templayer=top->bottomchild;
	top->bottomchild=this;

	// Tweak this layer
	parent=top;
	uppersibling=templayer;
	uppersibling->lowersibling=this;
	lowersibling=NULL;

	layerlock->Unlock();
	is_moving_window=true;
	parent->RebuildRegions(true);
}
