// ****************************************************************************
//
//		OsSupportWDM.cpp
//
//		Implementation file for WDM support services to the CEchoGals
//		generic driver class.
//
//		This will need to be rewritten for each new target OS.
//
//		Set editor tabs to 3 for your viewing pleasure.
//
//		Copyright Echo Digital Audio Corporation (c) 1998 - 2002
//		All rights reserved
//		www.echoaudio.com
//
//		Modifications for OpenBeOS Copyright Andrew Bachmann (c) 2002
//		All rights reserved
//		www.openbeos.org
//
//		Permission is hereby granted, free of charge, to any person obtaining a
//		copy of this software and associated documentation files (the
//		"Software"), to deal with the Software without restriction, including
//		without limitation the rights to use, copy, modify, merge, publish,
//		distribute, sublicense, and/or sell copies of the Software, and to
//		permit persons to whom the Software is furnished to do so, subject to
//		the following conditions:
//		
//		- Redistributions of source code must retain the above copyright
//		notice, this list of conditions and the following disclaimers.
//		
//		- Redistributions in binary form must reproduce the above copyright
//		notice, this list of conditions and the following disclaimers in the
//		documentation and/or other materials provided with the distribution.
//		
//		- Neither the name of Echo Digital Audio, nor the names of its
//		contributors may be used to endorse or promote products derived from
//		this Software without specific prior written permission.
//
//		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//		IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
//		ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//		TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//		SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//
// ****************************************************************************

#include "CEchoGals.h"
#include <stdarg.h>
#include <RealtimeAlloc.h>
#include <OS.h>
#include <Alert.h>

/****************************************************************************

	Memory management - not part of COsSupport

 ****************************************************************************/

//===========================================================================
//
// Init the memory tracking counter; right now, this just uses a simple
// scheme that tracks the number of allocations
//
//===========================================================================

DWORD gdwAllocNonPagedCount = 0;
rtm_pool * echo_pool = 0;

void OsAllocateInit()
{
	gdwAllocNonPagedCount = 0;
	rtm_create_pool(&echo_pool,0xFFFF, ECHO_POOL_TAG);
	if ( NULL == echo_pool )
	{
		ECHO_DEBUGPRINTF( ("OsAllocateInit : Failed to create the pool\n") );
		ECHO_DEBUGBREAK();
		return;
	}

}	// OsAllocateInit


//===========================================================================
//
// OsAllocateNonPaged is used to allocate a block of memory that will always
// be resident; that is, this particular block of memory won't be swapped
// out as virtual memory and can always be accessed at interrupt time.
//
// gdwAllocNonPagedCount tracks the number of times this has been
// successfully called.
//
// This is primarily used for overloading the new operator for the various
// generic classes.
//
//===========================================================================


