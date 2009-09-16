/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 */


#ifndef NETWORKSTATUSADDON_H
#define NETWORKSTATUSADDON_H

#include <Box.h>
#include <ListView.h>
#include <Button.h> 

#include "NetworkSetupAddOn.h"

class NetworkStatusAddOn : public NetworkSetupAddOn, public BBox
{
public:
		NetworkStatusAddOn(image_id addon_image);
		~NetworkStatusAddOn();
		
		const char* 	Name();
		BView*			CreateView(BRect *bounds);

	enum {
		INTERFACE_SELECTED_MSG		= 'ifce',
		CONFIGURE_INTERFACE_MSG		= 'conf',
		ONOFF_INTERFACE_MSG			= 'onof'
	};

		void			AttachedToWindow();
		void			MessageReceived(BMessage* msg);

private:
		status_t 		_LookupDevices();
	
		BListView*		fListview;
		BButton*		fConfigure;
		BButton*		fOnOff;
		int				fSocket;		
};

#endif /*NETWORKSTATUSADDON_H*/
