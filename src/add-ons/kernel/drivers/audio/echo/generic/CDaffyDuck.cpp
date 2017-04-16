// ****************************************************************************
//
//		CDaffyDuck.CPP
//
//		Implementation file for the CDaffyDuck class.
//		Set editor tabs to 3 for your viewing pleasure.
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
//---------------------------------------------------------------------------
//
// The head pointer tracks the next free slot in the circular buffers
// The tail pointer tracks the oldest mapping.
//
// Fixme add integrity checks for all functions
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
	PCOsSupport 	pOsSupport
)
{
	//
	//	Stash stuff
	// 
	m_pOsSupport = pOsSupport;
	
}	// CDaffyDuck::CDaffyDuck()


//===========================================================================
//
// Destructor
//
//===========================================================================

CDaffyDuck::~CDaffyDuck()
{

	if (NULL != m_pDuckPage)
		m_pOsSupport->FreePhysPageBlock( PAGE_SIZE, m_pDuckPage);
	
}	// CDaffyDuck::~CDaffyDuck()




/****************************************************************************

	Setup and initialization

 ****************************************************************************/

//===========================================================================
//
// Reset - resets the mapping and duck entry circular buffers
//
//===========================================================================

void CDaffyDuck::Reset()
{
	//
	//	Zero stuff
	// 
	OsZeroMemory(m_Mappings,sizeof(m_Mappings));
	
	m_dwHead = 0;
	m_dwTail = 0;
	m_dwCount = 0;
	m_ullLastEndPos = 0;

	//
	// Set all duck entries to "end of list" except for the last one
	//
	DWORD i;
	
	for (i = 0; i < MAX_ENTRIES; i++)
	{
		m_DuckEntries[i].PhysAddr = 0;
		m_DuckEntries[i].dwSize = 0xffffffff;
	}

	//
	// Put the physical address of the duck at the end of 
	// the m_DuckEntries array so the DSP will wrap around
	// to the start of the duck.  
	//
													
	m_DuckEntries[MAX_ENTRIES].PhysAddr = SWAP( m_dwDuckEntriesPhys );
	m_DuckEntries[MAX_ENTRIES].dwSize = 0;

}	// Reset


//===========================================================================
//
// ResetStartPos - Takes the current list and re-calculates the
// DMA end position for each mapping, starting at DMA position zero.
//
//===========================================================================

