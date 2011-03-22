/*
 * Copyright 2006-2007 Advanced Micro Devices, Inc.  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*++

Module Name:

CD_Definitions.h

Abstract:

Defines Script Language commands

Revision History:

NEG:27.08.2002	Initiated.
--*/

#include "CD_Structs.h"
#ifndef _CD_DEFINITIONS_H
#define _CD_DEFINITIONS_H_
#ifdef DRIVER_PARSER
VOID *AllocateMemory(VOID *, UINT16);
VOID ReleaseMemory(DEVICE_DATA * , WORKING_TABLE_DATA* );
#endif
CD_STATUS ParseTable(DEVICE_DATA* pDeviceData, UINT8 IndexInMasterTable);
//CD_STATUS CD_MainLoop(PARSER_TEMP_DATA_POINTER pParserTempData);
CD_STATUS Main_Loop(DEVICE_DATA* pDeviceData,UINT16 *MasterTableOffset,UINT8 IndexInMasterTable);
UINT16* GetCommandMasterTablePointer(DEVICE_DATA*  pDeviceData);
#endif //CD_DEFINITIONS
