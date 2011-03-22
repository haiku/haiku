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

/**

Module Name:

    CD_Operations.c

Abstract:

		Functions Implementing Command Operations and other common functions

Revision History:

	NEG:27.09.2002	Initiated.
--*/
#define __SW_4

#include "Decoder.h"
#include	"atombios.h"



VOID PutDataRegister(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID PutDataPS(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID PutDataWS(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID PutDataFB(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID PutDataPLL(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID PutDataMC(PARSER_TEMP_DATA STACK_BASED * pParserTempData);

UINT32 GetParametersDirect32(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersDirect16(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersDirect8(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);

UINT32 GetParametersRegister(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersPS(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersWS(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersFB(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersPLL(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersMC(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);

VOID SkipParameters16(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
VOID SkipParameters8(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);

UINT32 GetParametersIndirect(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);
UINT32 GetParametersDirect(PARSER_TEMP_DATA STACK_BASED *	pParserTempData);

UINT16* GetDataMasterTablePointer(DEVICE_DATA STACK_BASED*  pDeviceData);
UINT8 GetTrueIndexInMasterTable(PARSER_TEMP_DATA STACK_BASED * pParserTempData, UINT8 IndexInMasterTable);


WRITE_IO_FUNCTION WritePCIFunctions[8] =   {
    WritePCIReg32,
    WritePCIReg16, WritePCIReg16, WritePCIReg16,
    WritePCIReg8,WritePCIReg8,WritePCIReg8,WritePCIReg8
};
WRITE_IO_FUNCTION WriteIOFunctions[8] =    {
    WriteSysIOReg32,
    WriteSysIOReg16,WriteSysIOReg16,WriteSysIOReg16,
    WriteSysIOReg8,WriteSysIOReg8,WriteSysIOReg8,WriteSysIOReg8
};
READ_IO_FUNCTION ReadPCIFunctions[8] =      {
    (READ_IO_FUNCTION)ReadPCIReg32,
    (READ_IO_FUNCTION)ReadPCIReg16,
    (READ_IO_FUNCTION)ReadPCIReg16,
    (READ_IO_FUNCTION)ReadPCIReg16,
    (READ_IO_FUNCTION)ReadPCIReg8,
    (READ_IO_FUNCTION)ReadPCIReg8,
    (READ_IO_FUNCTION)ReadPCIReg8,
    (READ_IO_FUNCTION)ReadPCIReg8
};
READ_IO_FUNCTION ReadIOFunctions[8] =       {
    (READ_IO_FUNCTION)ReadSysIOReg32,
    (READ_IO_FUNCTION)ReadSysIOReg16,
    (READ_IO_FUNCTION)ReadSysIOReg16,
    (READ_IO_FUNCTION)ReadSysIOReg16,
    (READ_IO_FUNCTION)ReadSysIOReg8,
    (READ_IO_FUNCTION)ReadSysIOReg8,
    (READ_IO_FUNCTION)ReadSysIOReg8,
    (READ_IO_FUNCTION)ReadSysIOReg8
};
READ_IO_FUNCTION GetParametersDirectArray[8]={
    GetParametersDirect32,
    GetParametersDirect16,GetParametersDirect16,GetParametersDirect16,
    GetParametersDirect8,GetParametersDirect8,GetParametersDirect8,
    GetParametersDirect8
};

COMMANDS_DECODER PutDataFunctions[6]   =     {
    PutDataRegister,
    PutDataPS,
    PutDataWS,
    PutDataFB,
    PutDataPLL,
    PutDataMC
};
CD_GET_PARAMETERS GetDestination[6]   =     {
    GetParametersRegister,
    GetParametersPS,
    GetParametersWS,
    GetParametersFB,
    GetParametersPLL,
    GetParametersMC
};

COMMANDS_DECODER SkipDestination[6]   =     {
    SkipParameters16,
    SkipParameters8,
    SkipParameters8,
    SkipParameters8,
    SkipParameters8,
    SkipParameters8
};

CD_GET_PARAMETERS GetSource[8]   =          {
    GetParametersRegister,
    GetParametersPS,
    GetParametersWS,
    GetParametersFB,
    GetParametersIndirect,
    GetParametersDirect,
    GetParametersPLL,
    GetParametersMC
};

UINT32 AlignmentMask[8] =                   {0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFF,0xFF,0xFF,0xFF};
UINT8  SourceAlignmentShift[8] =            {0,0,8,16,0,8,16,24};
UINT8  DestinationAlignmentShift[4] =       {0,8,16,24};

#define INDIRECTIO_ID         1
#define INDIRECTIO_END_OF_ID  9

VOID IndirectIOCommand(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID IndirectIOCommand_MOVE(PARSER_TEMP_DATA STACK_BASED * pParserTempData, UINT32 temp);
VOID IndirectIOCommand_MOVE_INDEX(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID IndirectIOCommand_MOVE_ATTR(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID IndirectIOCommand_MOVE_DATA(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID IndirectIOCommand_SET(PARSER_TEMP_DATA STACK_BASED * pParserTempData);
VOID IndirectIOCommand_CLEAR(PARSER_TEMP_DATA STACK_BASED * pParserTempData);


INDIRECT_IO_PARSER_COMMANDS  IndirectIOParserCommands[10]={
    {IndirectIOCommand,1},
    {IndirectIOCommand,2},
    {ReadIndReg32,3},
    {WriteIndReg32,3},
    {IndirectIOCommand_CLEAR,3},
    {IndirectIOCommand_SET,3},
    {IndirectIOCommand_MOVE_INDEX,4},
    {IndirectIOCommand_MOVE_ATTR,4},
    {IndirectIOCommand_MOVE_DATA,4},
    {IndirectIOCommand,3}
};


VOID IndirectIOCommand(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
}


VOID IndirectIOCommand_MOVE_INDEX(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->IndirectData &= ~((0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1])) << pParserTempData->IndirectIOTablePointer[3]);
    pParserTempData->IndirectData |=(((pParserTempData->Index >> pParserTempData->IndirectIOTablePointer[2]) &
				      (0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1]))) << pParserTempData->IndirectIOTablePointer[3]);
}

VOID IndirectIOCommand_MOVE_ATTR(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->IndirectData &= ~((0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1])) << pParserTempData->IndirectIOTablePointer[3]);
    pParserTempData->IndirectData |=(((pParserTempData->AttributesData >> pParserTempData->IndirectIOTablePointer[2])
				      & (0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1]))) << pParserTempData->IndirectIOTablePointer[3]);
}

VOID IndirectIOCommand_MOVE_DATA(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->IndirectData &= ~((0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1])) << pParserTempData->IndirectIOTablePointer[3]);
    pParserTempData->IndirectData |=(((pParserTempData->DestData32 >> pParserTempData->IndirectIOTablePointer[2])
				      & (0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1]))) << pParserTempData->IndirectIOTablePointer[3]);
}


VOID IndirectIOCommand_SET(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->IndirectData |= ((0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1])) << pParserTempData->IndirectIOTablePointer[2]);
}

VOID IndirectIOCommand_CLEAR(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->IndirectData &= ~((0xFFFFFFFF >> (32-pParserTempData->IndirectIOTablePointer[1])) << pParserTempData->IndirectIOTablePointer[2]);
}


UINT32 IndirectInputOutput(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    // if ((pParserTempData->IndirectData & 0x7f)==INDIRECT_IO_MM) pParserTempData->IndirectData|=pParserTempData->CurrentPortID;
//  pParserTempData->IndirectIOTablePointer=pParserTempData->IndirectIOTable;
    while (*pParserTempData->IndirectIOTablePointer)
    {
	if ((pParserTempData->IndirectIOTablePointer[0] == INDIRECTIO_ID) &&
            (pParserTempData->IndirectIOTablePointer[1] == pParserTempData->IndirectData))
	{
	    pParserTempData->IndirectIOTablePointer+=IndirectIOParserCommands[*pParserTempData->IndirectIOTablePointer].csize;
	    while (*pParserTempData->IndirectIOTablePointer != INDIRECTIO_END_OF_ID)
	    {
		IndirectIOParserCommands[*pParserTempData->IndirectIOTablePointer].func(pParserTempData);
		pParserTempData->IndirectIOTablePointer+=IndirectIOParserCommands[*pParserTempData->IndirectIOTablePointer].csize;
	    }
	    pParserTempData->IndirectIOTablePointer-=*(UINT16*)(pParserTempData->IndirectIOTablePointer+1);
	    pParserTempData->IndirectIOTablePointer++;
	    return pParserTempData->IndirectData;
	} else pParserTempData->IndirectIOTablePointer+=IndirectIOParserCommands[*pParserTempData->IndirectIOTablePointer].csize;
    }
    return 0;
}



VOID PutDataRegister(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Index=(UINT32)pParserTempData->pCmd->Parameters.WordXX.PA_Destination;
    pParserTempData->Index+=pParserTempData->CurrentRegBlock;
    switch(pParserTempData->Multipurpose.CurrentPort){
	case ATI_RegsPort:
	    if (pParserTempData->CurrentPortID == INDIRECT_IO_MM)
	    {
		if (pParserTempData->Index==0) pParserTempData->DestData32 <<= 2;
		WriteReg32( pParserTempData);
	    } else
	    {
		pParserTempData->IndirectData=pParserTempData->CurrentPortID+INDIRECT_IO_WRITE;
		IndirectInputOutput(pParserTempData);
	    }
	    break;
	case PCI_Port:
	    WritePCIFunctions[pParserTempData->pCmd->Header.Attribute.SourceAlignment](pParserTempData);
	    break;
	case SystemIO_Port:
	    WriteIOFunctions[pParserTempData->pCmd->Header.Attribute.SourceAlignment](pParserTempData);
	    break;
    }
}

VOID PutDataPS(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    *(pParserTempData->pDeviceData->pParameterSpace+pParserTempData->pCmd->Parameters.ByteXX.PA_Destination)=
	pParserTempData->DestData32;
}

VOID PutDataWS(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    if (pParserTempData->pCmd->Parameters.ByteXX.PA_Destination < WS_QUOTIENT_C)
	*(pParserTempData->pWorkingTableData->pWorkSpace+pParserTempData->pCmd->Parameters.ByteXX.PA_Destination) = pParserTempData->DestData32;
    else
	switch (pParserTempData->pCmd->Parameters.ByteXX.PA_Destination)
	{
	    case WS_REMINDER_C:
		pParserTempData->MultiplicationOrDivision.Division.Reminder32=pParserTempData->DestData32;
		break;
	    case WS_QUOTIENT_C:
		pParserTempData->MultiplicationOrDivision.Division.Quotient32=pParserTempData->DestData32;
		break;
	    case WS_DATAPTR_C:
#ifndef		UEFI_BUILD
		pParserTempData->CurrentDataBlock=(UINT16)pParserTempData->DestData32;
#else
		pParserTempData->CurrentDataBlock=(UINTN)pParserTempData->DestData32;
#endif
		break;
	    case WS_SHIFT_C:
		pParserTempData->Shift2MaskConverter=(UINT8)pParserTempData->DestData32;
		break;
	    case WS_FB_WINDOW_C:
		pParserTempData->CurrentFB_Window=pParserTempData->DestData32;
		break;
	    case WS_ATTRIBUTES_C:
		pParserTempData->AttributesData=(UINT16)pParserTempData->DestData32;
		break;
	    case WS_REGPTR_C:
		pParserTempData->CurrentRegBlock=(UINT16)pParserTempData->DestData32;
		break;
	}

}

VOID PutDataFB(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Index=(UINT32)pParserTempData->pCmd->Parameters.ByteXX.PA_Destination;
    //Make an Index from address first, then add to the Index
    pParserTempData->Index+=(pParserTempData->CurrentFB_Window>>2);
    WriteFrameBuffer32(pParserTempData);
}

VOID PutDataPLL(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Index=(UINT32)pParserTempData->pCmd->Parameters.ByteXX.PA_Destination;
    WritePLL32( pParserTempData );
}

VOID PutDataMC(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Index=(UINT32)pParserTempData->pCmd->Parameters.ByteXX.PA_Destination;
    WriteMC32( pParserTempData );
}


VOID SkipParameters8(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
}

VOID SkipParameters16(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT16);
}


UINT32 GetParametersRegister(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*(UINT16*)pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT16);
    pParserTempData->Index+=pParserTempData->CurrentRegBlock;
    switch(pParserTempData->Multipurpose.CurrentPort)
    {
	case PCI_Port:
	    return ReadPCIFunctions[pParserTempData->pCmd->Header.Attribute.SourceAlignment](pParserTempData);
	case SystemIO_Port:
	    return ReadIOFunctions[pParserTempData->pCmd->Header.Attribute.SourceAlignment](pParserTempData);
	case ATI_RegsPort:
	default:
	    if (pParserTempData->CurrentPortID == INDIRECT_IO_MM) return ReadReg32( pParserTempData );
	    else
	    {
		pParserTempData->IndirectData=pParserTempData->CurrentPortID+INDIRECT_IO_READ;
		return IndirectInputOutput(pParserTempData);
	    }
    }
}

UINT32 GetParametersPS(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    return *(pParserTempData->pDeviceData->pParameterSpace+pParserTempData->Index);
}

UINT32 GetParametersWS(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    if (pParserTempData->Index < WS_QUOTIENT_C)
	return *(pParserTempData->pWorkingTableData->pWorkSpace+pParserTempData->Index);
    else
	switch (pParserTempData->Index)
	{
	    case WS_REMINDER_C:
		return pParserTempData->MultiplicationOrDivision.Division.Reminder32;
	    case WS_QUOTIENT_C:
		return pParserTempData->MultiplicationOrDivision.Division.Quotient32;
	    case WS_DATAPTR_C:
		return (UINT32)pParserTempData->CurrentDataBlock;
	    case WS_OR_MASK_C:
		return ((UINT32)1) << pParserTempData->Shift2MaskConverter;
	    case WS_AND_MASK_C:
		return ~(((UINT32)1) << pParserTempData->Shift2MaskConverter);
	    case WS_FB_WINDOW_C:
		return pParserTempData->CurrentFB_Window;
	    case WS_ATTRIBUTES_C:
		return pParserTempData->AttributesData;
	    case WS_REGPTR_C:
		return (UINT32)pParserTempData->CurrentRegBlock;
	}
    return 0;

}

UINT32 GetParametersFB(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    pParserTempData->Index+=(pParserTempData->CurrentFB_Window>>2);
    return ReadFrameBuffer32(pParserTempData);
}

UINT32 GetParametersPLL(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    return ReadPLL32( pParserTempData );
}

UINT32 GetParametersMC(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    return ReadMC32( pParserTempData );
}


UINT32 GetParametersIndirect(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->Index=*(UINT16*)pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT16);
    return *(UINT32*)(RELATIVE_TO_BIOS_IMAGE(pParserTempData->Index)+pParserTempData->CurrentDataBlock);
}

