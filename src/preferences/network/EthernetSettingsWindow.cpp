/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * With code from:
 *		Axel Dorfler
 *		Hugo Santos
 */

#include <Application.h>

#include "EthernetSettingsWindow.h"
#include "EthernetSettingsView.h"


EthernetSettingsWindow::EthernetSettingsWindow() : BWindow(BRect(50, 50, 269, 302), 
	"Ethernet Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
			
	fEthView = new EthernetSettingsView(Bounds());
			
	AddChild(fEthView);
			
	Show();
}
	

void EthernetSettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInfo:
		fEthView->MessageReceived(message);
		break;
	default:
		BWindow::MessageReceived(message);
	}

}

EthernetSettingsWindow::~EthernetSettingsWindow()
{
}


bool EthernetSettingsWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
