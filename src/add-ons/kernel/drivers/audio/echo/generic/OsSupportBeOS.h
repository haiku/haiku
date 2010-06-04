// ****************************************************************************
//
//		OsSupportBeOS.H
//
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

//	Prevent problems with multiple includes
#ifndef _ECHOOSSUPPORTBEOS_
#define _ECHOOSSUPPORTBEOS_

#include <KernelExport.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include "queue.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include "util/kernel_cpp.h"

#if DEBUG > 0
// BeOS debug printf macro
#define ECHO_DEBUGPRINTF( strings ) TRACE(strings)
#define ECHO_DEBUGBREAK()                       kernel_debugger("echo driver debug break");
#define ECHO_DEBUG

#else

#define ECHO_DEBUGPRINTF( strings )
#define ECHO_DEBUGBREAK()

#endif

//
// Assert macro
//
//#define ECHO_ASSERT(exp)	ASSERT(exp)
#define ECHO_ASSERT(exp)

//
//	Specify OS specific types
//
typedef char 		*PCHAR ;
typedef uint8	 	BYTE;
typedef uint8 		*PBYTE;
typedef uint16	 	WORD;
typedef uint16	 	*PWORD;
typedef uint32	 	DWORD;
typedef unsigned long 	ULONG;
typedef unsigned long 	NUINT;
typedef long 		NINT;
typedef void 		*PVOID;
typedef DWORD 		*PDWORD;
#define VOID 		void
typedef bool 		BOOL;
typedef bool 		BOOLEAN;
typedef char 		*PTCHAR;
typedef char		TCHAR;
typedef char		CHAR;
typedef char*		LPSTR;
//typedef SInt32	INT;
typedef int32		INT32;
typedef int32	 	*PINT;
typedef int8		INT8;
typedef long		HANDLE;
typedef long		LONG;
typedef int64 		LONGLONG;
typedef uint64 		ULONGLONG ;
typedef uint64 		*PULONGLONG ;
typedef long 		*PKEVENT ;
#undef NULL
#define NULL 0
#define CALLBACK

#define CONST const
typedef	void **		PPVOID;

#ifndef PAGE_SIZE
#define PAGE_SIZE                B_PAGE_SIZE
#endif

#define WideToSInt64(x)         (*((int64*)(&x)))
#define WideToUInt64(x)         (*((uint64*)(&x)))

//
// Return Status Values
//
typedef unsigned long	ECHOSTATUS;


//
// Define our platform specific things here.
//
typedef struct _echo_mem {
	LIST_ENTRY(_echo_mem) next;
	void		*log_base;
	phys_addr_t	phy_base;
	area_id 	area;
	size_t		size;
} echo_mem;

//
//	Define generic byte swapping functions
//
#define SWAP(x)B_HOST_TO_LENDIAN_INT32(x)

//
//	Define what a physical address is on this OS
//
typedef	phys_addr_t		PHYS_ADDR;			// Define physical addr type
typedef	phys_addr_t*	PPHYS_ADDR;			// Define physical addr pointer type

typedef echo_mem* PPAGE_BLOCK;


//
//	Version information.
//	In NT, we want to get this from a resource
//
#define	APPVERSION			OsGetVersion()
#define	APPREVISION			OsGetRevision()
#define	APPRELEASE			OsGetRelease()

BYTE OsGetVersion();
BYTE OsGetRevision();
BYTE OsGetRelease();

//
//	Global Memory Management Functions
//

//
// This tag is used to mark all memory allocated by EchoGals.
//	Due to the way PoolMon displays things, we spell Echo backwards
//	so it displays correctly.
//
#define ECHO_POOL_TAG      'OHCE'


//
// OsAllocateInit - Set up memory tracking.  Call this once - not
// once per PCI card, just one time.
//
void OsAllocateInit();

