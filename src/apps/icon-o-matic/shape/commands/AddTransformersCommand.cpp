/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AddTransformersCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "ShapeContainer.h"
#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddTransformersCmd"


using std::nothrow;

// constructor
AddTransformersCommand::AddTransformersCommand(Shape* container,
											   Transformer** const transformers,
											   int32 count,
											   int32 index)
	: Command(),
	  fContainer(container),
	  fTransformers(transformers && count > 0 ?
	  				new (nothrow) Transformer*[count] : NULL),
	  fCount(count),
	  fIndex(index),
	  fTransformersAdded(false)
{
	if (!fContainer || !fTransformers)
		return;

	memcpy(fTransformers, transformers, sizeof(Transformer*) * fCount);
}

// destructor
AddTransformersCommand::~AddTransformersCommand()
{
	if (!fTransformersAdded && fTransformers) {
		for (int32 i = 0; i < fCount; i++)
			delete fTransformers[i];
	}
	delete[] fTransformers;
}

// InitCheck
status_t
AddTransformersCommand::InitCheck()
{
	return fContainer && fTransformers ? B_OK : B_NO_INIT;
}

// Perform
status_t
AddTransformersCommand::Perform()
{
	status_t ret = B_OK;

	// add transformers to container
	int32 index = fIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fTransformers[i]
			&& !fContainer->AddTransformer(fTransformers[i], index)) {
			ret = B_ERROR;
			// roll back
			for (int32 j = i - 1; j >= 0; j--)
				fContainer->RemoveTransformer(fTransformers[j]);
			break;
		}
		index++;
	}
	fTransformersAdded = true;

	return ret;
}

// Undo
status_t
AddTransformersCommand::Undo()
{
	status_t ret = B_OK;

	// add shapes to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		fContainer->RemoveTransformer(fTransformers[i]);
	}
	fTransformersAdded = false;

	return ret;
}

// GetName
void
AddTransformersCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Transformers");
	else
		name << B_TRANSLATE("Add Transformer");
}
