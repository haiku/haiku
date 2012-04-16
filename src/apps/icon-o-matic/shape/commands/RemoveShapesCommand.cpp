/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemoveShapesCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "ShapeContainer.h"
#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveShapesCmd"


using std::nothrow;

// constructor
RemoveShapesCommand::RemoveShapesCommand(ShapeContainer* container,
										 int32* const indices,
										 int32 count)
	: Command(),
	  fContainer(container),
	  fShapes(count > 0 ? new (nothrow) Shape*[count] : NULL),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fCount(count),
	  fShapesRemoved(false)
{
	if (!fContainer || !fShapes || !fIndices)
		return;

	memcpy(fIndices, indices, sizeof(int32) * fCount);
	for (int32 i = 0; i < fCount; i++)
		fShapes[i] = fContainer->ShapeAt(fIndices[i]);
}

// destructor
RemoveShapesCommand::~RemoveShapesCommand()
{
	if (fShapesRemoved && fShapes) {
		for (int32 i = 0; i < fCount; i++)
			fShapes[i]->Release();
	}
	delete[] fShapes;
	delete[] fIndices;
}

// InitCheck
status_t
RemoveShapesCommand::InitCheck()
{
	return fContainer && fShapes && fIndices ? B_OK : B_NO_INIT;
}

// Perform
status_t
RemoveShapesCommand::Perform()
{
	status_t ret = B_OK;

	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->RemoveShape(fShapes[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fShapesRemoved = true;

	return ret;
}

// Undo
status_t
RemoveShapesCommand::Undo()
{
	status_t ret = B_OK;

	// add shapes to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->AddShape(fShapes[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fShapesRemoved = false;

	return ret;
}

// GetName
void
RemoveShapesCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_MODIFIERS, "Move Shapes");
//	else
//		name << _GetString(MOVE_MODIFIER, "Move Shape");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Shapes");
	else
		name << B_TRANSLATE("Remove Shape");
}
