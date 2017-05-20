/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_CONNECTION_CONFIG_VIEW_H
#define NETWORK_CONNECTION_CONFIG_VIEW_H

#include "ConnectionConfigView.h"


class BMenuField;
class BTextControl;
class Setting;


class NetworkConnectionConfigView : public ConnectionConfigView{
public:
								NetworkConnectionConfigView();
	virtual						~NetworkConnectionConfigView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

protected:
	virtual	status_t			InitSpecific();

private:
			BMenuField*			fProtocolField;
			BTextControl*		fHostInput;
			BTextControl*		fPortInput;
			Settings*			fSettings;
			Setting*			fHostSetting;
			Setting*			fPortSetting;
};


#endif	// NETWORK_CONNECTION_CONFIG_VIEW_H
