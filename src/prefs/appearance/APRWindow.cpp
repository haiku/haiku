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
#include "MenuView.h"
#include "defs.h"
#include <ScrollView.h>
#include <ListItem.h>

APRWindow::APRWindow(BRect frame)
	: BWindow(frame, "Appearance", B_TITLED_WINDOW,
	  B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES )
{
	BView *topview=new BView(Bounds(),"topview",B_FOLLOW_ALL,B_WILL_DRAW);
	topview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topview);
	
	BRect r(10,10,110,Bounds().Height()-10);

	listview=new BListView(r,"prefslist");
	topview->AddChild(listview);
	listview->SetSelectionMessage(new BMessage(PREFS_CHOSEN));
	
	
	r.Set(100,100,385,445);
	r.OffsetTo(listview->Bounds().Width()+20,0);
	r.right=Bounds().right;

	listview->AddItem(new BStringItem("Colors"));
	colors=new APRView(r,"Colors",B_FOLLOW_ALL, B_WILL_DRAW);
	topview->AddChild(colors);
	viewlist.AddItem(colors);
	colors->Hide();

	// TODO: Finish CurView
	listview->AddItem(new BStringItem("Cursors"));
	cursors=new CurView(r,"Cursors",B_FOLLOW_ALL, B_WILL_DRAW);
	topview->AddChild(cursors);
	viewlist.AddItem(cursors);
	cursors->Hide();

	listview->AddItem(new BStringItem("Decorators"));
	decorators=new DecView(r,"Decorator",B_FOLLOW_ALL, B_WILL_DRAW);
	topview->AddChild(decorators);
	decorators->Hide();
	viewlist.AddItem(decorators);
	decorators->SetColors(*colors->currentset);

	// This code works properly IIRC, but will remain disabled for now.
	
	// One of the cool things about Appearance is that eventually we will probably 
	// integrate all appearance-related preference panels into this to reduce the number
	// of preference applications and to make the entire preferences area more organized.
	// This is the route that Zeta took with theirs, but ours will not be a hack -- it will
	// be done right. B^>
	
	// This won't amount to much -- we won't bother with the scrollbar prefs app and just
	// set sensible defaults. It should just be menu and fonts, so not much work, either.
	
//	listview->AddItem(new BStringItem("Menu"));
//	MenuView *menuview=new MenuView(r,"Menu",B_FOLLOW_ALL, B_WILL_DRAW);
//	topview->AddChild(menuview);
//	viewlist.AddItem(menuview);
//	menuview->Hide();

	listview->Select(0);
}

bool APRWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

void APRWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case SET_UI_COLORS:
		{
			decorators->SetColors(*colors->currentset);
			break;
		}
		case PREFS_CHOSEN:
		{
			int32 selectionindex=listview->CurrentSelection();
			if(selectionindex==-1)
				break;
			
			// Hide any visible preference views
			for(int32 i=0; i<listview->CountItems(); i++)
			{
				BView *v=(BView*)viewlist.ItemAt(i);
				
				if(!v)
					continue;
				
				if(!v->IsHidden())
					v->Hide();
			}
			BView *visible=(BView*)viewlist.ItemAt(selectionindex);
			visible->Show();
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
