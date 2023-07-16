/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "RemovePathsCommand.h"

#include <new>

#include <Catalog.h>
#include <List.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Container.h"
#include "PathSourceShape.h"
#include "Shape.h"
#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemovePathsCmd"


using std::nothrow;


RemovePathsCommand::RemovePathsCommand(
		Container<VectorPath>* container, const int32* indices, int32 count)
	: RemoveCommand<VectorPath>(container, indices, count),
	  fShapes(indices && count > 0 ? new (nothrow) BList[count] : NULL)
{
	if (!fShapes)
		return;

	// get the shapes associated with each path
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i]) {
			int32 listenerCount = fItems[i]->CountListeners();
			for (int32 j = 0; j < listenerCount; j++) {
				PathSourceShape* shape
					= dynamic_cast<PathSourceShape*>(fItems[i]->ListenerAtFast(j));
				if (shape)
					fShapes[i].AddItem((void*)shape);
			}
		}
	}
}


RemovePathsCommand::~RemovePathsCommand()
{
	delete[] fShapes;
}


status_t
RemovePathsCommand::InitCheck()
{
	return fShapes ? B_OK : B_NO_INIT;
}


status_t
RemovePathsCommand::Perform()
{
	// remove paths from the container
	status_t ret = RemoveCommand<VectorPath>::Perform();
	if (ret != B_OK)
		return ret;
	fItemsRemoved = false; // We're not done yet!

	// remove paths from shapes that reference them
	for (int32 i = 0; i < fCount; i++) {
		if (!fItems[i])
			continue;
		int32 shapeCount = fShapes[i].CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			PathSourceShape* shape = (PathSourceShape*)fShapes[i].ItemAtFast(j);
			shape->Paths()->RemoveItem(fItems[i]);
		}
	}
	fItemsRemoved = true;

	return ret;
}


status_t
RemovePathsCommand::Undo()
{
	// add paths to the container
	status_t ret = RemoveCommand<VectorPath>::Undo();
	if (ret != B_OK)
		return ret;
	fItemsRemoved = true; // We're not done yet!

	// add paths to shapes which previously referenced them
	for (int32 i = 0; i < fCount; i++) {
		if (!fItems[i])
			continue;
		int32 shapeCount = fShapes[i].CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			PathSourceShape* shape = (PathSourceShape*)fShapes[i].ItemAtFast(j);
			shape->Paths()->AddItem(fItems[i]);
		}
	}

	fItemsRemoved = false;

	return ret;
}


void
RemovePathsCommand::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Remove {0, plural, "
		"one{Path} other{Paths}}"));
	format.Format(name, fCount);
}
