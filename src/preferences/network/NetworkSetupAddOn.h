/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef NETWORK_SETUP_ADD_ON_H
#define NETWORK_SETUP_ADD_ON_H


#include <image.h>
#include <Resources.h>
#include <View.h>


class NetworkSetupProfile;

class NetworkSetupAddOn {
public:
								NetworkSetupAddOn(image_id image);
	virtual						~NetworkSetupAddOn();

	virtual	BView*				CreateView() = 0;
	virtual	status_t			Save();
	virtual	status_t			Revert();

	virtual const char*			Name();
	virtual status_t			ProfileChanged(NetworkSetupProfile* newProfile);

			NetworkSetupProfile*
								Profile();
			bool 				IsDirty();
			void 				SetDirty(bool dirty = true);
			image_id			ImageId();
			BResources*			Resources();

private:
			bool 				fIsDirty;
			NetworkSetupProfile*
								fProfile;
			image_id			fImage;
			BResources*			fResources;
};


extern "C" {

#define NETWORK_SETUP_ADDON_INSTANCIATE_FUNC_NAME "get_nth_addon"
typedef NetworkSetupAddOn* (*network_setup_addon_instantiate)(image_id image,
	int index);

extern NetworkSetupAddOn* get_nth_addon(image_id image, int index);

}


#endif // NETWORKSETUPADDON_H
