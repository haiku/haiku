#include <stdlib.h>

#include "NetworkSetupProfile.h"


NetworkSetupProfile::NetworkSetupProfile()
{
	root = new BEntry();
	path = new BPath();
}

NetworkSetupProfile::NetworkSetupProfile(const char *path)
{
	SetTo(path);
}

NetworkSetupProfile::NetworkSetupProfile(const entry_ref *ref)
{
	SetTo(ref);
}

NetworkSetupProfile::NetworkSetupProfile(BEntry *entry)
{
	SetTo(entry);
}


NetworkSetupProfile::~NetworkSetupProfile()
{
	delete root;
}


status_t NetworkSetupProfile::SetTo(const char *path)
{
	BEntry *entry = new BEntry(path);
	SetTo(entry);
	return B_OK;
}


status_t NetworkSetupProfile::SetTo(const entry_ref *ref)
{
	BEntry *entry = new BEntry(ref);
	SetTo(entry);
	return B_OK;
}


status_t NetworkSetupProfile::SetTo(BEntry *entry)
{
	delete root;
	delete path;
	root = entry;
	return B_OK;
}


const char * NetworkSetupProfile::Name()
{
	if (!name) {
		root->GetPath(path);
		name = path->Leaf();
	};

	return name;
}


status_t NetworkSetupProfile::SetName(const char *name)
{
	return B_OK;
}


void NetworkSetupProfile::Unset()
{
}


bool NetworkSetupProfile::Exists()
{
	return root->Exists();
}


status_t NetworkSetupProfile::Create()
{
	return B_ERROR;
}


status_t NetworkSetupProfile::Delete()
{
	return B_ERROR;
}


bool NetworkSetupProfile::IsDefault()
{
	return is_default;
}


bool NetworkSetupProfile::IsActive()
{
	return is_active;
}


status_t NetworkSetupProfile::MakeActive()
{
	return B_ERROR;
}
