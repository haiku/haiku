/*
 * Copyright 2003-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		Michael Lotz, mmlr@mlotz.ch
 *		Marcus Overhagen, marcus@overhagen.de
 */


#include <Shape.h>

#include <Message.h>
#include <Point.h>
#include <Rect.h>

#include <ShapePrivate.h>

#include <new>
#include <stdlib.h>
#include <string.h>


//	#pragma mark - BShapeIterator


BShapeIterator::BShapeIterator()
{
}


BShapeIterator::~BShapeIterator()
{
}


status_t
BShapeIterator::Iterate(BShape* shape)
{
	shape_data* data = (shape_data*)shape->fPrivateData;
	BPoint* points = data->ptList;

	for (int32 i = 0; i < data->opCount; i++) {
		int32 op = data->opList[i] & 0xFF000000;

		if ((op & OP_MOVETO) != 0) {
			IterateMoveTo(points);
			points++;
		}

		if ((op & OP_LINETO) != 0) {
			int32 count = data->opList[i] & 0x00FFFFFF;
			IterateLineTo(count, points);
			points += count;
		}

		if ((op & OP_BEZIERTO) != 0) {
			int32 count = data->opList[i] & 0x00FFFFFF;
			IterateBezierTo(count / 3, points);
			points += count;
		}

		if ((op & OP_LARGE_ARC_TO_CW) != 0 || (op & OP_LARGE_ARC_TO_CCW) != 0
			|| (op & OP_SMALL_ARC_TO_CW) != 0
			|| (op & OP_SMALL_ARC_TO_CCW) != 0) {
			int32 count = data->opList[i] & 0x00FFFFFF;
			for (int32 i = 0; i < count / 3; i++) {
				IterateArcTo(points[0].x, points[0].y, points[1].x,
					op & (OP_LARGE_ARC_TO_CW | OP_LARGE_ARC_TO_CCW),
					op & (OP_SMALL_ARC_TO_CCW | OP_LARGE_ARC_TO_CCW),
					points[2]);
				points += 3;
			}
		}

		if ((op & OP_CLOSE) != 0)
			IterateClose();
	}

	return B_OK;
}


status_t
BShapeIterator::IterateMoveTo(BPoint* point)
{
	return B_OK;
}


status_t
BShapeIterator::IterateLineTo(int32 lineCount, BPoint* linePoints)
{
	return B_OK;
}


status_t
BShapeIterator::IterateBezierTo(int32 bezierCount, BPoint* bezierPoints)
{
	return B_OK;
}


status_t
BShapeIterator::IterateClose()
{
	return B_OK;
}


status_t
BShapeIterator::IterateArcTo(float& rx, float& ry, float& angle, bool largeArc,
	bool counterClockWise, BPoint& point)
{
	return B_OK;
}


// #pragma mark - BShapeIterator FBC padding


void BShapeIterator::_ReservedShapeIterator2() {}
void BShapeIterator::_ReservedShapeIterator3() {}
void BShapeIterator::_ReservedShapeIterator4() {}


// #pragma mark - BShape


BShape::BShape()
{
	InitData();
}


BShape::BShape(const BShape& other)
{
	InitData();
	AddShape(&other);
}


BShape::BShape(BMessage* archive)
	:
	BArchivable(archive)
{
	InitData();

	shape_data* data = (shape_data*)fPrivateData;

	ssize_t size = 0;
	int32 count = 0;
	type_code type = 0;
	archive->GetInfo("ops", &type, &count);
	if (!AllocateOps(count))
		return;

	int32 i = 0;
	const uint32* opPtr;
	while (archive->FindData("ops", B_INT32_TYPE, i++,
			(const void**)&opPtr, &size) == B_OK) {
		data->opList[data->opCount++] = *opPtr;
	}

	archive->GetInfo("pts", &type, &count);
	if (!AllocatePts(count)) {
		Clear();
		return;
	}

	i = 0;
	const BPoint* ptPtr;
	while (archive->FindData("pts", B_POINT_TYPE, i++,
			(const void**)&ptPtr, &size) == B_OK) {
		data->ptList[data->ptCount++] = *ptPtr;
	}
}


