// ****************************************************************************
//
//		CDaffyDuck.CPP
//
//		Implementation file for the CDaffyDuck class.
//		Set editor tabs to 3 for your viewing pleasure.
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
//---------------------------------------------------------------------------
//
// The head pointer tracks the next free slot in the circular buffers
// The tail pointer tracks the oldest mapping.
//
//****************************************************************************

#include "CEchoGals.h"



/****************************************************************************

	Construction/destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CDaffyDuck::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CDaffyDuck::operator new( size_t Size )


VOID  CDaffyDuck::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck::operator delete memory free failed\n"));
	}
	
}	// VOID  CDaffyDuck::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor
//
//===========================================================================

CDaffyDuck::CDaffyDuck
(
	PCOsSupport 	pOsSupport,
	CDspCommObject	*pDspCommObject,
	WORD				wPipeIndex
)
{
	//
	// Allocate the page for the duck entries
	// 
	if ( ECHOSTATUS_OK != pOsSupport->OsPageAllocate( 1,
																	  (PVOID *) &m_DuckEntries,
																	  &m_dwDuckEntriesPhys ) )
	{
		ECHO_DEBUGPRINTF( ("CDaffyDuck: memory allocation failed\n") );
		return;
	}
	
	//
	//	Stash stuff
	// 
	m_pOsSupport = pOsSupport;
	m_pDspCommObject = pDspCommObject;
	m_wPipeIndex = wPipeIndex;
	
	//
	// Put the physical address of the duck at the end of 
	// the m_DuckEntries array so the DSP will wrap around
	// to the start of the duck.  
	//
	m_DuckEntries[MAX_ENTRIES].PhysAddr = SWAP( m_dwDuckEntriesPhys );
	
}	// CDaffyDuck::CDaffyDuck()


//===========================================================================
//
// Destructor
//
//===========================================================================

CDaffyDuck::~CDaffyDuck()
{

	m_pOsSupport->OsPageFree(1,m_DuckEntries,m_dwDuckEntriesPhys);

}	// CDaffyDuck::~CDaffyDuck()




/****************************************************************************

	Setup and initialization

 ****************************************************************************/

//===========================================================================
//
// InitCheck - returns ECHOSTATUS_OK if the duck created OK
//
//===========================================================================

ECHOSTATUS CDaffyDuck::InitCheck()
{
	if (NULL == m_DuckEntries)
		return ECHOSTATUS_NO_MEM;
		
	return ECHOSTATUS_OK;
	
}	// InitCheck


//===========================================================================
//
// Reset - resets the mapping and duck entry circular buffers
//
//===========================================================================

void CDaffyDuck::Reset()
{
	//
	//	Zero everything
	// 
	OsZeroMemory(m_DuckEntries,4096);
	OsZeroMemory(m_Mappings,sizeof(m_Mappings));
	
	m_dwHead = 0;
	m_dwTail = 0;
	m_dwCount = 0;
	m_ullLastEndPos = 0;

	//
	// Put the physical address of the duck at the end of 
	// the m_DuckEntries array so the DSP will wrap around
	// to the start of the duck.  
	//
	m_DuckEntries[MAX_ENTRIES].PhysAddr = SWAP( m_dwDuckEntriesPhys );
	
}	// Reset


//===========================================================================
//
// ResetStartPos - Takes the current list and re-calculates the
// DMA end position for each mapping, starting at DMA position zero.
//
//===========================================================================

void CDaffyDuck::ResetStartPos()
{
	DWORD dwRemaining,dwIndex,dwNumEntries;

	m_ullLastEndPos = 0L;
	
	//
	// See if the mapping at the buffer tail has been consumed
	//
	dwRemaining = m_dwCount;
	dwIndex = m_dwTail;
	while (0 != dwRemaining)
	{
		m_Mappings[dwIndex].ullEndPos = 
			m_ullLastEndPos + SWAP( m_DuckEntries[dwIndex].dwSize );
			
		m_ullLastEndPos = m_Mappings[dwIndex].ullEndPos;
	
		dwNumEntries = m_Mappings[m_dwTail].dwNumEntries;
		
		ASSERT(dwRemaining >= dwNumEntries);

		dwIndex += dwNumEntries;
		if (dwIndex >= MAX_ENTRIES)
			dwIndex -= MAX_ENTRIES;
			
		dwRemaining -= dwNumEntries;
	}
	
}	// ResetStartPos




