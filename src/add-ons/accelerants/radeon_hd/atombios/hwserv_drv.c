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

    hwserv_drv.c
    
Abstract:

		Functions defined in the Command Decoder Specification document

Revision History:

	NEG:27.09.2002	Initiated.
--*/
#include "CD_binding.h"
#include "CD_hw_services.h"

//trace settings
#if DEBUG_OUTPUT_DEVICE & 1
	#define TRACE_USING_STDERR          //define it to use stderr as trace output,
#endif
#if DEBUG_OUTPUT_DEVICE & 2
	#define TRACE_USING_RS232
#endif
#if DEBUG_OUTPUT_DEVICE & 4
	#define TRACE_USING_LPT
#endif


#if DEBUG_PARSER == 4
	#define IO_TRACE					//IO access trace switch, undefine it to turn off
	#define PCI_TRACE					//PCI access trace switch, undefine it to turn off
	#define MEM_TRACE					//MEM access trace switch, undefine it to turn off
#endif

UINT32 CailReadATIRegister(VOID*,UINT32);
VOID   CailWriteATIRegister(VOID*,UINT32,UINT32);
VOID*  CailAllocateMemory(VOID*,UINT16);
VOID   CailReleaseMemory(VOID *,VOID *);
VOID   CailDelayMicroSeconds(VOID *,UINT32 );
VOID   CailReadPCIConfigData(VOID*,VOID*,UINT32,UINT16);
VOID   CailWritePCIConfigData(VOID*,VOID*,UINT32,UINT16);
UINT32 CailReadFBData(VOID*,UINT32);
VOID   CailWriteFBData(VOID*,UINT32,UINT32);
ULONG  CailReadPLL(VOID *Context ,ULONG Address);
VOID   CailWritePLL(VOID *Context,ULONG Address,ULONG Data);
ULONG  CailReadMC(VOID *Context ,ULONG Address);
VOID   CailWriteMC(VOID *Context ,ULONG Address,ULONG Data);


#if DEBUG_PARSER>0
VOID   CailVideoDebugPrint(VOID*,ULONG_PTR, UINT16);
#endif
// Delay function
#if ( defined ENABLE_PARSER_DELAY || defined ENABLE_ALL_SERVICE_FUNCTIONS )

VOID	DelayMilliseconds(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
	    CailDelayMicroSeconds(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->SourceData32*1000);
}

VOID	DelayMicroseconds(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
	    CailDelayMicroSeconds(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->SourceData32);
}
#endif

VOID	PostCharOutput(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
}

VOID	CallerDebugFunc(PARSER_TEMP_DATA STACK_BASED * pParserTempData)
{
}


// PCI READ Access

#if ( defined ENABLE_PARSER_PCIREAD8 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT8   ReadPCIReg8(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    UINT8 rvl;
    CailReadPCIConfigData(pWorkingTableData->pDeviceData->CAIL,&rvl,pWorkingTableData->Index,sizeof(UINT8));
	return rvl;
}
#endif


#if ( defined ENABLE_PARSER_PCIREAD16 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT16	ReadPCIReg16(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    UINT16 rvl;
    CailReadPCIConfigData(pWorkingTableData->pDeviceData->CAIL,&rvl,pWorkingTableData->Index,sizeof(UINT16));
    return rvl;

}
#endif



#if ( defined ENABLE_PARSER_PCIREAD32 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT32  ReadPCIReg32   (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    UINT32 rvl;
    CailReadPCIConfigData(pWorkingTableData->pDeviceData->CAIL,&rvl,pWorkingTableData->Index,sizeof(UINT32));
    return rvl;
}
#endif


// PCI WRITE Access

#if ( defined ENABLE_PARSER_PCIWRITE8 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID	WritePCIReg8	(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    CailWritePCIConfigData(pWorkingTableData->pDeviceData->CAIL,&(pWorkingTableData->DestData32),pWorkingTableData->Index,sizeof(UINT8));

}

#endif


#if ( defined ENABLE_PARSER_PCIWRITE16 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID    WritePCIReg16  (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

        CailWritePCIConfigData(pWorkingTableData->pDeviceData->CAIL,&(pWorkingTableData->DestData32),pWorkingTableData->Index,sizeof(UINT16));
}

#endif


#if ( defined ENABLE_PARSER_PCIWRITE32 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID    WritePCIReg32  (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWritePCIConfigData(pWorkingTableData->pDeviceData->CAIL,&(pWorkingTableData->DestData32),pWorkingTableData->Index,sizeof(UINT32));
}
#endif




// System IO Access
#if ( defined ENABLE_PARSER_SYS_IOREAD8 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT8   ReadSysIOReg8    (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    UINT8 rvl;
    rvl=0;
    //rvl= (UINT8) ReadGenericPciCfg(dev,reg,sizeof(UINT8));
	return rvl;
}
#endif


