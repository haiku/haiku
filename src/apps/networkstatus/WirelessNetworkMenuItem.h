/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef WIRELESS_NETWORK_MENU_ITEM_H
#define WIRELESS_NETWORK_MENU_ITEM_H


#include <MenuItem.h>


class WirelessNetworkMenuItem : public BMenuItem {
public:
								WirelessNetworkMenuItem(const char* name,
									int32 signalQuality, int32 authenticationMode,
									BMessage* message);
	virtual						~WirelessNetworkMenuItem();

			void				SetSignalQuality(int32 quality);
			int32				SignalQuality() const
									{ return fQuality; }
			BString 			AuthenticationName(int32 mode);

protected:
	virtual	void				DrawContent();
	virtual	void				Highlight(bool isHighlighted);
	virtual	void				GetContentSize(float* width, float* height);
			void				DrawRadioIcon();

private:
			int32				fQuality;
};


#endif	// WIRELESS_NETWORK_MENU_ITEM_H
