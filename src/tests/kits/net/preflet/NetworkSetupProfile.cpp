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


bool NetworkSetupProfile::Exists()
{
	return root->Exists();
}


status_t NetworkSetupProfile::Delete()
{
	return B_ERROR;
}


bool NetworkSetupProfile::IsDefault()
{
	return is_default;
}


bool NetworkSetupProfile::IsCurrent()
{
	return is_current;
}


status_t NetworkSetupProfile::MakeCurrent()
{
	return B_ERROR;
}

// #pragma mark -


NetworkSetupProfile * NetworkSetupProfile::Default()
{
	return NULL;
}


NetworkSetupProfile * NetworkSetupProfile::Current()
{
	return NULL;
}

