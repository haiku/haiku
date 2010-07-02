/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BLUETOOTH_MAIN_H
#define BLUETOOTH_MAIN_H

#include <Application.h>

class BluetoothWindow;

class BluetoothApplication : public BApplication
{

public:
	BluetoothApplication(void);
	virtual void ReadyToRun();
	virtual void MessageReceived(BMessage*);
	virtual void AboutRequested();

private:
	BluetoothWindow*	fWindow;
};

#endif
