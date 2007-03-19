// ****************************************************************************
//
//		CMia.cpp
//
//		Implementation file for the CMia driver class.
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

#include "CMia.h"

#define MIA_ANALOG_OUTPUT_LATENCY	63
#define MIA_ANALOG_INPUT_LATENCY		62



/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CMia::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CMia::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID CMia::operator new( size_t Size )


VOID  CMia::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CMia::operator delete memory free failed\n"));
	}
}	// VOID CMia::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CMia::CMia( PCOsSupport pOsSupport )
	  : CEchoGalsVmixer( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CMia::CMia() is born!\n" ) );
	
	//
	// Mia's virtual outputs make things tricky, since a pipe can
	// go to either bus.  
	//
	m_wAnalogOutputLatency = MIA_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = MIA_ANALOG_INPUT_LATENCY;
	
}

CMia::~CMia()
{
	ECHO_DEBUGPRINTF( ( "CMia::~CMia() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CMia::InitHw()
{
	ECHOSTATUS	Status;
	WORD			i;

	//
	// Call the base method
	//
	if ( ECHOSTATUS_OK != ( Status = CEchoGals::InitHw() ) )
		return Status;

	//
	// Create the DSP comm object
	//
	ECHO_ASSERT(NULL == m_pDspCommObject );
	m_pDspCommObject = new CMiaDspCommObject( (PDWORD) m_pvSharedMemory,
															 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CMia::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag; set the flags to indicate that
	// Mia can handle super-interleave.
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
	// Set defaults for +4/-10
	//
	for (i = 0; i < GetFirstDigitalBusOut(); i++ )
	{
		GetDspCommObject()->
			SetNominalLevel( i, FALSE );	// FALSE is +4 here
	}
	for ( i = 0; i < GetFirstDigitalBusIn(); i++ )
	{
		GetDspCommObject()->
			SetNominalLevel( GetNumBussesOut() + i, FALSE );
	}

	//
	// Set the digital mode to S/PDIF RCA
	//
	SetDigitalMode( DIGITAL_MODE_SPDIF_RCA );
	
	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	
	//
	// Is this a Mia MIDI card?
	//
	if (MIA_MIDI_REV == m_pOsSupport->GetCardRev())
	{
		Status = m_MidiIn.Init( this );
	}
		
	
	ECHO_DEBUGPRINTF( ( "CMia::InitHw()\n" ) );
	return Status;

}	// ECHOSTATUS CMia::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CMia::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS	Status;
	WORD			i;

	Status = GetBaseCapabilities(pCapabilities);
	if ( ECHOSTATUS_OK != Status )
		return Status;
		
	//
	// Add nominal level control to analog ins & outs
	//
	for (i = 0 ; i < GetFirstDigitalBusOut(); i++)
	{
		pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	for (i = 0 ; i < GetFirstDigitalBusIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_SPDIF;
	pCapabilities->dwOutClockTypes = 0;
	
	return Status;

}	// ECHOSTATUS CMia::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CMia::QueryAudioSampleRate
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
			("CMia::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CMia::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CMia::QueryAudioSampleRate


void CMia::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 32000;
	dwMaxRate = 96000;
}



//===========================================================================
//
// GetInputClockDetect returns a bitmask consisting of all the input
// clocks currently connected to the hardware; this changes as the user
// connects and disconnects clock inputs.  
//
// You should use this information to determine which clocks the user is
// allowed to select.
//
// Mia supports S/PDIF input clock.
//
//===========================================================================

ECHOSTATUS CMia::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	//ECHO_DEBUGPRINTF(("CMia::GetInputClockDetect\n"));

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		//ECHO_DEBUGPRINTF( ("CMia::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	//
	// Map the DSP clock detect bits to the generic driver clock detect bits
	//	
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();	

	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL;	
	
	if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_SPDIF))
		dwClockDetectBits |= ECHO_CLOCK_BIT_SPDIF;
		
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect



// *** Mia.cpp ***
