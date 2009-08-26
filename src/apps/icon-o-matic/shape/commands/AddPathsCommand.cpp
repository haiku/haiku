/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AddPathsCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "PathContainer.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
AddPathsCommand::AddPathsCommand(PathContainer* container,
								 VectorPath** const paths,
								 int32 count,
								 bool ownsPaths,
								 int32 index)
	: Command(),
	  fContainer(container),
	  fPaths(paths && count > 0 ? new (nothrow) VectorPath*[count] : NULL),
	  fCount(count),
	  fOwnsPaths(ownsPaths),
	  fIndex(index),
	  fPathsAdded(false)
{
	if (!fContainer || !fPaths)
		return;

	memcpy(fPaths, paths, sizeof(VectorPath*) * fCount);

	if (!fOwnsPaths) {
		// Add references to paths
		for (int32 i = 0; i < fCount; i++) {
			if (fPaths[i] != NULL)
				fPaths[i]->Acquire();
		}
	}
}

// destructor
AddPathsCommand::~AddPathsCommand()
{
	if (!fPathsAdded && fPaths) {
		for (int32 i = 0; i < fCount; i++) {
			if (fPaths[i] != NULL)
				fPaths[i]->Release();
		}
	}
	delete[] fPaths;
}

// InitCheck
status_t
AddPathsCommand::InitCheck()
{
	return fContainer && fPaths ? B_OK : B_NO_INIT;
}

// Perform
status_t
AddPathsCommand::Perform()
{
	// add paths to container
	for (int32 i = 0; i < fCount; i++) {
		if (fPaths[i] && !fContainer->AddPath(fPaths[i], fIndex + i)) {
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemovePath(fPaths[j]);
			return B_ERROR;
		}
	}
	fPathsAdded = true;

	return B_OK;
}

// Undo
status_t
AddPathsCommand::Undo()
{
	// remove paths from container
	for (int32 i = 0; i < fCount; i++) {
		fContainer->RemovePath(fPaths[i]);
	}
	fPathsAdded = false;

	return B_OK;
}

// GetName
void
AddPathsCommand::GetName(BString& name)
{
	if (fOwnsPaths) {
		if (fCount > 1)
			name << "Add Paths";
		else
			name << "Add Path";
	} else {
		if (fCount > 1)
			name << "Assign Paths";
		else
			name << "Assign Path";
	}
}
