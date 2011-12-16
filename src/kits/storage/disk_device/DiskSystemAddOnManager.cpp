/*
 * Copyright 2007-2009, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <DiskSystemAddOnManager.h>

#include <exception>
#include <new>
#include <set>
#include <string>

#include <stdio.h>

#include <Directory.h>
#include <Entry.h>
#include <image.h>
#include <Path.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include <DiskSystemAddOn.h>


#undef TRACE
#define TRACE(format...)
//#define TRACE(format...)	printf(format)


using std::nothrow;


static pthread_once_t sManagerInitOnce = PTHREAD_ONCE_INIT;
DiskSystemAddOnManager* DiskSystemAddOnManager::sManager = NULL;


// AddOnImage
struct DiskSystemAddOnManager::AddOnImage {
	AddOnImage(image_id image)
		: image(image),
		  refCount(0)
	{
	}

	~AddOnImage()
	{
		unload_add_on(image);
	}

	image_id			image;
	int32				refCount;
};


// AddOn
struct DiskSystemAddOnManager::AddOn {
	AddOn(AddOnImage* image, BDiskSystemAddOn* addOn)
		: image(image),
		  addOn(addOn),
		  refCount(1)
	{
	}

	AddOnImage*			image;
	BDiskSystemAddOn*	addOn;
	int32				refCount;
};


// StringSet
struct DiskSystemAddOnManager::StringSet : std::set<std::string> {
};


// Default
DiskSystemAddOnManager*
DiskSystemAddOnManager::Default()
{
	if (sManager == NULL)
		pthread_once(&sManagerInitOnce, &_InitSingleton);

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
status_t
DiskSystemAddOnManager::LoadDiskSystems()
{
	AutoLocker<BLocker> _(fLock);

	if (++fLoadCount > 1)
		return B_OK;

	StringSet alreadyLoaded;
	status_t error = _LoadAddOns(alreadyLoaded, B_USER_ADDONS_DIRECTORY);

	if (error == B_OK)
		error = _LoadAddOns(alreadyLoaded, B_COMMON_ADDONS_DIRECTORY);

	if (error == B_OK)
		error = _LoadAddOns(alreadyLoaded, B_SYSTEM_ADDONS_DIRECTORY);

	if (error != B_OK)
		UnloadDiskSystems();

	return error;
}


// UnloadDiskSystems
void
DiskSystemAddOnManager::UnloadDiskSystems()
{
	AutoLocker<BLocker> _(fLock);

	if (fLoadCount == 0 || --fLoadCount > 0)
		return;

	fAddOnsToBeUnloaded.AddList(&fAddOns);
	fAddOns.MakeEmpty();

	// put all add-ons -- that will cause them to be deleted as soon as they're
	// unused
	for (int32 i = fAddOnsToBeUnloaded.CountItems() - 1; i >= 0; i--)
		_PutAddOn(i);
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
			if (addOn->refCount > 1) {
				addOn->refCount--;
			} else {
				debugger("Unbalanced call to "
					"DiskSystemAddOnManager::PutAddOn()");
			}
			return;
		}
	}

	for (int32 i = 0;
		 AddOn* addOn = (AddOn*)fAddOnsToBeUnloaded.ItemAt(i); i++) {
		if (_addOn == addOn->addOn) {
			_PutAddOn(i);
			return;
		}
	}

	debugger("DiskSystemAddOnManager::PutAddOn(): disk system not found");
}


// constructor
DiskSystemAddOnManager::DiskSystemAddOnManager()
	: fLock("disk system add-ons manager"),
	  fAddOns(),
	  fAddOnsToBeUnloaded(),
	  fLoadCount(0)
{
}


/*static*/ void
DiskSystemAddOnManager::_InitSingleton()
{
	sManager = new DiskSystemAddOnManager();
}


// _AddOnAt
DiskSystemAddOnManager::AddOn*
DiskSystemAddOnManager::_AddOnAt(int32 index) const
{
	return (AddOn*)fAddOns.ItemAt(index);
}


// _PutAddOn
void
DiskSystemAddOnManager::_PutAddOn(int32 index)
{
	AddOn* addOn = (AddOn*)fAddOnsToBeUnloaded.ItemAt(index);
	if (!addOn)
		return;

	if (--addOn->refCount == 0) {
		if (--addOn->image->refCount == 0)
			delete addOn->image;

		fAddOnsToBeUnloaded.RemoveItem(index);
		delete addOn;
	}
}


// _LoadAddOns
status_t
DiskSystemAddOnManager::_LoadAddOns(StringSet& alreadyLoaded,
	directory_which addOnDir)
{
	// get the add-on directory path
	BPath path;
	status_t error = find_directory(addOnDir, &path, false);
	if (error != B_OK)
		return error;

	TRACE("DiskSystemAddOnManager::_LoadAddOns(): %s\n", path.Path());

	error = path.Append("disk_systems");
	if (error != B_OK)
		return error;

	if (!BEntry(path.Path()).Exists())
		return B_OK;

	// open the directory and iterate through its entries
	BDirectory directory;
	error = directory.SetTo(path.Path());
	if (error != B_OK)
		return error;

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		// skip, if already loaded
		if (alreadyLoaded.find(ref.name) != alreadyLoaded.end()) {
			TRACE("  skipping \"%s\" -- already loaded\n", ref.name);
			continue;
		}

		// get the entry path
		BPath entryPath;
		error = entryPath.SetTo(&ref);
		if (error != B_OK) {
			if (error == B_NO_MEMORY)
				return error;
			TRACE("  skipping \"%s\" -- failed to get path\n", ref.name);
			continue;
		}

		// load the add-on
		image_id image = load_add_on(entryPath.Path());
		if (image < 0) {
			TRACE("  skipping \"%s\" -- failed to load add-on\n", ref.name);
			continue;
		}

		AddOnImage* addOnImage = new(nothrow) AddOnImage(image);
		if (!addOnImage) {
			unload_add_on(image);
			return B_NO_MEMORY;
		}
		ObjectDeleter<AddOnImage> addOnImageDeleter(addOnImage);

		// get the add-on objects
		status_t (*getAddOns)(BList*);
		error = get_image_symbol(image, "get_disk_system_add_ons",
			B_SYMBOL_TYPE_TEXT, (void**)&getAddOns);
		if (error != B_OK) {
			TRACE("  skipping \"%s\" -- function symbol not found\n", ref.name);
			continue;
		}

		BList addOns;
		error = getAddOns(&addOns);
		if (error != B_OK || addOns.IsEmpty()) {
			TRACE("  skipping \"%s\" -- getting add-ons failed\n", ref.name);
			continue;
		}

		// create and add AddOn objects
		int32 count = addOns.CountItems();
		for (int32 i = 0; i < count; i++) {
			BDiskSystemAddOn* diskSystemAddOn
				= (BDiskSystemAddOn*)addOns.ItemAt(i);
			AddOn* addOn = new(nothrow) AddOn(addOnImage, diskSystemAddOn);
			if (!addOn)
				return B_NO_MEMORY;

			if (fAddOns.AddItem(addOn)) {
				addOnImage->refCount++;
				addOnImageDeleter.Detach();
			} else {
				delete addOn;
				return B_NO_MEMORY;
			}
		}

		TRACE("  got %ld BDiskSystemAddOn(s) from add-on \"%s\"\n", count,
			ref.name);

		// add the add-on name to the set of already loaded add-ons
		try {
			alreadyLoaded.insert(ref.name);
		} catch (std::bad_alloc& exception) {
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}
