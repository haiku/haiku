// ****************************************************************************
//
//		CGina.cpp
//
//		Implementation file for the CGina driver class.  CGina is for 
// 	20-bit Gina, not Gina24.
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

#include "CGina.h"

#define GINA20_ANALOG_OUTPUT_LATENCY		137
#define GINA20_ANALOG_INPUT_LATENCY			240
#define GINA20_DIGITAL_OUTPUT_LATENCY		112
#define GINA20_DIGITAL_INPUT_LATENCY		208


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CGina::operator new( size_t Size )
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
	
}	// PVOID CGina::operator new( size_t Size )

VOID  CGina::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CGina::operator delete memory free failed\n"));
	}
}	// VOID  CGina::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CGina::CGina( PCOsSupport pOsSupport )
		: CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CGina::CGina() is born!\n" ) );
	
	m_wAnalogOutputLatency = GINA20_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = GINA20_ANALOG_INPUT_LATENCY;
	m_wDigitalOutputLatency = GINA20_DIGITAL_OUTPUT_LATENCY;
	m_wDigitalInputLatency = GINA20_DIGITAL_INPUT_LATENCY;
}

CGina::~CGina()
{
	ECHO_DEBUGPRINTF( ( "CGina::~CGina() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CGina::InitHw()
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
	m_pDspCommObject = new CGinaDspCommObject( (PDWORD) m_pvSharedMemory,
															 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CGina::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}
															 
	//
	// Load the firmware
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag
	//
	m_wFlags &= ~ECHOGALS_FLAG_BADBOARD;

	//
	//	Must call this here after DSP is init to 
	//	init gains and mutes
	//
	Status = InitLineLevels();
	
	//
	// Set the S/PDIF output format to "professional"
	//
	SetProfessionalSpdif( TRUE );

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	
	ECHO_DEBUGPRINTF( ( "CGina::InitHw()\n" ) );
	return Status;
	
}	// ECHOSTATUS CGina::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CGina::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{ 
	WORD i;

	GetBaseCapabilities(pCapabilities);
										
	//
	// Add input gain to input busses
	//
	for (i = 0; i < GetFirstDigitalBusIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_GAIN |
													ECHOCAPS_MUTE;
	}
												  
	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_SPDIF;													  
	
	return ECHOSTATUS_OK;
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
// Gina20 only supports S/PDIF input clock.
//
//===========================================================================

ECHOSTATUS CGina::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CGina::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();
	
	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL;
	
	if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_SPDIF))
		dwClockDetectBits |= ECHO_CLOCK_BIT_SPDIF;
	
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CGina::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate != 44100 &&
		  dwSampleRate != 48000 )
	{
		ECHO_DEBUGPRINTF(
			("CGina::QueryAudioSampleRate() - rate %ld invalid\n",
			dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}	

	ECHO_DEBUGPRINTF( ( "CGina::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CGina::QueryAudioSampleRate


void CGina::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 44100;
	dwMaxRate = 48000;
}

// *** CGina.cpp ***