/****************************************************************************

	Mapping management

 ****************************************************************************/

//===========================================================================
//
// AddMapping
//
// Take a mapping and add it to the circular buffer.
//
// Note that the m_DuckEntries array is read by the DSP; entries must
// therefore be stored in little-endian format. 
//
// The buffer pointed to by dwPhysAddr and dwBytes must be
// physically contiguous.
//
//===========================================================================

ECHOSTATUS CDaffyDuck::AddMapping
(
	DWORD			dwPhysAddr,
	DWORD			dwBytes,
	DWORD			dwTag,
	DWORD			dwInterrupt,
	DWORD			&dwNumFreeEntries
)
{
#ifdef INTEGRITY_CHECK
	CheckIntegrity();
#endif	

	//
	// The caller should only be trying to do this if 
	// there are at least two free entries
	//
	ASSERT((MAX_ENTRIES - m_dwCount) >= 2);
	
	//
	//	At least two slots are available in the circular 
	// buffer, so it's OK to add either a regular mapping or 
	// a mapping with a double zero
	//
	m_DuckEntries[m_dwHead].PhysAddr = SWAP( dwPhysAddr );
	m_DuckEntries[m_dwHead].dwSize 	= SWAP( dwBytes );
	
	m_Mappings[m_dwHead].dwTag			= dwTag;
	m_Mappings[m_dwHead].ullEndPos	= m_ullLastEndPos + dwBytes;
	
	m_ullLastEndPos = m_Mappings[m_dwHead].ullEndPos;
	
	/*
	ECHO_DEBUGPRINTF(("CDaffyDuck::AddMapping  m_dwCount %ld  pipe index %d  end pos %I64x\n",
							m_dwCount,m_wPipeIndex,m_ullLastEndPos));	
	*/
		
	//
	// If the caller wants an interrupt after this mapping, then
	// dwInterrupt will be non-zero
	//
	if (dwInterrupt)
	{
		DWORD dwNext;

		//
		// Interrupt wanted - need to add an extra slot with the double zero
		//
		m_Mappings[m_dwHead].dwNumEntries = 2;

		//
		// Put in the double zero so the DSP will
		// generate an interrupt
		//
		dwNext = m_dwHead + 1;
		if (dwNext >= MAX_ENTRIES)
			dwNext -= MAX_ENTRIES;	
	
		m_DuckEntries[dwNext].PhysAddr 	= 0;	// no need to swap zero!
		m_DuckEntries[dwNext].dwSize 		= 0;
		
		m_dwCount += 2;				
	}
	else
	{
		m_Mappings[m_dwHead].dwNumEntries = 1;

		m_dwCount++;
	}
	
	//
	// Move the head to point to the next empty slot
	//		
	m_dwHead += m_Mappings[m_dwHead].dwNumEntries;
	if (m_dwHead >= MAX_ENTRIES)
		m_dwHead -= MAX_ENTRIES;	// circular buffer wrap

	//
	// Return value to the caller
	//	
	dwNumFreeEntries = MAX_ENTRIES - m_dwCount;
	
	//
	// Tell the DSP about the new buffer if the
	// interrupt flag is set 
	//
	if (dwInterrupt != 0)
	{
		m_pDspCommObject->AddBuffer( m_wPipeIndex );
	}


//#ifdef _DEBUG
#if 0
	DWORD hi,lo;
	
	hi = (DWORD) (m_ullLastEndPos >> 32);
	lo = (DWORD) (m_ullLastEndPos & 0xffffffffL);
	
	ECHO_DEBUGPRINTF(("Added tag %ld, end pos 0x%08lx%08lx (int %ld)\n",dwTag,hi,lo,dwInterrupt));

#ifdef INTEGRITY_CHECK
	CheckIntegrity();
#endif

	ECHO_DEBUGPRINTF(("Daffy duck count is %ld\n",m_dwCount));
	
#endif
	
	return ECHOSTATUS_OK;
	
}	// AddMapping


