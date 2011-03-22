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

CD_Struct.h

Abstract:

Defines Script Language commands

Revision History:

NEG:26.08.2002	Initiated.
--*/

#include "CD_binding.h"
#ifndef _CD_STRUCTS_H_
#define _CD_STRUCTS_H_

#ifdef		UEFI_BUILD
typedef	UINT16**	PTABLE_UNIT_TYPE;
typedef	UINTN		TABLE_UNIT_TYPE;
#else
typedef	UINT16*		PTABLE_UNIT_TYPE;
typedef	UINT16		TABLE_UNIT_TYPE;
#endif

#include <regsdef.h> //This important file is dynamically generated based on the ASIC!!!!

#define PARSER_MAJOR_REVISION 5
#define PARSER_MINOR_REVISION 0

//#include "atombios.h"
#if (PARSER_TYPE==DRIVER_TYPE_PARSER)
#ifdef FGL_LINUX
#pragma pack(push,1)
#else
#pragma pack(push)
#pragma pack(1)
#endif
#endif

#include "CD_Common_Types.h"
#include "CD_Opcodes.h"
typedef UINT16				WORK_SPACE_SIZE;
typedef enum _CD_STATUS{
    CD_SUCCESS,
    CD_CALL_TABLE,
    CD_COMPLETED=0x10,
    CD_GENERAL_ERROR=0x80,
    CD_INVALID_OPCODE,
    CD_NOT_IMPLEMENTED,
    CD_EXEC_TABLE_NOT_FOUND,
    CD_EXEC_PARAMETER_ERROR,
    CD_EXEC_PARSER_ERROR,
    CD_INVALID_DESTINATION_TYPE,
    CD_UNEXPECTED_BEHAVIOR,
    CD_INVALID_SWITCH_OPERAND_SIZE
}CD_STATUS;

#define PARSER_STRINGS                  0
#define PARSER_DEC                      1
#define PARSER_HEX                      2

#define DB_CURRENT_COMMAND_TABLE	0xFF

#define TABLE_FORMAT_BIOS		0
#define TABLE_FORMAT_EASF		1

#define EASF_TABLE_INDEX_MASK		0xfc
#define EASF_TABLE_ATTR_MASK		0x03

#define CD_ERROR(a)    (((INTN) (a)) > CD_COMPLETED)
#define CD_ERROR_OR_COMPLETED(a)    (((INTN) (a)) > CD_SUCCESS)


#if (BIOS_PARSER==1)
#ifdef _H2INC
#define STACK_BASED
#else
extern __segment farstack;
#define STACK_BASED __based(farstack)
#endif
#else
#define STACK_BASED
#endif

typedef enum _COMPARE_FLAGS{
    Below,
    Equal,
    Above,
    NotEqual,
    Overflow,
    NoCondition
}COMPARE_FLAGS;

typedef UINT16 IO_BASE_ADDR;

typedef struct _BUS_DEV_FUNC_PCI_ADDR{
    UINT8   Register;
    UINT8   Function;
    UINT8   Device;
    UINT8   Bus;
} BUS_DEV_FUNC_PCI_ADDR;

typedef struct _BUS_DEV_FUNC{
    UINT8   Function : 3;
    UINT8   Device   : 5;
    UINT8   Bus;
} BUS_DEV_FUNC;

#ifndef	UEFI_BUILD
typedef struct _PCI_CONFIG_ACCESS_CF8{
    UINT32  Reg     : 8;
    UINT32  Func    : 3;
    UINT32  Dev     : 5;
    UINT32  Bus     : 8;
    UINT32  Reserved: 7;
    UINT32  Enable  : 1;
} PCI_CONFIG_ACCESS_CF8;
#endif

typedef enum _MEM_RESOURCE {
    Stack_Resource,
    FrameBuffer_Resource,
    BIOS_Image_Resource
}MEM_RESOURCE;

typedef enum _PORTS{
    ATI_RegsPort,
    PCI_Port,
    SystemIO_Port
}PORTS;

