/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ResolvableFamily.h"

#include "Package.h"
#include "Volume.h"


void
ResolvableFamily::AddResolvable(Resolvable* resolvable,
	ResolvableDependencyList& dependenciesToUpdate)
{
	// Find the insertion point in the list. We sort by mount type -- the more
	// specific the higher the priority.
	MountType mountType
		= resolvable->Package()->Domain()->Volume()->MountType();
	Resolvable* otherResolvable = NULL;
	for (FamilyResolvableList::Iterator it = fResolvables.GetIterator();
			(otherResolvable = it.Next()) != NULL;) {
		if (otherResolvable->Package()->Domain()->Volume()->MountType()
				<= mountType) {
			break;
		}
	}

	fResolvables.InsertBefore(otherResolvable, resolvable);
	resolvable->SetFamily(this);

	// all dependencies after the inserted resolvable potentially need to be
	// updated
	while ((resolvable = fResolvables.GetNext(resolvable)) != NULL)
		resolvable->MoveDependencies(dependenciesToUpdate);
}


void
ResolvableFamily::RemoveResolvable(Resolvable* resolvable,
	ResolvableDependencyList& dependenciesToUpdate)
{
	resolvable->SetFamily(NULL);
	fResolvables.Remove(resolvable);
}


bool
ResolvableFamily::ResolveDependency(Dependency* dependency)
{
	for (FamilyResolvableList::Iterator it = fResolvables.GetIterator();
			Resolvable* resolvable = it.Next();) {
		if (dependency->ResolvableVersionMatches(resolvable->Version())) {
			resolvable->AddDependency(dependency);
			return true;
		}
	}

	return false;
}
