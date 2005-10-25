/*
 * ftl.h 1.5 1999/07/20 16:07:58
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 * are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _LINUX_FTL_H
#define _LINUX_FTL_H

typedef struct erase_unit_header_t {
    u_char	LinkTargetTuple[5];
    u_char	DataOrgTuple[10];
    u_char	NumTransferUnits;
    u_int	EraseCount;
    u_short	LogicalEUN;
    u_char	BlockSize;
    u_char	EraseUnitSize;
    u_short	FirstPhysicalEUN;
    u_short	NumEraseUnits;
    u_int	FormattedSize;
    u_int	FirstVMAddress;
    u_short	NumVMPages;
    u_char	Flags;
    u_char	Code;
    u_int	SerialNumber;
    u_int	AltEUHOffset;
    u_int	BAMOffset;
    u_char	Reserved[12];
    u_char	EndTuple[2];
} erase_unit_header_t;

/* Flags in erase_unit_header_t */
#define HIDDEN_AREA		0x01
#define REVERSE_POLARITY	0x02
#define DOUBLE_BAI		0x04

/* Definitions for block allocation information */

#define BLOCK_FREE(b)		((b) == 0xffffffff)
#define BLOCK_DELETED(b)	(((b) == 0) || ((b) == 0xfffffffe))

#define BLOCK_TYPE(b)		((b) & 0x7f)
#define BLOCK_ADDRESS(b)	((b) & ~0x7f)
#define BLOCK_NUMBER(b)		((b) >> 9)
#define BLOCK_CONTROL		0x30
#define BLOCK_DATA		0x40
#define BLOCK_REPLACEMENT	0x60
#define BLOCK_BAD		0x70

#endif /* _LINUX_FTL_H */