typedef enum _OPERAND_TYPE {
    typeRegister,
    typeParamSpace,
    typeWorkSpace,
    typeFrameBuffer,
    typeIndirect,
    typeDirect,
    typePLL,
    typeMC
}OPERAND_TYPE;

typedef enum _DESTINATION_OPERAND_TYPE {
    destRegister,
    destParamSpace,
    destWorkSpace,
    destFrameBuffer,
    destPLL,
    destMC
}DESTINATION_OPERAND_TYPE;

typedef enum _SOURCE_OPERAND_TYPE {
    sourceRegister,
    sourceParamSpace,
    sourceWorkSpace,
    sourceFrameBuffer,
    sourceIndirect,
    sourceDirect,
    sourcePLL,
    sourceMC
}SOURCE_OPERAND_TYPE;

typedef enum _ALIGNMENT_TYPE {
    alignmentDword,
    alignmentLowerWord,
    alignmentMiddleWord,
    alignmentUpperWord,
    alignmentByte0,
    alignmentByte1,
    alignmentByte2,
    alignmentByte3
}ALIGNMENT_TYPE;


#define INDIRECT_IO_READ    0
#define INDIRECT_IO_WRITE   0x80
#define INDIRECT_IO_MM      0
#define INDIRECT_IO_PLL     1
#define INDIRECT_IO_MC      2

typedef struct _PARAMETERS_TYPE{
    UINT8	Destination;
    UINT8	Source;
}PARAMETERS_TYPE;
/* The following structures don't used to allocate any type of objects(variables).
   they are serve the only purpose: Get proper access to data(commands), found in the tables*/
typedef struct _PA_BYTE_BYTE{
    UINT8		PA_Destination;
    UINT8		PA_Source;
    UINT8		PA_Padding[8];
}PA_BYTE_BYTE;
typedef struct _PA_BYTE_WORD{
    UINT8		PA_Destination;
    UINT16		PA_Source;
    UINT8		PA_Padding[7];
}PA_BYTE_WORD;
typedef struct _PA_BYTE_DWORD{
    UINT8		PA_Destination;
    UINT32		PA_Source;
    UINT8		PA_Padding[5];
}PA_BYTE_DWORD;
typedef struct _PA_WORD_BYTE{
    UINT16		PA_Destination;
    UINT8		PA_Source;
    UINT8		PA_Padding[7];
}PA_WORD_BYTE;
typedef struct _PA_WORD_WORD{
    UINT16		PA_Destination;
    UINT16		PA_Source;
    UINT8		PA_Padding[6];
}PA_WORD_WORD;
typedef struct _PA_WORD_DWORD{
    UINT16		PA_Destination;
    UINT32		PA_Source;
    UINT8		PA_Padding[4];
}PA_WORD_DWORD;
typedef struct _PA_WORD_XX{
    UINT16		PA_Destination;
    UINT8		PA_Padding[8];
}PA_WORD_XX;
typedef struct _PA_BYTE_XX{
    UINT8		PA_Destination;
    UINT8		PA_Padding[9];
}PA_BYTE_XX;
/*The following 6 definitions used for Mask operation*/
typedef struct _PA_BYTE_BYTE_BYTE{
    UINT8		PA_Destination;
    UINT8		PA_AndMaskByte;
    UINT8		PA_OrMaskByte;
    UINT8		PA_Padding[7];
}PA_BYTE_BYTE_BYTE;
typedef struct _PA_BYTE_WORD_WORD{
    UINT8		PA_Destination;
    UINT16		PA_AndMaskWord;
    UINT16		PA_OrMaskWord;
    UINT8		PA_Padding[5];
}PA_BYTE_WORD_WORD;
typedef struct _PA_BYTE_DWORD_DWORD{
    UINT8		PA_Destination;
    UINT32		PA_AndMaskDword;
    UINT32		PA_OrMaskDword;
    UINT8		PA_Padding;
}PA_BYTE_DWORD_DWORD;
typedef struct _PA_WORD_BYTE_BYTE{
    UINT16		PA_Destination;
    UINT8		PA_AndMaskByte;
    UINT8		PA_OrMaskByte;
    UINT8		PA_Padding[6];
}PA_WORD_BYTE_BYTE;
typedef struct _PA_WORD_WORD_WORD{
    UINT16		PA_Destination;
    UINT16		PA_AndMaskWord;
    UINT16		PA_OrMaskWord;
    UINT8		PA_Padding[4];
}PA_WORD_WORD_WORD;
typedef struct _PA_WORD_DWORD_DWORD{
    UINT16		PA_Destination;
    UINT32		PA_AndMaskDword;
    UINT32		PA_OrMaskDword;
}PA_WORD_DWORD_DWORD;


