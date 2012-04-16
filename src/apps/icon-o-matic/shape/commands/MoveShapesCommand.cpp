/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MoveShapesCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "ShapeContainer.h"
#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MoveShapesCommand"


using std::nothrow;

// constructor
MoveShapesCommand::MoveShapesCommand(ShapeContainer* container,
									 Shape** shapes,
									 int32 count,
									 int32 toIndex)
	: Command(),
	  fContainer(container),
	  fShapes(shapes),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fToIndex(toIndex),
	  fCount(count)
{
	if (!fContainer || !fShapes || !fIndices)
		return;

	// init original shape indices and
	// adjust toIndex compensating for items that
	// are removed before that index
	int32 itemsBeforeIndex = 0;
	for (int32 i = 0; i < fCount; i++) {
		fIndices[i] = fContainer->IndexOf(fShapes[i]);
		if (fIndices[i] >= 0 && fIndices[i] < fToIndex)
			itemsBeforeIndex++;
	}
	fToIndex -= itemsBeforeIndex;
}

// destructor
MoveShapesCommand::~MoveShapesCommand()
{
	delete[] fShapes;
	delete[] fIndices;
}

// InitCheck
status_t
MoveShapesCommand::InitCheck()
{
	if (!fContainer || !fShapes || !fIndices)
		return B_NO_INIT;

	// analyse the move, don't return B_OK in case
	// the container state does not change...

	int32 index = fIndices[0];
		// NOTE: fIndices == NULL if fCount < 1

	if (index != fToIndex) {
		// a change is guaranteed
		return B_OK;
	}

	// the insertion index is the same as the index of the first
	// moved item, a change only occures if the indices of the
	// moved items is not contiguous
	bool isContiguous = true;
	for (int32 i = 1; i < fCount; i++) {
		if (fIndices[i] != index + 1) {
			isContiguous = false;
			break;
		}
		index = fIndices[i];
	}
	if (isContiguous) {
		// the container state will not change because of the move
		return B_ERROR;
	}

	return B_OK;
}

// Perform
status_t
MoveShapesCommand::Perform()
{
	status_t ret = B_OK;

	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->RemoveShape(fShapes[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add shapes to container at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->AddShape(fShapes[i], index++)) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}

// Undo
status_t
MoveShapesCommand::Undo()
{
	status_t ret = B_OK;

	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->RemoveShape(fShapes[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add shapes to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fShapes[i] && !fContainer->AddShape(fShapes[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}

// GetName
void
MoveShapesCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_MODIFIERS, "Move Shapes");
//	else
//		name << _GetString(MOVE_MODIFIER, "Move Shape");
	if (fCount > 1)
		name << B_TRANSLATE("Move Shapes");
	else
		name << B_TRANSLATE("Move Shape");
}
