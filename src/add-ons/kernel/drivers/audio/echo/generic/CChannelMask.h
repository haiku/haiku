// ****************************************************************************
//
//		CChannelMask.h
//
//		Include file for interfacing with the CChannelMask and CChMaskDsp
//		classes.
//		Set editor tabs to 3 for your viewing pleasure.
//
// 	CChannelMask is a handy way to specify a group of pipes simultaneously.
//		It should really be called "CPipeMask", but the class name predates
//		the term "pipe".
//	
//		CChMaskDsp is used in the comm page to specify a group of channels
//		at once; these are read by the DSP and must therefore be kept
//		in little-endian format.
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
#ifndef _CHMASKOBJECT_
#define _CHMASKOBJECT_

//
//	Defines
//
typedef unsigned long	CH_MASK;

#define	CH_MASK_BITS	(sizeof( CH_MASK ) * 8)
													// Max bits per mask entry

#define	ECHO_INVALID_CHANNEL	((WORD)(-1))
													// Marks unused channel #

typedef unsigned long		CH_MASK_DSP;
#define	CH_MASK_DSP_BITS	(sizeof( CH_MASK_DSP ) * 8)
													// Max bits per mask entry

/****************************************************************************

	CChannelMask

 ****************************************************************************/

class CChannelMask
{
protected:

#ifdef ECHO_OS9
	friend class CInOutChannelMask;
#endif

	CH_MASK	m_Mask;						// One bit per output or input channel

public:

	CChannelMask();
	~CChannelMask() {}

   CH_MASK GetMask()
   {
      return m_Mask;
   }

	// Returns TRUE if no bits set
	BOOL IsEmpty();

	// Set the wPipeIndex bit in the mask
	void SetIndexInMask( WORD wPipeIndex );

	// Clear the wPipeIndex bit in the mask
	void ClearIndexInMask( WORD wPipeIndex );

	// Return the next bit set starting with wStartPipeIndex as an index.
	//	If nothing set, returns ECHO_INVALID_CHANNEL.
	//	Use this interface for enumerating thru a channel mask.
	WORD GetIndexFromMask( WORD wStartPipeIndex );
	
	// Test driver channel index in mask.
	//	Return TRUE if set	
	BOOL TestIndexInMask( WORD wPipeIndex );

	// Clear all bits in the mask
	void Clear();

	// Clear bits in this mask that are in SrcMask
	void ClearMask( CChannelMask SrcMask );

	//	Return TRUE if any bits in source mask are set in this mask
	BOOL Test( CChannelMask * pSrcMask );

	//
	//	Return TRUE if the Test Mask contains all of the channels
	//	enabled in this instance.
	//	Use to be sure all channels in this instance exist in
	//	another instance.
	//
	BOOL IsSubsetOf( CChannelMask& TstMask );

	//
	//	Return TRUE if the Test Mask contains at least one of the channels
	//	enabled in this instance.
	//	Use to find out if any channels in this instance exist in
	//	another instance.
	//
	BOOL IsIntersectionOf( CChannelMask& TstMask );

	//
	//	Overload new & delete so memory for this object is allocated
	//	from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid );

	//
	//	Macintosh compiler likes "class" after friend, PC doesn't care
	//
	friend class CChMaskDsp;

	//	Return TRUE if source mask equals this mask
	friend BOOLEAN operator == ( CONST CChannelMask &LVal,
										  CONST CChannelMask &RVal );

	// Copy mask bits in source to this mask
	CChannelMask& operator = (CONST CChannelMask & RVal);

	// Add mask bits in source to this mask
	VOID operator += (CONST CChannelMask & RVal);

	// Subtract mask bits in source to this mask
	VOID operator -= (CONST CChannelMask & RVal);

	// AND mask bits in source to this mask
	VOID operator &= (CONST CChannelMask & RVal);

	// OR mask bits in source to this mask
	VOID operator |= (CONST CChannelMask & RVal);

protected :

	//
	//	Store an output bit mask and an input bitmask.
	//	We assume here that the # of outputs fits in one mask reg
	//
	void SetMask( CH_MASK OutMask, CH_MASK InMask, int nOutputs );
	void SetOutMask( CH_MASK OutMask, int nOutputs );
	void SetInMask( CH_MASK InMask, int nOutputs );

	//
	//	Retrieve an output bit mask and an input bitmask.
	//	We assume here that the # of outputs fits in one mask reg
	//
	void GetMask( CH_MASK & OutMask, CH_MASK & InMask, int nOutputs );
	CH_MASK GetOutMask( int nOutputs );
	CH_MASK GetInMask( int nOutputs );

};	// class CChannelMask

typedef	CChannelMask *		PCChannelMask;


/****************************************************************************

	CChMaskDsp

 ****************************************************************************/

class CChMaskDsp
{
public:
	CH_MASK_DSP	m_Mask;		// One bit per output or input channel
														// 32 bits total
	CChMaskDsp();
	~CChMaskDsp() {}

	// Returns TRUE if no bits set
	BOOL IsEmpty();

	// Set the wPipeIndex bit in the mask
	void SetIndexInMask( WORD wPipeIndex );

	// Clear the wPipeIndex bit in the mask
	void ClearIndexInMask( WORD wPipeIndex );
	
	// Test pipe index in mask.
	//	Return TRUE if set	
	BOOL TestIndexInMask( WORD wPipeIndex );

	// Clear all bits in the mask
	void Clear();

	//
	//	Overload new & delete so memory for this object is allocated
	//	from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid );

	//
	//	Macintosh compiler likes "class" after friend, PC doesn't care
	//
	friend class CChannelMask;

	// Copy mask bits in source to this mask
	CChMaskDsp& operator = (CONST CChannelMask & RVal);

protected :

};	// class CChMaskDsp

typedef	CChMaskDsp *		PCChMaskDsp;

#endif // _CHMASKOBJECT_

//	CChannelMask.h
