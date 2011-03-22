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

#ifdef NT_BUILD
#ifdef LH_BUILD
#include <ntddk.h>
#else
#include <miniport.h>
#endif // LH_BUILD
#endif // NT_BUILD


#if ((defined DBG) || (defined DEBUG))
#define DEBUG_PARSER				1   // enable parser debug output
#endif

#define USE_SWITCH_COMMAND			1
#define	DRIVER_TYPE_PARSER		0x48

#define PARSER_TYPE DRIVER_TYPE_PARSER

#define AllocateWorkSpace(x,y)      AllocateMemory(pDeviceData,y)
#define FreeWorkSpace(x,y)          ReleaseMemory(x,y)

#define RELATIVE_TO_BIOS_IMAGE( x ) ((ULONG_PTR)x + (ULONG_PTR)((DEVICE_DATA*)pParserTempData->pDeviceData->pBIOS_Image))
#define RELATIVE_TO_TABLE( x )      (x + (UCHAR *)(pParserTempData->pWorkingTableData->pTableHead))

