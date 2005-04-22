// ****************************************************************************
//
//		CIndigoDJ.cpp
//
//		Implementation file for the CIndigoDJ driver class.
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

#include "CIndigoDJ.h"

#define INDIGO_DJ_OUTPUT_LATENCY_SINGLE_SPEED	44
#define INDIGO_DJ_OUTPUT_LATENCY_DOUBLE_SPEED	37


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CIndigoDJ::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CIndigoDJ::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CIndigoDJ::operator new( size_t Size )


VOID  CIndigoDJ::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CIndigoDJ::operator delete memory free failed\n"));
	}
}	// VOID CIndigoDJ::operator delete( PVOID pVoid )



//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CIndigoDJ::CIndigoDJ( PCOsSupport pOsSupport )
	  : CIndigo( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CIndigoDJ::CIndigoDJ() is born!\n" ) );
}

CIndigoDJ::~CIndigoDJ()
{
	ECHO_DEBUGPRINTF( ( "CIndigoDJ::~CIndigoDJ() is toast!\n" ) );
}



//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CIndigoDJ::InitHw()
{
	ECHOSTATUS	Status;

	//
	// Call the base method
	//
	if ( ECHOSTATUS_OK != ( Status = CEchoGals::InitHw() ) )
		return Status;

	//
	// Create the DSP comm object
	//
	m_pDspCommObject = new CIndigoDJDspCommObject( (PDWORD) m_pvSharedMemory,
																 m_pOsSupport );
 	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CIndigoDJ::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Do flags
	//
	m_wFlags &= ~ECHOGALS_FLAG_BADBOARD;
	m_wFlags |= ECHOGALS_ROFLAG_SUPER_INTERLEAVE_OK;

	//
	//	Must call this here after DSP is init to 
	//	init gains and mutes
	//
	Status = InitLineLevels();
	if ( ECHOSTATUS_OK != Status )
		return Status;

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();

	ECHO_DEBUGPRINTF( ( "CIndigo::InitHw()\n" ) );
	return Status;

}	// ECHOSTATUS CIndigo::InitHw()


//===========================================================================
//
// GetAudioLatency - returns the latency for a single pipe
//
//===========================================================================

void CIndigoDJ::GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency)
{
	DWORD dwLatency;
	DWORD dwSampleRate;

	dwSampleRate = GetDspCommObject()->GetSampleRate();
	if (FALSE == pLatency->wIsInput)
	{
		//
		// The latency depends on the current sample rate
		//
		if (dwSampleRate < 50000)
			dwLatency = INDIGO_DJ_OUTPUT_LATENCY_SINGLE_SPEED;
		else
			dwLatency = INDIGO_DJ_OUTPUT_LATENCY_DOUBLE_SPEED;
	}
	else
	{
		//
		// Inputs?  What inputs?
		//
		dwLatency = 0;
	}
	
	pLatency->dwLatency = dwLatency;

}	// GetAudioLatency


// *** CIndigoDJ.cpp ***
