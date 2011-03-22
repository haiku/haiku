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

Decoder.h

Abstract:

Includes all helper headers

Revision History:

NEG:27.08.2002	Initiated.
--*/
#ifndef _DECODER_H_
#define _DECODER_H_
#define WS_QUOTIENT_C														64
#define WS_REMINDER_C														(WS_QUOTIENT_C+1)
#define WS_DATAPTR_C														(WS_REMINDER_C+1)
#define WS_SHIFT_C													    (WS_DATAPTR_C+1)
#define WS_OR_MASK_C													  (WS_SHIFT_C+1)
#define WS_AND_MASK_C													  (WS_OR_MASK_C+1)
#define WS_FB_WINDOW_C                          (WS_AND_MASK_C+1)
#define WS_ATTRIBUTES_C                         (WS_FB_WINDOW_C+1)
#define WS_REGPTR_C                             (WS_ATTRIBUTES_C+1)
#define PARSER_VERSION_MAJOR                   0x00000000
#define PARSER_VERSION_MINOR                   0x0000000E
#define PARSER_VERSION                         (PARSER_VERSION_MAJOR | PARSER_VERSION_MINOR)
#include "CD_binding.h"
#include "CD_Common_Types.h"
#include "CD_hw_services.h"
#include "CD_Structs.h"
#include "CD_Definitions.h"
#include "CD_Opcodes.h"

#define	SOURCE_ONLY_CMD_TYPE		0//0xFE
#define SOURCE_DESTINATION_CMD_TYPE	1//0xFD
#define	DESTINATION_ONLY_CMD_TYPE	2//0xFC

#define	ACCESS_TYPE_BYTE			0//0xF9
#define	ACCESS_TYPE_WORD			1//0xF8
#define	ACCESS_TYPE_DWORD			2//0xF7
#define	SWITCH_TYPE_ACCESS			3//0xF6

#define CD_CONTINUE					0//0xFB
#define CD_STOP						1//0xFA


#define IS_END_OF_TABLE(cmd) ((cmd) == EOT_OPCODE)
#define IS_COMMAND_VALID(cmd) (((cmd)<=LastValidCommand)&&((cmd)>=FirstValidCommand))
#define IS_IT_SHIFT_COMMAND(Opcode) ((Opcode<=SHIFT_RIGHT_MC_OPCODE)&&(Opcode>=SHIFT_LEFT_REG_OPCODE))
#define IS_IT_XXXX_COMMAND(Group, Opcode) ((Opcode<=Group##_MC_OPCODE)&&(Opcode>=Group##_REG_OPCODE))
#define	CheckCaseAndAdjustIP_Macro(size) \
	if (pParserTempData->SourceData32==(UINT32)((CASE_OFFSET*)pParserTempData->pWorkingTableData->IP)->XX_Access.size##.Access.Value){\
		pParserTempData->CommandSpecific.ContinueSwitch = CD_STOP;\
		pParserTempData->pWorkingTableData->IP =(COMMAND_HEADER_POINTER *) RELATIVE_TO_TABLE(((CASE_OFFSET*)pParserTempData->pWorkingTableData->IP)->XX_Access.size##.Access.JumpOffset);\
	}else{\
		pParserTempData->pWorkingTableData->IP+=(sizeof (CASE_##size##ACCESS)\
		+sizeof(((CASE_OFFSET*)pParserTempData->pWorkingTableData->IP)->CaseSignature));\
	}

#endif
/*	pWorkingTableData->pCmd->Header.Attribute.SourceAlignment=alignmentLowerWord;\*/

// EOF
