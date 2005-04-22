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

	m_MaskRegs[ 0 ] = SWAP( OutMask );
	m_MaskRegs[ 0 ] |= SWAP( (CH_MASK) ( InMask << nOutputs ) );
	m_MaskRegs[ 1 ] = SWAP( (CH_MASK) ( InMask >> ( CH_MASK_BITS - nOutputs ) ) );

}	// void CChannelMask::SetMask( ... )


void CChannelMask::SetOutMask( CH_MASK OutMask, int nOutputs )
{

	m_MaskRegs[ 0 ] &= SWAP( (CH_MASK) ( (CH_MASK) -1 << nOutputs ) );
	m_MaskRegs[ 0 ] |= SWAP( OutMask );

}	// void CChannelMask::SetOutMask( CH_MASK OutMask, int nOutputs )


void CChannelMask::SetInMask( CH_MASK InMask, int nOutputs )
{
	m_MaskRegs[ 0 ] &= SWAP( (CH_MASK) (~( (CH_MASK) -1 << nOutputs ) ) );
	m_MaskRegs[ 0 ] |= SWAP( (CH_MASK) ( InMask << nOutputs ) );
	m_MaskRegs[ 1 ] = SWAP( (CH_MASK) ( InMask >> ( CH_MASK_BITS - nOutputs ) ) );
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
	return  SWAP( m_MaskRegs[ 0 ] ) & ~( (CH_MASK) -1 << nOutputs );
}	// CH_MASK CChannelMask::GetOutMask( int nOutputs )

CH_MASK CChannelMask::GetInMask( int nOutputs )
{
	return ( SWAP( m_MaskRegs[ 0 ] ) >> nOutputs ) |
			  ( SWAP( m_MaskRegs[ 1 ] ) << ( CH_MASK_BITS - nOutputs ) );
}	// CH_MASK CChannelMask::GetIntMask( int nOutputs )


//===========================================================================
//
// IsEmpty returns TRUE if mask has no bits set
//
//===========================================================================

BOOL CChannelMask::IsEmpty()
{
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		if ( 0 != m_MaskRegs[ i ] )
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

	m_MaskRegs[ wPipeIndex / CH_MASK_BITS ] |=
		  SWAP( (CH_MASK) (( (CH_MASK) 1 ) << ( wPipeIndex % CH_MASK_BITS ) ) );

}	// void CChannelMask::SetIndexInMask( WORD wPipeIndex )
	
	
// Clear driver channel index into DSP mask format
void CChannelMask::ClearIndexInMask( WORD wPipeIndex )
{

	m_MaskRegs[ wPipeIndex / CH_MASK_BITS ] &=
			SWAP( (CH_MASK)(~( ( (CH_MASK) 1 ) << ( wPipeIndex % CH_MASK_BITS ) ) ) );

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
	WORD		wIndex = wStartPipeIndex % CH_MASK_BITS;
	WORD		wCt	 = wStartPipeIndex / CH_MASK_BITS;
	CH_MASK	Mask;

	for ( ; wCt < CH_MASK_SZ; wCt++ )
	{
		Mask = SWAP( m_MaskRegs[ wCt ] );
		if ( 0 != Mask )
		{
			while( !( Mask & (1<<wIndex) ) &&
					 wIndex < CH_MASK_BITS )
				wIndex++;
			if ( wIndex < CH_MASK_BITS )	
				return( wIndex + ( wCt * CH_MASK_BITS ) );	
		}
		wIndex = 0;
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
	if ( SWAP( m_MaskRegs[ wPipeIndex / CH_MASK_BITS ] ) &
			  (CH_MASK)( ( (CH_MASK) 1 ) << ( wPipeIndex % CH_MASK_BITS ) ) 
		)
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
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
	{
		m_MaskRegs[ i ] &= ~SrcMask.m_MaskRegs[ i ];
	}

}	// void CChannelMask::ClearMask( CChannelMask SrcMask )

	
//===========================================================================
//	
//	Clear all channels in this mask
//
//===========================================================================

void CChannelMask::Clear()
{
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] = 0;

}	// void CChannelMask::Clear()

	
//===========================================================================
//
//	operator +=    Add channels in source mask to this mask
//
//===========================================================================

VOID CChannelMask::operator += (CONST CChannelMask & RVal)
{
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] |= RVal.m_MaskRegs[ i ];
}	// VOID operator += (CONST CChannelMask & RVal)

	
//===========================================================================
//
//	operator -=    Remove channels in source mask from this mask
//
//===========================================================================

VOID CChannelMask::operator -= (CONST CChannelMask & RVal)
{
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] &= ~RVal.m_MaskRegs[ i ];

}	// VOID operator -= (CONST CChannelMask & RVal)


//===========================================================================
//
// Test returns TRUE if any bits in source mask are set in this mask
//
//===========================================================================

