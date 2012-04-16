/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformBoxStates.h"

#include <math.h>
#include <stdio.h>

#include <Catalog.h>
#include <Cursor.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <View.h>

#include "cursors.h"
#include "support.h"

#include "TransformBox.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-TransformationBoxStates"


// constructor
DragState::DragState(TransformBox* parent)
	:
	fOrigin(0.0, 0.0),
	fParent(parent)
{
}


// SetOrigin
void
DragState::SetOrigin(BPoint origin)
{
	fOrigin = origin;
}


// ActionName
const char*
DragState::ActionName() const
{
	return B_TRANSLATE("Transformation");
}


// ActionNameIndex
uint32
DragState::ActionNameIndex() const
{
	return TRANSFORMATION;
}


// _SetViewCursor
void
DragState::_SetViewCursor(BView* view, const uchar* cursorData) const
{
	BCursor cursor(cursorData);
	view->SetViewCursor(&cursor);
}


// #pragma mark - DragCornerState


// constructor
DragCornerState::DragCornerState(TransformBox* parent, uint32 corner)
	:
	DragState(parent),
	fCorner(corner)
{
}


// SetOrigin
void
DragCornerState::SetOrigin(BPoint origin)
{
	fOldXScale = fParent->LocalXScale();
	fOldYScale = fParent->LocalYScale();

	fOldOffset = fParent->Translation();

	// copy the matrix at the start of the drag procedure
	fMatrix.reset();
	fMatrix.multiply(agg::trans_affine_scaling(fOldXScale, fOldYScale));
	fMatrix.multiply(agg::trans_affine_rotation(fParent->LocalRotation()
		* M_PI / 180.0));
	fMatrix.multiply(agg::trans_affine_translation(fParent->Translation().x,
		fParent->Translation().y));

	double x = origin.x;
	double y = origin.y;
	fMatrix.inverse_transform(&x, &y);
	origin.x = x;
	origin.y = y;

	BRect box = fParent->Box();
	switch (fCorner) {
		case LEFT_TOP_CORNER:
			fXOffsetFromCorner = origin.x - box.left;
			fYOffsetFromCorner = origin.y - box.top;
			fOldWidth = box.left - box.right;
			fOldHeight = box.top - box.bottom;
			origin.x = box.right;
			origin.y = box.bottom;
			break;
		case RIGHT_TOP_CORNER:
			fXOffsetFromCorner = origin.x - box.right;
			fYOffsetFromCorner = origin.y - box.top;
			fOldWidth = box.right - box.left;
			fOldHeight = box.top - box.bottom;
			origin.x = box.left;
			origin.y = box.bottom;
			break;
		case LEFT_BOTTOM_CORNER:
			fXOffsetFromCorner = origin.x - box.left;
			fYOffsetFromCorner = origin.y - box.bottom;
			fOldWidth = box.left - box.right;
			fOldHeight = box.bottom - box.top;
			origin.x = box.right;
			origin.y = box.top;
			break;
		case RIGHT_BOTTOM_CORNER:
			fXOffsetFromCorner = origin.x - box.right;
			fYOffsetFromCorner = origin.y - box.bottom;
			fOldWidth = box.right - box.left;
			fOldHeight = box.bottom - box.top;
			origin.x = box.left;
			origin.y = box.top;
			break;
	}
	DragState::SetOrigin(origin);
}


