/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "ResWindow.h"
#include "ResView.h"
#include "App.h"

ResWindow::ResWindow(const BRect &rect, const entry_ref *ref)
 :	BWindow(rect,"", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	be_app->PostMessage(M_REGISTER_WINDOW);
	
	ResView *child = new ResView(Bounds(), "resview", B_FOLLOW_ALL, B_WILL_DRAW, ref);
	AddChild(child);
	
	SetTitle(child->Filename());
}


ResWindow::~ResWindow(void)
{
}


bool
ResWindow::QuitRequested(void)
{
	be_app->PostMessage(M_UNREGISTER_WINDOW);
	return true;
}


