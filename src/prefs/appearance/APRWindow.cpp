//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		APRWindow.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	basic Window class for Appearance
//					
//  
//------------------------------------------------------------------------------
#include <Messenger.h>
#include "APRWindow.h"
#include "APRView.h"
#include "DecView.h"
#include "CurView.h"
#include "defs.h"

APRWindow::APRWindow(BRect frame)
	: BWindow(frame, "Appearance", B_TITLED_WINDOW,
	  B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES )
{
	tabview=new BTabView(Bounds(),"TabView");

	BTab *tab=NULL;
	
	// TODO: Finish CurView
	cursors=new CurView(Bounds(),"Cursors",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(cursors);
	tabview->AddTab(cursors,tab);

	colors=new APRView(Bounds(),"Colors",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(colors);
	tabview->AddTab(colors,tab);

	decorators=new DecView(Bounds(),"Decorator",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(decorators);
	tabview->AddTab(decorators,tab);

	AddChild(tabview);
	decorators->SetColors(*colors->currentset);
}

bool APRWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

void APRWindow::WorkspaceActivated(int32 wkspc, bool is_active)
{
	if(is_active)
	{
		BMessenger notifier(colors);
		notifier.SendMessage(new BMessage(B_WORKSPACE_ACTIVATED));
	}
}

#include <stdio.h>
void APRWindow::MessageReceived(BMessage *msg)
{
	if(msg->what==SET_UI_COLORS)
		decorators->SetColors(*colors->currentset);
	else
		BWindow::MessageReceived(msg);
}