typedef union _PARAMETER_ACCESS {
    PA_BYTE_XX			ByteXX;
    PA_BYTE_BYTE		ByteByte;
    PA_BYTE_WORD		ByteWord;
    PA_BYTE_DWORD		ByteDword;
    PA_WORD_BYTE		WordByte;
    PA_WORD_WORD		WordWord;
    PA_WORD_DWORD		WordDword;
    PA_WORD_XX			WordXX;
/*The following 6 definitions used for Mask operation*/
    PA_BYTE_BYTE_BYTE	ByteByteAndByteOr;
    PA_BYTE_WORD_WORD	ByteWordAndWordOr;
    PA_BYTE_DWORD_DWORD	ByteDwordAndDwordOr;
    PA_WORD_BYTE_BYTE	WordByteAndByteOr;
    PA_WORD_WORD_WORD	WordWordAndWordOr;
    PA_WORD_DWORD_DWORD	WordDwordAndDwordOr;
}PARAMETER_ACCESS;

typedef	struct _COMMAND_ATTRIBUTE {
    UINT8		Source:3;
    UINT8		SourceAlignment:3;
    UINT8		DestinationAlignment:2;
}COMMAND_ATTRIBUTE;

typedef struct _SOURCE_DESTINATION_ALIGNMENT{
    UINT8					DestAlignment;
    UINT8					SrcAlignment;
}SOURCE_DESTINATION_ALIGNMENT;
typedef struct _MULTIPLICATION_RESULT{
    UINT32									Low32Bit;
    UINT32									High32Bit;
}MULTIPLICATION_RESULT;
typedef struct _DIVISION_RESULT{
    UINT32									Quotient32;
    UINT32									Reminder32;
}DIVISION_RESULT;
typedef union _DIVISION_MULTIPLICATION_RESULT{
    MULTIPLICATION_RESULT		Multiplication;
    DIVISION_RESULT					Division;
}DIVISION_MULTIPLICATION_RESULT;
typedef struct _COMMAND_HEADER {
    UINT8					Opcode;
    COMMAND_ATTRIBUTE		Attribute;
}COMMAND_HEADER;

typedef struct _GENERIC_ATTRIBUTE_COMMAND{
    COMMAND_HEADER			Header;
    PARAMETER_ACCESS		Parameters;
} GENERIC_ATTRIBUTE_COMMAND;

typedef struct	_COMMAND_TYPE_1{
    UINT8					Opcode;
    PARAMETER_ACCESS		Parameters;
} COMMAND_TYPE_1;

typedef struct	_COMMAND_TYPE_OPCODE_OFFSET16{
    UINT8					Opcode;
    UINT16					CD_Offset16;
} COMMAND_TYPE_OPCODE_OFFSET16;

typedef struct	_COMMAND_TYPE_OPCODE_OFFSET32{
    UINT8					Opcode;
    UINT32					CD_Offset32;
} COMMAND_TYPE_OPCODE_OFFSET32;

typedef struct	_COMMAND_TYPE_OPCODE_VALUE_BYTE{
    UINT8					Opcode;
    UINT8					Value;
} COMMAND_TYPE_OPCODE_VALUE_BYTE;