BOOL CChannelMask::Test( PCChannelMask pSrcMask )
{
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		if ( m_MaskRegs[ i ] & pSrcMask->m_MaskRegs[ i ] )
			return TRUE;
	return FALSE;	

}	// BOOL CChannelMask::Test( PChannelMask pSrcMask )


//===========================================================================
//
//	IsSubsetOf returns TRUE if all of the channels in TstMask are set in 
// m_MaskRegs.  
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
	for ( int i = 0; i < CH_MASK_SZ; i++ )
	{
		CH_MASK ThisMask;
		
		ThisMask = SWAP( m_MaskRegs[ i ]);
		if ( ( ThisMask & TstMask[ i ] ) != ThisMask )
			return FALSE;
	}

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
	for ( int i = 0; i < CH_MASK_SZ; i++ )
		if ( 0 != ( m_MaskRegs[ i ] & TstMask[ i ] ) )
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
	for ( int i = 0; i < CH_MASK_SZ; i++ )
		if ( LVal.m_MaskRegs[ i ] != RVal.m_MaskRegs[ i ] )
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

	for ( int i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] = RVal.m_MaskRegs[ i ];

	return *this;

}	// CChannelMask& CChannelMask::operator = (CONTS CChannelMask & RVal)


CChannelMask& CChannelMask::operator =(CONST CChMaskDsp & RVal)
{
	CH_MASK *	pMask = (CH_MASK *) &RVal.m_MaskRegs[ 0 ];

	m_MaskRegs[ 0 ] = *pMask;
	m_MaskRegs[ 1 ] = 0;

	return *this;

}	// CChannelMask& CChannelMask::operator =(CONST CChMaskDsp & RVal)


//===========================================================================
//
// Operator & performs a bitwise logical AND
//
//===========================================================================

VOID CChannelMask::operator &= (CONST CChannelMask & RVal)
{
	if ( &RVal == this )
		return;

	for ( int i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] &= RVal.m_MaskRegs[ i ];

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

	for ( int i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] |= RVal.m_MaskRegs[ i ];

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
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		if ( 0 != m_MaskRegs[ i ] )
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

	m_MaskRegs[ wPipeIndex / CH_MASK_DSP_BITS ] |=
		SWAP( (CH_MASK_DSP)(( (CH_MASK_DSP) 1 ) << ( wPipeIndex % CH_MASK_DSP_BITS ) ) );
}	// void CChMaskDsp::SetIndexInMask( WORD wPipeIndex )
	
	
// Clear driver channel index into DSP mask format
void CChMaskDsp::ClearIndexInMask( WORD wPipeIndex )
{
	m_MaskRegs[ wPipeIndex / CH_MASK_DSP_BITS ] &=
		SWAP( (CH_MASK_DSP)(~( ( (CH_MASK_DSP) 1 ) << ( wPipeIndex % CH_MASK_DSP_BITS ) ) ) );
		
}	// void CChMaskDsp::SetIndexInMask( WORD wPipeIndex )
	
	
//===========================================================================
//
// Returns TRUE if the bit specified by the pipe index is set.	
//
//===========================================================================

BOOL CChMaskDsp::TestIndexInMask( WORD wPipeIndex )
{
	if ( m_MaskRegs[ wPipeIndex / CH_MASK_DSP_BITS ] &
		  SWAP( (CH_MASK_DSP)( ( (CH_MASK_DSP) 1 ) << ( wPipeIndex % CH_MASK_DSP_BITS ) ) ) )
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
	int	i;

	for ( i = 0; i < CH_MASK_SZ; i++ )
		m_MaskRegs[ i ] = 0;
}	// void CChMaskDsp::Clear()

	
//===========================================================================
//
//	operator +=    Add channels in source mask to this mask
//
//===========================================================================

VOID CChMaskDsp::operator += (CONST CChannelMask & RVal)
{
	CH_MASK *	pMask = (CH_MASK *) &m_MaskRegs[ 0 ];

	*pMask |= RVal.m_MaskRegs[ 0 ];
}	// VOID operator += (CONST CChMaskDsp & RVal)

	
//===========================================================================
//
//	operator -=    Remove channels in source mask from this mask
//
//===========================================================================

VOID CChMaskDsp::operator -= (CONST CChannelMask & RVal)
{
	CH_MASK *	pMask = (CH_MASK *) &m_MaskRegs[ 0 ];

	*pMask &= ~RVal.m_MaskRegs[ 0 ];
}	// VOID operator += (CONST CChMaskDsp & RVal)


//===========================================================================
//
// Operator = just copies from one mask to another.
//
//===========================================================================

CChMaskDsp& CChMaskDsp::operator =(CONST CChannelMask & RVal)
{
	CH_MASK *	pMask = (CH_MASK *) &m_MaskRegs[ 0 ];

	*pMask = RVal.m_MaskRegs[ 0 ];
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
