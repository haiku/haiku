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

#include "settings.h"
#include <ObjectList.h>
#include <View.h>

class BButton;
class BMenuField;
class BTextControl;

static const uint32 kMsgApply = 'aply';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgClose = 'clse';
static const uint32 kMsgField = 'fild';
static const uint32 kMsgInfo = 'info';
static const uint32 kMsgMode = 'mode';

class EthernetSettingsView : public BView {
	public:
		EthernetSettingsView(BRect frame);
		virtual ~EthernetSettingsView();
		
		virtual void MessageReceived(BMessage* message);
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
		void SaveProfile(BString profileName);
		void LoadProfile(BString profileName);
	private:
		
		BButton* fApplyButton;
		BButton* fRevertButton;
		BMenuField* fDeviceMenuField;
		BMenuField* fTypeMenuField;
		BTextControl* fIPTextControl;
		BTextControl* fNetMaskTextControl;
		BTextControl* fGatewayTextControl;
		BTextControl* fPrimaryDNSTextControl;
		BTextControl* fSecondaryDNSTextControl;
		BObjectList<BString> fInterfaces;
		BObjectList<Settings> fSettings;
		Settings* fCurrentSettings;

		int32	fStatus;
		int		fSocket;
		void	_GatherInterfaces();
		bool	_PrepareRequest(struct ifreq& request, const char* name);
		void 	_ShowConfiguration(Settings* settings);
		void	_EnableTextControls(bool enable);
		void 	_SaveConfiguration();
		void 	_SaveDNSConfiguration();
		void 	_SaveAdaptersConfiguration();
		void	_ApplyControlsToConfiguration();
		status_t _GetPath(const char* name, BPath& path);
};

#endif /* ETHERNET_SETTINGS_VIEW_H */
