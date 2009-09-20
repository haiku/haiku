/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Axel Dörfler
 * 		Fredrik Modéen 
 *		Hugo Santos
 */
#ifndef ETHERNET_SETTINGS_VIEW_H
#define ETHERNET_SETTINGS_VIEW_H


#include "Setting.h"

#include <ObjectList.h>
#include <View.h>

#include <posix/regex.h>

class BButton;
class BMenuField;
class BPath;
class BTextControl;
class BStringView;


class EthernetSettingsView : public BView {
public:
								EthernetSettingsView(Setting* setting);
		virtual					~EthernetSettingsView();
		
		virtual	void			MessageReceived(BMessage* message);
		virtual	void			AttachedToWindow();
		virtual	void			DetachedFromWindow();
		
				void			SaveProfile(BString profileName);
				void			LoadProfile(BString profileName);

private:
				bool			_PrepareRequest(struct ifreq& request,
									const char* name);
				void 			_ShowConfiguration(Setting* setting);
				void			_EnableTextControls(bool enable);
				void			_SaveConfiguration();
				void			_SaveDNSConfiguration();
				void			_SaveAdaptersConfiguration();
				void			_ApplyControlsToConfiguration();
				status_t		_GetPath(const char* name, BPath& path);
				status_t		_TriggerAutoConfig(const char* device);

				bool			_ValidateControl(BTextControl* control);
private:
		
				BButton*		fApplyButton;
				BButton*		fRevertButton;

				BMenuField*		fDeviceMenuField;
				BMenuField*		fTypeMenuField;
				BTextControl*	fIPTextControl;
				BTextControl*	fNetMaskTextControl;
				BTextControl*	fGatewayTextControl;

				BTextControl*	fPrimaryDNSTextControl;
				BTextControl*	fSecondaryDNSTextControl;

				BStringView*	fErrorMessage;

					// TODO: DNS settings do not belong here, do they?
				BObjectList<BString> fInterfaces;
				
				Setting*		fCurrentSettings;
				int32			fStatus;
				int				fSocket;
};

#endif /* ETHERNET_SETTINGS_VIEW_H */
