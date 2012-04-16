/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FreezeTransformationCommand.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>

#include "GradientTransformable.h"
#include "Shape.h"
#include "Style.h"
#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-FreezeTransformationCmd"


using std::nothrow;

// constructor
FreezeTransformationCommand::FreezeTransformationCommand(
								   Shape** const shapes,
								   int32 count)
	: Command(),
	  fShapes(shapes && count > 0 ? new (nothrow) Shape*[count] : NULL),
	  fOriginalTransformations(count > 0 ? new (nothrow) double[
									count * Transformable::matrix_size]
							   : NULL),
	  fCount(count)
{
	if (!fShapes || !fOriginalTransformations)
		return;

	memcpy(fShapes, shapes, sizeof(Shape*) * fCount);

	bool initOk = false;

	for (int32 i = 0; i < fCount; i++) {
		if (!fShapes[i])
			continue;
		if (!fShapes[i]->IsIdentity())
			initOk = true;
		fShapes[i]->StoreTo(&fOriginalTransformations[
			i * Transformable::matrix_size]);
	}

	if (!initOk) {
		delete[] fShapes;
		fShapes = NULL;
		delete[] fOriginalTransformations;
		fOriginalTransformations = NULL;
	}
}

// destructor
FreezeTransformationCommand::~FreezeTransformationCommand()
{
	delete[] fShapes;
	delete[] fOriginalTransformations;
}

// InitCheck
status_t
FreezeTransformationCommand::InitCheck()
{
	return fShapes && fOriginalTransformations ? B_OK : B_NO_INIT;
}

// Perform
status_t
FreezeTransformationCommand::Perform()
{
	for (int32 i = 0; i < fCount; i++) {
		if (!fShapes[i] || fShapes[i]->IsIdentity())
			continue;

		_ApplyTransformation(fShapes[i], *(fShapes[i]));
		fShapes[i]->Reset();
	}

	return B_OK;
}

// Undo
status_t
FreezeTransformationCommand::Undo()
{
	for (int32 i = 0; i < fCount; i++) {
		if (!fShapes[i])
			continue;

		// restore original transformation
		fShapes[i]->LoadFrom(&fOriginalTransformations[
			i * Transformable::matrix_size]);

		Transformable transform(*(fShapes[i]));
		if (!transform.IsValid() || transform.IsIdentity())
			continue;

		transform.Invert();
		_ApplyTransformation(fShapes[i], transform);
	}

	return B_OK;
}

// GetName
void
FreezeTransformationCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Freeze Shapes");
	else
		name << B_TRANSLATE("Freeze Shape");
}

// #pragma mark -

// _ApplyTransformation
void
FreezeTransformationCommand::_ApplyTransformation(Shape* shape,
									const Transformable& transform)
{
	// apply inverse of old shape transformation to every assigned path
	int32 pathCount = shape->Paths()->CountPaths();
	for (int32 i = 0; i < pathCount; i++) {
		VectorPath* path = shape->Paths()->PathAtFast(i);
		int32 shapes = 0;
		int32 listeners = path->CountListeners();
		for (int32 j = 0; j < listeners; j++) {
			if (dynamic_cast<Shape*>(path->ListenerAtFast(j)))
				shapes++;
		}
		// only freeze transformation of path if only one
		// shape has it assigned
		if (shapes == 1) {
			path->ApplyTransform(transform);
		} else {
			printf("Not transfering transformation of \"%s\" onto "
				   "path \"%s\", because %ld other shapes "
				   "have it assigned.\n", shape->Name(), path->Name(),
				   shapes - 1);
		}
	}
	// take care of style too
	if (shape->Style() && shape->Style()->Gradient()) {
		// TODO: not if more than one shape have this style assigned!
		shape->Style()->Gradient()->Multiply(transform);
	}
}
