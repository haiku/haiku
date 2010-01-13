/*
 * Copyright (c) 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "GradientsWindow.h"


GradientsWindow::GradientsWindow()
	: BWindow(BRect(0, 0, 230, 490), "Gradients Test", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect field(10, 10, Bounds().Width() - 10, 30);
	fGradientsMenu = new BPopUpMenu("gradientsType");
	fLinearItem = new BMenuItem("Linear", new BMessage(MSG_LINEAR));
	fRadialItem = new BMenuItem("Radial", new BMessage(MSG_RADIAL));
	fRadialFocusItem = new BMenuItem("Radial focus",
		new BMessage(MSG_RADIAL_FOCUS));
	
	fDiamondItem = new BMenuItem("Diamond", new BMessage(MSG_DIAMOND));
	fConicItem = new BMenuItem("Conic", new BMessage(MSG_CONIC));
	fGradientsMenu->AddItem(fLinearItem);
	fGradientsMenu->AddItem(fRadialItem);
	fGradientsMenu->AddItem(fRadialFocusItem);
	fGradientsMenu->AddItem(fDiamondItem);
	fGradientsMenu->AddItem(fConicItem);
	fLinearItem->SetMarked(true);
	fGradientsTypeField = new BMenuField(field, "gradientsField",
		"Gradient type:", fGradientsMenu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	fGradientsTypeField->SetViewColor(255, 255, 255);
	fGradientsTypeField->SetDivider(110);
	AddChild(fGradientsTypeField);

	BRect bounds = Bounds();
	bounds.top = 40;
	fGradientsView = new GradientsView(bounds);
	AddChild(fGradientsView);

	MoveTo((BScreen().Frame().Width() - Bounds().Width()) / 2,
		(BScreen().Frame().Height() - Bounds().Height()) / 2 );
}


bool
GradientsWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
GradientsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_LINEAR:
			fGradientsView->SetType(BGradient::TYPE_LINEAR);
			break;
		case MSG_RADIAL:
			fGradientsView->SetType(BGradient::TYPE_RADIAL);
			break;
		case MSG_RADIAL_FOCUS:
			fGradientsView->SetType(BGradient::TYPE_RADIAL_FOCUS);
			break;
		case MSG_DIAMOND:
			fGradientsView->SetType(BGradient::TYPE_DIAMOND);
			break;
		case MSG_CONIC:
			fGradientsView->SetType(BGradient::TYPE_CONIC);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