//===========================================================================
//
// AddDoubleZero
//
// Adds a double zero to the circular buffer to cause the DSP to generate an
// IRQ.  If the buffer is full -or- the most recent entry already has a 
// double zero, nothing happens.
// 
//===========================================================================

void CDaffyDuck::AddDoubleZero()
{
	//
	// Skip if the circular buffer is full or empty
	//
	if ((MAX_ENTRIES == m_dwCount) || (0 == m_dwCount))
		return;
	
	//ECHO_DEBUGPRINTF(("CDaffyDuck::AddDoubleZero  m_dwCount %ld\n",m_dwCount));	

	//
	// Search back before the head and find the most recent entry
	//
	DWORD	dwEntry;
#ifdef _DEBUG
	DWORD dwCount = 0;
#endif


	do
	{
#ifdef _DEBUG
		dwCount++;
		if (dwCount > 2)
		{
			ECHO_DEBUGPRINTF(("CDaffyDuck::AddDoubleZero - shouldn't search back more "
									"than two entries!\n"));
			ECHO_DEBUGBREAK();
		}
#endif

		dwEntry = m_dwHead - 1;
		if (dwEntry >= 0x80000000)
			dwEntry += MAX_ENTRIES;		// circular buffer wrap

		if (m_Mappings[dwEntry].dwNumEntries != 0)
			break;


				
	} while (dwEntry != m_dwTail);

	//
	// Quit if this entry already has a double zero
	//
	ASSERT(m_Mappings[dwEntry].dwNumEntries <= 2);

	if (2 == m_Mappings[dwEntry].dwNumEntries)
		return;
		
	//
	// Add the double zero
	//
	DWORD	dwNext;
	
	dwNext = dwEntry + 1;
	if (dwNext >= MAX_ENTRIES)
		dwNext -= MAX_ENTRIES;

	m_DuckEntries[dwNext].PhysAddr 	= 0;
	m_DuckEntries[dwNext].dwSize 		= 0;

	m_Mappings[dwEntry].dwNumEntries += 1;
	
	m_dwCount++;
	
	m_dwHead = dwEntry + m_Mappings[dwEntry].dwNumEntries;
	if (m_dwHead >= MAX_ENTRIES)
		m_dwHead -= MAX_ENTRIES;
	
	//
	// Tell the DSP about it
	//	
	m_pDspCommObject->AddBuffer( m_wPipeIndex );	
	
	//ECHO_DEBUGPRINTF(("Added double zero\n"));
		
}	// AddDoubleZero


//===========================================================================
//
//	Wrap
// 
// Put a "next PLE" pointer at the end of the duck to make the DSP
// wrap around to the start; this creates a circular buffer.
//
//===========================================================================

void CDaffyDuck::Wrap()
{
	ASSERT(m_dwCount != MAX_ENTRIES);
	
	//
	// Put in the address of the start of the duck entries
	// and a count of zero; a count of zero tells the DSP 
	// "Go look again for a duck entry at this address"
	//	
	m_DuckEntries[m_dwHead].PhysAddr		= SWAP( m_dwDuckEntriesPhys );
	m_DuckEntries[m_dwHead].dwSize		= 0;

	m_Mappings[m_dwHead].dwNumEntries 	= 1;
	
	m_dwHead++;
	if (m_dwHead >= MAX_ENTRIES)
		m_dwHead -= MAX_ENTRIES;
	
	m_dwCount++;
	
	m_fWrapped = TRUE;
	
}	// Wrap


