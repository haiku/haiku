// ****************************************************************************
//
//		CIndigo.cpp
//
//		Implementation file for the CIndigo driver class.
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

#include "CIndigo.h"

#define INDIGO_ANALOG_OUTPUT_LATENCY	91


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CIndigo::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CIndigo::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CIndigo::operator new( size_t Size )


VOID  CIndigo::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CIndigo::operator delete memory free failed\n"));
	}
}	// VOID CIndigo::operator delete( PVOID pVoid )



//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CIndigo::CIndigo( PCOsSupport pOsSupport )
	  : CEchoGalsVmixer( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CIndigo::CIndigo() is born!\n" ) );
	
	m_wAnalogOutputLatency = INDIGO_ANALOG_OUTPUT_LATENCY;
}

CIndigo::~CIndigo()
{
	ECHO_DEBUGPRINTF( ( "CIndigo::~CIndigo() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CIndigo::InitHw()
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
	m_pDspCommObject = new CIndigoDspCommObject( (PDWORD) m_pvSharedMemory,
															 m_pOsSupport );
 	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CIndigo::InitHw - could not create DSP comm object\n"));
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




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CIndigo::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS Status;

	Status = GetBaseCapabilities(pCapabilities);
	if ( ECHOSTATUS_OK != Status )
		return Status;
		
	pCapabilities->dwOutClockTypes = 0;

	pCapabilities->dwPipeInCaps[0] = ECHOCAPS_DUMMY;
	pCapabilities->dwPipeInCaps[1] = ECHOCAPS_DUMMY;
	pCapabilities->dwBusInCaps[0] = ECHOCAPS_DUMMY; 
	pCapabilities->dwBusInCaps[1] = ECHOCAPS_DUMMY; 

	return Status;

}	// ECHOSTATUS CIndigo::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CIndigo::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate != 32000 &&
		  dwSampleRate != 44100 &&
		  dwSampleRate != 48000 &&
		  dwSampleRate != 88200 &&
		  dwSampleRate != 96000 )
	{
		ECHO_DEBUGPRINTF(
			("CIndigo::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CIndigo::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CIndigo::QueryAudioSampleRate


void CIndigo::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 32000;
	dwMaxRate = 96000;
}


//===========================================================================
//
// Indigo & Indigo DJ don't have monitor mixers, so this works differently
// from other vmixer cards.
//
//===========================================================================

ECHOSTATUS CIndigo::AdjustMonitorsForBusOut(WORD wBusOut)
{
	return ECHOSTATUS_OK;
}	

// *** CIndigo.cpp ***
