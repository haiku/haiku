/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * With code from:
 *		Axel Dörfler
 *		Hugo Santos
 */

#include "NetworkApp.h"
#include "NetworkWindow.h"

NetworkApp::NetworkApp()
	: BApplication("application/x-vnd.Haiku-Network")
{
}

NetworkApp::~NetworkApp()
{
}

void
NetworkApp::ReadyToRun()
{
	fEthWindow = new NetworkWindow();
	fEthWindow->Show();
}

int
main(int argc, char** argv)
{
	NetworkApp app;
	app.Run();
	
	return 0;
}