ECHOSTATUS OsAllocateNonPaged
(
    DWORD	dwByteCt,				// Block size in bytes
    PPVOID	ppMemAddr				// Where to return memory ptr
)
{
	*ppMemAddr = rtm_alloc( echo_pool, dwByteCt );

	if ( NULL == *ppMemAddr )
	{
		ECHO_DEBUGPRINTF( ("OsAllocateNonPaged : Failed on %d bytes\n",
								 dwByteCt) );
		ECHO_DEBUGBREAK();				 
		return ECHOSTATUS_NO_MEM;
	}
	
	OsZeroMemory( *ppMemAddr, dwByteCt );
	
	gdwAllocNonPagedCount++;
	ECHO_DEBUGPRINTF(("gdwAllocNonPagedCount %d\n",gdwAllocNonPagedCount));
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS OsAllocateNonPaged


//===========================================================================
//
// OsFreeNonPaged is used to free memory allocated by OsAllocateNonPaged.
//
// gdwAllocNonPagedCount tracks the number of times this has been
// successfully called.
//
//===========================================================================

ECHOSTATUS OsFreeNonPaged
(
    PVOID	pMemAddr
)
{
	rtm_free( pMemAddr );

	gdwAllocNonPagedCount--;
	ECHO_DEBUGPRINTF(("gdwAllocNonPagedCount %d\n",gdwAllocNonPagedCount));

	return ECHOSTATUS_OK;

}	// ECHOSTATUS OsFreeNonPaged


//***********************************************************************

//
// This class is uniquely defined for each OS.  It provides
//	information that other components may require.
//
// For example, in Windows NT it contains a device object used by various
//	memory management methods.
//
// An instance of this class must be constructed and initialized prior to
//	constructing the CEchoGals derived object.  The CEchoGals and
//	CDspCommObject classes must have access to it during their respective 
// construction times.
//
//***********************************************************************

//===========================================================================
//
//	Construction/destruction
//
//===========================================================================

COsSupport::COsSupport
(
	DWORD				dwDeviceId		// PCI bus subsystem ID
)
{
	m_ullStartTime = system_time(); // All system time calls relative to this

	m_dwDeviceId = dwDeviceId;

}	// COsSupport::COsSupport()


COsSupport::~COsSupport()
{
}	// COsSupport::~COsSupport()


//===========================================================================
//
//	Timer methods
//
//===========================================================================

//---------------------------------------------------------------------------
//
// Return the system time in microseconds.
// Return error status if the OS doesn't support this function.
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::OsGetSystemTime
(
	PULONGLONG	pullTime					// Where to return system time
)
{
	*pullTime = ULONGLONG(system_time());

	return ECHOSTATUS_OK;

}	// ECHOSTATUS COsSupport::OsGetSystemTime


//---------------------------------------------------------------------------
//
// Stall execution for dwTime microseconds.
// Return error status if the OS doesn't support this 
// function.
//
//---------------------------------------------------------------------------

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


//===========================================================================
//
//	More memory management
//
//===========================================================================

//---------------------------------------------------------------------------
//
// Allocate locked, non-pageable, physically contiguous memory pages
//	in the drivers address space.  Used to allocate memory for the DSP
//	communications area.
//
// Currently, none of the generic code ever sets dwPageCt to higher than
// one.  On the platforms we've developed for so far, a page is 4096
// bytes and is aligned on a 4096 byte boundary.  None of the structures
// allocated by OsPageAllocate are more than 4096 bytes; memory
// allocated by this routine should be at least aligned on a 32 byte
// boundary and be physically contiguous.
//
// Note that this code is not 64 bit ready, since it's only using
// the low 32 bits of the physical address.
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::OsPageAllocate
(
	DWORD			dwPageCt,				// How many pages to allocate
	PPVOID		ppPageAddr,				// Where to return the memory ptr
	PPHYS_ADDR	pPhysicalPageAddr		// Where to return the physical PCI address
)
{
	PHYSICAL_ADDRESS		LogicalAddress;

	*ppPageAddr = rtm_alloc ( echo_pool, dwPageCt );
	
	if (NULL != *ppPageAddr)
	{
		physical_entry PhysTemp;
		
		get_memory_map(ppPageAddr, dwPageCt, &PhysTemp, 1);
		
		PhysTemp = MmGetPhysicalAddress(*ppPageAddr); // XXX: ?
		*pPhysicalPageAddr = PhysTemp.address;
	}

	OsZeroMemory( *ppPageAddr, dwPageCt * PAGE_SIZE );
	
	return ECHOSTATUS_OK;

}	// ECHOSTATUS COsSupport::OsPageAllocate


//---------------------------------------------------------------------------
//
// Unlock and free non-pageable, physically contiguous memory pages in 
//	the drivers address space; the inverse of OsPageAllocate
//
//---------------------------------------------------------------------------

ECHOSTATUS COsSupport::OsPageFree
(
		DWORD			dwPageCt,				// How many pages to free
		PVOID			pPageAddr,				// Virtual memory ptr
		PHYS_ADDR	PhysicalPageAddr		// Physical PCI addr
)
{
	PHYSICAL_ADDRESS		LogicalAddress;

	if (NULL == pPageAddr)
		return ECHOSTATUS_OK;
	
	rtm_free(pPageAddr);	

	return ECHOSTATUS_OK;

}	// ECHOSTATUS COsSupport::OsPageFree


//---------------------------------------------------------------------------
//
//	Display an error message w/title; currently not supported under WDM.
//
//---------------------------------------------------------------------------

void COsSupport::EchoErrorMsg
(
	PCHAR pszMsg,
	PCHAR pszTitle
)
{
	BAlert alert(pszTitle,pszMsg,"Ok",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
	alert.Go();
}	// void COsSupport::EchoErrorMsg( PCHAR )


//---------------------------------------------------------------------------
//
// Overload new & delete so memory for this object is allocated from 
//	non-paged memory.
//
//---------------------------------------------------------------------------

PVOID COsSupport::operator new( size_t Size )
{
	PVOID 		pMemory;
	
	// not from our pool since that may not be around yet
	pMemory = rtm_alloc(NULL,Size);
	
	if ( NULL == pMemory )
	{
		ECHO_DEBUGPRINTF(("COsSupport::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID COsSupport::operator new( size_t Size )


VOID COsSupport::operator delete( PVOID pVoid )
{
	status_t status;
	status = rtm_free(pVoid);
	
	if (status != B_OK) {
		ECHO_DEBUGPRINTF(("COsSupport::operator delete "
								"memory free failed\n"));
	}
	
}	// VOID COsSupport::operator delete( PVOID pVoid )

