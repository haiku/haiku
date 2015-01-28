/*
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkSetupAddOn.h"

#include <stdio.h>
#include <stdlib.h>


NetworkSetupAddOn::NetworkSetupAddOn(image_id image)
	:
	fIsDirty(false),
	fProfile(NULL),
	fImage(image),
	fResources(NULL)
{
}


NetworkSetupAddOn::~NetworkSetupAddOn()
{
	delete fResources;
}


status_t
NetworkSetupAddOn::Save()
{
	return B_OK;
}


status_t
NetworkSetupAddOn::Revert()
{
	return B_OK;
}


status_t
NetworkSetupAddOn::ProfileChanged(NetworkSetupProfile* newProfile)
{
	fProfile = newProfile;
	return B_OK;
}


bool
NetworkSetupAddOn::IsDirty()
{
	return fIsDirty;
}


void
NetworkSetupAddOn::SetDirty(bool dirty)
{
	fIsDirty = dirty;
}


NetworkSetupProfile*
NetworkSetupAddOn::Profile()
{
	return fProfile;
}


image_id
NetworkSetupAddOn::ImageId()
{
	return fImage;
}


const char*
NetworkSetupAddOn::Name()
{
	return "Dummy NetworkSetupAddon";
}


BResources*
NetworkSetupAddOn::Resources()
{
	if (fResources == NULL) {
		image_info info;
		if (get_image_info(fImage, &info) != B_OK)
			return NULL;

		BResources* resources = new BResources();
		BFile file(info.name, B_READ_ONLY);
		if (resources->SetTo(&file) == B_OK)
			fResources = resources;
		else
			delete resources;
	}
	return fResources;
}
