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
#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H

#include <Window.h>
#include "EthernetSettingsView.h"

class NetworkWindow : public BWindow {
	public:
		NetworkWindow();
		virtual ~NetworkWindow();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* mesage);

		private:
			EthernetSettingsView *fEthView;
			
};


#endif /* NETWORK_WINDOW_H */
