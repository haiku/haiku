/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Michael Lotz <mmlr@mlotz.ch>
 *
 * Description:
 *		BShape encapsulates a Postscript-style "path"
 */

#include <stdlib.h>
#include <string.h>

#include <Shape.h>
#include <Point.h>
#include <Rect.h>
#include <Errors.h>
#include <Message.h>


// NOTE: changing these defines will break Painter,
// currently located in src/servers/app/drawing/Painter/Painter.cpp
#define OP_LINETO		0x10000000
#define OP_BEZIERTO		0x20000000
#define OP_CLOSE		0x40000000
#define OP_MOVETO		0x80000000


struct shape_data {
	uint32	*opList;
	int32	opCount;
	int32	opSize;
	int32	opBlockSize;
	BPoint	*ptList;
	int32	ptCount;
	int32	ptSize;
	int32	ptBlockSize;
};


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
	AllocateOps(count);

	int32 i = 0;
	const uint32 *opPtr;
	while (archive->FindData("ops", B_INT32_TYPE, i++, (const void **)&opPtr, &size) == B_OK)
		data->opList[data->opCount++] = *opPtr;

	archive->GetInfo("pts", &type, &count);
	AllocatePts(count);

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
	status_t error = BArchivable::Archive(archive, deep);

	if (error != B_OK)
		return error;

	shape_data *data = (shape_data*)fPrivateData;

	// If no valid shape data, return
	if (data->opCount == 0 || data->ptCount == 0)
		return error;

	// Avoids allocation for each point
	archive->AddData("pts", B_POINT_TYPE, data->ptList, sizeof(BPoint), true,
		data->ptCount);

	for (int32 i = 1; i < data->ptCount; i++)
		archive->AddPoint("pts", data->ptList[i]);

	// Avoids allocation for each op
	archive->AddData("ops", B_INT32_TYPE, data->opList, sizeof(int32), true,
		data->opCount);

	for (int32 i = 1; i < data->opCount; i++)
		archive->AddInt32("ops", data->opList[i]);

	return error;
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

	AllocateOps(otherData->opCount);
	memcpy(data->opList + data->opCount * sizeof(uint32), otherData->opList,
		otherData->opCount * sizeof(uint32));
	data->opCount += otherData->opCount;

	AllocatePts(otherData->ptCount);
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

	fBuildingOp = OP_MOVETO;

	// Add op
	AllocateOps(1);
	data->opList[data->opCount++] = fBuildingOp;

	// Add point
	AllocatePts(1);
	data->ptList[data->ptCount++] = point;

	return B_OK;
}


status_t
BShape::LineTo(BPoint point)
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is LineTo increase the count
	// Otherwise add the op
	if (fBuildingOp & OP_LINETO || fBuildingOp == OP_MOVETO) {
		fBuildingOp |= OP_LINETO;
		fBuildingOp += 1;
		data->opList[data->opCount - 1] = fBuildingOp;
	} else {
		fBuildingOp = OP_LINETO + 1;
		AllocateOps(1);
		data->opList[data->opCount++] = fBuildingOp;
	}

	// Add point
	AllocatePts(1);
	data->ptList[data->ptCount++] = point;

	return B_OK;
}


status_t
BShape::BezierTo(BPoint controlPoints[3])
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is BezierTo increase the count
	// Otherwise add the op
	if (fBuildingOp & OP_BEZIERTO || fBuildingOp == OP_MOVETO) {
		fBuildingOp |= OP_BEZIERTO;
		fBuildingOp += 3;
		data->opList[data->opCount - 1] = fBuildingOp;
	} else {
		fBuildingOp = OP_BEZIERTO + 3;
		AllocateOps(1);
		data->opList[data->opCount++] = fBuildingOp;
	}

	// Add points
	AllocatePts(3);
	data->ptList[data->ptCount++] = controlPoints[0];
	data->ptList[data->ptCount++] = controlPoints[1];
	data->ptList[data->ptCount++] = controlPoints[2];

	return B_OK;
}


status_t
BShape::Close()
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is Close or MoveTo, ignore this
	if (fBuildingOp == OP_CLOSE || fBuildingOp == OP_MOVETO)
		return B_OK;


	// ToDo: Decide about that, it's not BeOS compatible
	// If there was any op before we can attach the close to it
	/*if (fBuildingOp) {
		fBuildingOp |= OP_CLOSE;
		data->opList[data->opCount - 1] = fBuildingOp;
		return B_OK;
	}*/

	fBuildingOp = OP_CLOSE;
	AllocateOps(1);
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
BShape::SetData(int32 opCount, int32 ptCount, uint32 *opList,
				BPoint *ptList)
{
	Clear();

	shape_data *data = (shape_data*)fPrivateData;

	AllocateOps(opCount);
	memcpy(data->opList, opList, opCount * sizeof(uint32));
	data->opCount = opCount;
	fBuildingOp = data->opList[data->opCount - 1];

	AllocatePts(ptCount);
	memcpy(data->ptList, ptList, ptCount * sizeof(BPoint));
	data->ptCount = ptCount;
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
	data->opBlockSize = 255;
	data->ptList = NULL;
	data->ptCount = 0;
	data->ptSize = 0;
	data->ptBlockSize = 255;
}


inline void
BShape::AllocateOps(int32 count)
{
	shape_data *data = (shape_data*)fPrivateData;

	while (data->opSize < data->opCount + count) {
		int32 new_size = ((data->opCount + data->opBlockSize) /
			data->opBlockSize) * data->opBlockSize;
		data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
		data->opSize = new_size;
		count -= data->opBlockSize;
	}
}


inline void
BShape::AllocatePts(int32 count)
{
	shape_data *data = (shape_data*)fPrivateData;

	while (data->ptSize < data->ptCount + count) {
		int32 new_size = ((data->ptCount + data->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;
		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(BPoint));
		data->ptSize = new_size;
		count -= data->ptBlockSize;
	}
}


//	#pragma mark -
//	R4.5 compatibility


#if __GNUC__ < 3

extern "C" BRect
Bounds__6BShape(BShape *self)
{
	return self->Bounds();
}

#endif	// __GNUC__ < 3
