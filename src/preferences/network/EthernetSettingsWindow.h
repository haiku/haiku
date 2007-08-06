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
#ifndef ETHERNET_SETTINGS_WINDOW_H
#define ETHERNET_SETTINGS_WINDOW_H

#include <Window.h>
#include "EthernetSettingsView.h"

class EthernetSettingsWindow : public BWindow {
	public:
		EthernetSettingsWindow();
		virtual ~EthernetSettingsWindow();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* mesage);

		private:
			EthernetSettingsView *fEthView;
			
};


#endif /* ETHERNET_SETTINGS_WINDOW_H */
