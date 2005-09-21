// ****************************************************************************
//
//		CDaffyDuck.H
//
//		Include file for interfacing with the CDaffyDuck class.
//
//		A daffy duck maintains a scatter-gather list used by the DSP to 
//		transfer audio data via bus mastering.
//
//		The scatter-gather list takes the form of a circular buffer of
//		duck entries; each duck entry is an address/count pair that points
//		to an audio data buffer somewhere in memory.  
//
//		Although the scatter-gather list itself is a circular buffer, that
// 	does not mean that the audio data pointed to by the scatter-gather
//		list is necessarily a circular buffer.  The audio buffers pointed to
// 	by the SG list may be either a non-circular linked list of buffers
//		or a ring buffer.
//
//		If you want a ring DMA buffer for your audio data, refer to the 
//		Wrap() method, below.
//
//		The name "daffy duck" is an inside joke that dates back to the 
//		original VxD for Windows 95.
//
//		Set editor tabs to 3 for your viewing pleasure.
//
//---------------------------------------------------------------------------
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

#ifndef _DAFFYDUCKOBJECT_
#define _DAFFYDUCKOBJECT_

#ifdef _DEBUG
//#define INTEGRITY_CHECK
#endif

//
// DUCKENTRY is a single entry for the scatter-gather list.  A DUCKENTRY
// struct is read by the DSP.
//
typedef struct
{
	DWORD		PhysAddr;
	DWORD		dwSize;
}	DUCKENTRY, * PDUCKENTRY;


//
// The CDaffyDuck class keeps a parallel array of MAPPING structures
// that corresponds to the DUCKENTRY array.  You can't just pack everything
// into one struct since the DSP reads the DUCKENTRY structs and wouldn't 
// know what to do with the other fields.
//
typedef struct
{
	NUINT			Tag;				// Unique ID for this mapping
	ULONGLONG	ullEndPos;		// The cumulative 64-bit end position for this mapping
										// = (previous ullEndPos) + (# of bytes mapped)
}	MAPPING;


class CDspCommObject;

class CDaffyDuck
{
public:

	//
	// Number of entries in the circular buffer.
	//
	// If MAX_ENTRIES is set too high, SONAR crashes in Windows XP if you are
	// using super-interleave mode.  64 seems to work.
	//
	enum
	{
		MAX_ENTRIES 		= 64,					// Note this must be a power of 2
		ENTRY_INDEX_MASK 	= MAX_ENTRIES-1
	};
	
	//
	//	Destructor
	//
	~CDaffyDuck();
	
	static CDaffyDuck * MakeDaffyDuck(COsSupport *pOsSupport);

protected:

	//
	// Protected constructor
	//
	CDaffyDuck(PCOsSupport 	pOsSupport);
	
	DWORD			m_dwPipeIndex;
	
	PPAGE_BLOCK	m_pDuckPage;

	DUCKENTRY	*m_DuckEntries;			// Points to a locked physical page (4096 bytes)
													// These are for the benefit of the DSP
	MAPPING		m_Mappings[MAX_ENTRIES];// Parallel circular buffer to m_DuckEntries;
													// these are for the benefit of port class
	
	DWORD			m_dwHead;					// Index where next mapping will be added;
													// points to an empty slot
	DWORD			m_dwTail;					// Next mapping to discard (read index)
	DWORD			m_dwCount;					// Number of entries in the circular buffer
	
	DWORD			m_dwDuckEntriesPhys;		// The DSP needs this - physical address
													// of page pointed to by m_DuckEntries
													
	ULONGLONG	m_ullLastEndPos;			// Used to calculate ullEndPos for new entries
													
	PCOsSupport	m_pOsSupport;
	
	BOOL			m_fWrapped;
	
#ifdef INTEGRITY_CHECK
	void CheckIntegrity();
#endif

	void EjectTail();
	
public:

	void Reset();
	
	void ResetStartPos();
	
	//
	// Call AddMapping to add a buffer of audio data to the scatter-gather list.
	// Note that dwPhysAddr will be asserted on the PCI bus; you will need
	// to make the appropriate translation between the virtual address of
	// the page and the bus address *before* calling this function.
	//
	// The buffer must be physically contiguous.
	//
	// The Tag parameter is a unique ID for this mapping that is used by 
	// ReleaseUsedMapping and RevokeMappings; if you are building a circular 
	// buffer, the tag isn't important.
	//
	// dwInterrupt is true if you want the DSP to generate an IRQ after it
	// consumes this audio buffer.
	// 
	// dwNumFreeEntries is useful if you are calling this in a loop and
	// want to know when to stop;
	//
	ECHOSTATUS AddMapping
	(
		DWORD			dwPhysAddr,
		DWORD			dwBytes,
		NUINT			Tag,						// Unique ID for this mapping
		DWORD			dwInterrupt,			// Set TRUE if you want an IRQ after this mapping
		DWORD			&dwNumFreeEntries		// Return value - number of slots left in the list
	);
	
	//
	// AddDoubleZero is used to have the DSP generate an interrupt; 
	// calling AddDoubleZero will cause the DSP to interrupt after it finishes the
	// previous duck entry.
	//
	ECHOSTATUS AddDoubleZero();

	//
	// Call Wrap if you are creating a circular DMA buffer; to make a circular
	// double buffer, do this:
	//
	//	AddMapping() 		Several times
	// AddDoubleZero()	First half-buffer interrupt
	// AddMapping()		Several more times
	// AddDoubleZero()	Second half-buffer interrupt
	// Wrap()				Wraps the scatter list around to make a circular buffer
	//
	// Once you call Wrap, you shouldn't add any more mappings.
	//
	void Wrap();

	//
	// Call ReleaseUsedMapping to conditionally remove the oldest duck entries.  
	//
	// The return value is the number of tags written to the Tags array.
	//	
	DWORD ReleaseUsedMappings
	(
		ULONGLONG	ullDmaPos,
		NUINT		 	*Tags,
		DWORD			dwMaxTags
	);

	//
	// Adjusts the duck so that DMA will start from a given position; useful
	// when resuming from pause
	//
	void AdjustStartPos(ULONGLONG ullPos);

	//
	// This returns the physical address of the start of the scatter-gather 
	// list; used to tell the DSP where to start looking for duck entries.
	//
	DWORD GetPhysStartAddr();
	
	//
	// Any more room in the s.g. list?
	//
	DWORD	GetNumFreeEntries()
	{
		return MAX_ENTRIES - m_dwCount;
	}
	
	//
	// RevokeMappings is here specifically to support WDM; it removes
	// any entries from the list if their tag is >= dwFirstTag and <= dwLastTag.
	//
	DWORD RevokeMappings
	(
		NUINT		 FirstTag,
		NUINT		 LastTag
	);
	
	//
	// Returns TRUE if Wrap has been called for this duck
	//
	BOOL Wrapped()
	{
		return m_fWrapped;
	}
		
	//
	// CleanUpTail is used to clean out any non-audio entries from the tail 
	// of the list that might be left over from earlier
	//	
	void CleanUpTail();
	
	//
	// Spew out some info
	//
	VOID DbgDump();
	
	//
	//	Overload new & delete to make sure these objects are allocated from
	// non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 
	
};		// class CDaffyDuck

typedef CDaffyDuck * PCDaffyDuck;

#endif // _DAFFYDUCKOBJECT_

// *** CDaffyDuck.H ***