void CDaffyDuck::ResetStartPos()
{
	DWORD dwRemaining,dwIndex;

	m_ullLastEndPos = 0L;
	
	//
	// Re-calculate the end positions
	//
	dwRemaining = m_dwCount;
	dwIndex = m_dwTail;
	while (0 != dwRemaining)
	{
		if (	( 0 != m_DuckEntries[ dwIndex ].dwSize) &&
				( 0 != m_DuckEntries[ dwIndex ].PhysAddr ) )
		{
			m_Mappings[dwIndex].ullEndPos = 
				m_ullLastEndPos + SWAP( m_DuckEntries[ dwIndex ].dwSize );
	
			m_ullLastEndPos = m_Mappings[ dwIndex ].ullEndPos;
		}
		else
		{
			m_Mappings[dwIndex].ullEndPos = m_ullLastEndPos;
		}
			
		dwIndex++;
		dwIndex &= ENTRY_INDEX_MASK;
			
		dwRemaining--;
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
	NUINT 		Tag,
	DWORD			dwInterrupt,
	DWORD			&dwNumFreeEntries
)
{
#ifdef INTEGRITY_CHECK
	CheckIntegrity();
#endif	

	//
	// There must always be at least one free entry for the "end of list" 
	// entry.  dwInterrupt may be non-zero, so make sure that there's enough
	// room for two more entries
	//
	if ((MAX_ENTRIES - m_dwCount) < 3)
	{
		ECHO_DEBUGPRINTF(("AddMapping - duck is full\n"));
		return ECHOSTATUS_DUCK_FULL;
	}
	
	//
	//	At least two slots are available in the circular 
	// buffer, so it's OK to add either a regular mapping or 
	// a mapping with a double zero
	//
	m_DuckEntries[m_dwHead].PhysAddr = SWAP( dwPhysAddr );
	m_DuckEntries[m_dwHead].dwSize 	= SWAP( dwBytes );
	
	m_Mappings[m_dwHead].Tag			= Tag;
	m_Mappings[m_dwHead].ullEndPos	= m_ullLastEndPos + dwBytes;
	
	m_ullLastEndPos = m_Mappings[m_dwHead].ullEndPos;
	
	//
	// If the caller wants an interrupt after this mapping, then
	// dwInterrupt will be non-zero
	//
	if (dwInterrupt)
	{
		DWORD dwNext;

		//
		// Put in the double zero so the DSP will
		// generate an interrupt
		//
		dwNext = m_dwHead + 1;
		dwNext &= ENTRY_INDEX_MASK;
	
		m_DuckEntries[dwNext].PhysAddr 	= 0;	// no need to swap zero!
		m_DuckEntries[dwNext].dwSize 		= 0;
		
		m_Mappings[dwNext].ullEndPos = m_ullLastEndPos;

		m_dwHead += 2;
		m_dwCount += 2;
	}
	else
	{
		m_dwHead++;
		m_dwCount++;
	}
	
	//
	// Wrap the head index
	//		
	m_dwHead &=	ENTRY_INDEX_MASK;

	//
	// Return value to the caller
	//	
	dwNumFreeEntries = MAX_ENTRIES - m_dwCount;
	
//#ifdef _DEBUG
#if 0
	DWORD hi,lo;
	
	hi = (DWORD) (m_ullLastEndPos >> 32);
	lo = (DWORD) (m_ullLastEndPos & 0xffffffffL);
	
	ECHO_DEBUGPRINTF(("Added tag %ld, end pos 0x%08lx%08lx (int %ld)\n",Tag,hi,lo,dwInterrupt));

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
// IRQ. 
// 
//===========================================================================

ECHOSTATUS CDaffyDuck::AddDoubleZero()
{
	//
	// There must always be at least one free entry for the "end of list" 
	// entry. 
	//
	if ((MAX_ENTRIES - m_dwCount) < 2)
	{
		ECHO_DEBUGPRINTF(("AddDoubleZero - duck is full\n"));
		return ECHOSTATUS_DUCK_FULL;
	}
	
	//ECHO_DEBUGPRINTF(("CDaffyDuck::AddDoubleZero  m_dwCount %ld\n",m_dwCount));	

	//
	// Add the double zero
	//
	m_DuckEntries[m_dwHead].PhysAddr 	= 0;
	m_DuckEntries[m_dwHead].dwSize 		= 0;
	m_Mappings[m_dwHead].ullEndPos		= m_ullLastEndPos;
	
	//
	// Housekeeping
	//		
	m_dwHead++;
	m_dwHead &=	ENTRY_INDEX_MASK;

	m_dwCount++;
	
	return ECHOSTATUS_OK;
		
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
	ECHO_ASSERT(m_dwCount != MAX_ENTRIES);
	
	//
	// Put in the address of the start of the duck entries
	// and a count of zero; a count of zero tells the DSP 
	// "Go look again for a duck entry at this address"
	//	
	m_DuckEntries[m_dwHead].PhysAddr		= SWAP( m_dwDuckEntriesPhys );
	m_DuckEntries[m_dwHead].dwSize		= 0;

	m_dwHead++;
	m_dwHead &= ENTRY_INDEX_MASK;
	
	m_dwCount++;
	
	m_fWrapped = TRUE;
	
}	// Wrap



//===========================================================================
//
// ReleaseUsedMappings
//
// Find all the mapping that've been consumed and return a list of tags
//
// Return value is the number of tags written to the array
//
//===========================================================================

DWORD CDaffyDuck::ReleaseUsedMappings
(
	ULONGLONG 	ullDmaPos,
	NUINT 		*Tags,		// an array of tags
	DWORD			dwMaxTags	// the max number of tags in the array
)
{
	DWORD dwTempAddr,dwTempSize;
	NUINT Tag;
	DWORD dwTagsFree;
	
	dwTagsFree = dwMaxTags;
	while ( (0 != m_dwCount) && (0 != dwTagsFree) )
	{
		//
		// Get the data from the tail
		//	
		Tag = m_Mappings[m_dwTail].Tag;
		dwTempAddr = SWAP( m_DuckEntries[m_dwTail].PhysAddr );
		dwTempSize = SWAP( m_DuckEntries[m_dwTail].dwSize );
		
		//
		// Is this an audio data mapping?
		//
		if ( (0 != dwTempAddr) && (0 != dwTempSize) )
		{
			//
			// Is this audio data mapping done?
			//
			if ( ullDmaPos < m_Mappings[m_dwTail].ullEndPos )
				break;
				
			//
			// This one's done
			//
			*Tags = Tag;
			Tags++;
			dwTagsFree--;
				
			EjectTail();
		}
		else
		{
			//
			// Is this non-audio data mapping done?
			//
			if ( ullDmaPos <= m_Mappings[m_dwTail].ullEndPos )
				break;
				
			//
			// Pop it
			// 
			EjectTail();
		}
	}
	
	//
	// Return the number written
	//
	return dwMaxTags - dwTagsFree;

}	// ReleaseUsedMappings


//===========================================================================
//
// RevokeMappings
//
// Returns the number actually revoked
//
//===========================================================================

DWORD CDaffyDuck::RevokeMappings(NUINT FirstTag,NUINT LastTag)
{
	NUINT	Offset;
	DWORD	dwNumRevoked;
	
	dwNumRevoked = 0;


	//----------------------------------------------------------------------
	//
	// The tags may be 32 bit counters, so it is possible that they will
	// wrap around (that is, dwLastTag may be less than dwFirstTag).  If the
	// tags have wrapped, use an offset so the compares work correctly.
	//
	//----------------------------------------------------------------------

	Offset = 0;	
	if (LastTag < FirstTag)
	{
		Offset = LastTag;
		
		LastTag -= Offset;
		FirstTag -= Offset;
	}


	//----------------------------------------------------------------------
	//
	// Go through the list and revoke stuff
	//
	//----------------------------------------------------------------------
	
	DWORD dwCount;
	DWORD dwCurrentIndex;
	DWORD dwNextIndex;
	NUINT AdjustedTag;
	
	dwCurrentIndex = m_dwTail;
	dwCount = m_dwCount;
	while (dwCount != 0)
	{
		//
		// Get info for this mapping
		//
		AdjustedTag = m_Mappings[dwCurrentIndex].Tag - Offset;
		
		//
		// Only check this mapping if it contains audio data
		//
		if (	(0 != m_DuckEntries[dwCurrentIndex].PhysAddr) &&
				(0 != m_DuckEntries[dwCurrentIndex].dwSize) )
		{
			//
			// See if the current mapping needs to be revoked
			//
			if ((FirstTag <= AdjustedTag) && 
				 (AdjustedTag <= LastTag))
			{
				//
				// Revoke this tag
				//
				dwNumRevoked++;
				
				//
				// Change this duck into a duck entry pointer; the DSP
				// will see that the size is zero and re-fetch the duck entry
				// from the address specified in PhysAddr.
				//				
				dwNextIndex = dwCurrentIndex + 1;
				dwNextIndex &= ENTRY_INDEX_MASK;
				
				m_DuckEntries[dwCurrentIndex].PhysAddr = 
					m_dwDuckEntriesPhys + (dwNextIndex * sizeof(DUCKENTRY) );
				m_DuckEntries[dwCurrentIndex].dwSize = 0;
				
			}			
		}
		
		dwCurrentIndex++;
		dwCurrentIndex &= ENTRY_INDEX_MASK;

		dwCount--;
	}	
			

	//----------------------------------------------------------------------
	//
	// If any mappings were revoked, do various housekeeping tasks
	//
	//----------------------------------------------------------------------
	
	if (0 != dwNumRevoked)
	{
		CleanUpTail();
		ResetStartPos();
	}

	return dwNumRevoked;

}	// RevokeMappings



//===========================================================================
//
// CleanUpTail
//
// Removes any non-audio mappings from the tail of the list; stops
// removing if it finds an audio mapping
//
//===========================================================================

void CDaffyDuck::CleanUpTail()
{
	while (0 != m_dwCount)
	{
		//
		// Quit the loop at the first audio data entry
		//
		if (	(0 != m_DuckEntries[ m_dwTail ].PhysAddr) &&
				(0 != m_DuckEntries[ m_dwTail ].dwSize) )
			break;

		//
		// Pop goes the weasel
		//
		EjectTail();
	}

}	// CleanUpTail




//===========================================================================
//
// EjectTail
//
// Removes a single mapping from the tail of the list
//
//===========================================================================

void CDaffyDuck::EjectTail()
{
#ifdef _DEBUG
	if (0 == m_dwCount)
	{
		ECHO_DEBUGPRINTF(("EjectTail called with zero count!\n"));
		ECHO_DEBUGBREAK();
		return;
	}
#endif

	//
	//	Mark this entry with the "end of list" values
	// 
	m_DuckEntries[ m_dwTail ].PhysAddr = 0;
	m_DuckEntries[ m_dwTail ].dwSize = 0xffffffff;

	//
	// Move the tail forward and decrement the count
	//	
	m_dwTail++;
	m_dwTail &= ENTRY_INDEX_MASK;

	m_dwCount--;
	
} // EjectTail



//===========================================================================
//
// Adjusts the duck so that DMA will start from a given position; useful
// when resuming from pause
//
//===========================================================================

void CDaffyDuck::AdjustStartPos(ULONGLONG ullPos)
{
	DWORD dwCount,dwIndex;
	ULONGLONG ullMapStartPos;
	DWORD dwPhysAddr;
	DWORD dwSize;
			
	
	dwCount = m_dwCount;
	dwIndex = m_dwTail;
	while (0 != dwCount)
	{
		//
		// Check DMA pos
		//
		if (ullPos >= m_Mappings[dwIndex].ullEndPos)
			break;
		
		dwSize = SWAP(m_DuckEntries[dwIndex].dwSize);
		ullMapStartPos = m_Mappings[dwIndex].ullEndPos - dwSize;
		if (ullPos >= ullMapStartPos)
		{
			dwPhysAddr = SWAP(m_DuckEntries[dwIndex].PhysAddr);
			if ( (0 != dwPhysAddr) && (0 != dwSize) )
			{
				DWORD dwDelta;
				
				dwDelta = (DWORD) (m_Mappings[dwIndex].ullEndPos - ullPos);
				dwPhysAddr += dwDelta;
				dwSize -= dwDelta;
				
				m_DuckEntries[dwIndex].PhysAddr = SWAP(dwPhysAddr);						
				m_DuckEntries[dwIndex].dwSize = SWAP(dwSize);
				break;
			}
		}
			
		dwCount--;
		dwIndex++;
		dwIndex &= ENTRY_INDEX_MASK;	
	}

}


//===========================================================================
//
// GetPhysStartAddr
//
// This returns the physical address of the start of the scatter-gather 
// list; used to tell the DSP where to start looking for duck entries.
//
//===========================================================================

DWORD CDaffyDuck::GetPhysStartAddr()
{
	return m_dwDuckEntriesPhys + (m_dwTail * sizeof(DUCKENTRY));
}


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
	DWORD dwDiff,dwCount,dwTemp,dwSum;
	
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
	
	dwTemp = m_dwTail;
	dwCount = m_dwCount;
	dwSum = 0;
	while (dwCount)
	{
		dwSum += m_Mappings[dwTemp].dwNumEntries;
		
		dwCount--;
		dwTemp++;
		dwTemp &= ENTRY_INDEX_MASK;
	}
	
	if (dwSum != m_dwCount)
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck integrity check fail!  dwSum %ld  m_dwCount %ld\n",
								dwSum,m_dwCount));
		ECHO_DEBUGBREAK();
	}
	
}	// CheckIntegrity

