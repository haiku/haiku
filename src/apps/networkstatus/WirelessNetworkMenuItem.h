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
									int32 signalQuality, bool encrypted,
									BMessage* message);
	virtual						~WirelessNetworkMenuItem();

			void				SetSignalQuality(int32 quality);
			int32				SignalQuality() const
									{ return fQuality; }
			bool				IsEncrypted() const
									{ return fIsEncrypted; }

protected:
	virtual	void				DrawContent();
	virtual	void				Highlight(bool isHighlighted);
	virtual	void				GetContentSize(float* width, float* height);
			void				DrawRadioIcon();

private:
			int32				fQuality;
			bool				fIsEncrypted;
};


#endif	// WIRELESS_NETWORK_MENU_ITEM_H
