/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef INTERFACE_WINDOW_H
#define INTERFACE_WINDOW_H


#include "NetworkSettings.h"
#include "InterfaceAddressView.h"
#include "InterfaceHardwareView.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>
#include <Window.h>

#include <map>


enum {
	MSG_IP_SAVE = 'ipap',
	MSG_IP_REVERT = 'iprv'
};


typedef std::map<int, InterfaceAddressView*> IPViewMap;


class InterfaceWindow : public BWindow {
public:
								InterfaceWindow(NetworkSettings* settings);
	virtual						~InterfaceWindow();
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* mesage);

private:
			status_t			_PopulateTabs();

			NetworkSettings*	fNetworkSettings;
			BButton*			fApplyButton;
			BButton*			fRevertButton;
			BTabView*			fTabView;

			IPViewMap			fTabIPView;
			InterfaceHardwareView* fTabHardwareView;
};


#endif  /* INTERFACE_WINDOW_H */