#endif // INTEGRITY_CHECK


VOID CDaffyDuck::DbgDump()
{
	ECHO_DEBUGPRINTF(("duck list starts at virt %p, phys %08x\n",m_DuckEntries,m_dwDuckEntriesPhys));
	ECHO_DEBUGPRINTF(("count %d  head %d  tail %d\n",m_dwCount,m_dwHead,m_dwTail));
	ECHO_DEBUGPRINTF(("Head phys %08x   tail phys %08x\n",
				(m_dwHead * sizeof(DUCKENTRY)) + m_dwDuckEntriesPhys,
				(m_dwTail * sizeof(DUCKENTRY)) + m_dwDuckEntriesPhys));
				
	DWORD idx,count;
	
	idx = m_dwTail;
	count = m_dwCount;
	while (count != 0)
	{
		ECHO_DEBUGPRINTF(("\t%08x :  %08x  %08x\n",(idx * sizeof(DUCKENTRY)) + m_dwDuckEntriesPhys,
														m_DuckEntries[idx].dwSize,m_DuckEntries[idx].PhysAddr));
		count--;
		idx ++;
		idx &= ENTRY_INDEX_MASK;
	}
}

//===========================================================================
//
// This function is used to create a CDaffyDuck object to 
// manage a scatter-gather list for a newly opened pipe.  Call
// this instead of using "new CDaffyDuck" directly.
//
//===========================================================================

