#include <stdlib.h>

#include "NetworkSetupAddOn.h"


NetworkSetupAddOn::NetworkSetupAddOn()
	: fIsDirty(false), fProfile(NULL)
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


status_t NetworkSetupAddOn::SetProfile(NetworkProfile *new_profile)
{
	fProfile = new_profile;
	return B_OK;
}

const char * NetworkSetupAddOn::Name()
{
	return "Dummy NetworkSetupAddon";
}

