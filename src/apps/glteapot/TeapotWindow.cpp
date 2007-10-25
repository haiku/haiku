/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <InterfaceKit.h>
#include <Rect.h>
#include <Point.h>

#include "TeapotWindow.h"

TeapotWindow::TeapotWindow(BRect rect, char* name, window_type wt, ulong something)
	: BDirectWindow(rect,name,wt,something)
{
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
	
	BRect r = rect;
	r.OffsetTo(BPoint(100,100));

	Lock();
	r = Bounds();
	r.bottom = r.top + 14;
	BMenuBar*	mb = new BMenuBar(r,"main menu");

	BMenu*	m;
	BMessage msg (bmsgAddModel);

	mb->AddItem(m = new BMenu("File"));
	AddChild(mb);
	
	mb->ResizeToPreferred();

	r = Bounds();
	r.top = mb->Bounds().bottom+1;
	BView *sv = new BView(r,"subview",B_FOLLOW_ALL,0);
	AddChild(sv);
	
	r = sv->Bounds();
	fObjectView = new ObjectView(r,"objectView", B_FOLLOW_ALL_SIDES,type);
	sv->AddChild(fObjectView);
	fObjectView = fObjectView;
	
	BMenuItem*	item;
	msg.AddInt32("num",256);
	m->AddItem(item = new BMenuItem("Add a teapot",new BMessage(msg), 'N'));
	item->SetTarget(fObjectView);	
	m->AddSeparatorItem();
	m->AddItem(item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	msg.RemoveName("num");
	mb->AddItem(m = new BMenu("Options"));
	m->AddItem(item = new BMenuItem("Perspective",new BMessage(bmsgPerspective)));
	item->SetTarget(fObjectView);
	item->SetMarked(false);
	m->AddItem(item = new BMenuItem("FPS Display",new BMessage(bmsgFPS)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	m->AddItem(item = new BMenuItem("Filled polygons",new BMessage(bmsgFilled)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	m->AddItem(item = new BMenuItem("Lighting",new BMessage(bmsgLighting)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	m->AddItem(item = new BMenuItem("Backface culling",new BMessage(bmsgCulling)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	m->AddItem(item = new BMenuItem("Z-buffered",new BMessage(bmsgZBuffer)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	m->AddItem(item = new BMenuItem("Gouraud shading",new BMessage(bmsgGouraud)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
//	m->AddItem(item = new BMenuItem("Texture mapped",new BMessage(bmsgTextured)));
//	item->SetTarget(fObjectView);
	m->AddItem(item = new BMenuItem("Fog",new BMessage(bmsgFog)));
	item->SetTarget(fObjectView);

	BMenu *sm;
	mb->AddItem(m = new BMenu("Lights"));
	msg.what = bmsgLights;

	msg.AddInt32("num",1);
	m->AddItem(item = new BMenuItem(sm = new BMenu("Upper center"),NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color",lightNone);
	sm->AddItem(item = new BMenuItem("Off",new BMessage(msg)));
	item->SetTarget(fObjectView);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(item = new BMenuItem("White",new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(item = new BMenuItem("Yellow",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(item = new BMenuItem("Blue",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(item = new BMenuItem("Red",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(item = new BMenuItem("Green",new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num",2);
	m->AddItem(item = new BMenuItem(sm = new BMenu("Lower left"),NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color",lightNone);
	sm->AddItem(item = new BMenuItem("Off",new BMessage(msg)));
	item->SetTarget(fObjectView);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(item = new BMenuItem("White",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(item = new BMenuItem("Yellow",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(item = new BMenuItem("Blue",new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(item = new BMenuItem("Red",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(item = new BMenuItem("Green",new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num",3);
	m->AddItem(item = new BMenuItem(sm = new BMenu("Right"),NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color",lightNone);
	sm->AddItem(item = new BMenuItem("Off",new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	sm->AddSeparatorItem();
	msg.ReplaceInt32("color",lightWhite);
	sm->AddItem(item = new BMenuItem("White",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightYellow);
	sm->AddItem(item = new BMenuItem("Yellow",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightBlue);
	sm->AddItem(item = new BMenuItem("Blue",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightRed);
	sm->AddItem(item = new BMenuItem("Red",new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color",lightGreen);
	sm->AddItem(item = new BMenuItem("Green",new BMessage(msg)));
	item->SetTarget(fObjectView);

	float f = mb->Bounds().IntegerHeight()+1;
	SetSizeLimits(32,1024,32+f,1024+f);
	
	Unlock();
}


bool
TeapotWindow::QuitRequested()
{
//	printf("closing \n");
	fObjectView->EnableDirectMode( false );
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TeapotWindow::DirectConnected(direct_buffer_info* info)
{
	if (fObjectView)
		fObjectView->DirectConnected( info );	
	fObjectView->EnableDirectMode( true );
}


void
TeapotWindow::MessageReceived(BMessage* msg)
{
//	msg->PrintToStream();
	switch (msg->what) {
		default:
			BDirectWindow::MessageReceived(msg);
	}		
}
