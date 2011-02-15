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

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>
#include <Window.h>


enum {
	APPLY_MSG = 'aply',
	REVERT_MSG = 'rvrt'
};


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

			InterfaceAddressView* fIPv4TabView;
			InterfaceAddressView* fIPv6TabView;
};


#endif  /* INTERFACE_WINDOW_H */