//
// Allocate locked, non-pageable block of memory.  Does not have to be
//	physically contiguous.  Primarily used to implement the overloaded
//	new operator for classes that must remain memory resident.
//
ECHOSTATUS OsAllocateNonPaged
(
    DWORD	dwByteCt,
    PPVOID	ppMemAddr
);


//
// Unlock and free, non-pageable block of memory.
//
ECHOSTATUS OsFreeNonPaged
(
    PVOID	pMemAddr
);


//
// Copy memory
//
//!!! Anything faster we can use?
#define OsCopyMemory(pDest, pSrc, dwBytes) 	memcpy(pDest, pSrc, dwBytes)

//
// Set memory to zero
//
#define OsZeroMemory(pDest, dwBytes)			memset(pDest, 0, dwBytes)


//
// This class is uniquely defined for each OS.  It provides
//	information that other components may require.
// For example, in Windows NT it contains a device object used by various
//	memory management methods.
// Since static variables are used in place of globals, an instance must
//	be constructed and initialized by the OS Interface object prior to
//	constructing the CEchoGals derived object.  The CEchoGals and
//	CDspCommObject classes must have access to it during their respective
// construction times.
//
class COsSupport
{
public:
	//
	//	Construction/destruction
	//
	COsSupport
	(
		WORD				wDeviceId,		// PCI bus device id
		WORD				wCardRev			// Hardware revision number
	);

	~COsSupport();

	//
	//	Timer Methods
	//

	//
	// Return the system time in microseconds.
	// Return error status if the OS doesn't support this function.
	//
	ECHOSTATUS OsGetSystemTime
	(
		PULONGLONG	pullTime
	);


	//
	// Stall execution for dwTime microseconds.
	// Return error status if the OS doesn't support this function.
	//
	ECHOSTATUS OsSnooze
	(
		DWORD	dwTime						// Duration in micro seconds
	);


	//
	//	Memory Management Methods
	//

	//
	// Allocate a block of physical memory pages
	//
	ECHOSTATUS AllocPhysPageBlock
	(
		DWORD			dwBytes,
		PPAGE_BLOCK	&pPageBlock
	);

	//
	// Free a block of physical memory
	//
	ECHOSTATUS FreePhysPageBlock
	(
		DWORD			dwBytes,
		PPAGE_BLOCK	pPageBlock
	);

	//
	// Get the virtual address for a page block
	//
	PVOID GetPageBlockVirtAddress
	(
		PPAGE_BLOCK	pPageBlock
	);

	//
	// Get the physical address for a segment of a page block
	//
	ECHOSTATUS GetPageBlockPhysSegment
	(
		PPAGE_BLOCK	pPageBlock,			// pass in a previously allocated block
		DWORD			dwOffset,			// pass in the offset into the block
		PHYS_ADDR	&PhysAddr,			// returns the physical address
		DWORD			&dwSegmentSize		// and the length of the segment
	);


	//
	// Add additional methods here
	//

	//
	//	Display and/or log an error message w/title
	//
	void EchoErrorMsg
	(
		PCHAR pszMsg,
		PCHAR pszTitle
	);

	//
	//	Return PCI card device ID
	//
	WORD GetDeviceId()
		{ return( m_wDeviceId ); }

	//
	// Return the hardware revision number
	//
	WORD GetCardRev()
	{
		return m_wCardRev;
	}

	//
	// Get the current timestamp for MIDI input data
	//
	LONGLONG GetMidiInTimestamp()
	{
		return system_time();
	}

	//
	//	Overload new & delete so memory for this object is allocated
	//	from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID	operator delete( PVOID pVoid );

protected:

private:
	WORD					m_wDeviceId;		// PCI Device ID
	WORD					m_wCardRev;			// Hardware revision

	//
	// Define data here.
	//
#ifdef ECHO_DEBUG
	DWORD					m_dwPageBlockCount;
#endif

};		// class COsSupport

typedef COsSupport * PCOsSupport;

#endif // _ECHOOSSUPPORTBEOS_
