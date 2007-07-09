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

#include "EthernetSettings.h"
#include "EthernetSettingsWindow.h"

EthernetSettings::EthernetSettings()
	: BApplication("application/x-vnd.Haiku-EthernetSettings")
{
}

EthernetSettings::~EthernetSettings()
{
}

void EthernetSettings::ReadyToRun()
{
	fEthWindow = new EthernetSettingsWindow();
	fEthWindow->Show();
}

int main(int argc, char** argv)
{
	EthernetSettings app;
	app.Run();
	
	return 0;
}
