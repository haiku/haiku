// ****************************************************************************
//
//		OsSupportBeOS.cpp
//
//		Implementation file for BeOS support services to the CEchoGals
//		generic driver class
//		Set editor tabs to 3 for your viewing pleasure.
//
// ----------------------------------------------------------------------------
//
// This file is part of Echo Digital Audio's generic driver library.
// Copyright Echo Digital Audio Corporation (c) 1998 - 2005
// All rights reserved
// www.echoaudio.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// ****************************************************************************

#include "OsSupportBeOS.h"

#include "EchoGalsXface.h"

#include <KernelExport.h>
#include "util.h"

//
//	Version information.
//	In NT, we want to get this from a resource
//
BYTE OsGetVersion()
{
	// Use EngFindResource, for now hard code
	return( 1 );	
}	// BYTE OsGetVersion()

BYTE OsGetRevision()
{
	// Use EngFindResource, for now hard code
	return( 0 );	
}	// BYTE OsGetRevision()

BYTE OsGetRelease()
{
	// Use EngFindResource, for now hard code
	return( 0 );	
}	// BYTE OsGetRelease()

//
//	Global Memory Management Functions
//
DWORD gAllocNonPagedCount = 0;

LIST_HEAD(mems, _echo_mem) mems;


static echo_mem *
echo_mem_new(size_t size)
{
	echo_mem *mem = NULL;

	if ((mem = (echo_mem*)malloc(sizeof(*mem))) == NULL)
		return (NULL);

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "echo buffer");
	mem->size = size;
	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}
	return mem;
}

static void
echo_mem_delete(echo_mem *mem)
{
	if(mem->area > B_OK)
		delete_area(mem->area);
	free(mem);
}

echo_mem *
echo_mem_alloc(size_t size)
{
	echo_mem *mem = NULL;

	mem = echo_mem_new(size);
	if (mem == NULL)
		return (NULL);

	LIST_INSERT_HEAD(&mems, mem, next);

	return mem;
}

void
echo_mem_free(void *ptr)
{
	echo_mem *mem = NULL;

	LIST_FOREACH(mem, &mems, next) {
		if (mem->log_base != ptr)
			continue;
		LIST_REMOVE(mem, next);

		echo_mem_delete(mem);
		break;
	}
}

void OsAllocateInit()
{
	gAllocNonPagedCount = 0;

	/* Init mems list */
	LIST_INIT(&mems);
}

// ***********************************************************************
//
//  Allocate locked, non-pageable block of memory.  Does not have to be
//	physically contiguous.  Primarily used to implement the overloaded
//	new operator.
//
// ***********************************************************************

