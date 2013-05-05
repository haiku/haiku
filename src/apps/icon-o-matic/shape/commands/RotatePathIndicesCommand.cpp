/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "RotatePathIndicesCommand.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RotatePathIndiciesCmd"


RotatePathIndicesCommand::RotatePathIndicesCommand(VectorPath* path,
		bool clockWise)
	:
	PathCommand(path),
	fClockWise(clockWise)
{
}


RotatePathIndicesCommand::~RotatePathIndicesCommand()
{
}


status_t
RotatePathIndicesCommand::InitCheck()
{
	status_t ret = PathCommand::InitCheck();
	if (ret == B_OK && fPath->CountPoints() < 2)
		return B_NOT_SUPPORTED;
	return ret;
}


status_t
RotatePathIndicesCommand::Perform()
{
	return _Rotate(fClockWise);
}


status_t
RotatePathIndicesCommand::Undo()
{
	return _Rotate(!fClockWise);
}


void
RotatePathIndicesCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Rotate Path Indices");
}


status_t
RotatePathIndicesCommand::_Rotate(bool clockWise)
{
	BPoint point;
	BPoint pointIn;
	BPoint pointOut;
	bool connected;

	int32 removeIndex;
	int32 addIndex;
	if (!clockWise) {
		removeIndex = fPath->CountPoints() - 1;
		addIndex = 0;
	} else {
		removeIndex = 0;
		addIndex = fPath->CountPoints() - 1;
	}

	if (!fPath->GetPointsAt(removeIndex, point, pointIn, pointOut, &connected))
		return B_ERROR;

	fPath->RemovePoint(removeIndex);
	fPath->AddPoint(point, addIndex);
	fPath->SetPoint(addIndex, point, pointIn, pointOut, connected);

	return B_OK;
}

