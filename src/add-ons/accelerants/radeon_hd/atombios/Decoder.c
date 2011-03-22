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

    Decoder.c
    
Abstract:

		Commands Decoder

Revision History:

	NEG:24.09.2002	Initiated.
--*/
//#include "AtomBios.h"
#include "Decoder.h"
#include "atombios.h"
#include "CD_binding.h"
#include "CD_Common_Types.h"

#ifndef DISABLE_EASF
	#include "easf.h"
#endif



#define INDIRECT_IO_TABLE (((UINT16)&((ATOM_MASTER_LIST_OF_DATA_TABLES*)0)->IndirectIOAccess)/sizeof(TABLE_UNIT_TYPE) )
extern COMMANDS_PROPERTIES CallTable[];


UINT8 ProcessCommandProperties(PARSER_TEMP_DATA STACK_BASED *	pParserTempData)
{
  UINT8 opcode=((COMMAND_HEADER*)pParserTempData->pWorkingTableData->IP)->Opcode;
  pParserTempData->pWorkingTableData->IP+=CallTable[opcode].headersize;
  pParserTempData->ParametersType.Destination=CallTable[opcode].destination;
  pParserTempData->ParametersType.Source = pParserTempData->pCmd->Header.Attribute.Source;
  pParserTempData->CD_Mask.SrcAlignment=pParserTempData->pCmd->Header.Attribute.SourceAlignment;
  pParserTempData->CD_Mask.DestAlignment=pParserTempData->pCmd->Header.Attribute.DestinationAlignment;
  return opcode;
}

UINT16* GetCommandMasterTablePointer(DEVICE_DATA STACK_BASED*  pDeviceData)
{
	UINT16		*MasterTableOffset;
#ifndef DISABLE_EASF
	if (pDeviceData->format == TABLE_FORMAT_EASF)
	{
    /*
    make MasterTableOffset point to EASF_ASIC_SETUP_TABLE structure, including usSize.
    */
		MasterTableOffset = (UINT16 *) (pDeviceData->pBIOS_Image+((EASF_ASIC_DESCRIPTOR*)pDeviceData->pBIOS_Image)->usAsicSetupTable_Offset);
	} else
#endif
	{
#ifndef		UEFI_BUILD
		MasterTableOffset = (UINT16 *)(*(UINT16 *)(pDeviceData->pBIOS_Image+OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER) + pDeviceData->pBIOS_Image);
		MasterTableOffset = (UINT16 *)((ULONG)((ATOM_ROM_HEADER *)MasterTableOffset)->usMasterCommandTableOffset + pDeviceData->pBIOS_Image );
		MasterTableOffset =(UINT16 *) &(((ATOM_MASTER_COMMAND_TABLE *)MasterTableOffset)->ListOfCommandTables);
#else
	MasterTableOffset = (UINT16 *)(&(GetCommandMasterTable( )->ListOfCommandTables));
#endif
	}
	return MasterTableOffset;
}

UINT16* GetDataMasterTablePointer(DEVICE_DATA STACK_BASED*  pDeviceData)
{
	UINT16		*MasterTableOffset;
	
#ifndef		UEFI_BUILD
	MasterTableOffset = (UINT16 *)(*(UINT16 *)(pDeviceData->pBIOS_Image+OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER) + pDeviceData->pBIOS_Image);
	MasterTableOffset = (UINT16 *)((ULONG)((ATOM_ROM_HEADER *)MasterTableOffset)->usMasterDataTableOffset + pDeviceData->pBIOS_Image );
	MasterTableOffset =(UINT16 *) &(((ATOM_MASTER_DATA_TABLE *)MasterTableOffset)->ListOfDataTables);
#else
	MasterTableOffset = (UINT16 *)(&(GetDataMasterTable( )->ListOfDataTables));
#endif
	return MasterTableOffset;
}


UINT8 GetTrueIndexInMasterTable(PARSER_TEMP_DATA STACK_BASED * pParserTempData, UINT8 IndexInMasterTable)
{
#ifndef DISABLE_EASF
	UINT16 i;
	if ( pParserTempData->pDeviceData->format == TABLE_FORMAT_EASF)
	{
/*
		Consider EASF_ASIC_SETUP_TABLE structure pointed by pParserTempData->pCmd as UINT16[]
		((UINT16*)pParserTempData->pCmd)[0] = EASF_ASIC_SETUP_TABLE.usSize;
		((UINT16*)pParserTempData->pCmd)[1+n*4] = usFunctionID;
		usFunctionID has to be shifted left by 2 before compare it to the value provided by caller.
*/
		for (i=1; (i < ((UINT16*)pParserTempData->pCmd)[0] >> 1);i+=4)
	  		if ((UINT8)(((UINT16*)pParserTempData->pCmd)[i] << 2)==(IndexInMasterTable & EASF_TABLE_INDEX_MASK)) return (i+1+(IndexInMasterTable & EASF_TABLE_ATTR_MASK));
		return 1;
	} else
#endif
	{
		return IndexInMasterTable;
	}
}