// DragTo
void
DragCornerState::DragTo(BPoint current, uint32 modifiers)
{
	double x = current.x;
	double y = current.y;
	fMatrix.inverse_transform(&x, &y);

	double xScale = 1.0;
	double yScale = 1.0;
	BPoint translation(0.0, 0.0);
	switch (fCorner) {
		case LEFT_TOP_CORNER:
		case RIGHT_TOP_CORNER:
		case LEFT_BOTTOM_CORNER:
		case RIGHT_BOTTOM_CORNER:
			x -= fOrigin.x;
			y -= fOrigin.y;
			if (fOldWidth != 0.0)
				xScale = (x - fXOffsetFromCorner) / (fOldWidth);
			if (fOldHeight != 0.0)
				yScale = (y - fYOffsetFromCorner) / (fOldHeight);
			// constrain aspect ratio if shift is pressed
			if (modifiers & B_SHIFT_KEY) {
				if (fabs(xScale) > fabs(yScale))
					yScale = yScale > 0.0 ? fabs(xScale) : -fabs(xScale);
				else
					xScale = xScale > 0.0 ? fabs(yScale) : -fabs(yScale);
			}
			translation.x = fOrigin.x - fOrigin.x * xScale;
			translation.y = fOrigin.y - fOrigin.y * yScale;
			break;
	}
	x = translation.x;
	y = translation.y;
	fMatrix.transform(&x, &y);
	translation.x = x;
	translation.y = y;

	fParent->SetTranslationAndScale(translation, xScale * fOldXScale,
		yScale * fOldYScale);
}


// UpdateViewCursor
void
DragCornerState::UpdateViewCursor(BView* view, BPoint current) const
{
	float rotation = fmod(360.0 - fParent->ViewSpaceRotation() + 22.5, 180.0);
	bool flipX = fParent->LocalXScale() < 0.0;
	bool flipY = fParent->LocalYScale() < 0.0;
	if (rotation < 45.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view, flipY
						? kLeftTopRightBottomCursor : kLeftBottomRightTopCursor);
				} else {
					_SetViewCursor(view, flipY
						? kLeftBottomRightTopCursor : kLeftTopRightBottomCursor);
				}
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view, flipY
						? kLeftBottomRightTopCursor : kLeftTopRightBottomCursor);
				} else {
					_SetViewCursor(view, flipY
						? kLeftTopRightBottomCursor : kLeftBottomRightTopCursor);
				}
				break;
		}
	} else if (rotation < 90.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view,
						flipY ? kLeftRightCursor : kUpDownCursor);
				} else {
					_SetViewCursor(view,
						flipY ? kUpDownCursor : kLeftRightCursor);
				}
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view,
						flipY ? kUpDownCursor : kLeftRightCursor);
				} else {
					_SetViewCursor(view,
						flipY ? kLeftRightCursor : kUpDownCursor);
				}
				break;
		}
	} else if (rotation < 135.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view, flipY
						? kLeftBottomRightTopCursor : kLeftTopRightBottomCursor);
				} else {
					_SetViewCursor(view, flipY
						? kLeftTopRightBottomCursor : kLeftBottomRightTopCursor);
				}
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view, flipY
						? kLeftTopRightBottomCursor : kLeftBottomRightTopCursor);
				} else {
					_SetViewCursor(view, flipY
						? kLeftBottomRightTopCursor : kLeftTopRightBottomCursor);
				}
				break;
		}
	} else {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view,
						flipY ? kUpDownCursor : kLeftRightCursor);
				} else {
					_SetViewCursor(view,
						flipY ? kLeftRightCursor : kUpDownCursor);
				}
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX) {
					_SetViewCursor(view,
						flipY ? kLeftRightCursor : kUpDownCursor);
				} else {
					_SetViewCursor(view,
						flipY ? kUpDownCursor : kLeftRightCursor);
				}
				break;
		}
	}
}


// ActionName
const char*
DragCornerState::ActionName() const
{
	return B_TRANSLATE("Scale");
}


// ActionNameIndex
uint32
DragCornerState::ActionNameIndex() const
{
	return SCALE;
}


// #pragma mark - DragSideState


DragSideState::DragSideState(TransformBox* parent, uint32 side)
	:
	DragState(parent),
	fSide(side)
{
}