#if ( defined ENABLE_PARSER_SYS_IOREAD16 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT16	ReadSysIOReg16(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    UINT16 rvl;
    rvl=0;
    //rvl= (UINT16) ReadGenericPciCfg(dev,reg,sizeof(UINT16));
    return rvl;

}
#endif



#if ( defined ENABLE_PARSER_SYS_IOREAD32 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT32  ReadSysIOReg32   (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    UINT32 rvl;
    rvl=0;
    //rvl= (UINT32) ReadGenericPciCfg(dev,reg,sizeof(UINT32));
    return rvl;
}
#endif


// PCI WRITE Access

#if ( defined ENABLE_PARSER_SYS_IOWRITE8 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID	WriteSysIOReg8	(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    //WriteGenericPciCfg(dev,reg,sizeof(UINT8),(UINT32)value);
}

#endif


#if ( defined ENABLE_PARSER_SYS_IOWRITE16 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID    WriteSysIOReg16  (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{

    //WriteGenericPciCfg(dev,reg,sizeof(UINT16),(UINT32)value);
}

#endif


#if ( defined ENABLE_PARSER_SYS_IOWRITE32 || defined ENABLE_ALL_SERVICE_FUNCTIONS )
VOID    WriteSysIOReg32  (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    //WriteGenericPciCfg(dev,reg,sizeof(UINT32),(UINT32)value);
}
#endif

// ATI Registers Memory Mapped Access

#if ( defined ENABLE_PARSER_REGISTERS_MEMORY_ACCESS || defined ENABLE_ALL_SERVICE_FUNCTIONS)

UINT32	ReadReg32 (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    return CailReadATIRegister(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index);
}

VOID	WriteReg32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWriteATIRegister(pWorkingTableData->pDeviceData->CAIL,(UINT16)pWorkingTableData->Index,pWorkingTableData->DestData32 );
}


VOID	ReadIndReg32 (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    pWorkingTableData->IndirectData = CailReadATIRegister(pWorkingTableData->pDeviceData->CAIL,*(UINT16*)(pWorkingTableData->IndirectIOTablePointer+1));
}

VOID	WriteIndReg32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWriteATIRegister(pWorkingTableData->pDeviceData->CAIL,*(UINT16*)(pWorkingTableData->IndirectIOTablePointer+1),pWorkingTableData->IndirectData );
}

#endif

// ATI Registers IO Mapped Access

#if ( defined ENABLE_PARSER_REGISTERS_IO_ACCESS || defined ENABLE_ALL_SERVICE_FUNCTIONS )
UINT32	ReadRegIO (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    //return CailReadATIRegister(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index);
    return 0;
}
VOID	WriteRegIO(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
      //  return CailWriteATIRegister(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index,pWorkingTableData->DestData32 );
}
#endif

// access to Frame buffer, dummy function, need more information to implement it  
UINT32	ReadFrameBuffer32 (PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    
    return CailReadFBData(pWorkingTableData->pDeviceData->CAIL, (pWorkingTableData->Index <<2 ));

}

VOID	WriteFrameBuffer32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWriteFBData(pWorkingTableData->pDeviceData->CAIL,(pWorkingTableData->Index <<2), pWorkingTableData->DestData32);

}


VOID *AllocateMemory(DEVICE_DATA *pDeviceData , UINT16 MemSize)
{
    if(MemSize)
        return(CailAllocateMemory(pDeviceData->CAIL,MemSize));
    else
        return NULL;
}


VOID ReleaseMemory(DEVICE_DATA *pDeviceData , WORKING_TABLE_DATA* pWorkingTableData)
{
    if( pWorkingTableData)
        CailReleaseMemory(pDeviceData->CAIL, pWorkingTableData);
}


UINT32	ReadMC32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    UINT32 ReadData;
    ReadData=(UINT32)CailReadMC(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index);
    return ReadData;
}

VOID	WriteMC32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWriteMC(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index,pWorkingTableData->DestData32);    
}

UINT32	ReadPLL32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    UINT32 ReadData;
    ReadData=(UINT32)CailReadPLL(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index);
    return ReadData;

}

VOID	WritePLL32(PARSER_TEMP_DATA STACK_BASED * pWorkingTableData)
{
    CailWritePLL(pWorkingTableData->pDeviceData->CAIL,pWorkingTableData->Index,pWorkingTableData->DestData32);    

}



#if DEBUG_PARSER>0
VOID CD_print_string	(DEVICE_DATA *pDeviceData, UINT8 *str)
{
    CailVideoDebugPrint( pDeviceData->CAIL, (ULONG_PTR) str, PARSER_STRINGS);
}

VOID CD_print_value	(DEVICE_DATA *pDeviceData, ULONG_PTR value, UINT16 value_type )
{
    CailVideoDebugPrint( pDeviceData->CAIL, (ULONG_PTR)value, value_type);
}

#endif

// EOF
