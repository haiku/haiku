/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SHAPE_PRIVATE_H
#define SHAPE_PRIVATE_H

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

#endif
