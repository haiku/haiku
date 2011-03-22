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

#ifndef _HW_SERVICES_INTERFACE_
#define _HW_SERVICES_INTERFACE_

#include	"CD_Common_Types.h"
#include	"CD_Structs.h"


// CD - from Command Decoder
typedef	UINT16	CD_REG_INDEX;
typedef	UINT8	CD_PCI_OFFSET;
typedef	UINT16	CD_FB_OFFSET;
typedef	UINT16	CD_SYS_IO_PORT;
typedef	UINT8	CD_MEM_TYPE;
typedef	UINT8	CD_MEM_SIZE;

typedef VOID *	CD_VIRT_ADDR;
typedef UINT32	CD_PHYS_ADDR;
typedef UINT32	CD_IO_ADDR;

/***********************ATI Registers access routines**************************/

	VOID	ReadIndReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteIndReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT32	ReadReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT32	ReadPLL32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WritePLL32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT32	ReadMC32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteMC32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

/************************PCI Registers access routines*************************/

	UINT8	ReadPCIReg8(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT16	ReadPCIReg16(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT32	ReadPCIReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WritePCIReg8(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WritePCIReg16(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WritePCIReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

/***************************Frame buffer access routines************************/

	UINT32  ReadFrameBuffer32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteFrameBuffer32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

/******************System IO Registers access routines********************/

	UINT8	ReadSysIOReg8(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT16	ReadSysIOReg16(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	UINT32	ReadSysIOReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteSysIOReg8(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteSysIOReg16(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	WriteSysIOReg32(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

/****************************Delay routines****************************************/

	VOID	DelayMicroseconds(PARSER_TEMP_DATA STACK_BASED * pParserTempData); // take WORKING_TABLE_DATA->SourceData32 as a delay value

	VOID	DelayMilliseconds(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	PostCharOutput(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

	VOID	CallerDebugFunc(PARSER_TEMP_DATA STACK_BASED * pParserTempData);


//************************Tracing/Debugging routines and macroses******************/
#define KEYPRESSED	-1

#if (DEBUG_PARSER != 0)

#ifdef DRIVER_PARSER

VOID CD_print_string	(DEVICE_DATA STACK_BASED *pDeviceData, UINT8 *str);
VOID CD_print_value	    (DEVICE_DATA STACK_BASED *pDeviceData, ULONG_PTR value, UINT16 value_type );

// Level 1 : can use WorkingTableData or pDeviceData
#define CD_TRACE_DL1(string)			CD_print_string(pDeviceData, string);
#define CD_TRACETAB_DL1(string)			CD_TRACE_DL1("\n");CD_TRACE_DL1(string)
#define CD_TRACEDEC_DL1(value)			CD_print_value( pDeviceData, (ULONG_PTR)value, PARSER_DEC);
#define CD_TRACEHEX_DL1(value)			CD_print_value( pDeviceData, (ULONG_PTR)value, PARSER_HEX);

// Level 2:can use pWorkingTableData
#define CD_TRACE_DL2(string)			CD_print_string( pWorkingTableData->pParserTempData->pDeviceData, string);
#define CD_TRACETAB_DL2(string)			CD_TRACE_DL2("\n");CD_TRACE_DL2(string)
#define CD_TRACEDEC_DL2(value)			CD_print_value( pWorkingTableData->pParserTempData->pDeviceData, (ULONG_PTR)value, PARSER_DEC);
#define CD_TRACEHEX_DL2(value)			CD_print_value( pWorkingTableData->pParserTempData->pDeviceData, (ULONG_PTR)value, PARSER_HEX);

// Level 3:can use pWorkingTableData
#define CD_TRACE_DL3(string)			CD_print_string( pWorkingTableData->pParserTempData->pDeviceData, string);
#define CD_TRACETAB_DL3(string)			CD_TRACE_DL3("\n");CD_TRACE_DL3(string)
#define CD_TRACEDEC_DL3(value)			CD_print_value( pWorkingTableData->pParserTempData->pDeviceData, value, PARSER_DEC);
#define CD_TRACEHEX_DL3(value)			CD_print_value( pWorkingTableData->pParserTempData->pDeviceData, value, PARSER_HEX);

#define CD_TRACE(string)
#define CD_WAIT(what)
#define CD_BREAKPOINT()

#else


VOID CD_assert	(UINT8 *file, INTN lineno);		//output file/line to debug console
VOID CD_postcode(UINT8 value);					//output post code to debug console
VOID CD_print	(UINT8 *str);					//output text to debug console
VOID CD_print_dec(UINTN value);					//output value in decimal format to debug console
VOID CD_print_hex(UINT32 value, UINT8 len);		//output value in hexadecimal format to debug console
VOID CD_print_buf(UINT8 *p, UINTN len);			//output dump of memory to debug console
VOID CD_wait(INT32 what);						//wait for KEYPRESSED=-1 or Delay value	expires
VOID CD_breakpoint();							//insert int3 opcode or 0xF1 (for American Arium)

#define CD_ASSERT(condition)			if(!(condition)) CD_assert(__FILE__, __LINE__)
#define CD_POSTCODE(value)				CD_postcode(value)
#define CD_TRACE(string)				CD_print(string)
#define CD_TRACETAB(string)				CD_print(string)
#define CD_TRACEDEC(value)				CD_print_dec( (UINTN)(value))
#define CD_TRACEHEX(value)				CD_print_hex( (UINT32)(value), sizeof(value) )
#define CD_TRACEBUF(pointer, len)		CD_print_buf( (UINT8 *)(pointer), (UINTN) len)
#define CD_WAIT(what)					CD_wait((INT32)what)
#define CD_BREAKPOINT()					CD_breakpoint()

#if (DEBUG_PARSER == 4)
#define CD_ASSERT_DL4(condition)		if(!(condition)) CD_assert(__FILE__, __LINE__)
#define CD_POSTCODE_DL4(value)			CD_postcode(value)
#define CD_TRACE_DL4(string)			CD_print(string)
#define CD_TRACETAB_DL4(string)			CD_print("\n\t\t");CD_print(string)
#define CD_TRACEDEC_DL4(value)			CD_print_dec( (UINTN)(value))
#define CD_TRACEHEX_DL4(value)			CD_print_hex( (UINT32)(value), sizeof(value) )
#define CD_TRACEBUF_DL4(pointer, len)	CD_print_buf( (UINT8 *)(pointer), (UINTN) len)
#define CD_WAIT_DL4(what)				CD_wait((INT32)what)
#define CD_BREAKPOINT_DL4()				CD_breakpoint()
#else
#define CD_ASSERT_DL4(condition)
#define CD_POSTCODE_DL4(value)
#define CD_TRACE_DL4(string)
#define CD_TRACETAB_DL4(string)
#define CD_TRACEDEC_DL4(value)
#define CD_TRACEHEX_DL4(value)
#define CD_TRACEBUF_DL4(pointer, len)
#define CD_WAIT_DL4(what)
#define CD_BREAKPOINT_DL4()
#endif

#if (DEBUG_PARSER >= 3)
#define CD_ASSERT_DL3(condition)		if(!(condition)) CD_assert(__FILE__, __LINE__)
#define CD_POSTCODE_DL3(value)			CD_postcode(value)
#define CD_TRACE_DL3(string)			CD_print(string)
#define CD_TRACETAB_DL3(string)			CD_print("\n\t\t");CD_print(string)
#define CD_TRACEDEC_DL3(value)			CD_print_dec( (UINTN)(value))
#define CD_TRACEHEX_DL3(value)			CD_print_hex( (UINT32)(value), sizeof(value) )
#define CD_TRACEBUF_DL3(pointer, len)	CD_print_buf( (UINT8 *)(pointer), (UINTN) len)
#define CD_WAIT_DL3(what)				CD_wait((INT32)what)
#define CD_BREAKPOINT_DL3()				CD_breakpoint()
#else
#define CD_ASSERT_DL3(condition)
#define CD_POSTCODE_DL3(value)
#define CD_TRACE_DL3(string)
#define CD_TRACETAB_DL3(string)
#define CD_TRACEDEC_DL3(value)
#define CD_TRACEHEX_DL3(value)
#define CD_TRACEBUF_DL3(pointer, len)
#define CD_WAIT_DL3(what)
#define CD_BREAKPOINT_DL3()
#endif


#if (DEBUG_PARSER >= 2)
#define CD_ASSERT_DL2(condition)		if(!(condition)) CD_assert(__FILE__, __LINE__)
#define CD_POSTCODE_DL2(value)			CD_postcode(value)
#define CD_TRACE_DL2(string)			CD_print(string)
#define CD_TRACETAB_DL2(string)			CD_print("\n\t");CD_print(string)
#define CD_TRACEDEC_DL2(value)			CD_print_dec( (UINTN)(value))
#define CD_TRACEHEX_DL2(value)			CD_print_hex( (UINT32)(value), sizeof(value) )
#define CD_TRACEBUF_DL2(pointer, len)	CD_print_buf( (UINT8 *)(pointer), (UINTN) len)
#define CD_WAIT_DL2(what)				CD_wait((INT32)what)
#define CD_BREAKPOINT_DL2()				CD_breakpoint()
#else
#define CD_ASSERT_DL2(condition)
#define CD_POSTCODE_DL2(value)
#define CD_TRACE_DL2(string)
#define CD_TRACETAB_DL2(string)
#define CD_TRACEDEC_DL2(value)
#define CD_TRACEHEX_DL2(value)
#define CD_TRACEBUF_DL2(pointer, len)
#define CD_WAIT_DL2(what)
#define CD_BREAKPOINT_DL2()
#endif


#if (DEBUG_PARSER >= 1)
#define CD_ASSERT_DL1(condition)		if(!(condition)) CD_assert(__FILE__, __LINE__)
#define CD_POSTCODE_DL1(value)			CD_postcode(value)
#define CD_TRACE_DL1(string)			CD_print(string)
#define CD_TRACETAB_DL1(string)			CD_print("\n");CD_print(string)
#define CD_TRACEDEC_DL1(value)			CD_print_dec( (UINTN)(value))
#define CD_TRACEHEX_DL1(value)			CD_print_hex( (UINT32)(value), sizeof(value) )
#define CD_TRACEBUF_DL1(pointer, len)	CD_print_buf( (UINT8 *)(pointer), (UINTN) len)
#define CD_WAIT_DL1(what)				CD_wait((INT32)what)
#define CD_BREAKPOINT_DL1()				CD_breakpoint()
#else
#define CD_ASSERT_DL1(condition)
#define CD_POSTCODE_DL1(value)
#define CD_TRACE_DL1(string)
#define CD_TRACETAB_DL1(string)
#define CD_TRACEDEC_DL1(value)
#define CD_TRACEHEX_DL1(value)
#define CD_TRACEBUF_DL1(pointer, len)
#define CD_WAIT_DL1(what)
#define CD_BREAKPOINT_DL1()
#endif

#endif  //#ifdef DRIVER_PARSER


#else

#define CD_ASSERT(condition)
#define CD_POSTCODE(value)
#define CD_TRACE(string)
#define CD_TRACEDEC(value)
#define CD_TRACEHEX(value)
#define CD_TRACEBUF(pointer, len)
#define CD_WAIT(what)
#define CD_BREAKPOINT()

#define CD_ASSERT_DL4(condition)
#define CD_POSTCODE_DL4(value)
#define CD_TRACE_DL4(string)
#define CD_TRACETAB_DL4(string)
#define CD_TRACEDEC_DL4(value)
#define CD_TRACEHEX_DL4(value)
#define CD_TRACEBUF_DL4(pointer, len)
#define CD_WAIT_DL4(what)
#define CD_BREAKPOINT_DL4()

#define CD_ASSERT_DL3(condition)
#define CD_POSTCODE_DL3(value)
#define CD_TRACE_DL3(string)
#define CD_TRACETAB_DL3(string)
#define CD_TRACEDEC_DL3(value)
#define CD_TRACEHEX_DL3(value)
#define CD_TRACEBUF_DL3(pointer, len)
#define CD_WAIT_DL3(what)
#define CD_BREAKPOINT_DL3()

#define CD_ASSERT_DL2(condition)
#define CD_POSTCODE_DL2(value)
#define CD_TRACE_DL2(string)
#define CD_TRACETAB_DL2(string)
#define CD_TRACEDEC_DL2(value)
#define CD_TRACEHEX_DL2(value)
#define CD_TRACEBUF_DL2(pointer, len)
#define CD_WAIT_DL2(what)
#define CD_BREAKPOINT_DL2()

#define CD_ASSERT_DL1(condition)
#define CD_POSTCODE_DL1(value)
#define CD_TRACE_DL1(string)
#define CD_TRACETAB_DL1(string)
#define CD_TRACEDEC_DL1(value)
#define CD_TRACEHEX_DL1(value)
#define CD_TRACEBUF_DL1(pointer, len)
#define CD_WAIT_DL1(what)
#define CD_BREAKPOINT_DL1()


#endif  //#if (DEBUG_PARSER > 0)


#ifdef CHECKSTACK
VOID CD_fillstack(UINT16 size);
UINT16 CD_checkstack(UINT16 size);
#define CD_CHECKSTACK(stacksize)	CD_checkstack(stacksize)
#define CD_FILLSTACK(stacksize)		CD_fillstack(stacksize)
#else
#define CD_CHECKSTACK(stacksize)	0
#define CD_FILLSTACK(stacksize)
#endif


#endif
