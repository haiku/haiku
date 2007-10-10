/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "DiskSystemAddOnManager.h"

#include <image.h>

#include <AppMisc.h>
#include <AutoLocker.h>

#include <DiskSystemAddOn.h>


// sManager
DiskSystemAddOnManager* DiskSystemAddOnManager::sManager = NULL;


// AddOnImage
struct DiskSystemAddOnManager::AddOnImage {
	image_id			image;
	int32				refCount;
};


// AddOn
struct DiskSystemAddOnManager::AddOn {
	AddOnImage*			image;
	BDiskSystemAddOn*	addOn;
	int32				refCount;
};


// Default
DiskSystemAddOnManager*
DiskSystemAddOnManager::Default()
{
	if (!sManager) {
		DiskSystemAddOnManager* manager = new DiskSystemAddOnManager();

		BPrivate::gInitializationLock.Lock();

		// set manager, if no one beat us to it
		if (!sManager) {
			sManager = manager;
			manager = NULL;
		}

		BPrivate::gInitializationLock.Unlock();

		// delete the object we created, if someone else was quicker
		delete manager;
	}

	return sManager;
}


// Lock
bool
DiskSystemAddOnManager::Lock()
{
	return fLock.Lock();
}


// Unlock
void
DiskSystemAddOnManager::Unlock()
{
	fLock.Unlock();
}


// LoadDiskSystems
void
DiskSystemAddOnManager::LoadDiskSystems()
{
	AutoLocker<BLocker> _(fLock);

	if (++fLoadCount > 1)
		return;

	// TODO: Load the add-ons ...
}


// UnloadDiskSystems
void
DiskSystemAddOnManager::UnloadDiskSystems()
{
	AutoLocker<BLocker> _(fLock);

	if (--fLoadCount == 0)
		return;

	// TODO: Unload the add-ons ...
}


// CountAddOns
int32
DiskSystemAddOnManager::CountAddOns() const
{
	return fAddOns.CountItems();
}


// AddOnAt
BDiskSystemAddOn*
DiskSystemAddOnManager::AddOnAt(int32 index) const
{
	AddOn* addOn = _AddOnAt(index);
	return addOn ? addOn->addOn : NULL;
}


// GetAddOn
BDiskSystemAddOn*
DiskSystemAddOnManager::GetAddOn(const char* name)
{
	if (!name)
		return NULL;

	AutoLocker<BLocker> _(fLock);

	for (int32 i = 0; AddOn* addOn = _AddOnAt(i); i++) {
		if (strcmp(addOn->addOn->Name(), name) == 0) {
			addOn->refCount++;
			return addOn->addOn;
		}
	}

	return NULL;
}


// PutAddOn
void
DiskSystemAddOnManager::PutAddOn(BDiskSystemAddOn* _addOn)
{
	if (!_addOn)
		return;

	AutoLocker<BLocker> _(fLock);

	for (int32 i = 0; AddOn* addOn = _AddOnAt(i); i++) {
		if (_addOn == addOn->addOn) {
			if (addOn->refCount == 0)
				debugger("DiskSystemAddOnManager: unbalanced PutAddOn()");
			else
				addOn->refCount--;
			return;
		}
	}
}


// constructor
DiskSystemAddOnManager::DiskSystemAddOnManager()
	: fLock("disk system add-ons manager"),
	  fAddOns(),
	  fLoadCount(0)
{
}


// _AddOnAt
DiskSystemAddOnManager::AddOn*
DiskSystemAddOnManager::_AddOnAt(int32 index) const
{
	return (AddOn*)fAddOns.ItemAt(index);
}

