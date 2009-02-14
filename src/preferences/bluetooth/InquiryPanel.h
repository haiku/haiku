/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef INQUIRY_WINDOW_H
#define INQUIRY_WINDOW_H

#include <Application.h>
#include <Button.h>
#include <Window.h>
#include <Message.h>
#include <TabView.h>


#include "InquirySettingsView.h"

class RemoteDevicesView;
class ConnChanView;

class InquiryWindow : public BWindow 
{
public:
			InquiryWindow(BRect frame); 
	bool	QuitRequested(void);
	void	MessageReceived(BMessage *message);
	
private:
		RemoteDevicesView*		fRemoteDevices;
		ConnChanView*			fConnChan;
		BButton*				fDefaultsButton;
		BButton*				fRevertButton;
		BMenuBar*				fMenubar;

};



#endif
