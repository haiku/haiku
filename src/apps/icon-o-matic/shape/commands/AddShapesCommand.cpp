/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AddShapesCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "Selection.h"
#include "ShapeContainer.h"
#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddShapesCmd"


using std::nothrow;

// constructor
AddShapesCommand::AddShapesCommand(ShapeContainer* container,
								   Shape** const shapes,
								   int32 count,
								   int32 index,
								   Selection* selection)
	: Command(),
	  fContainer(container),
	  fShapes(shapes && count > 0 ? new (nothrow) Shape*[count] : NULL),
	  fCount(count),
	  fIndex(index),
	  fShapesAdded(false),

	  fSelection(selection)
{
	if (!fContainer || !fShapes)
		return;

	memcpy(fShapes, shapes, sizeof(Shape*) * fCount);
}

// destructor
AddShapesCommand::~AddShapesCommand()
{
	if (!fShapesAdded && fShapes) {
		for (int32 i = 0; i < fCount; i++)
			fShapes[i]->Release();
	}
	delete[] fShapes;
}

// InitCheck
status_t
AddShapesCommand::InitCheck()
{
	return fContainer && fShapes ? B_OK : B_NO_INIT;
}

// Perform
status_t
AddShapesCommand::Perform()
{
	status_t ret = B_OK;

	// add shapes to container
	int32 index = fIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->AddShape(fShapes[i], index)) {
			ret = B_ERROR;
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemoveShape(fShapes[j]);
			break;
		}
		index++;
	}
	fShapesAdded = true;

	return ret;
}

// Undo
status_t
AddShapesCommand::Undo()
{
	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fSelection)
			fSelection->Deselect(fShapes[i]);
		fContainer->RemoveShape(fShapes[i]);
	}
	fShapesAdded = false;

	return B_OK;
}

// GetName
void
AddShapesCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Shapes");
	else
		name << B_TRANSLATE("Add Shape");
}
