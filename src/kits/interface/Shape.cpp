//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Shape.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BShape encapsulates a Postscript-style "path"
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <Shape.h>
#include <Point.h>
#include <Rect.h>
#include <Errors.h>
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
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

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BShapeIterator::BShapeIterator()
{
}
//------------------------------------------------------------------------------
BShapeIterator::~BShapeIterator()
{
}
//------------------------------------------------------------------------------
status_t BShapeIterator::Iterate(BShape *shape)
{
	shape_data *data = (shape_data*)shape->fPrivateData;
	BPoint *points = data->ptList;

	for (int32 i = 0; i < data->opCount; i++)
	{
		switch (data->opList[i] & 0xFF000000)
		{
			case OP_LINETO:
			{
				int32 count = data->opList[i] & 0x00FFFFFF;
				IterateLineTo(count, points);
				points += count;
				break;
			}
			case (OP_MOVETO | OP_LINETO):
			{
				int32 count = data->opList[i] & 0x00FFFFFF;
				IterateMoveTo(points);
				points++;
				IterateLineTo(count, points);
				points += count;
				break;
			}
			case OP_BEZIERTO:
			{
				int32 count = data->opList[i] & 0x00FFFFFF;
				IterateBezierTo(count, points);
				points += count;
				break;
			}
			case (OP_MOVETO | OP_BEZIERTO):
			{
				int32 count = data->opList[i] & 0x00FFFFFF;
				IterateMoveTo(points);
				points++;
				IterateBezierTo(count, points);
				points += count;
				break;
			}
			case OP_CLOSE:
			case OP_CLOSE | OP_LINETO | OP_BEZIERTO:
			{
				IterateClose();
				break;
			}
		}
	}

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShapeIterator::IterateBezierTo(int32 bezierCount,
										 BPoint *bezierPoints)
{
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShapeIterator::IterateClose()
{
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShapeIterator::IterateLineTo(int32 lineCount, BPoint *linePoints)
{
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShapeIterator::IterateMoveTo ( BPoint *point )
{
	return B_OK;
}
//------------------------------------------------------------------------------
void BShapeIterator::_ReservedShapeIterator1() {}
void BShapeIterator::_ReservedShapeIterator2() {}
void BShapeIterator::_ReservedShapeIterator3() {}
void BShapeIterator::_ReservedShapeIterator4() {}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
BShape::BShape()
{
	InitData();
}
//------------------------------------------------------------------------------
BShape::BShape(const BShape &copyFrom)
{
	InitData();
	AddShape(&copyFrom);
}
//------------------------------------------------------------------------------
BShape::BShape(BMessage *archive)
	:	BArchivable(archive)
{
	InitData();

	shape_data *data = (shape_data*)fPrivateData;
	ssize_t size;
	int32 i = 0;

	const uint32 *opPtr;

	while (archive->FindData("ops", B_INT32_TYPE, i++, (const void**)&opPtr, &size) == B_OK)
	{
		if (data->opSize < data->opCount + 1)
		{
			int32 new_size = ((data->opCount + data->opBlockSize) /
				data->opBlockSize) * data->opBlockSize;
			data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
			data->opSize = new_size;
		}

		data->opList[data->opCount++] = *opPtr;
	}

	const BPoint *ptPtr;

	while (archive->FindData("pts", B_POINT_TYPE, i++, (const void**)&ptPtr, &size) == B_OK)
	{
		if (data->ptSize < data->ptCount + 1)
		{
			int32 new_size = ((data->ptCount + data->ptBlockSize) /
				data->ptBlockSize) * data->ptBlockSize;
			data->ptList = (BPoint*)realloc(data->ptList,
				new_size * sizeof(BPoint));
			data->ptSize = new_size;
		}

		data->ptList[data->ptCount++] = *ptPtr;
	}
}
//------------------------------------------------------------------------------
BShape::~BShape()
{
	shape_data *data = (shape_data*)fPrivateData;

	free(data->opList);
	free(data->ptList);

	delete (shape_data*)fPrivateData;
}
//------------------------------------------------------------------------------
status_t BShape::Archive(BMessage *archive, bool deep) const
{
	status_t err = BArchivable::Archive(archive, deep);
	int32 i;

	if (err != B_OK)
		return err;

	shape_data *data = (shape_data*)fPrivateData;

	// If no valid shape data, return
	if (data->opCount == 0 || data->ptCount == 0)
		return err;

	// Avoids allocation for each point
	archive->AddData("pts", B_POINT_TYPE, data->ptList, sizeof(BPoint), true,
		data->ptCount);

	for (i = 1; i < data->ptCount; i++)
		archive->AddPoint("pts", data->ptList[i]);

	// Avoids allocation for each op
	archive->AddData("ops", B_INT32_TYPE, data->opList, sizeof(int32), true,
			data->opCount);

	for (i = 1; i < data->opCount; i++)
		archive->AddInt32("ops", data->opList[i]);

	archive->AddInt32("ops", fBuildingOp);

	return err;
}
//------------------------------------------------------------------------------
BArchivable *BShape::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BShape"))
		return new BShape(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
void BShape::Clear()
{
	shape_data *data = (shape_data*)fPrivateData;

	if (data->opList)
	{
		free(data->opList);
		data->opList = NULL;
		data->opCount = 0;
	}

	if (data->ptList)
	{
		free(data->ptList);
		data->ptList = NULL;
		data->ptCount = 0;
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
		if (bounds.left > data->ptList[0].x)
			bounds.left = data->ptList[0].x;
		if (bounds.top > data->ptList[0].y)
			bounds.top = data->ptList[0].y;
		if (bounds.right < data->ptList[0].x)
			bounds.right = data->ptList[0].x;
		if (bounds.bottom < data->ptList[0].y)
			bounds.bottom = data->ptList[0].y;
	}

	return bounds;
}


status_t
BShape::AddShape(const BShape *otherShape)
{
	shape_data *data = (shape_data*)fPrivateData;
	shape_data *otherData = (shape_data*)otherShape->fPrivateData;

	if (data->opSize < data->opCount + otherData->opCount)
	{
		int32 new_size = ((data->opCount + otherData->opBlockSize) /
			data->opBlockSize) * data->opBlockSize;
		data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
		data->opSize = new_size;
	}

	memcpy(data->opList + data->opCount * sizeof(uint32), otherData->opList,
		otherData->opCount * sizeof(uint32));
	data->opCount += otherData->opCount;

	if (data->ptSize < data->ptCount + otherData->ptCount)
	{
		int32 new_size = ((data->ptCount + otherData->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;
		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(uint32));
		data->ptSize = new_size;
	}

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
	// If there was a previous op, add the op
	if (fBuildingOp == OP_MOVETO)
	{
		data->ptList[data->ptCount - 1] = point;

		return B_OK;
	}
	else if (fBuildingOp != 0)
	{
		if (data->opSize < data->opCount + 1)
		{
			int32 new_size = ((data->opCount + data->opBlockSize) /
				data->opBlockSize) * data->opBlockSize;
			data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
			data->opSize = new_size;
		}

		data->opList[data->opCount++] = fBuildingOp;
	}

	// BuildingOp is MoveTo
	fBuildingOp = OP_MOVETO;

	// Add point
	if (data->ptSize < data->ptCount + 1)
	{
		int32 new_size = ((data->ptCount + data->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;
		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(BPoint));
		data->ptSize = new_size;
	}

	data->ptList[data->ptCount++] = point;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShape::LineTo(BPoint point)
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is LineTo increase the count
	// If there was a previous op, add the op
	if (fBuildingOp == OP_MOVETO)
	{
		fBuildingOp = OP_LINETO | OP_MOVETO | 0x01;
	}
	else if (fBuildingOp & OP_LINETO)
	{
		fBuildingOp++;
	}
	else
	{
		if (fBuildingOp != 0)
		{
			if (data->opSize < data->opCount + 1)
			{
				int32 new_size = ((data->opCount + data->opBlockSize) /
					data->opBlockSize) * data->opBlockSize;
				data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
				data->opSize = new_size;
			}

			data->opList[data->opCount++] = fBuildingOp;
		}

		fBuildingOp = OP_LINETO | 0x01;
	}

	// Add point
	if (data->ptSize < data->ptCount + 1)
	{
		int32 new_size = ((data->ptCount + data->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;
		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(BPoint));
		data->ptSize = new_size;
	}

	data->ptList[data->ptCount++] = point;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShape::BezierTo(BPoint controlPoints[3])
{
	shape_data *data = (shape_data*)fPrivateData;

	// If the last op is MoveTo, replace the op and set the count
	// If the last op is BezierTo increase the count
	// If there was a previous op, add the op
	if (fBuildingOp == OP_MOVETO)
	{
		fBuildingOp = OP_BEZIERTO | OP_MOVETO | 0x03;
	}
	else if (fBuildingOp & OP_BEZIERTO)
	{
		fBuildingOp += 3;
	}
	else
	{
		if (fBuildingOp != 0)
		{
			if (data->opSize < data->opCount + 1)
			{
				int32 new_size = ((data->opCount + data->opBlockSize) /
					data->opBlockSize) * data->opBlockSize;
				data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
				data->opSize = new_size;
			}

			data->opList[data->opCount++] = fBuildingOp;
		}

		fBuildingOp = OP_BEZIERTO | 0x03;
	}

	// Add points
	if (data->ptSize < data->ptCount + 3)
	{
		int32 new_size = ((data->ptCount + data->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;
		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(BPoint));
		data->ptSize = new_size;
	}

	data->ptList[data->ptCount++] = controlPoints[0];
	data->ptList[data->ptCount++] = controlPoints[1];
	data->ptList[data->ptCount++] = controlPoints[2];

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShape::Close()
{
	shape_data *data = (shape_data*)fPrivateData;

	// If there was a previous line/bezier op, add the op
	if (fBuildingOp & (OP_LINETO | OP_BEZIERTO))
	{
		if (data->opSize < data->opCount + 1)
		{
			int32 new_size = ((data->opCount + data->opBlockSize) /
				data->opBlockSize) * data->opBlockSize;
			data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
			data->opSize = new_size;
		}

		data->opList[data->opCount++] = fBuildingOp;
		fBuildingOp = OP_CLOSE;
	}

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShape::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BShape::_ReservedShape1() {}
void BShape::_ReservedShape2() {}
void BShape::_ReservedShape3() {}
void BShape::_ReservedShape4() {}
//------------------------------------------------------------------------------
void BShape::GetData(int32 *opCount, int32 *ptCount, uint32 **opList,
					 BPoint **ptList)
{
	shape_data *data = (shape_data*)fPrivateData;

	*opCount = data->opCount;
	*ptCount = data->ptCount;
	*opList = data->opList;
	*ptList =data->ptList;
}
//------------------------------------------------------------------------------
void BShape::SetData(int32 opCount, int32 ptCount, uint32 *opList,
					 BPoint *ptList)
{
	shape_data *data = (shape_data*)fPrivateData;

	if (data->opSize < opCount)
	{
		int32 new_size = ((opCount + data->opBlockSize) /
			data->opBlockSize) * data->opBlockSize;

		data->opList = (uint32*)realloc(data->opList, new_size * sizeof(uint32));
		data->opSize = new_size;
	}
	
	memcpy(data->opList, opList, opCount * sizeof(uint32));
	data->opCount = opCount;

	if (data->ptSize < ptCount)
	{
		int32 new_size = ((ptCount + data->ptBlockSize) /
			data->ptBlockSize) * data->ptBlockSize;

		data->ptList = (BPoint*)realloc(data->ptList, new_size * sizeof(BPoint));
		data->ptSize = new_size;
	}

	memcpy(data->ptList, ptList, ptCount * sizeof(BPoint));
	data->ptCount = ptCount;
}
//------------------------------------------------------------------------------
void BShape::InitData()
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


//	#pragma mark -
//	R4.5 compatibility


#if __GNUC__ < 3

extern "C" BRect
Bounds__6BShape(BShape *self)
{
	return self->Bounds();
}

// ToDo: Add those:
//		BShape(BShape &copyFrom);
//		status_t AddShape(BShape *other);

#endif	// __GNUC__ < 3
