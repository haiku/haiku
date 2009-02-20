/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <stdio.h>

#include <Alert.h>

#include "BluetoothMain.h"
#include "BluetoothWindow.h"
#include "defs.h"



BluetoothApplication::BluetoothApplication(void)
 :	BApplication(BLUETOOTH_APP_SIGNATURE)
{
	fWindow = new BluetoothWindow(BRect(100, 100, 550, 420));
	fWindow->Show();
}


void
BluetoothApplication::AboutRequested()
{
	
	(new BAlert("about", "Haiku Bluetooth System, (ARCE)

Created by Oliver Ruiz Dorantes

With support of:
	- Mika Lindqvist
	- Maksym Yevmenkin

Thanks to the individuals who helped...

Shipping/donating hardware:
	- Henry Jair Abril Florez(el Colombian)
		 & Stefanie Bartolich
	- Dennis d'Entremont
	- Luroh
	- Pieter Panman

Economically:
	- Karl von Dorf, Andrea Bernardi (OSDrawer),
	- Matt M, Doug F, Hubert H,
	- Sebastian B, Andrew M, Jared E,
	- Frederik H, Tom S, Ferry B,
	- Greg G, David F, Richard S, Martin W:

With patches:
	- Fredrik Ekdahl
	- Andreas Färber

Testing:
	- Petter H. Juliussen
	- Raynald Lesieur
	- Adrien Destugues
	- Jörg Meyer
	
Who gave me all the knowledge:
	- the yellowTAB team", "OK"))->Go();
	
}


int
main(int, char**)
{	
	BluetoothApplication myApplication;
	myApplication.Run();

	return 0;
}
