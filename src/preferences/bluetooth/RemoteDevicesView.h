/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef REMOTE_DEVICES_VIEW_H_
#define REMOTE_DEVICES_VIEW_H_

#include <View.h>
#include <ColorControl.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <Invoker.h>


class RemoteDevicesView : public BView
{
public:
			RemoteDevicesView(const char *name, uint32 flags);
			~RemoteDevicesView(void);
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);

	void	LoadSettings(void);
	bool	IsDefaultable(void);

protected:

	void	SetCurrentColor(rgb_color color);
	void	UpdateControls();
	void	UpdateAllColors();

	BButton*		addButton;
	BButton*		removeButton;
	BButton*		pairButton;
	BButton*		disconnectButton;
	BButton*		blockButton;
	BButton*		availButton;
	BListView*		fDeviceList;
	BScrollView*	fScrollView;


};

#endif
