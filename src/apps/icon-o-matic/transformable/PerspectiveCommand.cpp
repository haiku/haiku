/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2006-2009, 2023, Haiku.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */


#include "PerspectiveCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "PerspectiveTransformer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-PerspectiveCommand"


PerspectiveCommand::PerspectiveCommand(PerspectiveBox* box,
	PerspectiveTransformer* transformer, BPoint leftTop, BPoint rightTop
	, BPoint leftBottom, BPoint rightBottom)
	: fTransformBox(box),
	  fTransformer(transformer),
	  fOldLeftTop(leftTop),
	  fOldRightTop(rightTop),
	  fOldLeftBottom(leftBottom),
	  fOldRightBottom(rightBottom),
	  fNewLeftTop(leftTop),
	  fNewRightTop(rightTop),
	  fNewLeftBottom(leftBottom),
	  fNewRightBottom(rightBottom)
{
	if (fTransformer == NULL)
		return;

	fTransformer->AcquireReference();

	if (fTransformBox != NULL)
		fTransformBox->AddListener(this);
}


PerspectiveCommand::~PerspectiveCommand()
{
	if (fTransformer != NULL)
		fTransformer->ReleaseReference();

	if (fTransformBox != NULL)
		fTransformBox->RemoveListener(this);
}


// pragma mark -


status_t
PerspectiveCommand::InitCheck()
{
	if (fTransformer != NULL
		&& (fOldLeftTop != fNewLeftTop
			|| fOldRightTop != fNewRightTop
			|| fOldLeftBottom != fNewLeftBottom
			|| fOldRightBottom != fNewRightBottom))
		return B_OK;

	return B_NO_INIT;
}


status_t
PerspectiveCommand::Perform()
{
	// objects are already transformed
	return B_OK;
}


status_t
PerspectiveCommand::Undo()
{
	if (fTransformBox != NULL) {
		fTransformBox->TransformTo(fOldLeftTop, fOldRightTop, fOldLeftBottom, fOldRightBottom);
		return B_OK;
	}

	fTransformer->TransformTo(fOldLeftTop, fOldRightTop, fOldLeftBottom, fOldRightBottom);
	return B_OK;
}


status_t
PerspectiveCommand::Redo()
{
	if (fTransformBox != NULL) {
		fTransformBox->TransformTo(fNewLeftTop, fNewRightTop, fNewLeftBottom, fNewRightBottom);
		return B_OK;
	}

	fTransformer->TransformTo(fNewLeftTop, fNewRightTop, fNewLeftBottom, fNewRightBottom);
	return B_OK;
}


void
PerspectiveCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Change perspective");
}


// pragma mark -


void
PerspectiveCommand::PerspectiveBoxDeleted(const PerspectiveBox* box)
{
	if (fTransformBox == box) {
		if (fTransformBox != NULL)
			fTransformBox->RemoveListener(this);
		fTransformBox = NULL;
	}
}


// #pragma mark -


void
PerspectiveCommand::SetNewPerspective(
	BPoint leftTop, BPoint rightTop, BPoint leftBottom, BPoint rightBottom)
{
	fNewLeftTop = leftTop;
	fNewRightTop = rightTop;
	fNewLeftBottom = leftBottom;
	fNewRightBottom = rightBottom;
}