//===========================================================================
//
// ReleaseUsedMapping
//
// Find a mapping that's been consumed and return it
//
//===========================================================================

ECHOSTATUS CDaffyDuck::ReleaseUsedMapping
(
	ULONGLONG 	ullDmaPos,
	DWORD			&dwTag
)
{

#ifdef INTERGRITY_CHECK
	CheckIntegrity();	
#endif

	//
	// See if there are any mappings and if the oldest mapping has
	// been consumed by the DSP.
	//
	if ((0 != m_dwCount) && (ullDmaPos >= m_Mappings[m_dwTail].ullEndPos))
	{
		DWORD dwNumEntries;
	
		//
		// Time to release the mapping at the tail
		//
		dwTag = m_Mappings[m_dwTail].dwTag;
		
		//
		// Remove the mapping from the circular buffer,
		// taking dwNumEntries into consideration 
		//
		dwNumEntries = m_Mappings[m_dwTail].dwNumEntries;
		m_Mappings[m_dwTail].dwNumEntries = 0;
		ASSERT(m_dwCount >= dwNumEntries);
		
		m_dwTail += dwNumEntries;
		if (m_dwTail >= MAX_ENTRIES)
			m_dwTail -= MAX_ENTRIES;
			
		m_dwCount -= dwNumEntries;

#ifdef INTEGRITY_CHECK
		CheckIntegrity();
#endif	

#if 0
//#ifdef _DEBUG
		ULONGLONG ullRemaining;
		
		ullRemaining = m_ullLastEndPos - ullDmaPos;

		ECHO_DEBUGPRINTF(("Released mapping for pipe index %d - bytes remaining %I64x\n",
								m_wPipeIndex,ullRemaining));
#endif								

		return ECHOSTATUS_OK;
	}

	//
	// No bananas today, thanks.
	//
	return ECHOSTATUS_NO_DATA;
		
}	// ReleaseUsedMappings


//===========================================================================
//
// RevokeMappings
//
// Returns the number actually revoked
//
//===========================================================================

DWORD CDaffyDuck::RevokeMappings(DWORD dwFirstTag,DWORD dwLastTag)
{
	MAPPING 		*TempMappings = NULL;
	DUCKENTRY	*TempEntries = NULL;
	DWORD			dwNumTemp;
	DWORD			dwOffset;
	DWORD			dwNumRevoked = 0;
	
	//
	// Allocate the arrays
	//
	OsAllocateNonPaged(	sizeof(MAPPING) * MAX_ENTRIES,	// fixme make this part of the duck
							  	(void **) &TempMappings );
	OsAllocateNonPaged(	sizeof(DUCKENTRY) * MAX_ENTRIES,						  
								(void **) &TempEntries );
	if (	(NULL == TempMappings) ||
			(NULL == TempEntries))
	{
		goto exit;		
	}
	
	//
	// Tweak the end position so it's set OK
	// when the duck is rebuilt
	//
	SetLastEndPos(GetStartPos());
	

	//----------------------------------------------------------------------
	//
	// The tags are 32 bit counters, so it is possible that they will
	// wrap around (that is, dwLastTag may be less than dwFirstTag).  If the
	// tags have wrapped, use an offset so the compares work correctly.
	//
	//----------------------------------------------------------------------

	dwOffset = 0;	
	if (dwLastTag < dwFirstTag)
	{
		dwOffset = 0x80000000;
		
		dwLastTag -= dwOffset;
		dwFirstTag -= dwOffset;
	}
	
	//----------------------------------------------------------------------
	//
	// Empty out the duck into the temp arrays, throwing away
	// the revoked mappings.
	//
	//----------------------------------------------------------------------
	
	ECHOSTATUS 	Status;
	DWORD			dwAdjustedTag;

	dwNumTemp = 0;	
	do	
	{
		Status = GetTail(	TempEntries + dwNumTemp,
								TempMappings + dwNumTemp);
		if (ECHOSTATUS_OK == Status)
		{
			dwAdjustedTag = TempMappings[dwNumTemp].dwTag - dwOffset;

			if ((dwFirstTag <= dwAdjustedTag) && 
				 (dwAdjustedTag <= dwLastTag))
			{
				//
				// Revoke this tag
				//
				dwNumRevoked++;
			}
			else
			{
				//
				// Save this tag
				//
				dwNumTemp++;						
			}
			
		}
	} while (ECHOSTATUS_OK == Status);
	

	//----------------------------------------------------------------------
	//
	// Add all the saved mappings back into this duck.
	//
	//----------------------------------------------------------------------
		
	DWORD i,dwDummy;
	
	//
	// Partially reset this duck
	//
	m_dwHead = 0;
	m_dwTail = 0;
	m_dwCount = 0;

	//
	// Put all the non-revoked mappings back
	//	
	for (i = 0; i < dwNumTemp; i++)
	{
		AddMapping(	TempEntries[i].PhysAddr,
						TempEntries[i].dwSize,
						TempMappings[i].dwTag,
						TempMappings[i].dwNumEntries - 1,
						dwDummy);
	}
	
	ECHO_DEBUGPRINTF(("CDaffyDuck::RevokeMappings for pipe index %d - m_dwHead %ld  m_dwTail %ld  m_dwCount %ld\n",
							m_wPipeIndex,m_dwHead,m_dwTail,m_dwCount));

exit:
	if (TempEntries) OsFreeNonPaged(TempEntries);
	if (TempMappings) OsFreeNonPaged(TempMappings);	
	
	return dwNumRevoked;	

}	// RevokeMappings


