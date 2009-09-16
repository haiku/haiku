/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 */


#ifndef INTERFACESTATUSITEM_H
#define INTERFACESTATUSITEM_H

#include <String.h>
#include <ListItem.h>
#include <Bitmap.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include "NetworkStatusAddOn.h"
#include "Setting.h"

class InterfaceStatusItem : public BListItem 
{
	public:
		InterfaceStatusItem(const char* name, NetworkStatusAddOn* addon);
		~InterfaceStatusItem();

		void DrawItem(BView* owner, BRect bounds, bool complete);
		void Update(BView* owner, const BFont* font);
		
		inline const char*		Name()		{ return fSetting->GetName(); }
		inline bool				Enabled()	{ return fEnabled; } 
		inline void				SetEnabled(bool enable){ fEnabled = enable; }
		inline Setting*			GetSetting()	{ return fSetting; } 

	private:
		void 					_InitIcon();

		bool					fEnabled;
		BBitmap* 				fIcon;
		Setting*				fSetting;
		NetworkStatusAddOn* 	fAddon;
};
#endif /*INTERFACESTATUSITEM_H*/
