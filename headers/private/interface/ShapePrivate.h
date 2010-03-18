/*
 * Copyright 2003-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SHAPE_PRIVATE_H
#define SHAPE_PRIVATE_H

#define OP_LINETO			0x10000000
#define OP_BEZIERTO			0x20000000
#define OP_CLOSE			0x40000000
#define OP_MOVETO			0x80000000
#define OP_LARGE_ARC_TO_CW	0x01000000
#define OP_LARGE_ARC_TO_CCW	0x02000000
#define OP_SMALL_ARC_TO_CW	0x04000000
#define OP_SMALL_ARC_TO_CCW	0x08000000


struct shape_data {
	uint32	*opList;
	int32	opCount;
	int32	opSize;
	BPoint	*ptList;
	int32	ptCount;
	int32	ptSize;
};

#endif
