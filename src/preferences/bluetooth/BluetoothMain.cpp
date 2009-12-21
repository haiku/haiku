/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>

#include "BluetoothMain.h"
#include "BluetoothWindow.h"
#include "defs.h"


#define TR_CONTEXT "main"

BluetoothApplication::BluetoothApplication(void)
 :	BApplication(BLUETOOTH_APP_SIGNATURE)
{
	be_locale->GetAppCatalog(&fCatalog);

	fWindow = new BluetoothWindow(BRect(100, 100, 550, 420));
	fWindow->Show();
}


void
BluetoothApplication::AboutRequested()
{
	
	(new BAlert("about", TR("Haiku bluetooth system, (ARCE)\n\n"
                             "Created by Oliver Ruiz Dorantes\n\n"
                             "With support of:\n"
                             "	- Mika Lindqvist\n"
                             "	- Maksym Yevmenkin\n\n"
                             "Thanks to the individuals who helped...\n\n"
                             "Shipping/donating hardware:\n"
                             "	- Henry Jair Abril Florez(el Colombian)\n"
                             "		 & Stefanie Bartolich\n"
                             "	- Edwin Erik Amsler\n"
                             "	- Dennis d'Entremont\n"
                             "	- Luroh\n"
                             "	- Pieter Panman\n\n"
                             "Economically:\n"
                             "	- Karl vom Dorff, Andrea Bernardi (OSDrawer),\n"
                             "	- Matt M, Doug F, Hubert H,\n"
                             "	- Sebastian B, Andrew M, Jared E,\n"
                             "	- Frederik H, Tom S, Ferry B,\n"
                             "	- Greg G, David F, Richard S, Martin W:\n\n"
                             "With patches:\n"
                             "	- Michael Weirauch\n"
                             "	- Fredrik Ekdahl\n"
                             "	- Raynald Lesieur\n"
                             "	- Andreas Färber\n"
                             "	- Joerg Meyer\n"
                             "Testing:\n"
                             "	- Petter H. Juliussen\n"
                             "	- Adrien Destugues\n\n"
                             "Who gave me all the knowledge:\n"
                             "	- the yellowTAB team"), TR("Ok")))->Go();
	
}

void
BluetoothApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgAddToRemoteList:
			fWindow->PostMessage(message);
			break;
		default:
			BApplication::MessageReceived(message);
	}
}

int
main(int, char**)
{	
	BluetoothApplication myApplication;
	myApplication.Run();

	return 0;
}
