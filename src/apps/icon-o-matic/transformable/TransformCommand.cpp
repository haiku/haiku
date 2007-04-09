/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformCommand.h"

#include <stdio.h>

// constructor
TransformCommand::TransformCommand(BPoint pivot,
								   BPoint translation,
								   double rotation,
								   double xScale,
								   double yScale,
								   const char* actionName,
								   uint32 nameIndex)
	: Command(),
	  fOldPivot(pivot),
	  fOldTranslation(translation),
	  fOldRotation(rotation),
	  fOldXScale(xScale),
	  fOldYScale(yScale),

	  fNewPivot(pivot),
	  fNewTranslation(translation),
	  fNewRotation(rotation),
	  fNewXScale(xScale),
	  fNewYScale(yScale),

	  fName(actionName),
	  fNameIndex(nameIndex)
{
}

// constructor
TransformCommand::TransformCommand(const char* actionName,
								   uint32 nameIndex)
	: Command(),
	  fOldPivot(B_ORIGIN),
	  fOldTranslation(B_ORIGIN),
	  fOldRotation(0.0),
	  fOldXScale(1.0),
	  fOldYScale(1.0),

	  fNewPivot(B_ORIGIN),
	  fNewTranslation(B_ORIGIN),
	  fNewRotation(0.0),
	  fNewXScale(1.0),
	  fNewYScale(1.0),

	  fName(actionName),
	  fNameIndex(nameIndex)
{
}

// destructor
TransformCommand::~TransformCommand()
{
}

// InitCheck
status_t
TransformCommand::InitCheck()
{
	if ((fNewPivot != fOldPivot
		 || fNewTranslation != fOldTranslation
		 || fNewRotation != fOldRotation
		 || fNewXScale != fOldXScale
		 || fNewYScale != fOldYScale))
		return B_OK;
	
	return B_NO_INIT;
}

// Perform
status_t
TransformCommand::Perform()
{
	// objects are already transformed
	return B_OK;
}

// Undo
status_t
TransformCommand::Undo()
{
	_SetTransformation(fOldPivot,
					   fOldTranslation,
					   fOldRotation,
					   fOldXScale,
					   fOldYScale);
	return B_OK;
}

// Redo
status_t
TransformCommand::Redo()
{
	_SetTransformation(fNewPivot,
					   fNewTranslation,
					   fNewRotation,
					   fNewXScale,
					   fNewYScale);
	return B_OK;
}

// GetName
void
TransformCommand::GetName(BString& name)
{
	name << _GetString(fNameIndex, fName.String());
}

// SetNewTransformation
void
TransformCommand::SetNewTransformation(BPoint pivot,
									   BPoint translation,
									   double rotation,
									   double xScale,
									   double yScale)
{
	fNewPivot = pivot;
	fNewTranslation = translation;
	fNewRotation = rotation;
	fNewXScale = xScale;
	fNewYScale = yScale;
}

// SetNewTranslation
void
TransformCommand::SetNewTranslation(BPoint translation)
{
	// NOTE: convinience method for nudging
	fNewTranslation = translation;
}

// SetName
void
TransformCommand::SetName(const char* actionName, uint32 nameIndex)
{
	fName.SetTo(actionName);
	fNameIndex = nameIndex;
}