BShape::~BShape()
{
	shape_data* data = (shape_data*)fPrivateData;
	if (!data->fOwnsMemory) {
		free(data->opList);
		free(data->ptList);
	}

	data->ReleaseReference();
}


status_t
BShape::Archive(BMessage* archive, bool deep) const
{
	status_t result = BArchivable::Archive(archive, deep);

	if (result != B_OK)
		return result;

	shape_data* data = (shape_data*)fPrivateData;

	// If no valid shape data, return
	if (data->opCount == 0 || data->ptCount == 0)
		return result;

	// Avoids allocation for each point
	result = archive->AddData("pts", B_POINT_TYPE, data->ptList,
		sizeof(BPoint), true, data->ptCount);
	if (result != B_OK)
		return result;

	for (int32 i = 1; i < data->ptCount && result == B_OK; i++)
		result = archive->AddPoint("pts", data->ptList[i]);

	// Avoids allocation for each op
	if (result == B_OK) {
		result = archive->AddData("ops", B_INT32_TYPE, data->opList,
			sizeof(int32), true, data->opCount);
	}

	for (int32 i = 1; i < data->opCount && result == B_OK; i++)
		result = archive->AddInt32("ops", data->opList[i]);

	return result;
}


BArchivable*
BShape::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BShape"))
		return new BShape(archive);
	else
		return NULL;
}


BShape&
BShape::operator=(const BShape& other)
{
	if (this != &other) {
		Clear();
		AddShape(&other);
	}

	return *this;
}


bool
BShape::operator==(const BShape& other) const
{
	if (this == &other)
		return true;

	shape_data* data = (shape_data*)fPrivateData;
	shape_data* otherData = (shape_data*)other.fPrivateData;

	if (data->opCount != otherData->opCount)
		return false;

	if (data->ptCount != otherData->ptCount)
		return false;

	return memcmp(data->opList, otherData->opList,
			data->opCount * sizeof(uint32)) == 0
		&& memcmp(data->ptList, otherData->ptList,
			data->ptCount * sizeof(BPoint)) == 0;
}


bool
BShape::operator!=(const BShape& other) const
{
	return !(*this == other);
}


void
BShape::Clear()
{
	shape_data* data = (shape_data*)fPrivateData;

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
	shape_data* data = (shape_data*)fPrivateData;
	return data->DetermineBoundingBox();
}


BPoint
BShape::CurrentPosition() const
{
	shape_data* data = (shape_data*)fPrivateData;

	if (data->ptCount == 0)
		return B_ORIGIN;

	return data->ptList[data->ptCount - 1];
}


status_t
BShape::AddShape(const BShape* otherShape)
{
	shape_data* data = (shape_data*)fPrivateData;
	shape_data* otherData = (shape_data*)otherShape->fPrivateData;

	if (!AllocateOps(otherData->opCount) || !AllocatePts(otherData->ptCount))
		return B_NO_MEMORY;

	memcpy(data->opList + data->opCount, otherData->opList,
		otherData->opCount * sizeof(uint32));
	data->opCount += otherData->opCount;

	memcpy((void*)(data->ptList + data->ptCount), otherData->ptList,
		otherData->ptCount * sizeof(BPoint));
	data->ptCount += otherData->ptCount;

	fBuildingOp = otherShape->fBuildingOp;

	return B_OK;
}


status_t
BShape::MoveTo(BPoint point)
{
	shape_data* data = (shape_data*)fPrivateData;

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

	shape_data* data = (shape_data*)fPrivateData;

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
	return BezierTo(controlPoints[0], controlPoints[1], controlPoints[2]);
}


status_t
BShape::BezierTo(const BPoint& control1, const BPoint& control2,
	const BPoint& endPoint)
{
	if (!AllocatePts(3))
		return B_NO_MEMORY;

	shape_data* data = (shape_data*)fPrivateData;

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
	data->ptList[data->ptCount++] = control1;
	data->ptList[data->ptCount++] = control2;
	data->ptList[data->ptCount++] = endPoint;

	return B_OK;
}