// SetOrigin
void
DragSideState::SetOrigin(BPoint origin)
{
	fOldXScale = fParent->LocalXScale();
	fOldYScale = fParent->LocalYScale();

	fOldOffset = fParent->Translation();

	// copy the matrix at the start of the drag procedure
	fMatrix.reset();
	fMatrix.multiply(agg::trans_affine_scaling(fOldXScale, fOldYScale));
	fMatrix.multiply(agg::trans_affine_rotation(fParent->LocalRotation()
		* M_PI / 180.0));
	fMatrix.multiply(agg::trans_affine_translation(fParent->Translation().x,
		fParent->Translation().y));

	double x = origin.x;
	double y = origin.y;
	fMatrix.inverse_transform(&x, &y);
	origin.x = x;
	origin.y = y;

	BRect box = fParent->Box();
	switch (fSide) {
		case LEFT_SIDE:
			fOffsetFromSide = origin.x - box.left;
			fOldSideDist = box.left - box.right;
			origin.x = box.right;
			break;
		case RIGHT_SIDE:
			fOffsetFromSide = origin.x - box.right;
			fOldSideDist = box.right - box.left;
			origin.x = box.left;
			break;
		case TOP_SIDE:
			fOffsetFromSide = origin.y - box.top;
			fOldSideDist = box.top - box.bottom;
			origin.y = box.bottom;
			break;
		case BOTTOM_SIDE:
			fOffsetFromSide = origin.y - box.bottom;
			fOldSideDist = box.bottom - box.top;
			origin.y = box.top;
			break;
	}
	DragState::SetOrigin(origin);
}


// DragTo
void
DragSideState::DragTo(BPoint current, uint32 modifiers)
{
	double x = current.x;
	double y = current.y;
	fMatrix.inverse_transform(&x, &y);

	double xScale = 1.0;
	double yScale = 1.0;
	BPoint translation(0.0, 0.0);
	switch (fSide) {
		case LEFT_SIDE:
		case RIGHT_SIDE:
			x -= fOrigin.x;
			if (fOldSideDist != 0.0)
				xScale = (x - fOffsetFromSide) / (fOldSideDist);
			translation.x = fOrigin.x - fOrigin.x * xScale;
			break;
		case TOP_SIDE:
		case BOTTOM_SIDE:
			y -= fOrigin.y;
			if (fOldSideDist != 0.0)
				yScale = (y - fOffsetFromSide) / (fOldSideDist);
			translation.y = fOrigin.y - fOrigin.y * yScale;
			break;
	}
	x = translation.x;
	y = translation.y;
	fMatrix.transform(&x, &y);
	translation.x = x;
	translation.y = y;

	fParent->SetTranslationAndScale(translation, xScale * fOldXScale,
		yScale * fOldYScale);
}


// UpdateViewCursor
void
DragSideState::UpdateViewCursor(BView* view, BPoint current) const
{
	float rotation = fmod(360.0 - fParent->ViewSpaceRotation() + 22.5, 180.0);
	if (rotation < 45.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftRightCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kUpDownCursor);
				break;
		}
	} else if (rotation < 90.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftBottomRightTopCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftTopRightBottomCursor);
				break;
		}
	} else if (rotation < 135.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kUpDownCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftRightCursor);
				break;
		}
	} else {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftTopRightBottomCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftBottomRightTopCursor);
				break;
		}
	}
}


// ActionName
const char*
DragSideState::ActionName() const
{
	return B_TRANSLATE("Scale");
}


// ActionNameIndex
uint32
DragSideState::ActionNameIndex() const
{
	return SCALE;
}


// #pragma mark - DragBoxState


// SetOrigin
void
DragBoxState::SetOrigin(BPoint origin)
{
	fOldTranslation = fParent->Translation();
	DragState::SetOrigin(origin);
}


// DragTo
void
DragBoxState::DragTo(BPoint current, uint32 modifiers)
{
	BPoint offset = current - fOrigin;
	BPoint newTranslation = fOldTranslation + offset;
	if (modifiers & B_SHIFT_KEY) {
		if (fabs(offset.x) > fabs(offset.y))
			newTranslation.y = fOldTranslation.y;
		else
			newTranslation.x = fOldTranslation.x;
	}
	fParent->TranslateBy(newTranslation - fParent->Translation());
}


