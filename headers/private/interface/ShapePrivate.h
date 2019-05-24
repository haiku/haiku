/*
 * Copyright 2003-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Adrian Oanca, adioanca@cotty.iren.ro
 */
#ifndef SHAPE_PRIVATE_H
#define SHAPE_PRIVATE_H

#include <Point.h>
#include <Rect.h>
#include <Referenceable.h>

#include <string.h>
#include <stdio.h>


#define OP_LINETO			0x10000000
#define OP_BEZIERTO			0x20000000
#define OP_CLOSE			0x40000000
#define OP_MOVETO			0x80000000
#define OP_LARGE_ARC_TO_CW	0x01000000
#define OP_LARGE_ARC_TO_CCW	0x02000000
#define OP_SMALL_ARC_TO_CW	0x04000000
#define OP_SMALL_ARC_TO_CCW	0x08000000


class shape_data : public BReferenceable {
public:
	uint32*	opList;
	BPoint*	ptList;
	int32	opCount;
	int32	opSize;
	int32	ptCount;
	int32	ptSize;

	bool    fOwnsMemory;

	shape_data()
		:
		fOwnsMemory(false)
	{
	}

	~shape_data()
	{
		if (fOwnsMemory) {
			delete[] opList;
			delete[] ptList;
		}
	}

	shape_data(const shape_data& other)
	{
		opList = new(std::nothrow) uint32[other.opCount];
		ptList = new(std::nothrow) BPoint[other.ptCount];
		fOwnsMemory = true;
		opCount = other.opCount;
		opSize = other.opSize;
		ptCount = other.ptCount;
		ptSize = other.ptSize;
		memcpy((void*)opList, other.opList, opSize);
		memcpy((void*)ptList, other.ptList, ptSize);
	}

	BRect DetermineBoundingBox() const
	{
		BRect bounds;

		if (ptCount == 0)
			return bounds;

		// TODO: This implementation doesn't take into account curves at all.
		bounds.left = ptList[0].x;
		bounds.top = ptList[0].y;
		bounds.right = ptList[0].x;
		bounds.bottom = ptList[0].y;

		for (int32 i = 1; i < ptCount; i++) {
			if (bounds.left > ptList[i].x)
				bounds.left = ptList[i].x;

			if (bounds.top > ptList[i].y)
				bounds.top = ptList[i].y;

			if (bounds.right < ptList[i].x)
				bounds.right = ptList[i].x;

			if (bounds.bottom < ptList[i].y)
				bounds.bottom = ptList[i].y;
		}

		return bounds;
	}
};


#endif	// SHAPE_PRIVATE_H