CDaffyDuck * CDaffyDuck::MakeDaffyDuck(COsSupport *pOsSupport)
{
	ECHOSTATUS 	Status = ECHOSTATUS_OK;
	CDaffyDuck 	*pDuck;
	
	pDuck = new CDaffyDuck(	pOsSupport );
	if (NULL == pDuck)
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck::CDaffyDuck - duck entry malloc failed\n"));
		return NULL;
	}
		
	//
	// Allocate the page for the duck entries
	//
	DWORD dwSegmentSize;
	PHYS_ADDR PhysAddr;
	PPAGE_BLOCK pPageBlock;
	
	Status = pOsSupport->AllocPhysPageBlock( PAGE_SIZE, pPageBlock);
	if (ECHOSTATUS_OK != Status)
	{
		ECHO_DEBUGPRINTF(("CDaffyDuck::CDaffyDuck - duck entry page block malloc failed\n"));
		delete pDuck;
		return NULL;
	}
	
	pDuck->m_pDuckPage = pPageBlock;
	
	pDuck->m_DuckEntries = (DUCKENTRY *) pOsSupport->GetPageBlockVirtAddress( pPageBlock );
	pOsSupport->GetPageBlockPhysSegment(pPageBlock,
													0,
													PhysAddr,
													dwSegmentSize);

	pDuck->m_dwDuckEntriesPhys = PhysAddr;

	//
	// Finish initializing
	// 
	pDuck->Reset();

	return pDuck;
		
}	// MakeDaffyDuck



// *** CDaffyDuck.cpp ***

