/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemoveTransformersCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "ShapeContainer.h"
#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveTransformersCmd"


using std::nothrow;

// constructor
RemoveTransformersCommand::RemoveTransformersCommand(Shape* container,
													 const int32* indices,
													 int32 count)
	: Command(),
	  fContainer(container),
	  fTransformers(count > 0 ? new (nothrow) Transformer*[count] : NULL),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fCount(count),
	  fTransformersRemoved(false)
{
	if (!fContainer || !fTransformers || !fIndices)
		return;

	memcpy(fIndices, indices, sizeof(int32) * fCount);
	for (int32 i = 0; i < fCount; i++)
		fTransformers[i] = fContainer->TransformerAt(fIndices[i]);
}

// destructor
RemoveTransformersCommand::~RemoveTransformersCommand()
{
	if (fTransformersRemoved && fTransformers) {
		for (int32 i = 0; i < fCount; i++)
			delete fTransformers[i];
	}
	delete[] fTransformers;
	delete[] fIndices;
}

// InitCheck
status_t
RemoveTransformersCommand::InitCheck()
{
	return fContainer && fTransformers && fIndices ? B_OK : B_NO_INIT;
}

// Perform
status_t
RemoveTransformersCommand::Perform()
{
	status_t ret = B_OK;

	// remove shapes from container
	for (int32 i = 0; i < fCount; i++) {
		if (fTransformers[i]
			&& !fContainer->RemoveTransformer(fTransformers[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fTransformersRemoved = true;

	return ret;
}

// Undo
status_t
RemoveTransformersCommand::Undo()
{
	status_t ret = B_OK;

	// add shapes to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fTransformers[i]
			&& !fContainer->AddTransformer(fTransformers[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fTransformersRemoved = false;

	return ret;
}

// GetName
void
RemoveTransformersCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_TRANSFORMERS, "Move Transformers");
//	else
//		name << _GetString(MOVE_TRANSFORMER, "Move Transformer");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Transformers");
	else
		name << B_TRANSLATE("Remove Transformer");
}
