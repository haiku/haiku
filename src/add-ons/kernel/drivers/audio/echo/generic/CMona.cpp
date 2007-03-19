// ****************************************************************************
//
//		CMona.cpp
//
//		Implementation file for the CMona driver class.
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

#include "CMona.h"

#define MONA_ANALOG_OUTPUT_LATENCY		59
#define MONA_ANALOG_INPUT_LATENCY		71
#define MONA_DIGITAL_OUTPUT_LATENCY		32
#define MONA_DIGITAL_INPUT_LATENCY		32



/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CMona::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CMona::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CMona::operator new( size_t Size )


VOID  CMona::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CMona::operator delete memory free failed\n"));
	}
}	// VOID CMona::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CMona::CMona( PCOsSupport pOsSupport )
		: CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CMona::CMona() is born!\n" ) );

	m_wAnalogOutputLatency = MONA_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = MONA_ANALOG_INPUT_LATENCY;
	m_wDigitalOutputLatency = MONA_DIGITAL_OUTPUT_LATENCY;
	m_wDigitalInputLatency = MONA_DIGITAL_INPUT_LATENCY;
}

CMona::~CMona()
{
	ECHO_DEBUGPRINTF( ( "CMona::~CMona() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CMona::InitHw()
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
	ECHO_ASSERT(NULL == m_pDspCommObject );
	m_pDspCommObject = new CMonaDspCommObject( (PDWORD) m_pvSharedMemory,
															 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CMona::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP, the PCI card ASIC, and the external box ASIC
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag; set the flags to indicate that
	// Mona can handle super-interleave and supports the digital 
	// input auto-mute.
	//
	m_wFlags &= ~ECHOGALS_FLAG_BADBOARD;
	m_wFlags |= ECHOGALS_ROFLAG_SUPER_INTERLEAVE_OK |
				ECHOGALS_ROFLAG_DIGITAL_IN_AUTOMUTE;
	
	//
	//	Must call this here after DSP is init to 
	//	init gains and mutes
	//
	Status = InitLineLevels();
	if ( ECHOSTATUS_OK != Status )
		return Status;
		
	//
	// Set the digital mode to S/PDIF RCA
	//
	SetDigitalMode( DIGITAL_MODE_SPDIF_RCA );

	//
	// Set the S/PDIF output format to "professional"
	//
	SetProfessionalSpdif( TRUE );

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();

	ECHO_DEBUGPRINTF( ( "CMona::InitHw()\n" ) );
	return Status;

}	// ECHOSTATUS CMona::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CMona::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS	Status;

	Status = GetBaseCapabilities(pCapabilities);
	if ( ECHOSTATUS_OK != Status )
		return Status;

	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_WORD		|
												ECHO_CLOCK_BIT_SPDIF		|
												ECHO_CLOCK_BIT_ADAT;

	return Status;

}	// ECHOSTATUS CMona::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CMona::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate !=  8000 &&
		  dwSampleRate != 11025 &&
		  dwSampleRate != 16000 &&
		  dwSampleRate != 22050 &&
		  dwSampleRate != 32000 &&
		  dwSampleRate != 44100 &&
		  dwSampleRate != 48000 &&
		  dwSampleRate != 88200 &&
		  dwSampleRate != 96000 )
	{
		ECHO_DEBUGPRINTF(
			("CMona::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}
	if ( dwSampleRate >= 88200 && DIGITAL_MODE_ADAT == GetDigitalMode() )
	{
		ECHO_DEBUGPRINTF(
			("CMona::QueryAudioSampleRate() Sample rate cannot be "
			 "set to 88,200 Hz or 96,000 Hz in ADAT mode\n") );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CMona::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;
}	// ECHOSTATUS CMona::QueryAudioSampleRate


void CMona::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 8000;
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
// Mona supports S/PDIF, word, and ADAT input clocks.
//
//===========================================================================

ECHOSTATUS CMona::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	//ECHO_DEBUGPRINTF(("CMona::GetInputClockDetect\n"));

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CMona::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	//
	// Map the DSP clock detect bits to the generic driver clock detect bits
	//	
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();	

	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL;	
	
	if (0 != (dwClocksFromDsp & GML_CLOCK_DETECT_BIT_SPDIF))
		dwClockDetectBits |= ECHO_CLOCK_BIT_SPDIF;
		
	if (0 != (dwClocksFromDsp & GML_CLOCK_DETECT_BIT_ADAT))
		dwClockDetectBits |= ECHO_CLOCK_BIT_ADAT;
		
	if (0 != (dwClocksFromDsp & GML_CLOCK_DETECT_BIT_WORD))
		dwClockDetectBits |= ECHO_CLOCK_BIT_WORD;

	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


// *** CMona.cpp ***
