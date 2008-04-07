/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Axel Dörfler
 *		Hugo Santos
 */
#ifndef ETHERNET_SETTINGS_VIEW_H
#define ETHERNET_SETTINGS_VIEW_H


#include "settings.h"

#include <ObjectList.h>
#include <View.h>

class BButton;
class BMenuField;
class BPath;
class BTextControl;


class EthernetSettingsView : public BView {
public:
								EthernetSettingsView();
		virtual					~EthernetSettingsView();
		
		virtual	void			MessageReceived(BMessage* message);
		virtual	void			AttachedToWindow();
		virtual	void			DetachedFromWindow();
		
				void			SaveProfile(BString profileName);
				void			LoadProfile(BString profileName);

private:
				void			_GatherInterfaces();
				bool			_PrepareRequest(struct ifreq& request,
									const char* name);
				void 			_ShowConfiguration(Settings* settings);
				void			_EnableTextControls(bool enable);
				void			_SaveConfiguration();
				void			_SaveDNSConfiguration();
				void			_SaveAdaptersConfiguration();
				void			_ApplyControlsToConfiguration();
				status_t		_GetPath(const char* name, BPath& path);
private:
		
				BButton*		fApplyButton;
				BButton*		fRevertButton;
					// TODO: buttons should be moved to window instead

				BMenuField*		fDeviceMenuField;
				BMenuField*		fTypeMenuField;
				BTextControl*	fIPTextControl;
				BTextControl*	fNetMaskTextControl;
				BTextControl*	fGatewayTextControl;

				BTextControl*	fPrimaryDNSTextControl;
				BTextControl*	fSecondaryDNSTextControl;
					// TODO: DNS settings do not belong here, do they?
				BObjectList<BString> fInterfaces;
					// TODO: the view should not know about the interfaces,
					// it should only display the given interface, move
					// one level up.
				BObjectList<Settings> fSettings;
					// TODO: the view should not know about a list
					// of settings, instead it should be configured
					// to a specific setting from the code one level up
				Settings*		fCurrentSettings;

				int32			fStatus;
				int				fSocket;
};

#endif /* ETHERNET_SETTINGS_VIEW_H */
