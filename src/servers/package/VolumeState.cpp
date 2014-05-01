/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "VolumeState.h"

#include <AutoDeleter.h>
#include <AutoLocker.h>


VolumeState::VolumeState()
	:
	fPackagesByFileName(),
	fPackagesByNodeRef()
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
	return fPackagesByFileName.Init() == B_OK
		&& fPackagesByNodeRef.Init() == B_OK;
}


void
VolumeState::AddPackage(Package* package)
{
	fPackagesByFileName.Insert(package);
	fPackagesByNodeRef.Insert(package);
}


void
VolumeState::RemovePackage(Package* package)
{
	fPackagesByFileName.Remove(package);
	fPackagesByNodeRef.Remove(package);
}


void
VolumeState::SetPackageActive(Package* package, bool active)
{
	package->SetActive(active);
}


void
VolumeState::ActivationChanged(const PackageSet& activatedPackage,
	const PackageSet& deactivatePackages)
{
	for (PackageSet::iterator it = activatedPackage.begin();
			it != activatedPackage.end(); ++it) {
		(*it)->SetActive(true);
	}

	for (PackageSet::iterator it = deactivatePackages.begin();
			it != deactivatePackages.end(); ++it) {
		Package* package = *it;
		RemovePackage(package);
		delete package;
	}
}


VolumeState*
VolumeState::Clone() const
{
	VolumeState* clone = new(std::nothrow) VolumeState;
	if (clone == NULL)
		return NULL;
	ObjectDeleter<VolumeState> cloneDeleter(clone);

	for (PackageFileNameHashTable::Iterator it
				= fPackagesByFileName.GetIterator();
			Package* package = it.Next();) {
		Package* clonedPackage = package->Clone();
		if (clonedPackage == NULL)
			return NULL;
		clone->AddPackage(clonedPackage);
	}

	return cloneDeleter.Detach();
}
