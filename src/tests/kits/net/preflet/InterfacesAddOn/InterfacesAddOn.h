/*
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 */
#ifndef INTERFACES_ADDON_H
#define INTERFACES_ADDON_H


#include <Box.h>
#include <ListView.h>
#include <Button.h> 

#include "NetworkSetupAddOn.h"


class InterfacesAddOn : public NetworkSetupAddOn, public BBox
{
public:
		InterfacesAddOn(image_id addon_image);
		~InterfacesAddOn();
		
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
		BListView*		fListview;
		BButton*		fConfigure;
		BButton*		fOnOff;
};


#endif /*INTERFACES_ADDON_H*/

