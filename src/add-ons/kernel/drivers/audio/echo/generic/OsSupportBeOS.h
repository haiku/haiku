// ****************************************************************************
//
//		OsSupportBeOS.H
//
//		Include file for BeOS Support Services to the CEchoGals
//		generic driver class
//
//		Set editor tabs to 3 for your viewing pleasure.
//
// ----------------------------------------------------------------------------
//
//		Copyright Echo Digital Audio Corporation (c) 1998 - 2002
//		All rights reserved
//		www.echoaudio.com
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

//#ifdef _DEBUG
//#pragma optimize("",off)
//#endif

//	Prevent problems with multiple includes
#ifndef _ECHOOSSUPPORTBEOS_
#define _ECHOOSSUPPORTBEOS_

#include <KernelExport.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include "kernel_cpp.h"

#if DEBUG > 0
// BeOS debug printf macro
#define ECHO_DEBUGPRINTF( strings ) TRACE(strings)
#define ECHO_DEBUGBREAK()			kernel_debugger("echo driver debug break");
#define ECHO_DEBUG

#else

#define ECHO_DEBUGPRINTF( strings )
#define ECHO_DEBUGBREAK()

#endif

//
//	Specify OS specific types
//
typedef void **			PPVOID;
typedef int8			INT8;
typedef int32			INT32;
typedef uint16			WORD;
typedef uint16 *		PWORD;
typedef uint32			DWORD;
typedef uint32 *		PDWORD;
typedef void *			PVOID;
#define VOID			void
typedef int8			BYTE;
typedef int8 *			PBYTE;
typedef unsigned long	ULONG;
typedef signed long long		LONGLONG;
typedef unsigned long long		ULONGLONG;
typedef unsigned long long *	PULONGLONG;
typedef char			CHAR;
typedef char *			PCHAR;
typedef	bool			BOOL;
typedef bool			BOOLEAN;
#define CONST			const

#define PAGE_SIZE		B_PAGE_SIZE

//
// Return Status Values
//
typedef unsigned long	ECHOSTATUS;


//
//	Define generic byte swapping functions
//
#define SWAP(x)	B_HOST_TO_LENDIAN_INT32(x)

//
//	Define what a physical address is on this OS
//
typedef	uint32		PHYS_ADDR;			// Define physical addr type
typedef	uint32 *	PPHYS_ADDR;		// Define physical addr pointer type

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
#define OsCopyMemory(pDest,pSrc,dwBytes) 	memcpy(pDest,pSrc,dwBytes)

//
// Set memory to zero
//
#define OsZeroMemory(pDest,dwBytes)			memset(pDest,0,dwBytes)


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
		DWORD				dwDeviceId		// PCI bus device id
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
	//	Stall execution for dwTime microseconds.
	// Return error status if the OS doesn't support this function.
	//
	ECHOSTATUS OsSnooze
	(
		DWORD	dwTime
	);


	//
	//	Memory Management Methods
	//

	//
	// Allocate locked, non-pageable, physically contiguous memory pages
	//	in the drivers address space.  Used to allocate memory for the DSP
	//	communications area and Ducks.
	//
	ECHOSTATUS OsPageAllocate
	(
		DWORD			dwPageCt,				// How many pages to allocate
		PPVOID		ppPageAddr,				// Where to return the memory ptr
		PPHYS_ADDR	pPhysicalPageAddr		// Where to return the physical PCI addr
	);

	//
	// Unlock and free non-pageable, physically contiguous memory pages
	//	in the drivers address space.
	// Used to free memory for the DSP communications area and Ducks.
	//
	ECHOSTATUS OsPageFree
	(
		DWORD			dwPageCt,				// How many pages to free
		PVOID			pPageAddr,				// Virtual memory ptr
		PHYS_ADDR	PhysicalPageAddr		// Physical PCI addr
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
	DWORD GetDeviceId()
		{ return( m_dwDeviceId ); }
		
	//
	//	Overload new & delete so memory for this object is allocated 
	//	from non-paged memory.
	//
	PVOID	operator new( size_t Size );
	VOID	operator delete( PVOID pVoid ); 

protected:

private:
	DWORD					m_dwDeviceId;		// PCI Device ID

	//
	// Define data here.
	//

	//KIRQL					m_IrqlCurrent;		// Old IRQ level
	bigtime_t			m_ullStartTime;	// All system time relative to this time
	class CPtrQueue *	m_pPtrQue;			// Store read only ptrs so they
													// can be unmapped
													
};		// class COsSupport

typedef COsSupport * PCOsSupport;

#endif // _ECHOOSSUPPORTBEOS_
