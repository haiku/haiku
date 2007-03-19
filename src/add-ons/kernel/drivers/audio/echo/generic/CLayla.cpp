// ****************************************************************************
//
//		CLayla.cpp
//
//		Implementation file for the CLayla driver class; this is for 20-bit 
//		Layla.
//		
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

#include "CLayla.h"

#define LAYLA20_ANALOG_OUTPUT_LATENCY		57
#define LAYLA20_ANALOG_INPUT_LATENCY		64
#define LAYLA20_DIGITAL_OUTPUT_LATENCY		32
#define LAYLA20_DIGITAL_INPUT_LATENCY		32


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CLayla::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CLayla::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID CLayla::operator new( size_t Size )


VOID  CLayla::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF( ("CLayla::operator delete memory free failed\n") );
	}
}	// VOID  CLayla::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CLayla::CLayla( PCOsSupport pOsSupport )
		 : CEchoGalsMTC( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CLayla::CLayla() is born!\n" ) );

	m_wAnalogOutputLatency = LAYLA20_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = LAYLA20_ANALOG_INPUT_LATENCY;
	m_wDigitalOutputLatency = LAYLA20_DIGITAL_OUTPUT_LATENCY;
	m_wDigitalInputLatency = LAYLA20_DIGITAL_INPUT_LATENCY;

}	// CLayla::CLayla()


CLayla::~CLayla()
{
	ECHO_DEBUGPRINTF( ( "CLayla::~CLayla() is toast!\n" ) );
}	// CLayla::~CLayla()




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CLayla::InitHw()
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
	m_pDspCommObject = new CLaylaDspCommObject( (PDWORD) m_pvSharedMemory,
																m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CLayla::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP and the external box ASIC
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag; set the flag to indicate that
	// Darla24 can handle super-interleave.
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
	// Initialize the MIDI input
	//	
	Status = m_MidiIn.Init( this );
	if ( ECHOSTATUS_OK != Status )
		return Status;
	
	ECHO_DEBUGPRINTF(("\tMIDI input init done\n"));		

	//
	// Set defaults for +4/-10
	//
	for (i = 0; i < GetFirstDigitalBusOut(); i++ )
	{
		ECHO_DEBUGPRINTF(("\tSetting nominal output level %d\n",i));
		GetDspCommObject()->SetNominalLevel( i, TRUE );	// TRUE is -10 here
	}
	for ( i = 0; i < GetFirstDigitalBusIn(); i++ )
	{
		ECHO_DEBUGPRINTF(("\tSetting nominal input level %d\n",i));
		GetDspCommObject()->SetNominalLevel( GetNumBussesOut() + i, TRUE );
	}
	
	ECHO_DEBUGPRINTF(("\tNominal levels done\n"));	

	//
	// Set the S/PDIF output format to "professional"
	//
	SetProfessionalSpdif( TRUE );
	ECHO_DEBUGPRINTF(("\tSet S/PDIF format OK\n"));		

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	ECHO_DEBUGPRINTF( ( "\tCLayla::InitHw() finished\n" ) );

	return Status;

}	// ECHOSTATUS CLayla::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for Layla20
//
//===========================================================================

ECHOSTATUS CLayla::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS	Status;
	WORD			i;

	Status = GetBaseCapabilities(pCapabilities);

	//
	// Add input gain and nominal level to input busses
	//
	for (i = 0; i < GetFirstDigitalBusIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_GAIN |
													ECHOCAPS_MUTE |
													ECHOCAPS_NOMINAL_LEVEL;
	}
	
	//
	// Add nominal levels to output busses
	//
	for (i = 0; i < GetFirstDigitalBusOut(); i++)
	{
		pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	if ( ECHOSTATUS_OK != Status )
		return Status;

	pCapabilities->dwInClockTypes |= 	ECHO_CLOCK_BIT_WORD  |
													ECHO_CLOCK_BIT_SUPER |
													ECHO_CLOCK_BIT_SPDIF |
													ECHO_CLOCK_BIT_MTC;

	pCapabilities->dwOutClockTypes |= 	ECHO_CLOCK_BIT_WORD |
													ECHO_CLOCK_BIT_SUPER;

	return Status;

}	// ECHOSTATUS CLayla::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CLayla::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate <  8000 ||
		  dwSampleRate > 50000 )
	{
		ECHO_DEBUGPRINTF(
			("CLayla::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}	
	ECHO_DEBUGPRINTF( ( "CLayla::QueryAudioSampleRate() %ld Hz OK\n",
							  dwSampleRate ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CLayla::QueryAudioSampleRate


void CLayla::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 8000;
	dwMaxRate = 50000;
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
// Layla20 supports S/PDIF clock, word clock, and super clock.
//
//===========================================================================

ECHOSTATUS CLayla::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CLayla::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();
	
	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL | ECHO_CLOCK_BIT_MTC;
	
	if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_SPDIF))
		dwClockDetectBits |= ECHO_CLOCK_BIT_SPDIF;
		
	if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_WORD))
	{
		if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_SUPER))
			dwClockDetectBits |= ECHO_CLOCK_BIT_SUPER;
		else
			dwClockDetectBits |= ECHO_CLOCK_BIT_WORD;
	}
	
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


// *** CLayla.cpp ***
