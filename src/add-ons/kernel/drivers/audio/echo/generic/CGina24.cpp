// ****************************************************************************
//
//		CGina24.cpp
//
//		Implementation file for the CGina24 driver class.
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

#include "CGina24.h"

#define GINA24_ANALOG_OUTPUT_LATENCY		59
#define GINA24_ANALOG_INPUT_LATENCY			71
#define GINA24_DIGITAL_OUTPUT_LATENCY		32
#define GINA24_DIGITAL_INPUT_LATENCY		32



/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CGina24::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CGina::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CGina24::operator new( size_t Size )


VOID  CGina24::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CGina24::operator delete memory free failed\n"));
	}
}	// VOID CGina24::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CGina24::CGina24( PCOsSupport pOsSupport )
		 : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CGina24::CGina24() is born!\n" ) );

	m_wAnalogOutputLatency = GINA24_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = GINA24_ANALOG_INPUT_LATENCY;
	m_wDigitalOutputLatency = GINA24_DIGITAL_OUTPUT_LATENCY;
	m_wDigitalInputLatency = GINA24_DIGITAL_INPUT_LATENCY;

}

CGina24::~CGina24()
{
	ECHO_DEBUGPRINTF( ( "CGina24::~CGina24() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CGina24::InitHw()
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
	m_pDspCommObject = new CGina24DspCommObject( (PDWORD) m_pvSharedMemory,
																 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CGina24::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	ECHO_ASSERT(GetDspCommObject() );

	//
	// Load the DSP and the ASIC on the PCI card
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag; set the flags to indicate that
	// Gina24 can handle super-interleave and supports the digital
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
	// Set defaults for +4/-10
	//
	WORD i;		
	for (i = 0; i < GetFirstDigitalBusOut(); i++ )
	{
		Status = GetDspCommObject()->
						SetNominalLevel( i, FALSE );	// FALSE is +4 here
	}
	for ( i = 0; i < GetFirstDigitalBusIn(); i++ )
	{
		Status = GetDspCommObject()->
						SetNominalLevel( GetNumBussesOut() + i, FALSE );
	}
	
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

	ECHO_DEBUGPRINTF( ( "CGina24::InitHw()\n" ) );
	return Status;
	
}	// ECHOSTATUS CGina24::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for Gina24
//
//===========================================================================

ECHOSTATUS CGina24::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS	Status;

	Status = GetBaseCapabilities(pCapabilities);
						  
	//
	// Add nominal level control to analog ins & outs
	//
	WORD i;
	for (i = 0 ; i < GetFirstDigitalBusOut(); i++)
	{
		pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	for (i = 0 ; i < GetFirstDigitalBusIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	if ( ECHOSTATUS_OK != Status )
		return Status;

	pCapabilities->dwInClockTypes |= 
		ECHO_CLOCK_BIT_SPDIF |
		ECHO_CLOCK_BIT_ESYNC | 
		ECHO_CLOCK_BIT_ESYNC96 | 
		ECHO_CLOCK_BIT_ADAT;

	return Status;
}	// ECHOSTATUS CGina24::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CGina24::QueryAudioSampleRate
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
			("CGina24::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}
	if ( dwSampleRate >= 88200 && DIGITAL_MODE_ADAT == GetDigitalMode() )
	{
		ECHO_DEBUGPRINTF(
			("CGina24::QueryAudioSampleRate() Sample rate cannot be "
			 "set to 88,200 Hz or 96,000 Hz in ADAT mode\n") );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CGina24::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;
}	// ECHOSTATUS CGina24::QueryAudioSampleRate


void CGina24::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
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
// Gina24 supports S/PDIF, Esync, and ADAT input clocks.
//
//===========================================================================

ECHOSTATUS CGina24::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	//ECHO_DEBUGPRINTF(("CGina24::GetInputClockDetect\n"));

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CGina24::GetInputClockDetect: DSP Dead!\n") );
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
		
	if (0 != (dwClocksFromDsp & GML_CLOCK_DETECT_BIT_ESYNC))
		dwClockDetectBits |= ECHO_CLOCK_BIT_ESYNC | ECHO_CLOCK_BIT_ESYNC96;
		
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


// *** CGina24.cpp ***
