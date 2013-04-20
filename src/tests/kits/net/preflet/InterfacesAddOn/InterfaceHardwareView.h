/*
 * Copyright 2004-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef INTERFACE_HARDWARE_VIEW_H
#define INTERFACE_HARDWARE_VIEW_H


#include "NetworkSettings.h"

#include <GroupView.h>


class BMessage;
class BRect;
class BStringView;

class InterfaceHardwareView : public BGroupView {
public:
								InterfaceHardwareView(BRect frame,
									NetworkSettings* settings);
	virtual						~InterfaceHardwareView();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();
			status_t			Revert();
			status_t			Save();

private:
			status_t			Update();

			void				_EnableFields(bool enabled);

			NetworkSettings*	fSettings;

			BStringView*		fStatusField;
			BStringView*		fMacAddressField;
			BStringView*		fLinkSpeedField;
			BStringView*		fLinkTxField;
			BStringView*		fLinkRxField;
};


#endif // INTERFACE_HARDWARE_VIEW_H

