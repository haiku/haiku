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
#include <KernelExport.h>
#include "queue.h"
#include "util.h"

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

typedef struct _echo_mem {
	LIST_ENTRY(_echo_mem) next;
	void	*log_base;
	void	*phy_base;
	area_id area;
	size_t	size;
} echo_mem;

LIST_HEAD(, _echo_mem) mems;


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
	echo_mem 		*mem = NULL;
	
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
	gdwAllocNonPagedCount = 0;
	/* Init mems list */
	LIST_INIT(&mems);

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

// OsAllocateNonPaged is used to (surprise!) allocate non-paged memory.  Non-paged 
// memory is defined as memory that is always physically resident - that is, the virtual 
// memory manager won't swap this memory to disk.  Nonpaged memory can be accessed 
// from the interrupt handler. 
// Nonpaged memory has no particular requirements on address alignment. 

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
	
	gdwAllocNonPagedCount++;
	ECHO_DEBUGPRINTF(("gdwAllocNonPagedCount %ld\n",gdwAllocNonPagedCount));
	
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
	echo_mem_free( pMemAddr );

	gdwAllocNonPagedCount--;
	ECHO_DEBUGPRINTF(("gdwAllocNonPagedCount %ld\n",gdwAllocNonPagedCount));

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

// COsSupport::OsPageAllocate is used for allocating memory that is read and written by 
// the DSP.  This means that any memory allocated by OsPageAllocate must be nonpaged 
// and should be aligned on at least a 32 byte address boundary.  It must be aligned on at 
// least a 4 byte boundary. 

ECHOSTATUS COsSupport::OsPageAllocate
(
	DWORD			dwPageCt,				// How many pages to allocate
	PPVOID		ppPageAddr,				// Where to return the memory ptr
	PPHYS_ADDR	pPhysicalPageAddr		// Where to return the physical PCI address
)
{
	
	echo_mem *mem = echo_mem_alloc ( dwPageCt * B_PAGE_SIZE );
	if(mem)
		*ppPageAddr = mem->log_base;
	
	if (NULL != *ppPageAddr)
	{
		*pPhysicalPageAddr = (PHYS_ADDR) mem->phy_base;
	}

	OsZeroMemory( *ppPageAddr, dwPageCt * B_PAGE_SIZE );
	
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
	if (NULL == pPageAddr)
		return ECHOSTATUS_OK;
	
	echo_mem_free (pPageAddr);	

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
	//BAlert alert(pszTitle,pszMsg,"Ok",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
	//alert.Go();
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

	pMemory = malloc(Size);
	
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
	free(pVoid);	
}	// VOID COsSupport::operator delete( PVOID pVoid )

