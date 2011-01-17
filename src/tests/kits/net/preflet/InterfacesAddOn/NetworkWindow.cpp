/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * 		Fredrik Modéen
 */

#include "NetworkWindow.h"

#include <Application.h>
#include <GroupLayout.h>

#include "EthernetSettingsView.h"
#include "Settings.h"


NetworkWindow::NetworkWindow(Settings* settings)
	: BWindow(BRect(50, 50, 269, 302), "Network", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	fEthernetView = new EthernetSettingsView(); // settings);
	GetLayout()->AddView(fEthernetView);

	SetTitle(settings->Name());
}


NetworkWindow::~NetworkWindow()
{
}


void
NetworkWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
	}

}


bool
NetworkWindow::QuitRequested()
{
	return true;
}
