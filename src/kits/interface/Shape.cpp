/*
 * Copyright (c) 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Marcus Overhagen <marcus@overhagen.de>
 */

/*! BShape encapsulates a Postscript-style "path" */

#include <Shape.h>

#include <Message.h>
#include <Point.h>
#include <Rect.h>

#include <ShapePrivate.h>

#include <new>
#include <stdlib.h>
#include <string.h>




BShapeIterator::BShapeIterator()
{
}


BShapeIterator::~BShapeIterator()
{
}


status_t
BShapeIterator::Iterate(BShape *shape)
{
	shape_data *data = (shape_data*)shape->fPrivateData;
	BPoint *points = data->ptList;

	for (int32 i = 0; i < data->opCount; i++) {
		int32 op = data->opList[i] & 0xFF000000;

		if (op & OP_MOVETO) {
			IterateMoveTo(points);
			points++;
		}

		if (op & OP_LINETO) {
			int32 count = data->opList[i] & 0x00FFFFFF;
			IterateLineTo(count, points);
			points += count;
		}

		if (op & OP_BEZIERTO) {
			int32 count = data->opList[i] & 0x00FFFFFF;
			IterateBezierTo(count / 3, points);
			points += count;
		}

		if (op & OP_CLOSE) {
			IterateClose();
		}
	}

	return B_OK;
}


status_t
BShapeIterator::IterateBezierTo(int32 bezierCount,
										 BPoint *bezierPoints)
{
	return B_OK;
}


status_t
BShapeIterator::IterateClose()
{
	return B_OK;
}


status_t
BShapeIterator::IterateLineTo(int32 lineCount, BPoint *linePoints)
{
	return B_OK;
}


status_t
BShapeIterator::IterateMoveTo ( BPoint *point )
{
	return B_OK;
}


void BShapeIterator::_ReservedShapeIterator1() {}
void BShapeIterator::_ReservedShapeIterator2() {}
void BShapeIterator::_ReservedShapeIterator3() {}
void BShapeIterator::_ReservedShapeIterator4() {}


BShape::BShape()
{
	InitData();
}


BShape::BShape(const BShape &copyFrom)
{
	InitData();
	AddShape(&copyFrom);
}


BShape::BShape(BMessage *archive)
	:	BArchivable(archive)
{
	InitData();

	shape_data *data = (shape_data*)fPrivateData;

	ssize_t size = 0;
	int32 count = 0;
	type_code type = 0;
	archive->GetInfo("ops", &type, &count);
	if (!AllocateOps(count))
		return;

	int32 i = 0;
	const uint32 *opPtr;
	while (archive->FindData("ops", B_INT32_TYPE, i++, (const void **)&opPtr, &size) == B_OK)
		data->opList[data->opCount++] = *opPtr;

	archive->GetInfo("pts", &type, &count);
	if (!AllocatePts(count)) {
		Clear();
		return;
	}

	i = 0;
	const BPoint *ptPtr;
	while (archive->FindData("pts", B_POINT_TYPE, i++, (const void **)&ptPtr, &size) == B_OK)
		data->ptList[data->ptCount++] = *ptPtr;
}


BShape::~BShape()
{
	shape_data *data = (shape_data*)fPrivateData;

	free(data->opList);
	free(data->ptList);

	delete (shape_data*)fPrivateData;
}


status_t
BShape::Archive(BMessage *archive, bool deep) const
{
	status_t err = BArchivable::Archive(archive, deep);

	if (err != B_OK)
		return err;

	shape_data *data = (shape_data*)fPrivateData;

	// If no valid shape data, return
	if (data->opCount == 0 || data->ptCount == 0)
		return err;

	// Avoids allocation for each point
	err = archive->AddData("pts", B_POINT_TYPE, data->ptList, sizeof(BPoint), true,
		data->ptCount);
	if (err != B_OK)
		return err;

	for (int32 i = 1; i < data->ptCount && err == B_OK; i++)
		err = archive->AddPoint("pts", data->ptList[i]);

	// Avoids allocation for each op
	if (err == B_OK)
		err = archive->AddData("ops", B_INT32_TYPE, data->opList, sizeof(int32), true,
			data->opCount);

	for (int32 i = 1; i < data->opCount && err == B_OK ; i++)
		err = archive->AddInt32("ops", data->opList[i]);

	return err;
}


