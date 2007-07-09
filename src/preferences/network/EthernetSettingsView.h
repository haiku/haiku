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
#ifndef ETHERNET_SETTINGS_VIEW_H
#define ETHERNET_SETTINGS_VIEW_H

#include <ObjectList.h>
#include <View.h>

class BButton;
class BMenuField;
class BTextControl;

static const uint32 kMsgApply = 'aply';
static const uint32 kMsgClose = 'clse';
static const uint32 kMsgOK = 'okok';
static const uint32 kMsgField = 'fild';
static const uint32 kMsgInfo = 'info';

class EthernetSettingsView : public BView {
	public:
		EthernetSettingsView(BRect rect);
		virtual ~EthernetSettingsView();
		
		virtual void MessageReceived(BMessage* message);
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
	private:
		
		BButton* fCloseButton;
		BButton* fApplyButton;
		BButton* fOKButton;
		BMenuField* fDeviceMenuField;
		BMenuField* fTypeMenuField;
		BTextControl* fIPTextControl;
		BTextControl* fNetMaskTextControl;
		BTextControl* fGatewayTextControl;
		BTextControl* fPrimaryDNSTextControl;
		BTextControl* fSecondaryDNSTextControl;
		BObjectList<BString> fInterfaces;
		int32	fStatus;
		int		fSocket;
		void	_GatherInterfaces();
		bool	_PrepareRequest(struct ifreq& request, const char* name);
		void 	_ShowConfiguration(BMessage* message);

};

#endif /* ETHERNET_SETTINGS_VIEW_H */