status_t
BShape::ArcTo(float rx, float ry, float angle, bool largeArc,
	bool counterClockWise, const BPoint& point)
{
	if (!AllocatePts(3))
		return B_NO_MEMORY;

	shape_data* data = (shape_data*)fPrivateData;

	uint32 op;
	if (largeArc) {
		if (counterClockWise)
			op = OP_LARGE_ARC_TO_CCW;
		else
			op = OP_LARGE_ARC_TO_CW;
	} else {
		if (counterClockWise)
			op = OP_SMALL_ARC_TO_CCW;
		else
			op = OP_SMALL_ARC_TO_CW;
	}

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is ArcTo increase the count
	// Otherwise add the op
	if (fBuildingOp == op || fBuildingOp == (op | OP_MOVETO)) {
		fBuildingOp |= op;
		fBuildingOp += 3;
		data->opList[data->opCount - 1] = fBuildingOp;
	} else {
		if (!AllocateOps(1))
			return B_NO_MEMORY;

		fBuildingOp = op + 3;
		data->opList[data->opCount++] = fBuildingOp;
	}

	// Add points
	data->ptList[data->ptCount++] = BPoint(rx, ry);
	data->ptList[data->ptCount++] = BPoint(angle, 0);
	data->ptList[data->ptCount++] = point;

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

	shape_data* data = (shape_data*)fPrivateData;

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


//	#pragma mark - BShape private methods


status_t
BShape::Perform(perform_code code, void* data)
{
	return BArchivable::Perform(code, data);
}


//	#pragma mark - BShape FBC methods


void BShape::_ReservedShape1() {}
void BShape::_ReservedShape2() {}
void BShape::_ReservedShape3() {}
void BShape::_ReservedShape4() {}


//	#pragma mark - BShape private methods


void
BShape::GetData(int32* opCount, int32* ptCount, uint32** opList,
	BPoint** ptList)
{
	shape_data* data = (shape_data*)fPrivateData;

	*opCount = data->opCount;
	*ptCount = data->ptCount;
	*opList = data->opList;
	*ptList = data->ptList;
}


void
BShape::SetData(int32 opCount, int32 ptCount, const uint32* opList,
	const BPoint* ptList)
{
	Clear();

	if (opCount == 0)
		return;

	shape_data* data = (shape_data*)fPrivateData;

	if (!AllocateOps(opCount) || !AllocatePts(ptCount))
		return;

	memcpy(data->opList, opList, opCount * sizeof(uint32));
	data->opCount = opCount;
	fBuildingOp = data->opList[data->opCount - 1];

	if (ptCount > 0) {
		memcpy((void*)data->ptList, ptList, ptCount * sizeof(BPoint));
		data->ptCount = ptCount;
	}
}


void
BShape::InitData()
{
	fPrivateData = new shape_data;
	shape_data* data = (shape_data*)fPrivateData;

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
	shape_data* data = (shape_data*)fPrivateData;

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
	shape_data* data = (shape_data*)fPrivateData;

	int32 newSize = (data->ptCount + count + 255) / 256 * 256;
	if (data->ptSize >= newSize)
		return true;

	BPoint* resizedArray = (BPoint*)realloc((void*)data->ptList,
		newSize * sizeof(BPoint));
	if (resizedArray) {
		data->ptList = resizedArray;
		data->ptSize = newSize;
		return true;
	}
	return false;
}


//	#pragma mark - BShape binary compatibility methods


#if __GNUC__ < 3


extern "C" BShape*
__6BShapeR6BShape(void* self, BShape& copyFrom)
{
	return new (self) BShape(copyFrom);
		// we need to instantiate the object in the provided memory
}


extern "C" BRect
Bounds__6BShape(BShape* self)
{
	return self->Bounds();
}


extern "C" void
_ReservedShapeIterator1__14BShapeIterator(BShapeIterator* self)
{
}


#else // __GNUC__ < 3


extern "C" void
_ZN14BShapeIterator23_ReservedShapeIterator1Ev(BShapeIterator* self)
{
}


#endif // __GNUC__ >= 3
