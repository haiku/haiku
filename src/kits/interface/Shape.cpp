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

#include "Shape.h"
#include "Point.h"
#include "Rect.h"
#include <Support/Errors.h>
#include <App/Message.h>

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
	:	fState(0),
		fBuildingOp(0),
		fPrivateData(NULL)
{
	InitData();
}
//------------------------------------------------------------------------------
BShape::BShape(const BShape &copyFrom)
	:	fState(0),
		fBuildingOp(0),
		fPrivateData(NULL)
{
	InitData();
}
//------------------------------------------------------------------------------
BShape::BShape(BMessage *archive)
	:	fState(0),
		fBuildingOp(0),
		fPrivateData(NULL)
{
	InitData();
}
//------------------------------------------------------------------------------
BShape::~BShape()
{
	shape_data *data = (shape_data*)fPrivateData;

	free(data->opList);
	free(data->ptList);

	delete fPrivateData;
}
//------------------------------------------------------------------------------
status_t BShape::Archive(BMessage *archive, bool deep) const
{
	status_t err = BArchivable::Archive(archive, deep);

	if (err != B_OK)
		return err;

	shape_data *data = (shape_data*)fPrivateData;

	// If no valid shape data, return
	if (data->opCount == 0 || data->ptCount == 0)
		return err;

	// Avoids allocation for each point
	archive->AddData("pts", B_POINT_TYPE, data->ptList, sizeof(BPoint), true,
		data->ptCount);

	for (int32 i = 1; i < data->ptCount; i++)
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
//------------------------------------------------------------------------------
BRect BShape::Bounds() const
{
	return BRect(0.0f, 0.0f, 0.0f, 0.0f);
}
//------------------------------------------------------------------------------
status_t BShape::AddShape(const BShape *otherShape)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BShape::MoveTo(BPoint point)
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
		if (data->opList)
			data->opList = (uint32*)realloc(data->opList,
			(data->opCount + 1) * sizeof(uint32));
		else
			data->opList = (uint32*)malloc(sizeof(uint32));

		data->opList[data->opCount++] = fBuildingOp;
	}

	// BuildingOp is MoveTo
	fBuildingOp = OP_MOVETO;

	// Add point
	if (data->ptList)
		data->ptList = (BPoint*)realloc(data->ptList,
		(data->ptCount + 1) * sizeof(BPoint));
	else
		data->ptList = (BPoint*)malloc(sizeof(BPoint));

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
			if (data->opList)
				data->opList = (uint32*)realloc(data->opList,
					(data->opCount + 1) * sizeof(uint32));
			else
				data->opList = (uint32*)malloc(sizeof(uint32));

			data->opList[data->opCount++] = fBuildingOp;
		}

		fBuildingOp = OP_LINETO | 0x01;
	}

	// Add point
	if (data->ptList)
		data->ptList = (BPoint*)realloc(data->ptList,
		(data->ptCount + 1) * sizeof(BPoint));
	else
		data->ptList = (BPoint*)malloc(sizeof(BPoint));

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
			if (data->opList)
				data->opList = (uint32*)realloc(data->opList,
					(data->opCount + 1) * sizeof(uint32));
			else
				data->opList = (uint32*)malloc(sizeof(uint32));

			data->opList[data->opCount++] = fBuildingOp;
		}

		fBuildingOp = OP_BEZIERTO | 0x03;
	}

	// Add points
	if (data->ptList)
		data->ptList = (BPoint*)realloc(data->ptList,
		(data->ptCount + 3) * sizeof(BPoint));
	else
		data->ptList = (BPoint*)malloc(3 * sizeof(BPoint));

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
		if (data->opList)
			data->opList = (uint32*)realloc(data->opList,
			(data->opCount + 1) * sizeof(uint32));
		else
			data->opList = (uint32*)malloc(sizeof(uint32));

		data->opList[data->opCount++] = fBuildingOp;
		fBuildingOp = OP_CLOSE;
	}

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BShape::Perform(perform_code d, void *arg)
{
	return B_ERROR;
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
	if (opList == NULL || ptList == NULL)
		return;

	shape_data *data = (shape_data*)fPrivateData;

	if (data->opCount)
	{
		*opList = new uint32[data->opCount];
		memcpy(*opList, data->opList, data->opCount * sizeof(uint32));
		*opCount = data->opCount;
	}
	else
	{
		*opList = NULL;
		*opCount = 0;
	}

	if (data->ptCount)
	{
		*ptList = new BPoint[data->ptCount];
		memcpy(*ptList, data->ptList, data->ptCount * sizeof(BPoint));
		*ptCount = data->ptCount;
	}
	else
	{
		*ptList = NULL;
		*ptCount = 0;
	}
}
//------------------------------------------------------------------------------
void BShape::SetData(int32 opCount, int32 ptCount, uint32 *opList,
					 BPoint *ptList)
{
	shape_data *data = (shape_data*)fPrivateData;

	if (data->opCount)
		delete data->opList;

	if (opCount)
	{
		data->opList = new uint32[opCount];
		memcpy(data->opList, opList, opCount * sizeof(uint32));
		data->opCount = opCount;
	}
	else
	{
		data->opList = NULL;
		data->opCount = 0;
	}

	if (data->ptCount)
		delete data->ptList;

	if (opCount)
	{
		data->ptList = new BPoint[opCount];
		memcpy(data->ptList, ptList, ptCount * sizeof(BPoint));
		data->ptCount = ptCount;
	}
	else
	{
		data->ptList = NULL;
		data->ptCount = 0;
	}
}
//------------------------------------------------------------------------------
void BShape::InitData()
{
	fPrivateData = new shape_data;
	shape_data *data = (shape_data*)fPrivateData;

	data->opList = NULL;
	data->opCount = 0;
	data->opSize = 0;
	data->opBlockSize = 255;
	data->ptList = NULL;
	data->ptCount = 0;
	data->ptSize = 0;
	data->ptBlockSize = 255;
}
//------------------------------------------------------------------------------