CD_STATUS ParseTable(DEVICE_DATA STACK_BASED* pDeviceData, UINT8 IndexInMasterTable)
{
	PARSER_TEMP_DATA	ParserTempData;
  WORKING_TABLE_DATA STACK_BASED* prevWorkingTableData;

  ParserTempData.pDeviceData=(DEVICE_DATA*)pDeviceData;
#ifndef DISABLE_EASF
  if (pDeviceData->format == TABLE_FORMAT_EASF)
  {
      ParserTempData.IndirectIOTablePointer = 0;
  } else
#endif
  {
    ParserTempData.pCmd=(GENERIC_ATTRIBUTE_COMMAND*)GetDataMasterTablePointer(pDeviceData);
    ParserTempData.IndirectIOTablePointer=(UINT8*)((ULONG)(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[INDIRECT_IO_TABLE]) + pDeviceData->pBIOS_Image);
    ParserTempData.IndirectIOTablePointer+=sizeof(ATOM_COMMON_TABLE_HEADER);
  }

	ParserTempData.pCmd=(GENERIC_ATTRIBUTE_COMMAND*)GetCommandMasterTablePointer(pDeviceData);
    IndexInMasterTable=GetTrueIndexInMasterTable((PARSER_TEMP_DATA STACK_BASED *)&ParserTempData,IndexInMasterTable);
	if(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]!=0 )  // if the offset is not ZERO
	{
		ParserTempData.CommandSpecific.IndexInMasterTable=IndexInMasterTable;
		ParserTempData.Multipurpose.CurrentPort=ATI_RegsPort;
		ParserTempData.CurrentPortID=INDIRECT_IO_MM;
		ParserTempData.CurrentRegBlock=0;
		ParserTempData.CurrentFB_Window=0;
    prevWorkingTableData=NULL;
		ParserTempData.Status=CD_CALL_TABLE;

		do{

			if (ParserTempData.Status==CD_CALL_TABLE)
      {
				IndexInMasterTable=ParserTempData.CommandSpecific.IndexInMasterTable;
				if(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]!=0)  // if the offset is not ZERO
					{
#ifndef		UEFI_BUILD
  					ParserTempData.pWorkingTableData =(WORKING_TABLE_DATA STACK_BASED*) AllocateWorkSpace(pDeviceData,
								((ATOM_COMMON_ROM_COMMAND_TABLE_HEADER*)(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]+pDeviceData->pBIOS_Image))->TableAttribute.WS_SizeInBytes+sizeof(WORKING_TABLE_DATA));
#else
  					ParserTempData.pWorkingTableData =(WORKING_TABLE_DATA STACK_BASED*) AllocateWorkSpace(pDeviceData,
								((ATOM_COMMON_ROM_COMMAND_TABLE_HEADER*)(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]))->TableAttribute.WS_SizeInBytes+sizeof(WORKING_TABLE_DATA));
#endif
            if (ParserTempData.pWorkingTableData!=NULL)
            {
						  ParserTempData.pWorkingTableData->pWorkSpace=(WORKSPACE_POINTER STACK_BASED*)((UINT8*)ParserTempData.pWorkingTableData+sizeof(WORKING_TABLE_DATA));
#ifndef		UEFI_BUILD
						  ParserTempData.pWorkingTableData->pTableHead  = (UINT8 *)(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]+pDeviceData->pBIOS_Image);
#else
						  ParserTempData.pWorkingTableData->pTableHead  = (UINT8 *)(((PTABLE_UNIT_TYPE)ParserTempData.pCmd)[IndexInMasterTable]);
#endif
	 					  ParserTempData.pWorkingTableData->IP=((UINT8*)ParserTempData.pWorkingTableData->pTableHead)+sizeof(ATOM_COMMON_ROM_COMMAND_TABLE_HEADER);
              ParserTempData.pWorkingTableData->prevWorkingTableData=prevWorkingTableData;
              prevWorkingTableData=ParserTempData.pWorkingTableData;
              ParserTempData.Status = CD_SUCCESS;
            } else ParserTempData.Status = CD_UNEXPECTED_BEHAVIOR;
					} else ParserTempData.Status = CD_EXEC_TABLE_NOT_FOUND;
			}
			if (!CD_ERROR(ParserTempData.Status))
			{
        ParserTempData.Status = CD_SUCCESS;
				while (!CD_ERROR_OR_COMPLETED(ParserTempData.Status))  
        {

					if (IS_COMMAND_VALID(((COMMAND_HEADER*)ParserTempData.pWorkingTableData->IP)->Opcode))
          {
						ParserTempData.pCmd = (GENERIC_ATTRIBUTE_COMMAND*)ParserTempData.pWorkingTableData->IP;

						if (IS_END_OF_TABLE(((COMMAND_HEADER*)ParserTempData.pWorkingTableData->IP)->Opcode))
						{
							ParserTempData.Status=CD_COMPLETED;
              prevWorkingTableData=ParserTempData.pWorkingTableData->prevWorkingTableData;

							FreeWorkSpace(pDeviceData, ParserTempData.pWorkingTableData);
              ParserTempData.pWorkingTableData=prevWorkingTableData;
              if (prevWorkingTableData!=NULL)
              {
							  ParserTempData.pDeviceData->pParameterSpace-=
								  		(((ATOM_COMMON_ROM_COMMAND_TABLE_HEADER*)ParserTempData.pWorkingTableData->
									  		pTableHead)->TableAttribute.PS_SizeInBytes>>2);
              } 
						// if there is a parent table where to return, then restore PS_pointer to the original state
						}
						else
						{
              IndexInMasterTable=ProcessCommandProperties((PARSER_TEMP_DATA STACK_BASED *)&ParserTempData);
							(*CallTable[IndexInMasterTable].function)((PARSER_TEMP_DATA STACK_BASED *)&ParserTempData);
#if (PARSER_TYPE!=DRIVER_TYPE_PARSER)
              BIOS_STACK_MODIFIER();
#endif
						}
					}
					else
					{
						ParserTempData.Status=CD_INVALID_OPCODE;
						break;
					}

				}	// while
			}	// if
			else
				break;
		} while (prevWorkingTableData!=NULL);
    if (ParserTempData.Status == CD_COMPLETED) return CD_SUCCESS;
		return ParserTempData.Status;
	} else return CD_SUCCESS;
}

// EOF

