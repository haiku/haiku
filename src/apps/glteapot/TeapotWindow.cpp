/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <InterfaceKit.h>
#include <Rect.h>
#include <Point.h>
#include <new>

#include "TeapotWindow.h"

TeapotWindow::TeapotWindow(BRect rect, char* name, window_type wt, ulong something)
	: BDirectWindow(rect, name, wt, something)
{
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
	
	Lock();
	BRect bounds = Bounds();
	bounds.bottom = bounds.top + 14;
	BMenuBar* menuBar = new BMenuBar(bounds, "main menu");

	BMenu* menu;
	BMessage msg(kMsgAddModel);

	menuBar->AddItem(menu = new BMenu("File"));
	AddChild(menuBar);
	
	menuBar->ResizeToPreferred();

	bounds = Bounds();
	bounds.top = menuBar->Bounds().bottom + 1;
	BView *subView = new BView(bounds, "subview", B_FOLLOW_ALL, 0);
	AddChild(subView);
	
	bounds = subView->Bounds();
	fObjectView = new(std::nothrow) ObjectView(bounds, "objectView", B_FOLLOW_ALL_SIDES, type);
	subView->AddChild(fObjectView);	
	
	BMenuItem*	item;
	msg.AddInt32("num", 256);
	menu->AddItem(item = new BMenuItem("Add a teapot", new BMessage(msg), 'N'));
	item->SetTarget(fObjectView);	
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	msg.RemoveName("num");
	menuBar->AddItem(menu = new BMenu("Options"));
	menu->AddItem(item = new BMenuItem("Perspective", new BMessage(kMsgPerspective)));
	item->SetTarget(fObjectView);
	item->SetMarked(false);
	menu->AddItem(item = new BMenuItem("FPS Display", new BMessage(kMsgFPS)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Filled polygons", new BMessage(kMsgFilled)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Lighting", new BMessage(kMsgLighting)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Backface culling", new BMessage(kMsgCulling)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Z-buffered", new BMessage(kMsgZBuffer)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Gouraud shading", new BMessage(kMsgGouraud)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
//	menu->AddItem(item = new BMenuItem("Texture mapped", new BMessage(kMsgTextured)));
//	item->SetTarget(fObjectView);
	menu->AddItem(item = new BMenuItem("Fog", new BMessage(kMsgFog)));
	item->SetTarget(fObjectView);

	BMenu *subMenu;
	menuBar->AddItem(menu = new BMenu("Lights"));
	msg.what = kMsgLights;

	msg.AddInt32("num", 1);
	menu->AddItem(item = new BMenuItem(subMenu = new BMenu("Upper center"), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem("Off", new BMessage(msg)));
	item->SetTarget(fObjectView);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem("White", new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem("Yellow", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem("Blue", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem("Red", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem("Green", new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num", 2);
	menu->AddItem(item = new BMenuItem(subMenu = new BMenu("Lower left"), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem("Off", new BMessage(msg)));
	item->SetTarget(fObjectView);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem("White", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem("Yellow", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem("Blue", new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem("Red", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem("Green", new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num", 3);
	menu->AddItem(item = new BMenuItem(subMenu = new BMenu("Right"), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem("Off", new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem("White", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem("Yellow", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem("Blue", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem("Red", new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem("Green", new BMessage(msg)));
	item->SetTarget(fObjectView);

	float f = menuBar->Bounds().IntegerHeight() + 1;
	SetSizeLimits(32, 1024, 32 + f, 1024 + f);
			//TODO: verify, adding an height to x seems strange
	Unlock();
}


bool
TeapotWindow::QuitRequested()
{
	if (fObjectView != NULL)
		fObjectView->EnableDirectMode(false);
		
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TeapotWindow::DirectConnected(direct_buffer_info* info)
{
	if (fObjectView != NULL) {
		fObjectView->DirectConnected(info);	
		fObjectView->EnableDirectMode(true);
	}
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
