/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemovePathsCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "PathContainer.h"
#include "Shape.h"
#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemovePathsCmd"


using std::nothrow;

// constructor
RemovePathsCommand::RemovePathsCommand(PathContainer* container,
									   VectorPath** const paths,
									   int32 count)
	: Command(),
	  fContainer(container),
	  fInfos(paths && count > 0 ? new (nothrow) PathInfo[count] : NULL),
	  fCount(count),
	  fPathsRemoved(false)
{
	if (!fContainer || !fInfos)
		return;

	for (int32 i = 0; i < fCount; i++) {
		fInfos[i].path = paths[i];
		fInfos[i].index = fContainer->IndexOf(paths[i]);
		if (paths[i]) {
			int32 listenerCount = paths[i]->CountListeners();
			for (int32 j = 0; j < listenerCount; j++) {
				Shape* shape = dynamic_cast<Shape*>(paths[i]->ListenerAtFast(j));
				if (shape)
					fInfos[i].shapes.AddItem((void*)shape);
			}
		}
	}
}

// destructor
RemovePathsCommand::~RemovePathsCommand()
{
	if (fPathsRemoved && fInfos) {
		for (int32 i = 0; i < fCount; i++) {
			if (fInfos[i].path)
				fInfos[i].path->Release();
		}
	}
	delete[] fInfos;
}

// InitCheck
status_t
RemovePathsCommand::InitCheck()
{
	return fContainer && fInfos ? B_OK : B_NO_INIT;
}

// Perform
status_t
RemovePathsCommand::Perform()
{
	// remove paths from container and shapes that reference them
	for (int32 i = 0; i < fCount; i++) {
		if (!fInfos[i].path)
			continue;
		fContainer->RemovePath(fInfos[i].path);
		int32 shapeCount = fInfos[i].shapes.CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			Shape* shape = (Shape*)fInfos[i].shapes.ItemAtFast(j);
			shape->Paths()->RemovePath(fInfos[i].path);
		}
	}
	fPathsRemoved = true;

	return B_OK;
}

// Undo
status_t
RemovePathsCommand::Undo()
{
	status_t ret = B_OK;

	// add paths to container and shapes which previously referenced them
	for (int32 i = 0; i < fCount; i++) {
		if (!fInfos[i].path)
			continue;
		if (!fContainer->AddPath(fInfos[i].path, fInfos[i].index)) {
			// roll back
			ret = B_ERROR;
			for (int32 j = i - 1; j >= 0; j--) {
				fContainer->RemovePath(fInfos[j].path);
				int32 shapeCount = fInfos[j].shapes.CountItems();
				for (int32 k = 0; k < shapeCount; k++) {
					Shape* shape = (Shape*)fInfos[j].shapes.ItemAtFast(k);
					shape->Paths()->RemovePath(fInfos[j].path);
				}
			}
			break;
		}
		int32 shapeCount = fInfos[i].shapes.CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			Shape* shape = (Shape*)fInfos[i].shapes.ItemAtFast(j);
			shape->Paths()->AddPath(fInfos[i].path);
		}
	}
	fPathsRemoved = false;

	return ret;
}

// GetName
void
RemovePathsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Remove Paths");
	else
		name << B_TRANSLATE("Remove Path");
}
