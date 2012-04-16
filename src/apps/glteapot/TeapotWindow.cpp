/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <new>

#include <Catalog.h>
#include <InterfaceKit.h>
#include <Point.h>
#include <Rect.h>

#include "TeapotWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TeapotWindow"

TeapotWindow::TeapotWindow(BRect rect, const char* name, window_type wt,
	ulong something)
	:
	BDirectWindow(rect, name, wt, something)
{
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;

	Lock();
	BRect bounds = Bounds();
	bounds.bottom = bounds.top + 14;
	BMenuBar* menuBar = new BMenuBar(bounds, "main menu");

	BMenu* menu;
	BMessage msg(kMsgAddModel);

	menuBar->AddItem(menu = new BMenu(B_TRANSLATE("File")));
	AddChild(menuBar);

	menuBar->ResizeToPreferred();

	bounds = Bounds();
	bounds.top = menuBar->Bounds().bottom + 1;
	BView *subView = new BView(bounds, "subview", B_FOLLOW_ALL, 0);
	AddChild(subView);

	bounds = subView->Bounds();
	fObjectView = new(std::nothrow) ObjectView(bounds, "objectView",
		B_FOLLOW_ALL_SIDES, type);
	subView->AddChild(fObjectView);

	BMenuItem*	item;
	msg.AddInt32("num", 256);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Add a teapot"),
		new BMessage(msg), 'N'));
	item->SetTarget(fObjectView);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	msg.RemoveName("num");
	menuBar->AddItem(menu = new BMenu(B_TRANSLATE("Options")));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Perspective"),
		new BMessage(kMsgPerspective)));
	item->SetTarget(fObjectView);
	item->SetMarked(false);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("FPS display"),
		new BMessage(kMsgFPS)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Filled polygons"),
		new BMessage(kMsgFilled)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Lighting"),
		new BMessage(kMsgLighting)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Backface culling"),
		new BMessage(kMsgCulling)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Z-buffered"),
		new BMessage(kMsgZBuffer)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Gouraud shading"),
		new BMessage(kMsgGouraud)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
//	menu->AddItem(item = new BMenuItem("Texture mapped", new BMessage(kMsgTextured)));
//	item->SetTarget(fObjectView);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Fog"),
		new BMessage(kMsgFog)));
	item->SetTarget(fObjectView);

	BMenu *subMenu;
	menuBar->AddItem(menu = new BMenu(B_TRANSLATE("Lights")));
	msg.what = kMsgLights;

	msg.AddInt32("num", 1);
	menu->AddItem(item = new BMenuItem(subMenu =
		new BMenu(B_TRANSLATE("Upper center")), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Off"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("White"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Yellow"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Blue"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Red"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Green"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num", 2);
	menu->AddItem(item = new BMenuItem(subMenu =
		new BMenu(B_TRANSLATE("Lower left")), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Off"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("White"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Yellow"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Blue"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Red"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Green"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);

	msg.RemoveName("color");

	msg.ReplaceInt32("num", 3);
	menu->AddItem(item = new BMenuItem(subMenu =
		new BMenu(B_TRANSLATE("Right")), NULL));
	item->SetTarget(fObjectView);
	msg.AddInt32("color", lightNone);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Off"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	item->SetMarked(true);
	subMenu->AddSeparatorItem();
	msg.ReplaceInt32("color", lightWhite);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("White"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightYellow);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Yellow"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightBlue);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Blue"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightRed);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Red"),
		new BMessage(msg)));
	item->SetTarget(fObjectView);
	msg.ReplaceInt32("color", lightGreen);
	subMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Green"),
		new BMessage(msg)));
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
	// TODO: Direct rendering causes the mouse to flicker due
	// to the lack of a Hardware Cursor, however without
	// it the teapot freezes on mouse move. (bug?)
	#if 1
	if (fObjectView != NULL) {
		fObjectView->DirectConnected(info);
		fObjectView->EnableDirectMode(true);
	}
	#endif
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