// UpdateViewCursor
void
DragBoxState::UpdateViewCursor(BView* view, BPoint current) const
{
	_SetViewCursor(view, kMoveCursor);
}


// ActionName
const char*
DragBoxState::ActionName() const
{
	return B_TRANSLATE("Move");
}


// ActionNameIndex
uint32
DragBoxState::ActionNameIndex() const
{
	return MOVE;
}


// #pragma mark - RotateBoxState


// constructor
RotateBoxState::RotateBoxState(TransformBox* parent)
	:
	DragState(parent),
	fOldAngle(0.0)
{
}


// SetOrigin
void
RotateBoxState::SetOrigin(BPoint origin)
{
	DragState::SetOrigin(origin);
	fOldAngle = fParent->LocalRotation();
}


// DragTo
void
RotateBoxState::DragTo(BPoint current, uint32 modifiers)
{
	double angle = calc_angle(fParent->Center(), fOrigin, current);

	if (modifiers & B_SHIFT_KEY) {
		if (angle < 0.0)
			angle -= 22.5;
		else
			angle += 22.5;
		angle = 45.0 * ((int32)angle / 45);
	}

	double newAngle = fOldAngle + angle;

	fParent->RotateBy(fParent->Center(), newAngle - fParent->LocalRotation());
}


// UpdateViewCursor
void
RotateBoxState::UpdateViewCursor(BView* view, BPoint current) const
{
	BPoint origin(fParent->Center());
	fParent->TransformToCanvas(origin);
	fParent->TransformToCanvas(current);
	BPoint from = origin + BPoint(sinf(22.5 * 180.0 / M_PI) * 50.0,
		-cosf(22.5 * 180.0 / M_PI) * 50.0);

	float rotation = calc_angle(origin, from, current) + 180.0;

	if (rotation < 45.0) {
		_SetViewCursor(view, kRotateLCursor);
	} else if (rotation < 90.0) {
		_SetViewCursor(view, kRotateLTCursor);
	} else if (rotation < 135.0) {
		_SetViewCursor(view, kRotateTCursor);
	} else if (rotation < 180.0) {
		_SetViewCursor(view, kRotateRTCursor);
	} else if (rotation < 225.0) {
		_SetViewCursor(view, kRotateRCursor);
	} else if (rotation < 270.0) {
		_SetViewCursor(view, kRotateRBCursor);
	} else if (rotation < 315.0) {
		_SetViewCursor(view, kRotateBCursor);
	} else {
		_SetViewCursor(view, kRotateLBCursor);
	}
}


// ActionName
const char*
RotateBoxState::ActionName() const
{
	return B_TRANSLATE("Rotate");
}


// ActionNameIndex
uint32
RotateBoxState::ActionNameIndex() const
{
	return ROTATE;
}


// #pragma mark - OffsetCenterState


// SetOrigin
void
OffsetCenterState::SetOrigin(BPoint origin)
{
	fParent->InverseTransform(&origin);
	DragState::SetOrigin(origin);
}


// DragTo
void
OffsetCenterState::DragTo(BPoint current, uint32 modifiers)
{
	fParent->InverseTransform(&current);
	fParent->OffsetCenter(current - fOrigin);
	fOrigin = current;
}


// UpdateViewCursor
void
OffsetCenterState::UpdateViewCursor(BView* view, BPoint current) const
{
	_SetViewCursor(view, kPathMoveCursor);
}


// ActionName
const char*
OffsetCenterState::ActionName() const
{
	return B_TRANSLATE("Move Pivot");
}


// ActionNameIndex
uint32
OffsetCenterState::ActionNameIndex() const
{
	return MOVE_PIVOT;
}
