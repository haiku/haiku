// FileSystem.cpp

#include "FileSystem.h"

#include "AutoLocker.h"
#include "Volume.h"


FileSystem* FileSystem::sInstance = NULL;


// constructor
FileSystem::FileSystem()
{
	sInstance = this;
}


// destructor
FileSystem::~FileSystem()
{
	sInstance = NULL;
}


/*static*/ FileSystem*
FileSystem::GetInstance()
{
	return sInstance;
}


void
FileSystem::RegisterVolume(Volume* volume)
{
	AutoLocker<Locker> _(fLock);
	fVolumes.Add(volume);
}


void
FileSystem::UnregisterVolume(Volume* volume)
{
	AutoLocker<Locker> _(fLock);
	fVolumes.Remove(volume);
}


Volume*
FileSystem::VolumeWithID(dev_t id)
{
	AutoLocker<Locker> _(fLock);

	VolumeList::Iterator it = fVolumes.GetIterator();
	while (Volume* volume = it.Next()) {
		if (volume->GetID() == id)
			return volume;
	}

	return NULL;
}
