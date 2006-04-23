#include <Message.h>
#include <stdio.h>
#include <string.h>

#include "ControllerView.h"

ControllerView::ControllerView(BRect frame, Controller *ctrl)
 :	BView(frame, "controller", B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
 ,	fController(ctrl)
{
	SetViewColor(128,0,0);
}


ControllerView::~ControllerView()
{
}


void
ControllerView::AttachedToWindow()
{
}	


void
ControllerView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}


void
ControllerView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BView::MessageReceived(msg);
	}
}

