/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACE_WINDOW_H
#define INTERFACE_WINDOW_H


#include "NetworkSettings.h"
#include "InterfaceAddressView.h"
#include "InterfaceHardwareView.h"

#include <map>

#include <Window.h>


enum {
	MSG_IP_SAVE = 'ipap',
	MSG_IP_REVERT = 'iprv'
};


typedef std::map<int, InterfaceAddressView*> IPViewMap;


class BButton;
class BTabView;

class InterfaceWindow : public BWindow {
public:
									InterfaceWindow(NetworkSettings* settings);
	virtual							~InterfaceWindow();

	virtual	void					MessageReceived(BMessage* mesage);
	virtual	bool					QuitRequested();

private:
			status_t				_PopulateTabs();

			NetworkSettings*		fNetworkSettings;

			BButton*				fRevertButton;
			BButton*				fApplyButton;

			BTabView*				fTabView;

			IPViewMap				fTabIPView;
			InterfaceHardwareView*	fTabHardwareView;
};


#endif  // INTERFACE_WINDOW_H
