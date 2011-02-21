/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef NETWORKSETUPADDON_H
#define NETWORKSETUPADDON_H


#include <interface/View.h>
#include <kernel/image.h>		// for image_id
#include <storage/Resources.h>


class NetworkSetupProfile;

class NetworkSetupAddOn {
	public:
		NetworkSetupAddOn(image_id addon_image);
		virtual ~NetworkSetupAddOn();

		virtual BView *			CreateView(BRect* bounds);
		virtual status_t		Save();
		virtual status_t		Revert();

		virtual const char *	Name();
		virtual status_t		ProfileChanged(NetworkSetupProfile*
									new_profile);

		NetworkSetupProfile * 	Profile();
		bool 					IsDirty();
		void 					SetDirty(bool dirty = true);
		image_id				ImageId();
		BResources*				Resources();

	private:
		bool 					fIsDirty;
		NetworkSetupProfile*	fProfile;
		image_id				fAddonImage;
		BResources*				fAddonResources;
};

extern "C" {

#define NETWORK_SETUP_ADDON_INSTANCIATE_FUNC_NAME "get_nth_addon"
typedef NetworkSetupAddOn* (*network_setup_addon_instantiate)(image_id image,
	int index);

extern NetworkSetupAddOn* get_nth_addon(image_id image, int index);

}


#endif // ifdef NETWORKSETUPADDON_H

