/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BLUETOOTH_WINDOW_H
#define BLUETOOTH_WINDOW_H

#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>


#include "BluetoothSettingsView.h"

class RemoteDevicesView;
class ConnChanView;

class BluetoothWindow : public BWindow 
{
public:
			BluetoothWindow(BRect frame); 
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *message);
	
private:
		RemoteDevicesView*		fRemoteDevices;
		ConnChanView*			fConnChan;
		BButton*				fDefaultsButton;
		BButton*				fRevertButton;
		BMenuBar*				fMenubar;

		BluetoothSettingsView*	fSettingsView;

};

#endif
