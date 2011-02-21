/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */


#include "NetworkSetupAddOn.h"

#include <kernel/image.h>
#include <storage/Resources.h>

#include <stdio.h>
#include <stdlib.h>


NetworkSetupAddOn::NetworkSetupAddOn(image_id image)
	:
	fIsDirty(false),
	fProfile(NULL),
	fAddonImage(image),
	fAddonResources(NULL)
{
}


NetworkSetupAddOn::~NetworkSetupAddOn()
{
	delete fAddonResources;
}


BView*
NetworkSetupAddOn::CreateView(BRect* bounds)
{
	BView *v = new BView(*bounds, "a view", B_FOLLOW_ALL, B_WILL_DRAW);
	v->SetViewColor((rand() % 256), (rand() % 256), (rand() % 256));
	return v;
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
NetworkSetupAddOn::ProfileChanged(NetworkSetupProfile* new_profile)
{
	fProfile = new_profile;
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
	return fAddonImage;
}


const char*
NetworkSetupAddOn::Name()
{
	return "Dummy NetworkSetupAddon";
}


BResources*
NetworkSetupAddOn::Resources()
{
	if (!fAddonResources) {
		image_info info;
		if (get_image_info(fAddonImage, &info) != B_OK)
			return NULL;

		BResources *resources = new BResources();
		BFile addon_file(info.name, O_RDONLY);
		if (resources->SetTo(&addon_file) == B_OK)
			fAddonResources = resources;
		else
			delete resources;
	}
	return fAddonResources;
}
