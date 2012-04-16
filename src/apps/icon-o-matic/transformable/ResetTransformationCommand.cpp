/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ResetTransformationCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "ChannelTransform.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ResetTransformationCmd"


using std::nothrow;

// constructor
ResetTransformationCommand::ResetTransformationCommand(
								Transformable** const objects,
								int32 count)
	: Command(),
	  fObjects(objects && count > 0 ?
	  		   new (nothrow) Transformable*[count] : NULL),
	  fOriginals(objects && count > 0 ?
	  			 new (nothrow) double[
	  			 	count * Transformable::matrix_size] : NULL),
	  fCount(count)
{
	if (!fObjects || !fOriginals)
		return;

	memcpy(fObjects, objects, fCount * sizeof(Transformable*));

	int32 matrixSize = Transformable::matrix_size;
	for (int32 i = 0; i < fCount; i++) {
		fObjects[i]->StoreTo(&fOriginals[matrixSize * i]);
	}
}

// destructor
ResetTransformationCommand::~ResetTransformationCommand()
{
	delete[] fObjects;
	delete[] fOriginals;
}

// InitCheck
status_t
ResetTransformationCommand::InitCheck()
{
	return fObjects && fOriginals ? B_OK : B_NO_INIT;
}

// Perform
status_t
ResetTransformationCommand::Perform()
{
	// reset transformations
	for (int32 i = 0; i < fCount; i++) {
		if (fObjects[i])
			fObjects[i]->Reset();
	}
	return B_OK;
}

// Undo
status_t
ResetTransformationCommand::Undo()
{
	// reset transformations
	int32 matrixSize = Transformable::matrix_size;
	for (int32 i = 0; i < fCount; i++) {
		if (fObjects[i])
			fObjects[i]->LoadFrom(&fOriginals[i * matrixSize]);
	}
	return B_OK;
}

// GetName
void
ResetTransformationCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Reset Transformations");
	else
		name << B_TRANSLATE("Reset Transformation");
}
