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

//	Prevent problems with multiple includes

#ifndef _DAFFYDUCKOBJECT_
#define _DAFFYDUCKOBJECT_

#ifdef _DEBUG
#define INTEGRITY_CHECK
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
	DWORD			dwNumEntries;	// How many slots in the array are used
										// = 1 for an audio mapping
										// = 2 for an audio mapping + double zero
	DWORD			dwTag;			// Unique ID for this mapping
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
		MAX_ENTRIES = 64
	};
	
	//
	//	Construction/destruction
	//
	CDaffyDuck
	(
		PCOsSupport 	pOsSupport,
		CDspCommObject *pDspCommObject,
		WORD				wPipeIndex
	);

	~CDaffyDuck();

protected:

	WORD			m_wPipeIndex;

	DUCKENTRY	*m_DuckEntries;			// Points to a locked physical page (4096 bytes)
													// These are for the benefit of the DSP
	MAPPING		m_Mappings[MAX_ENTRIES];// Parallel circular buffer to m_DuckEntries;
													// these are for the benefit of port class
	
	DWORD			m_dwHead;					// Index for most recently adding mapping
	DWORD			m_dwTail;					// Next mapping to discard (read index)
	DWORD			m_dwCount;					// Number of entries in the circular buffer
	
	DWORD			m_dwDuckEntriesPhys;		// The DSP needs this - physical address
													// of page pointed to by m_DuckEntries
													
	ULONGLONG	m_ullLastEndPos;													
													
	PCOsSupport	m_pOsSupport;
	
	CDspCommObject *m_pDspCommObject;
	
	BOOL			m_fWrapped;

#ifdef INTEGRITY_CHECK
	void CheckIntegrity();
#endif
	
	ECHOSTATUS GetTail
	(
		DUCKENTRY	*pDuckEntry,
		MAPPING		*pMapping
	);

public:

	//
	// InitCheck should be called after constructing the CDaffyDuck; this
	// is needed because you can't use exceptions in the Windows kernel.
	//
	ECHOSTATUS InitCheck();
	
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
	// The dwTag parameter is a unique ID for this mapping that is used by 
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
		DWORD			dwTag,					// Unique ID for this mapping
		DWORD			dwInterrupt,			// Set TRUE if you want an IRQ after this mapping
		DWORD			&dwNumFreeEntries		// Return value - number of slots left in the list
	);
	
	//
	// AddDoubleZero is used to have the DSP generate an interrupt; 
	// calling AddDoubleZero will cause the DSP to interrupt after it finishes the
	// previous duck entry.
	//
	void AddDoubleZero();

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
	// Call ReleaseUsedMapping to conditionally remove the oldest duck entry.  
	// ReleaseUsedMapping looks at the DMA position you pass in; if the DMA position
	// is past the end of the oldest mapping, the oldest mapping is removed
	// from the list and the tag number is returned in the dwTag parameter. 
	//	
	ECHOSTATUS ReleaseUsedMapping
	(
		ULONGLONG	ullDmaPos,
		DWORD			&dwTag
	);
	
	//
	// This returns the physical address of the start of the scatter-gather 
	// list; used to tell the DSP where to start looking for duck entries.
	//
	DWORD GetPhysStartAddr()
	{
		return m_dwDuckEntriesPhys + (m_dwTail * sizeof(DUCKENTRY));
	}
	
	//
	// Any more room in the s.g. list?
	//
	DWORD	GetNumFreeEntries()
	{
		return MAX_ENTRIES - m_dwCount;
	}
	
	//
	// Used to reset the end pos for rebuilding the duck; this
	// is used by the WDM driver to handle revoking.
	//
	void SetLastEndPos(ULONGLONG ullPos)
	{
		m_ullLastEndPos = ullPos;
	}
	
	//
	// Returns the 64 bit DMA position of the start of the oldest entry
	//
	ULONGLONG GetStartPos()
	{
		return m_Mappings[ m_dwTail ].ullEndPos - 
					(ULONGLONG) (m_DuckEntries[ m_dwTail ].dwSize);
	}
	
	//
	// RevokeMappings is here specifically to support WDM; it removes
	// any entries from the list if their tag is >= dwFirstTag and <= dwLastTag.
	//
	DWORD RevokeMappings
	(
		DWORD dwFirstTag,
		DWORD dwLastTag
	);
	
	//
	// Returns TRUE if Wrap has been called for this duck
	//
	BOOL Wrapped()
	{
		return m_fWrapped;
	}
		
	
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
