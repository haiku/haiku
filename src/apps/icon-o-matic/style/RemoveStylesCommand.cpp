/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "RemoveStylesCommand.h"

#include <new>

#include <Catalog.h>
#include <Locale.h>

#include "PathSourceShape.h"
#include "Style.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveStylesCmd"


using std::nothrow;


RemoveStylesCommand::RemoveStylesCommand(Container<Style>* container,
		int32* const indices, int32 count)
	: RemoveCommand<Style>(container, indices, count),
	  fShapes(indices && count > 0 ? new (nothrow) BList[count] : NULL)
{
	if (!fShapes)
		return;

	// get the shapes associated with each style
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i]) {
			int32 listenerCount = fItems[i]->CountObservers();
			for (int32 j = 0; j < listenerCount; j++) {
				PathSourceShape* shape
					= dynamic_cast<PathSourceShape*>(fItems[i]->ObserverAtFast(j));
				if (shape != NULL)
					fShapes[i].AddItem((void*)shape);
			}
		}
	}
}


RemoveStylesCommand::~RemoveStylesCommand()
{
	delete[] fShapes;
}


status_t
RemoveStylesCommand::InitCheck()
{
	return fShapes ? B_OK : B_NO_INIT;
}


status_t
RemoveStylesCommand::Perform()
{
	// remove styles from the container
	status_t ret = RemoveCommand<Style>::Perform();
	if (ret != B_OK)
		return ret;
	fItemsRemoved = false; // We're not done yet!

	// remove styles from shapes that reference them
	for (int32 i = 0; i < fCount; i++) {
		if (!fItems[i])
			continue;
		int32 shapeCount = fShapes[i].CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			PathSourceShape* shape = (PathSourceShape*)fShapes[i].ItemAtFast(j);
			shape->SetStyle(fContainer->ItemAt(0));
		}
	}

	fItemsRemoved = true;

	return B_OK;
}


status_t
RemoveStylesCommand::Undo()
{
	// add styles to the container
	status_t ret = RemoveCommand<Style>::Undo();
	if (ret != B_OK)
		return ret;
	fItemsRemoved = true; // We're not done yet!

	// add styles to the shapes which previously referenced them
	for (int32 i = 0; i < fCount; i++) {
		if (!fItems[i])
			continue;
		int32 shapeCount = fShapes[i].CountItems();
		for (int32 j = 0; j < shapeCount; j++) {
			PathSourceShape* shape = (PathSourceShape*)fShapes[i].ItemAtFast(j);
			shape->SetStyle(fItems[i]);
		}
	}

	fItemsRemoved = false;

	return ret;
}


void
RemoveStylesCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Remove Styles");
	else
		name << B_TRANSLATE("Remove Style");
}