//===========================================================================
//
// GetTail
//
// Unconditionally returns the mapping information from the tail of the 
// circular buffer.
//
//===========================================================================

ECHOSTATUS CDaffyDuck::GetTail(DUCKENTRY *pDuckEntry,MAPPING *pMapping)
{
	//
	// Punt if there are no entries in the circular buffer
	//
	if (0 == m_dwCount)
		return ECHOSTATUS_NO_DATA;
		
	//
	// Copy the data
	//		
	OsCopyMemory(	pDuckEntry,
						m_DuckEntries + m_dwTail,
						sizeof(DUCKENTRY));
	OsCopyMemory(	pMapping,
						m_Mappings + m_dwTail,
						sizeof(MAPPING));
	
	//
	// Buffer housekeeping
	//
	DWORD dwNumEntries = m_Mappings[m_dwTail].dwNumEntries;
	m_Mappings[m_dwTail].dwNumEntries = 0;	
	
	ASSERT(m_dwCount >= dwNumEntries);

	m_dwTail += dwNumEntries;
	if (m_dwTail >= MAX_ENTRIES)
		m_dwTail -= MAX_ENTRIES;
		
	m_dwCount -= dwNumEntries;
	
#ifdef INTEGRITY_CHECK
	CheckIntegrity();
#endif	
	
	return ECHOSTATUS_OK;
	
}	// GetTail


//===========================================================================
//
// CheckIntegrity
//
// Debug code - makes sure that the buffer count, head, and tail all agree
//
//===========================================================================

#ifdef INTEGRITY_CHECK

void CDaffyDuck::CheckIntegrity()
{
	DWORD dwDiff;
	
	dwDiff = m_dwHead - m_dwTail;
	if (dwDiff > 0x80000000)
		dwDiff += MAX_ENTRIES;
	
	if ((0 == dwDiff) && (MAX_ENTRIES == m_dwCount))
		return;
	
	if (dwDiff != m_dwCount)
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck integrity check fail!  m_dwHead %ld  m_dwTail %ld  "
								"m_dwCount %ld  m_Mappings[m_dwHead].dwNumEntries %ld\n",
								m_dwHead,m_dwTail,m_dwCount,m_Mappings[m_dwHead].dwNumEntries));
		ECHO_DEBUGBREAK();		
	}
	
}	// CheckIntegrity

#endif // INTEGRITY_CHECK


// *** CDaffyDuck.cpp ***
