#include <stdlib.h>

#include "NetworkSetupAddOn.h"


NetworkSetupAddOn::NetworkSetupAddOn(image_id image)
	: is_dirty(false), profile(NULL), addon_image(image)
{
}

NetworkSetupAddOn::~NetworkSetupAddOn()
{
}

BView * NetworkSetupAddOn::CreateView(BRect *bounds)
{
	BView *v = new BView(*bounds, "a view", B_FOLLOW_ALL, B_WILL_DRAW);
	v->SetViewColor((rand() % 256), (rand() % 256), (rand() % 256));
	return v;
}


status_t NetworkSetupAddOn::Save()
{
	return B_OK;
}


status_t NetworkSetupAddOn::Revert()
{
	return B_OK;
}


status_t NetworkSetupAddOn::ProfileChanged(NetworkSetupProfile *new_profile)
{
	profile = new_profile;
	return B_OK;
}

const char * NetworkSetupAddOn::Name()
{
	return "Dummy NetworkSetupAddon";
}

