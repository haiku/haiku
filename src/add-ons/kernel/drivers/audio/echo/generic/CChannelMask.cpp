// ****************************************************************************
//
//		CChannelMask.cpp
//
//		Implementation file for the CChannelMask class.
//
// 	CChannelMask is a handy way to specify a group of pipes simultaneously.
//		It should really be called "CPipeMask", but the class name predates
//		the term "pipe".
//
// 	Since these masks are sometimes passed to the DSP, they must be kept in 
//	 	little-endian format; the class does this for you.
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

#include "CEchoGals.h"


/****************************************************************************

	CChannelMask

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CChannelMask::CChannelMask()
{

	Clear();

}	// CChannelMask::CChannelMask()


//===========================================================================
//
//	SetMask, SetOutMask, and SetInMask all allow you to just set 
// the masks directly.
//
//===========================================================================

void CChannelMask::SetMask( CH_MASK OutMask, CH_MASK InMask, int nOutputs )
{

	m_Mask = OutMask;
	m_Mask |= InMask << nOutputs;

}	// void CChannelMask::SetMask( ... )


void CChannelMask::SetOutMask( CH_MASK OutMask, int nOutputs )
{

	m_Mask &= ((CH_MASK) -1) << nOutputs;
	m_Mask |= OutMask;

}	// void CChannelMask::SetOutMask( CH_MASK OutMask, int nOutputs )


void CChannelMask::SetInMask( CH_MASK InMask, int nOutputs )
{
	m_Mask &= ~( (CH_MASK) -1 << nOutputs );
	m_Mask |= InMask << nOutputs;
}	// void CChannelMask::SetInMask( CH_MASK InMask, int nOutputs )


//===========================================================================
//
//	Retrieve an output bit mask and an input bitmask.
//
//===========================================================================

void CChannelMask::GetMask( CH_MASK & OutMask, CH_MASK & InMask, int nOutputs )
{
	OutMask = GetOutMask( nOutputs );
	InMask = GetInMask( nOutputs );
}	// void CChannelMask::GetMask( ... )

CH_MASK CChannelMask::GetOutMask( int nOutputs )
{
	return  m_Mask & ~( (CH_MASK) -1 << nOutputs );
}	// CH_MASK CChannelMask::GetOutMask( int nOutputs )

CH_MASK CChannelMask::GetInMask( int nOutputs )
{
	return  m_Mask >> nOutputs;
}	// CH_MASK CChannelMask::GetIntMask( int nOutputs )


//===========================================================================
//
// IsEmpty returns TRUE if mask has no bits set
//
//===========================================================================

BOOL CChannelMask::IsEmpty()
{
	if (0 != m_Mask)
		return FALSE;

	return TRUE;

}	// void CChannelMask::IsEmpty()

	
//===========================================================================
//
// Call SetIndexInMask and ClearIndexInMask to set or clear a single bit.
//
//===========================================================================

// Set driver channel index into DSP mask format
void CChannelMask::SetIndexInMask( WORD wPipeIndex )
{
	m_Mask |= 1 << wPipeIndex;

}	// void CChannelMask::SetIndexInMask( WORD wPipeIndex )
	
	
// Clear driver channel index into DSP mask format
void CChannelMask::ClearIndexInMask( WORD wPipeIndex )
{

	m_Mask &= ~((CH_MASK) 1 << wPipeIndex);
	
}	// void CChannelMask::ClearIndexInMask( WORD wPipeIndex )


//===========================================================================
//
// Use GetIndexFromMask	to search the mask for bits that are set.
//
// The search starts at the bit specified by wStartPipeIndex and returns
// the pipe index for the first non-zero bit found.
//
//	Returns ECHO_INVALID_CHANNEL if none are found.
//
//===========================================================================

WORD CChannelMask::GetIndexFromMask( WORD wStartPipeIndex )
{
	CH_MASK bit;
	WORD index;
	
	bit = 1 << wStartPipeIndex;
	index = wStartPipeIndex;
	while (bit != 0)
	{
		if (0 != (m_Mask & bit))
			return index;
	
		bit <<= 1;
		index++;
	}

	return( (WORD) ECHO_INVALID_CHANNEL );
	
}		// WORD CChannelMask::GetIndexFromMask( WORD wStartIndex )
	

//===========================================================================
//
// Returns TRUE if the bit specified by the pipe index is set.	
//
//===========================================================================

BOOL CChannelMask::TestIndexInMask( WORD wPipeIndex )
{
	if (0 != (m_Mask & ((CH_MASK) 1 << wPipeIndex)))
		return TRUE;
		
	return FALSE;
}	// BOOL CChannelMask::TestIndexInMask( WORD wPipeIndex )


//===========================================================================
//	
// Clear bits in this mask that are in SrcMask - this is just like 
// operator -=, below. 
//
//===========================================================================

void CChannelMask::ClearMask( CChannelMask SrcMask )
{
	m_Mask &= ~SrcMask.m_Mask;

}	// void CChannelMask::ClearMask( CChannelMask SrcMask )

	
//===========================================================================
//	
//	Clear all channels in this mask
//
//===========================================================================

void CChannelMask::Clear()
{
	m_Mask = 0;
}	// void CChannelMask::Clear()

	
//===========================================================================
//
//	operator +=    Add channels in source mask to this mask
//
//===========================================================================

VOID CChannelMask::operator += (CONST CChannelMask & RVal)
{
	m_Mask |= RVal.m_Mask;

}	// VOID operator += (CONST CChannelMask & RVal)

	
//===========================================================================
//
//	operator -=    Remove channels in source mask from this mask
//
//===========================================================================

VOID CChannelMask::operator -= (CONST CChannelMask & RVal)
{
	ClearMask(RVal);
}	// VOID operator -= (CONST CChannelMask & RVal)


//===========================================================================
//
// Test returns TRUE if any bits in source mask are set in this mask
//
//===========================================================================

BOOL CChannelMask::Test( PCChannelMask pSrcMask )
{
	if (0 != (m_Mask & pSrcMask->m_Mask))
		return TRUE;
	
	return FALSE;	

}	// BOOL CChannelMask::Test( PChannelMask pSrcMask )


//===========================================================================
//
//	IsSubsetOf returns TRUE if all of the channels in TstMask are set in 
// m_Mask.  
//
//	Use to be sure all channels in this instance exist in
//	another instance.
//
//===========================================================================

BOOL CChannelMask::IsSubsetOf
(
	CChannelMask& TstMask
)
{
	if ((m_Mask & TstMask.m_Mask) != TstMask.m_Mask)
		return FALSE;
		
	return TRUE;

}	// BOOL CChannelMask::IsSubsetOf


//===========================================================================
//
//	IsIntersectionOf returns TRUE if TstMask contains at least one of the 
// channels enabled in this instance.
//
//	Use this to find out if any channels in this instance exist in
//	another instance.
//
//===========================================================================

BOOL CChannelMask::IsIntersectionOf
(
	CChannelMask& TstMask
)
{
	if (0 != (m_Mask & TstMask.m_Mask))
		return TRUE;

	return FALSE;

}	// BOOL CChannelMask::IsIntersectionOf


//===========================================================================
//
// Operator == is just what you'd expect - it tells you if one mask is
// the same as another
//
//===========================================================================

BOOLEAN operator == ( CONST CChannelMask &LVal, CONST CChannelMask &RVal )
{
	if (LVal.m_Mask != RVal.m_Mask)
		return FALSE;

	return TRUE;

}	// BOOLEAN operator == ( CONST CChannelMask &LVal, CONST CChannelMask &RVal )


//===========================================================================
//
// Operator = just copies from one mask to another.
//
//===========================================================================

CChannelMask& CChannelMask::operator =(CONST CChannelMask & RVal)
{
	if ( &RVal == this )
		return *this;

	m_Mask = RVal.m_Mask;

	return *this;

}	// CChannelMask& CChannelMask::operator = (CONTS CChannelMask & RVal)


//===========================================================================
//
// Operator & performs a bitwise logical AND
//
//===========================================================================

VOID CChannelMask::operator &= (CONST CChannelMask & RVal)
{
	if ( &RVal == this )
		return;

	m_Mask &= RVal.m_Mask;

}	// VOID CChannelMask::operator &= (CONST CChannelMask & RVal)


//===========================================================================
//
// Operator & performs a bitwise logical OR
//
//===========================================================================

VOID CChannelMask::operator |= (CONST CChannelMask & RVal)
{
	if ( &RVal == this )
		return;

	m_Mask |= RVal.m_Mask;

}	// VOID CChannelMask::operator |= (CONST CChannelMask & RVal)


//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CChannelMask::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CChannelMask::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CChannelMask::operator new( size_t Size )


VOID  CChannelMask::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CChannelMask::operator delete memory free failed\n"));
	}
}	// VOID  CChannelMask::operator delete( PVOID pVoid )




/****************************************************************************

	CChMaskDsp

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CChMaskDsp::CChMaskDsp()
{

	Clear();

}	// CChMaskDsp::CChMaskDsp()


//===========================================================================
//
// IsEmpty returns TRUE if mask has no bits set
//
//===========================================================================

BOOL CChMaskDsp::IsEmpty()
{
	if (0 != m_Mask)
			return FALSE;

	return TRUE;

}	// void CChMaskDsp::IsEmpty()

	
//===========================================================================
//
// Call SetIndexInMask and ClearIndexInMask to set or clear a single bit.
//
//===========================================================================

// Set driver channel index into DSP mask format
void CChMaskDsp::SetIndexInMask( WORD wPipeIndex )
{
	CH_MASK_DSP bit,temp;
	
	temp = SWAP( m_Mask );
	bit = 1 << wPipeIndex;
	temp |= bit;
	m_Mask = SWAP( temp );

}	// void CChMaskDsp::SetIndexInMask( WORD wPipeIndex )
	
	
// Clear driver channel index into DSP mask format
void CChMaskDsp::ClearIndexInMask( WORD wPipeIndex )
{
	CH_MASK_DSP bit,temp;
	
	temp = SWAP( m_Mask );
	bit = 1 << wPipeIndex;
	temp &= ~bit;
	m_Mask = SWAP( temp );
		
}	// void CChMaskDsp::SetIndexInMask( WORD wPipeIndex )
	
	
//===========================================================================
//
// Returns TRUE if the bit specified by the pipe index is set.	
//
//===========================================================================

BOOL CChMaskDsp::TestIndexInMask( WORD wPipeIndex )
{
	CH_MASK_DSP temp,bit;
	
	temp = SWAP(m_Mask);
	bit = 1 << wPipeIndex;
	if (0 != (temp & bit))
		return TRUE;
		
	return FALSE;
	
}	// BOOL CChMaskDsp::TestIndexInMask( WORD wPipeIndex )

	
//===========================================================================
//	
//	Clear all channels in this mask
//
//===========================================================================

void CChMaskDsp::Clear()
{
	m_Mask = 0;
}	// void CChMaskDsp::Clear()

	
//===========================================================================
//
// Operator = just copies from one mask to another.
//
//===========================================================================

CChMaskDsp& CChMaskDsp::operator =(CONST CChannelMask & RVal)
{
	m_Mask = SWAP(RVal.m_Mask);

	return *this;

}	// CChMaskDsp& CChMaskDsp::operator =(CONST CChannelMask & RVal)


//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CChMaskDsp::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CChMaskDsp::operator new memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CChMaskDsp::operator new( size_t Size )


VOID  CChMaskDsp::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CChMaskDsp::operator delete memory free failed\n"));
	}
}	// VOID  CChMaskDsp::operator delete( PVOID pVoid )


// ChannelMask.cpp
