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
#include <GroupLayout.h>

#include "NetworkWindow.h"
#include "EthernetSettingsView.h"


NetworkWindow::NetworkWindow()
	: BWindow(BRect(50, 50, 269, 302), "Network", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	fEthView = new EthernetSettingsView();
	GetLayout()->AddView(fEthView);
}
	

void
NetworkWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
	}

}

NetworkWindow::~NetworkWindow()
{
}


bool
NetworkWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