typedef union  _COMMAND_SPECIFIC_UNION{
    UINT8	ContinueSwitch;
    UINT8	ControlOperandSourcePosition;
    UINT8	IndexInMasterTable;
} COMMAND_SPECIFIC_UNION;


typedef struct _CD_GENERIC_BYTE{
    UINT16					CommandType:3;
    UINT16					CurrentParameterSize:3;
    UINT16					CommandAccessType:3;
    UINT16					CurrentPort:2;
    UINT16					PS_SizeInDwordsUsedByCallingTable:5;
}CD_GENERIC_BYTE;

typedef UINT8	COMMAND_TYPE_OPCODE_ONLY;

typedef UINT8  COMMAND_HEADER_POINTER;


#if (PARSER_TYPE==BIOS_TYPE_PARSER)

typedef struct _DEVICE_DATA	{
    UINT32	STACK_BASED		*pParameterSpace;
    UINT8									*pBIOS_Image;
    UINT8							format;
#if (IO_INTERFACE==PARSER_INTERFACE)
    IO_BASE_ADDR					IOBase;
#endif
}  DEVICE_DATA;

#else

typedef struct _DEVICE_DATA	{
    UINT32							*pParameterSpace;
    VOID								*CAIL;
    UINT8 							*pBIOS_Image;
    UINT32							format;
} DEVICE_DATA;

#endif

struct _PARSER_TEMP_DATA;
typedef UINT32 WORKSPACE_POINTER;

struct	_WORKING_TABLE_DATA{
    UINT8																		* pTableHead;
    COMMAND_HEADER_POINTER									* IP;			// Commands pointer
    WORKSPACE_POINTER	STACK_BASED						* pWorkSpace;
    struct _WORKING_TABLE_DATA STACK_BASED  * prevWorkingTableData;
};



typedef struct	_PARSER_TEMP_DATA{
    DEVICE_DATA	STACK_BASED							*pDeviceData;
    struct _WORKING_TABLE_DATA STACK_BASED		*pWorkingTableData;
    UINT32															SourceData32;
    UINT32															DestData32;
    DIVISION_MULTIPLICATION_RESULT			MultiplicationOrDivision;
    UINT32															Index;
    UINT32					                    CurrentFB_Window;
    UINT32					                    IndirectData;
    UINT16															CurrentRegBlock;
    TABLE_UNIT_TYPE													CurrentDataBlock;
    UINT16                              AttributesData;
//  UINT8                               *IndirectIOTable;
    UINT8                               *IndirectIOTablePointer;
    GENERIC_ATTRIBUTE_COMMAND						*pCmd;			//CurrentCommand;
    SOURCE_DESTINATION_ALIGNMENT  			CD_Mask;
    PARAMETERS_TYPE											ParametersType;
    CD_GENERIC_BYTE											Multipurpose;
    UINT8																CompareFlags;
    COMMAND_SPECIFIC_UNION							CommandSpecific;
    CD_STATUS														Status;
    UINT8                               Shift2MaskConverter;
    UINT8															  CurrentPortID;
} PARSER_TEMP_DATA;


typedef struct _WORKING_TABLE_DATA  WORKING_TABLE_DATA;



typedef VOID (*COMMANDS_DECODER)(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
typedef VOID (*WRITE_IO_FUNCTION)(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
typedef UINT32 (*READ_IO_FUNCTION)(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
typedef UINT32 (*CD_GET_PARAMETERS)(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

typedef struct _COMMANDS_PROPERTIES
{
    COMMANDS_DECODER  function;
    UINT8             destination;
    UINT8             headersize;
} COMMANDS_PROPERTIES;

typedef struct _INDIRECT_IO_PARSER_COMMANDS
{
    COMMANDS_DECODER  func;
    UINT8             csize;
} INDIRECT_IO_PARSER_COMMANDS;

#if (PARSER_TYPE==DRIVER_TYPE_PARSER)
#pragma pack(pop)
#endif

#endif
