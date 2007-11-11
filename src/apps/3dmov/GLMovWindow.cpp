/*
  -----------------------------------------------------------------------------

	File:	GLMovWindow.cpp
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include "GLMovStrings.h"
#include "GLMovMessages.h"
#include "GLMovWindow.h"
#include "GLMovView.h"
#include "CubeView.h"

#include <stdio.h>

GLMovWindow::GLMovWindow(GLMovView *view, BRect frame, const char *title)
	: BDirectWindow(frame, title, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0)
{
	fGLView = view;
}

GLMovWindow::~GLMovWindow()
{
}

bool GLMovWindow::QuitRequested()
{
	be_app->Lock();
	if (be_app->CountWindows() < 2)
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	be_app->Unlock();
	return true;
}

void GLMovWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case MSG_OBJ1:
	case MSG_OBJ2:
	case MSG_OBJ3:
	case MSG_OBJ4:
		MakeWindow((object_type)(OBJ_CUBE + message->what - MSG_OBJ1));
		break;
	case 'Paus':
		break;
	default:
		BDirectWindow::MessageReceived(message);
	}
}

void GLMovWindow::DirectConnected(direct_buffer_info *info)
{
	fGLView->DirectConnected(info);
}


GLMovWindow *GLMovWindow::MakeWindow(object_type obj)
{
	GLMovWindow *win;
	GLMovView *view;
	BMenuBar *bar;
	BMenu *menu;
	BMenuItem *menuItem;
	BRect frame(0, 0, 400-1, 400-1);
	const char *title = "3dMov";

	bar = new BMenuBar(frame, "menu", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_ITEMS_IN_ROW, true);
	menu = new BMenu(STR_MENU);
	menu->AddItem(new BMenuItem(STR_CUBE, new BMessage(MSG_OBJ1), '1'));
	menu->AddItem(new BMenuItem(STR_SPHERE, new BMessage(MSG_OBJ2), '2'));
	menu->AddItem(new BMenuItem(STR_PULSE, new BMessage(MSG_OBJ3), '3'));
	menu->AddItem(new BMenuItem(STR_BOOK, new BMessage(MSG_OBJ4), '4'));
	menu->AddItem(new BSeparatorItem);
	menu->AddItem((menuItem = new BMenuItem(STR_PAUSE, new BMessage(MSG_PAUSE), 'P')));
	menuItem->SetTarget(be_app_messenger);
	menu->AddItem(new BMenuItem(STR_CLOSE, new BMessage(B_QUIT_REQUESTED), 'W'));
	menu->AddItem(new BSeparatorItem);
	menu->AddItem((menuItem = new BMenuItem(STR_ABOUT, new BMessage(B_ABOUT_REQUESTED))));
	menuItem->SetTarget(be_app_messenger);
	menu->AddItem((menuItem = new BMenuItem(STR_QUIT, new BMessage(B_QUIT_REQUESTED), 'Q')));
	menuItem->SetTarget(be_app_messenger);
	bar->AddItem(new BMenuItem(menu));
	bar->ResizeToPreferred();
	switch (obj) {
	case OBJ_CUBE:
	default:
		view = new CubeView(frame.OffsetByCopy(0, bar->Bounds().Height()+1));
		title = STR_CUBE;
		break;
	case OBJ_SPHERE:
		view = new GLMovView(frame.OffsetByCopy(0, bar->Bounds().Height()+1));
		title = STR_SPHERE;
		break;
	case OBJ_PULSE:
		view = new GLMovView(frame.OffsetByCopy(0, bar->Bounds().Height()+1));
		title = STR_PULSE;
		break;
	case OBJ_BOOK:
		frame = BRect(0, 0, 600-1, 400-1);
		view = new GLMovView(frame.OffsetByCopy(0, bar->Bounds().Height()+1));
		title = STR_BOOK;
		break;
	}
	view->SetViewColor(0,0,0);
	frame.bottom += bar->Bounds().Height()+1;
	frame.OffsetBySelf(50,50);
	win = new GLMovWindow(view, frame, title);
	win->AddChild(bar);
	win->AddChild(view);
	win->Show();
	return win;
}