ECHOSTATUS OsAllocateNonPaged
(
    DWORD	dwByteCt,				// Block size in bytes
    PPVOID	ppMemAddr				// Where to return memory ptr
)
{
	
	
	echo_mem * mem = echo_mem_alloc( dwByteCt );
	if(mem)
		*ppMemAddr = mem->log_base;
	
	if ( NULL == *ppMemAddr )
	{
		ECHO_DEBUGPRINTF( ("OsAllocateNonPaged : Failed on %ld bytes\n",
								 dwByteCt) );
		ECHO_DEBUGBREAK();				 
		return ECHOSTATUS_NO_MEM;
	}
	
	OsZeroMemory( *ppMemAddr, dwByteCt );
	
	gAllocNonPagedCount++;
	ECHO_DEBUGPRINTF(("gAllocNonPagedCount %ld\n",gAllocNonPagedCount));
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS OsAllocateNonPaged


// ***********************************************************************
//
// Unlock and free, non-pageable block of memory.
//
// ***********************************************************************
ECHOSTATUS OsFreeNonPaged
(
    PVOID	pMemAddr
)
{
	echo_mem_free( pMemAddr );

	gAllocNonPagedCount--;
	ECHO_DEBUGPRINTF(("gAllocNonPagedCount %ld\n",gAllocNonPagedCount));

	return ECHOSTATUS_OK;

}	// ECHOSTATUS OsFreeNonPaged



// ***********************************************************************
//
// This class is optional and uniquely defined for each OS.  It provides
//	information that other components may require.
// For example, in Windows NT it contains a device object used by various
//	memory management methods.
// Since static variables are used in place of globals, an instance must
//	be constructed and initialized by the OS Interface object prior to
//	constructing the CEchoGals derived object.  The CEchoGals and
//	CDspCommObject classes must have access to it during their respective 
// construction times.
//
// ***********************************************************************

COsSupport::COsSupport
(
	WORD				wDeviceId,		// PCI bus device ID
	WORD				wCardRev			// Card revision number
)
{
   ECHO_DEBUGPRINTF(("COsSupport::COsSupport born, device id = 0x%x.\n", wDeviceId));

	m_wDeviceId = wDeviceId;
	m_wCardRev = wCardRev;
}

COsSupport::~COsSupport()
{
	ECHO_DEBUGPRINTF(("COsSupport is all gone - m_dwPageBlockCount %ld\n",m_dwPageBlockCount));
}

//
//	Timer Methods
//

ECHOSTATUS COsSupport::OsGetSystemTime
(
	PULONGLONG	pullTime					// Where to return system time
)
{
	*pullTime = ULONGLONG(system_time());

	return ECHOSTATUS_OK;

}	// ECHOSTATUS COsSupport::OsGetSystemTime


ECHOSTATUS COsSupport::OsSnooze
(
	DWORD	dwTime						// Duration in micro seconds
)
{
	status_t status;
	status = snooze(bigtime_t(dwTime));
	switch (status) {
		case B_OK:
			return ECHOSTATUS_OK;
			break;
		case B_INTERRUPTED:
			return ECHOSTATUS_OPERATION_CANCELED; // maybe not appropriate, but anyway
			break;
		default:
			return ECHOSTATUS_NOT_SUPPORTED; // no generic error?
			break;
	}
}


//
//	Memory Management Methods
//

//---------------------------------------------------------------------------
//
//	Allocate a physical page block that can be used for DSP bus mastering.
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::AllocPhysPageBlock
(
	DWORD			dwBytes,
	PPAGE_BLOCK	&pPageBlock
)
{
	DWORD	dwRoundedBytes;
	
	//
	// Allocate
	//
	dwRoundedBytes = (dwBytes + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	ECHO_DEBUGPRINTF(("COsSupport::AllocPhysPageBlock - dwBytes %ld  dwRoundedBytes %ld\n",
							dwBytes,dwRoundedBytes));
	
	pPageBlock = echo_mem_alloc ( dwRoundedBytes );

	if (NULL == pPageBlock)
	{
		ECHO_DEBUGPRINTF(("AllocPhysPageBlock failed for %ld bytes\n",dwBytes));

		pPageBlock = NULL;
		return ECHOSTATUS_NO_MEM;
	}
	
	ECHO_DEBUGPRINTF(("\tIOBufferMemoryDescriptor is OK\n"));

	OsZeroMemory( pPageBlock->log_base, dwRoundedBytes );
	
#ifdef _DEBUG
	m_dwPageBlockCount++;
	ECHO_DEBUGPRINTF(("\tm_dwPageBlockCount %ld\n",m_dwPageBlockCount));
#endif
	
	return ECHOSTATUS_OK;

}	// AllocPageBlock


//---------------------------------------------------------------------------
//
//	Free a physical page block
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::FreePhysPageBlock
(
	DWORD			dwBytes,
	PPAGE_BLOCK	pPageBlock
)
{
	echo_mem_free(pPageBlock->log_base);
	
#ifdef _DEBUG
	m_dwPageBlockCount--;
	ECHO_DEBUGPRINTF(("\tm_dwPageBlockCount %ld\n",m_dwPageBlockCount));
#endif
	
	return ECHOSTATUS_OK;
	
}	// FreePageBlock


//---------------------------------------------------------------------------
//
//	Get the virtual address for the buffer corresponding to the MDL
//
//---------------------------------------------------------------------------

PVOID COsSupport::GetPageBlockVirtAddress
(
	PPAGE_BLOCK	pPageBlock
)
{

	return pPageBlock->log_base;
		
}	// GetPageBlockVirtAddress


//---------------------------------------------------------------------------
//
//	Get the physical address for part of the buffer corresponding to the MDL
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::GetPageBlockPhysSegment
(
	PPAGE_BLOCK	pPageBlock,			// pass in a previously allocated block
	DWORD			dwOffset,			// pass in the offset into the block
	PHYS_ADDR	&PhysAddr,			// returns the physical address
	DWORD			&dwSegmentSize		// and the length of the segment
)
{

	PhysAddr = ((PHYS_ADDR)pPageBlock->phy_base + dwOffset);
	
	return ECHOSTATUS_OK;

} // GetPageBlockPhysSegment


//
// Add additional methods here
//

//
//	Display an error message w/title
//
void COsSupport::EchoErrorMsg(PCHAR pszMsg, PCHAR pszTitle)
{
}

PVOID COsSupport::operator new(size_t size)
{
	PVOID 		pMemory;

	pMemory = malloc(size);
	
	if ( NULL == pMemory )
	{
		ECHO_DEBUGPRINTF(("COsSupport::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, size );
	}
		
	return pMemory;
}

VOID COsSupport::operator delete(PVOID memory)
{
	free(memory);
}

