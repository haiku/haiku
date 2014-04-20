/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "VolumeState.h"

#include <AutoLocker.h>


VolumeState::VolumeState()
	:
	fLock("volume state"),
	fPackagesByFileName(),
	fPackagesByNodeRef(),
	fChangeCount(0)
{
}


VolumeState::~VolumeState()
{
	fPackagesByFileName.Clear();

	Package* package = fPackagesByNodeRef.Clear(true);
	while (package != NULL) {
		Package* next = package->NodeRefHashTableNext();
		delete package;
		package = next;
	}
}


bool
VolumeState::Init()
{
	return fLock.InitCheck() == B_OK && fPackagesByFileName.Init() == B_OK
		&& fPackagesByNodeRef.Init() == B_OK;
}


void
VolumeState::AddPackage(Package* package)
{
	AutoLocker<BLocker> locker(fLock);
	fPackagesByFileName.Insert(package);
	fPackagesByNodeRef.Insert(package);
}


void
VolumeState::RemovePackage(Package* package)
{
	AutoLocker<BLocker> locker(fLock);
	_RemovePackage(package);
}


void
VolumeState::SetPackageActive(Package* package, bool active)
{
	AutoLocker<BLocker> locker(fLock);
	package->SetActive(active);
}


void
VolumeState::ActivationChanged(const PackageSet& activatedPackage,
	const PackageSet& deactivatePackages)
{
	AutoLocker<BLocker> locker(fLock);

	for (PackageSet::iterator it = activatedPackage.begin();
			it != activatedPackage.end(); ++it) {
		(*it)->SetActive(true);
		fChangeCount++;
	}

	for (PackageSet::iterator it = deactivatePackages.begin();
			it != deactivatePackages.end(); ++it) {
		Package* package = *it;
		_RemovePackage(package);
		delete package;
	}
}


void
VolumeState::_RemovePackage(Package* package)
{
	fPackagesByFileName.Remove(package);
	fPackagesByNodeRef.Remove(package);
	fChangeCount++;
}
