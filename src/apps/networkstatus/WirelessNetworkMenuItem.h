/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef WIRELESS_NETWORK_MENU_ITEM_H
#define WIRELESS_NETWORK_MENU_ITEM_H


#include <MenuItem.h>
#include <NetworkDevice.h>


class WirelessNetworkMenuItem : public BMenuItem {
public:
								WirelessNetworkMenuItem(
									wireless_network network,
									BMessage* message);
	virtual						~WirelessNetworkMenuItem();

			wireless_network	Network() const { return fNetwork; }

			BString 			AuthenticationName(int32 mode);

	static	int					CompareSignalStrength(const BMenuItem* a,
									const BMenuItem* b);

protected:
	virtual	void				DrawContent();
	virtual	void				Highlight(bool isHighlighted);
	virtual	void				GetContentSize(float* width, float* height);
			void				DrawRadioIcon();

private:
			wireless_network	fNetwork;
};


#endif	// WIRELESS_NETWORK_MENU_ITEM_H
