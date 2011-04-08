/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef INTERFACE_HARDWARE_VIEW_H
#define INTERFACE_HARDWARE_VIEW_H


#include "NetworkSettings.h"

#include <MenuField.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StringView.h>
#include <TextControl.h>
#include <GroupView.h>


class InterfaceHardwareView : public BGroupView {
public:
								InterfaceHardwareView(BRect frame,
									NetworkSettings* settings);
	virtual						~InterfaceHardwareView();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();
			status_t			RevertFields();
			status_t			SaveFields();


private:
			void				_EnableFields(bool enabled);

			NetworkSettings*	fSettings;

			BTextControl*		fStatusField;
			BTextControl*       fMACField;
			BTextControl*		fSpeedField;
};


#endif /* INTERFACE_HARDWARE_VIEW_H */

