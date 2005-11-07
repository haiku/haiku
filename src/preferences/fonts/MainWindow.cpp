/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 */


#include "MainWindow.h"


MainWindow::MainWindow()
	: BWindow( BRect(100,80,445,343) , "Fonts", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect r; 
	BTabView *tabView; 
	BBox *topLevelView;
	double buttonViewHeight = 43.0;
	
	r = Bounds(); 
	r.top += 10;
	r.bottom -= buttonViewHeight+1;
	
	tabView = new BTabView(r, "tabview", B_WIDTH_FROM_LABEL); 
	tabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR)); 
	
	r = tabView->Bounds(); 
	r.InsetBy(5, 5); 
	r.bottom -= tabView->TabHeight();
	r.right -=2;
	
	fSelectorView = new FontView(r);
	tabView->AddTab(fSelectorView); 
	
	r = Bounds();
	r.top = r.bottom - buttonViewHeight;
	
	fButtonView = new ButtonView(r);
	
	topLevelView = new BBox(Bounds(), "TopLevelView", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER);
	
	AddChild(topLevelView);
	topLevelView->AddChild(fButtonView);
	topLevelView->AddChild(tabView);
	
	MoveTo(fSettings.WindowCorner());
}


bool
MainWindow::QuitRequested()
{
	fSettings.SetWindowCorner(Frame().LeftTop());

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case M_ENABLE_REVERT:
			fButtonView->SetRevertState(true);
			break;

		case M_RESCAN_FONTS:
			fSelectorView->RescanFonts();
			break;

		case M_SET_DEFAULTS:
			fSelectorView->SetDefaults();
			fButtonView->SetRevertState(true);
			break;

		case M_REVERT:
			fSelectorView->Revert();
			fButtonView->SetRevertState(false);
			break;

		default: 
			BWindow::MessageReceived(message);
			break;
	}
}
