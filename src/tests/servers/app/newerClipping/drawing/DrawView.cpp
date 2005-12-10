
#include <stdio.h>

#include <Message.h>

#include "Desktop.h"

#include "DrawView.h"

// constructor
DrawView::DrawView(BRect frame)
	: BView(frame, "desktop", B_FOLLOW_ALL, 0),
	  fDesktop(NULL)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// destructor
DrawView::~DrawView()
{
}

// MouseDown
void
DrawView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	fDesktop->PostMessage(Window()->CurrentMessage());
}

// MouseUp
void
DrawView::MouseUp(BPoint where)
{
	fDesktop->PostMessage(Window()->CurrentMessage());
}

// MouseMoved
void
DrawView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	fDesktop->PostMessage(Window()->CurrentMessage());
}

// SetDesktop
void
DrawView::SetDesktop(Desktop* desktop)
{
	fDesktop = desktop;
}