BArchivable*
BShape::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BShape"))
		return new BShape(archive);
	else
		return NULL;
}


void
BShape::Clear()
{
	shape_data *data = (shape_data*)fPrivateData;

	data->opCount = 0;
	data->opSize = 0;
	if (data->opList) {
		free(data->opList);
		data->opList = NULL;
	}

	data->ptCount = 0;
	data->ptSize = 0;
	if (data->ptList) {
		free(data->ptList);
		data->ptList = NULL;
	}

	fState = 0;
	fBuildingOp = 0;
}


BRect
BShape::Bounds() const
{
	shape_data *data = (shape_data*)fPrivateData;
	BRect bounds;

	if (data->ptCount == 0)
		return bounds;

	bounds.left = data->ptList[0].x;
	bounds.top = data->ptList[0].y;
	bounds.right = data->ptList[0].x;
	bounds.bottom = data->ptList[0].y;

	for (int32 i = 1; i < data->ptCount; i++)
	{
		if (bounds.left > data->ptList[i].x)
			bounds.left = data->ptList[i].x;
		if (bounds.top > data->ptList[i].y)
			bounds.top = data->ptList[i].y;
		if (bounds.right < data->ptList[i].x)
			bounds.right = data->ptList[i].x;
		if (bounds.bottom < data->ptList[i].y)
			bounds.bottom = data->ptList[i].y;
	}

	return bounds;
}


status_t
BShape::AddShape(const BShape *otherShape)
{
	shape_data *data = (shape_data*)fPrivateData;
	shape_data *otherData = (shape_data*)otherShape->fPrivateData;

	if (!AllocateOps(otherData->opCount) || !AllocatePts(otherData->ptCount))
		return B_NO_MEMORY;

	memcpy(data->opList + data->opCount * sizeof(uint32), otherData->opList,
		otherData->opCount * sizeof(uint32));
	data->opCount += otherData->opCount;

	memcpy(data->ptList + data->ptCount * sizeof(BPoint), otherData->ptList,
		otherData->ptCount * sizeof(BPoint));
	data->ptCount += otherData->ptCount;

	fBuildingOp = otherShape->fBuildingOp;

	return B_OK;
}


status_t
BShape::MoveTo(BPoint point)
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the point
	if (fBuildingOp == OP_MOVETO) {
		data->ptList[data->ptCount - 1] = point;
		return B_OK;
	}

	if (!AllocateOps(1) || !AllocatePts(1))
		return B_NO_MEMORY;

	fBuildingOp = OP_MOVETO;

	// Add op
	data->opList[data->opCount++] = fBuildingOp;

	// Add point
	data->ptList[data->ptCount++] = point;

	return B_OK;
}


status_t
BShape::LineTo(BPoint point)
{
	if (!AllocatePts(1))
		return B_NO_MEMORY;

	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is LineTo increase the count
	// Otherwise add the op
	if (fBuildingOp & OP_LINETO || fBuildingOp == OP_MOVETO) {
		fBuildingOp |= OP_LINETO;
		fBuildingOp += 1;
		data->opList[data->opCount - 1] = fBuildingOp;
	} else {
		if (!AllocateOps(1))
			return B_NO_MEMORY;
		fBuildingOp = OP_LINETO + 1;
		data->opList[data->opCount++] = fBuildingOp;
	}

	// Add point
	data->ptList[data->ptCount++] = point;

	return B_OK;
}


status_t
BShape::BezierTo(BPoint controlPoints[3])
{
	if (!AllocatePts(3))
		return B_NO_MEMORY;

	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is BezierTo increase the count
	// Otherwise add the op
	if (fBuildingOp & OP_BEZIERTO || fBuildingOp == OP_MOVETO) {
		fBuildingOp |= OP_BEZIERTO;
		fBuildingOp += 3;
		data->opList[data->opCount - 1] = fBuildingOp;
	} else {
		if (!AllocateOps(1))
			return B_NO_MEMORY;
		fBuildingOp = OP_BEZIERTO + 3;
		data->opList[data->opCount++] = fBuildingOp;
	}

	// Add points
	data->ptList[data->ptCount++] = controlPoints[0];
	data->ptList[data->ptCount++] = controlPoints[1];
	data->ptList[data->ptCount++] = controlPoints[2];

	return B_OK;
}