UINT32 GetParametersDirect8(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->CD_Mask.SrcAlignment=alignmentByte0;
    pParserTempData->Index=*(UINT8*)pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT8);
    return pParserTempData->Index;
}

UINT32 GetParametersDirect16(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->CD_Mask.SrcAlignment=alignmentLowerWord;
    pParserTempData->Index=*(UINT16*)pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT16);
    return pParserTempData->Index;
}

UINT32 GetParametersDirect32(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    pParserTempData->CD_Mask.SrcAlignment=alignmentDword;
    pParserTempData->Index=*(UINT32*)pParserTempData->pWorkingTableData->IP;
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT32);
    return pParserTempData->Index;
}


UINT32 GetParametersDirect(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
    return GetParametersDirectArray[pParserTempData->pCmd->Header.Attribute.SourceAlignment](pParserTempData);
}


VOID CommonSourceDataTransformation(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->SourceData32 >>= SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 &=  AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 <<= DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment];
}

VOID CommonOperationDataTransformation(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->SourceData32 >>= SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 &= AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->DestData32   >>= DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment];
    pParserTempData->DestData32   &= AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
}

VOID ProcessMove(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    if (pParserTempData->CD_Mask.SrcAlignment!=alignmentDword)
    {
	pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    } else
    {
	SkipDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    }
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);

    if (pParserTempData->CD_Mask.SrcAlignment!=alignmentDword)
    {
	pParserTempData->DestData32 &= ~(AlignmentMask[pParserTempData->CD_Mask.SrcAlignment] << DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment]);
	CommonSourceDataTransformation(pParserTempData);
	pParserTempData->DestData32 |= pParserTempData->SourceData32;
    } else
    {
	pParserTempData->DestData32=pParserTempData->SourceData32;
    }
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessMask(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{

    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetParametersDirect(pParserTempData);
    pParserTempData->Index=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    pParserTempData->SourceData32 <<= DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment];
    pParserTempData->SourceData32 |= ~(AlignmentMask[pParserTempData->CD_Mask.SrcAlignment] << DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment]);
    pParserTempData->DestData32   &= pParserTempData->SourceData32;
    pParserTempData->Index        &= AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->Index        <<= DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment];
    pParserTempData->DestData32   |= pParserTempData->Index;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessAnd(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    pParserTempData->SourceData32 >>= SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 <<= DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment];
    pParserTempData->SourceData32 |= ~(AlignmentMask[pParserTempData->CD_Mask.SrcAlignment] << DestinationAlignmentShift[pParserTempData->CD_Mask.DestAlignment]);
    pParserTempData->DestData32   &= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessOr(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 |= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessXor(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 ^= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessShl(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 <<= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessShr(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 >>= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}


VOID ProcessADD(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 += pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessSUB(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonSourceDataTransformation(pParserTempData);
    pParserTempData->DestData32 -= pParserTempData->SourceData32;
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessMUL(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonOperationDataTransformation(pParserTempData);
    pParserTempData->MultiplicationOrDivision.Multiplication.Low32Bit=pParserTempData->DestData32 * pParserTempData->SourceData32;
}

VOID ProcessDIV(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);

    CommonOperationDataTransformation(pParserTempData);
    pParserTempData->MultiplicationOrDivision.Division.Quotient32=
	pParserTempData->DestData32 / pParserTempData->SourceData32;
    pParserTempData->MultiplicationOrDivision.Division.Reminder32=
	pParserTempData->DestData32 % pParserTempData->SourceData32;
}


VOID ProcessCompare(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);

    CommonOperationDataTransformation(pParserTempData);

    // Here we just set flags based on evaluation
    if (pParserTempData->DestData32==pParserTempData->SourceData32)
	pParserTempData->CompareFlags = Equal;
    else
	pParserTempData->CompareFlags =
	    (UINT8)((pParserTempData->DestData32<pParserTempData->SourceData32) ? Below : Above);

}

VOID ProcessClear(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->DestData32 &= ~(AlignmentMask[pParserTempData->CD_Mask.SrcAlignment] << SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment]);
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);

}

VOID ProcessShift(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    UINT32 mask = AlignmentMask[pParserTempData->CD_Mask.SrcAlignment] << SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetParametersDirect8(pParserTempData);

    // save original value of the destination
    pParserTempData->Index = pParserTempData->DestData32 & ~mask;
    pParserTempData->DestData32 &= mask;

    if (pParserTempData->pCmd->Header.Opcode < SHIFT_RIGHT_REG_OPCODE)
	pParserTempData->DestData32 <<= pParserTempData->SourceData32; else
	    pParserTempData->DestData32 >>= pParserTempData->SourceData32;

    // Clear any bits shifted out of masked area...
    pParserTempData->DestData32 &= mask;
    // ... and restore the area outside of masked with original values
    pParserTempData->DestData32 |= pParserTempData->Index;

    // write data back
    PutDataFunctions[pParserTempData->ParametersType.Destination](pParserTempData);
}

VOID ProcessTest(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->DestData32=GetDestination[pParserTempData->ParametersType.Destination](pParserTempData);
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    CommonOperationDataTransformation(pParserTempData);
    pParserTempData->CompareFlags =
	(UINT8)((pParserTempData->DestData32 & pParserTempData->SourceData32) ? NotEqual : Equal);

}

VOID ProcessSetFB_Base(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    pParserTempData->SourceData32 >>= SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 &= AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->CurrentFB_Window=pParserTempData->SourceData32;
}

VOID ProcessSwitch(PARSER_TEMP_DATA STACK_BASED * pParserTempData){
    pParserTempData->SourceData32=GetSource[pParserTempData->ParametersType.Source](pParserTempData);
    pParserTempData->SourceData32 >>= SourceAlignmentShift[pParserTempData->CD_Mask.SrcAlignment];
    pParserTempData->SourceData32 &= AlignmentMask[pParserTempData->CD_Mask.SrcAlignment];
    while ( *(UINT16*)pParserTempData->pWorkingTableData->IP != (((UINT16)NOP_OPCODE << 8)+NOP_OPCODE))
    {
	if (*pParserTempData->pWorkingTableData->IP == 'c')
	{
	    pParserTempData->pWorkingTableData->IP++;
	    pParserTempData->DestData32=GetParametersDirect(pParserTempData);
	    pParserTempData->Index=GetParametersDirect16(pParserTempData);
	    if (pParserTempData->SourceData32 == pParserTempData->DestData32)
	    {
		pParserTempData->pWorkingTableData->IP= RELATIVE_TO_TABLE(pParserTempData->Index);
		return;
	    }
	}
    }
    pParserTempData->pWorkingTableData->IP+=sizeof(UINT16);
}


VOID	cmdSetDataBlock(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    UINT8 value;
    UINT16* pMasterDataTable;
    value=((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.ByteXX.PA_Destination;
    if (value == 0) pParserTempData->CurrentDataBlock=0; else
    {
	if (value == DB_CURRENT_COMMAND_TABLE)
        {
	    pParserTempData->CurrentDataBlock= (UINT16)(pParserTempData->pWorkingTableData->pTableHead-pParserTempData->pDeviceData->pBIOS_Image);
        } else
	{
	    pMasterDataTable = GetDataMasterTablePointer(pParserTempData->pDeviceData);
	    pParserTempData->CurrentDataBlock= (TABLE_UNIT_TYPE)((PTABLE_UNIT_TYPE)pMasterDataTable)[value];
	}
    }
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
}

VOID	cmdSet_ATI_Port(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Multipurpose.CurrentPort=ATI_RegsPort;
    pParserTempData->CurrentPortID = (UINT8)((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.WordXX.PA_Destination;
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
}

VOID	cmdSet_Reg_Block(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->CurrentRegBlock = ((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.WordXX.PA_Destination;
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
}


//Atavism!!! Review!!!
VOID	cmdSet_X_Port(PARSER_TEMP_DATA STACK_BASED * pParserTempData){
    pParserTempData->Multipurpose.CurrentPort=pParserTempData->ParametersType.Destination;
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_ONLY);

}

VOID	cmdDelay_Millisec(PARSER_TEMP_DATA STACK_BASED * pParserTempData){
    pParserTempData->SourceData32 =
	((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.ByteXX.PA_Destination;
    DelayMilliseconds(pParserTempData);
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
}
VOID	cmdDelay_Microsec(PARSER_TEMP_DATA STACK_BASED * pParserTempData){
    pParserTempData->SourceData32 =
	((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.ByteXX.PA_Destination;
    DelayMicroseconds(pParserTempData);
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
}

VOID ProcessPostChar(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->SourceData32 =
	((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.ByteXX.PA_Destination;
    PostCharOutput(pParserTempData);
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
}

VOID ProcessDebug(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->SourceData32 =
	((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.ByteXX.PA_Destination;
    CallerDebugFunc(pParserTempData);
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
}


VOID ProcessDS(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->pWorkingTableData->IP+=((COMMAND_TYPE_1*)pParserTempData->pWorkingTableData->IP)->Parameters.WordXX.PA_Destination+sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
}


VOID	cmdCall_Table(PARSER_TEMP_DATA STACK_BASED * pParserTempData){
    UINT16*	MasterTableOffset;
    pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_VALUE_BYTE);
    MasterTableOffset = GetCommandMasterTablePointer(pParserTempData->pDeviceData);
    if(((PTABLE_UNIT_TYPE)MasterTableOffset)[((COMMAND_TYPE_OPCODE_VALUE_BYTE*)pParserTempData->pCmd)->Value]!=0 )  // if the offset is not ZERO
    {
	pParserTempData->CommandSpecific.IndexInMasterTable=GetTrueIndexInMasterTable(pParserTempData,((COMMAND_TYPE_OPCODE_VALUE_BYTE*)pParserTempData->pCmd)->Value);
	pParserTempData->Multipurpose.PS_SizeInDwordsUsedByCallingTable =
	    (((ATOM_COMMON_ROM_COMMAND_TABLE_HEADER *)pParserTempData->pWorkingTableData->pTableHead)->TableAttribute.PS_SizeInBytes>>2);
	pParserTempData->pDeviceData->pParameterSpace+=
	    pParserTempData->Multipurpose.PS_SizeInDwordsUsedByCallingTable;
	pParserTempData->Status=CD_CALL_TABLE;
	pParserTempData->pCmd=(GENERIC_ATTRIBUTE_COMMAND*)MasterTableOffset;
    }
}


VOID	cmdNOP_(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
}


static VOID NotImplemented(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    pParserTempData->Status = CD_NOT_IMPLEMENTED;
}


VOID ProcessJump(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    if ((pParserTempData->ParametersType.Destination == NoCondition) ||
	(pParserTempData->ParametersType.Destination == pParserTempData->CompareFlags ))
    {

	pParserTempData->pWorkingTableData->IP= RELATIVE_TO_TABLE(((COMMAND_TYPE_OPCODE_OFFSET16*)pParserTempData->pWorkingTableData->IP)->CD_Offset16);
    } else
    {
	pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
    }
}

VOID ProcessJumpE(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    if ((pParserTempData->CompareFlags == Equal) ||
	(pParserTempData->CompareFlags == pParserTempData->ParametersType.Destination))
    {

	pParserTempData->pWorkingTableData->IP= RELATIVE_TO_TABLE(((COMMAND_TYPE_OPCODE_OFFSET16*)pParserTempData->pWorkingTableData->IP)->CD_Offset16);
    } else
    {
	pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
    }
}

VOID ProcessJumpNE(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
    if (pParserTempData->CompareFlags != Equal)
    {

	pParserTempData->pWorkingTableData->IP= RELATIVE_TO_TABLE(((COMMAND_TYPE_OPCODE_OFFSET16*)pParserTempData->pWorkingTableData->IP)->CD_Offset16);
    } else
    {
	pParserTempData->pWorkingTableData->IP+=sizeof(COMMAND_TYPE_OPCODE_OFFSET16);
    }
}



COMMANDS_PROPERTIES CallTable[] =
{
    { NULL, 0,0},
    { ProcessMove,      destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessMove,      destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessMove,      destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessMove,      destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessMove,      destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessMove,      destMC,           sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessAnd,       destMC,           sizeof(COMMAND_HEADER)},
    { ProcessOr,        destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessOr,        destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessOr,        destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessOr,        destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessOr,        destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessOr,        destMC,           sizeof(COMMAND_HEADER)},
    { ProcessShift,     destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessShift,     destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessShift,     destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessShift,     destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessShift,     destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessShift,     destMC,           sizeof(COMMAND_HEADER)},
    { ProcessShift,     destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessShift,     destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessShift,     destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessShift,     destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessShift,     destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessShift,     destMC,           sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessMUL,       destMC,           sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessDIV,       destMC,           sizeof(COMMAND_HEADER)},
    { ProcessADD,       destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessADD,       destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessADD,       destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessADD,       destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessADD,       destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessADD,       destMC,           sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessSUB,       destMC,           sizeof(COMMAND_HEADER)},
    { cmdSet_ATI_Port,  ATI_RegsPort,     0},
    { cmdSet_X_Port,    PCI_Port,         0},
    { cmdSet_X_Port,    SystemIO_Port,    0},
    { cmdSet_Reg_Block,	0,                0},
    { ProcessSetFB_Base,0,                sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessCompare,   destMC,           sizeof(COMMAND_HEADER)},
    { ProcessSwitch,    0,              	sizeof(COMMAND_HEADER)},
    { ProcessJump,			NoCondition,      0},
    { ProcessJump,	    Equal,            0},
    { ProcessJump,      Below,	          0},
    { ProcessJump,      Above,	          0},
    { ProcessJumpE,     Below,            0},
    { ProcessJumpE,     Above,            0},
    { ProcessJumpNE,		0,                0},
    { ProcessTest,      destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessTest,      destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessTest,      destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessTest,      destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessTest,      destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessTest,      destMC,           sizeof(COMMAND_HEADER)},
    { cmdDelay_Millisec,0,                0},
    { cmdDelay_Microsec,0,                0},
    { cmdCall_Table,		0,                0},
    /*cmdRepeat*/	    { NotImplemented,   0,                0},
    { ProcessClear,     destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessClear,     destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessClear,     destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessClear,     destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessClear,     destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessClear,     destMC,           sizeof(COMMAND_HEADER)},
    { cmdNOP_,		      0,                sizeof(COMMAND_TYPE_OPCODE_ONLY)},
    /*cmdEOT*/        { cmdNOP_,		      0,                sizeof(COMMAND_TYPE_OPCODE_ONLY)},
    { ProcessMask,      destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessMask,      destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessMask,      destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessMask,      destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessMask,      destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessMask,      destMC,           sizeof(COMMAND_HEADER)},
    /*cmdPost_Card*/	{ ProcessPostChar,  0,                0},
    /*cmdBeep*/		    { NotImplemented,   0,                0},
    /*cmdSave_Reg*/	  { NotImplemented,   0,                0},
    /*cmdRestore_Reg*/{ NotImplemented,   0,                0},
    { cmdSetDataBlock,  0,                0},
    { ProcessXor,        destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessXor,        destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessXor,        destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessXor,        destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessXor,        destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessXor,        destMC,           sizeof(COMMAND_HEADER)},

    { ProcessShl,        destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessShl,        destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessShl,        destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessShl,        destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessShl,        destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessShl,        destMC,           sizeof(COMMAND_HEADER)},

    { ProcessShr,        destRegister,     sizeof(COMMAND_HEADER)},
    { ProcessShr,        destParamSpace,   sizeof(COMMAND_HEADER)},
    { ProcessShr,        destWorkSpace,    sizeof(COMMAND_HEADER)},
    { ProcessShr,        destFrameBuffer,  sizeof(COMMAND_HEADER)},
    { ProcessShr,        destPLL,          sizeof(COMMAND_HEADER)},
    { ProcessShr,        destMC,           sizeof(COMMAND_HEADER)},
    /*cmdDebug*/		{ ProcessDebug,  0,                0},
    { ProcessDS,  0,                0},

};

// EOF
