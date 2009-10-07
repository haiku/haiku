/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "MainWindow.h"
#include "RenderView.h"

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>

#include <stdio.h>

MainWindow::MainWindow(BRect frame, const char* title)
	:
	BDirectWindow(frame, title, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0)
{
	fRenderView = new RenderView(Bounds());
	fRenderView->SetViewColor(0, 0, 0);

	AddChild(fRenderView);
	Show();
}


MainWindow::~MainWindow()
{
}


bool
MainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BDirectWindow::MessageReceived(message);
	}
}


void
MainWindow::DirectConnected(direct_buffer_info* info)
{
	fRenderView->DirectConnected(info);
}