status_t
BShape::Close()
{
	// If the last op is Close or MoveTo, ignore this
	if (fBuildingOp == OP_CLOSE || fBuildingOp == OP_MOVETO)
		return B_OK;

	if (!AllocateOps(1))
		return B_NO_MEMORY;

	shape_data *data = (shape_data*)fPrivateData;

	// ToDo: Decide about that, it's not BeOS compatible
	// If there was any op before we can attach the close to it
	/*if (fBuildingOp) {
		fBuildingOp |= OP_CLOSE;
		data->opList[data->opCount - 1] = fBuildingOp;
		return B_OK;
	}*/

	fBuildingOp = OP_CLOSE;
	data->opList[data->opCount++] = fBuildingOp;

	return B_OK;
}


status_t
BShape::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}


void BShape::_ReservedShape1() {}
void BShape::_ReservedShape2() {}
void BShape::_ReservedShape3() {}
void BShape::_ReservedShape4() {}


void
BShape::GetData(int32 *opCount, int32 *ptCount, uint32 **opList,
					 BPoint **ptList)
{
	shape_data *data = (shape_data*)fPrivateData;

	*opCount = data->opCount;
	*ptCount = data->ptCount;
	*opList = data->opList;
	*ptList = data->ptList;
}


void
BShape::SetData(int32 opCount, int32 ptCount, const uint32 *opList,
				const BPoint *ptList)
{
	Clear();

	if (opCount == 0)
		return;

	shape_data *data = (shape_data*)fPrivateData;

	if (!AllocateOps(opCount) || !AllocatePts(ptCount))
		return;

	memcpy(data->opList, opList, opCount * sizeof(uint32));
	data->opCount = opCount;
	fBuildingOp = data->opList[data->opCount - 1];

	if (ptCount > 0) {
		memcpy(data->ptList, ptList, ptCount * sizeof(BPoint));
		data->ptCount = ptCount;
	}
}




void
BShape::InitData()
{
	fPrivateData = new shape_data;
	shape_data *data = (shape_data*)fPrivateData;

	fState = 0;
	fBuildingOp = 0;

	data->opList = NULL;
	data->opCount = 0;
	data->opSize = 0;
	data->ptList = NULL;
	data->ptCount = 0;
	data->ptSize = 0;
}


inline bool
BShape::AllocateOps(int32 count)
{
	shape_data *data = (shape_data*)fPrivateData;

	int32 newSize = (data->opCount + count + 255) / 256 * 256;
	if (data->opSize >= newSize)
		return true;

	uint32* resizedArray = (uint32*)realloc(data->opList, newSize * sizeof(uint32));
	if (resizedArray) {
		data->opList = resizedArray;
		data->opSize = newSize;
		return true;
	}
	return false;
}


inline bool
BShape::AllocatePts(int32 count)
{
	shape_data *data = (shape_data*)fPrivateData;

	int32 newSize = (data->ptCount + count + 255) / 256 * 256;
	if (data->ptSize >= newSize)
		return true;

	BPoint* resizedArray = (BPoint*)realloc(data->ptList, newSize * sizeof(BPoint));
	if (resizedArray) {
		data->ptList = resizedArray;
		data->ptSize = newSize;
		return true;
	}
	return false;
}


//	#pragma mark - R4.5 compatibility


#if __GNUC__ < 3

extern "C" BShape*
__6BShapeR6BShape(void* self, BShape& copyFrom)
{
	return new (self) BShape(copyFrom);
		// we need to instantiate the object in the provided memory
}


extern "C" BRect
Bounds__6BShape(BShape *self)
{
	return self->Bounds();
}

#endif	// __GNUC__ < 3
