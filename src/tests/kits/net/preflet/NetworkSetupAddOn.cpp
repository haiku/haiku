#include <stdlib.h>

#include <kernel/image.h>
#include <storage/Resources.h>

#include "NetworkSetupAddOn.h"


NetworkSetupAddOn::NetworkSetupAddOn(image_id image)
	: m_is_dirty(false), m_profile(NULL), m_addon_image(image), m_addon_resources(NULL)
{
}

NetworkSetupAddOn::~NetworkSetupAddOn()
{
	delete m_addon_resources;
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
	m_profile = new_profile;
	return B_OK;
}

const char * NetworkSetupAddOn::Name()
{
	return "Dummy NetworkSetupAddon";
}

BResources * NetworkSetupAddOn::Resources()
{
	if (!m_addon_resources) {
		image_info info;
		if (get_image_info(m_addon_image, &info) != B_OK)
			return NULL;
		
		BResources *resources = new BResources();
		BFile addon_file(info.name, O_RDONLY);
		if (resources->SetTo(&addon_file) == B_OK)
			m_addon_resources = resources;
		else
			delete resources;
	};
	return m_addon_resources;
}





