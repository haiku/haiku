// ****************************************************************************
//
//		CDarla24.cpp
//
//		Implementation file for the CDarla24 driver class.
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

#include "CDarla24.h"

#define DARLA24_ANALOG_OUTPUT_LATENCY	59
#define DARLA24_ANALOG_INPUT_LATENCY	71


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CDarla24::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CDarla24::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID CDarla24::operator new( size_t Size )


VOID  CDarla24::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CDarla24::operator delete memory free failed\n"));
	}
}	// VOID CDarla24::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CDarla24::CDarla24( PCOsSupport pOsSupport )
			: CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CDarla24::CDarla24() is born!\n" ) );
	
	m_wAnalogOutputLatency = DARLA24_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = DARLA24_ANALOG_INPUT_LATENCY;
	
}

CDarla24::~CDarla24()
{
	ECHO_DEBUGPRINTF( ( "CDarla24::~CDarla24() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CDarla24::InitHw()
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
	m_pDspCommObject = new CDarla24DspCommObject( (PDWORD) m_pvSharedMemory,
																 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CDarla24::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	//	Load the DSP
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
	//	init gains
	//
	Status = InitLineLevels();
	if ( ECHOSTATUS_OK != Status )
		return Status;
	
	//
	// Set defaults for +4/-10
	//
	for (i = 0; i < GetNumBusses(); i++ )
	{
		GetDspCommObject()->SetNominalLevel( i, FALSE );	// FALSE is +4 here
	}

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	ECHO_DEBUGPRINTF( ( "CDarla24::InitHw()\n" ) );
	
	return Status;
	
}	// ECHOSTATUS CDarla24::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CDarla24::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS	Status;
	WORD			i;
	
	Status = GetBaseCapabilities(pCapabilities);
						  
	//
	// Add nominal level control to in & out busses
	//
	for (i = 0 ; i < GetNumBussesOut(); i++)
	{
		pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	for (i = 0 ; i < GetNumBussesIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}
															
	if ( ECHOSTATUS_OK != Status )
		return Status;

	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_ESYNC;

	return Status;
	
}	// ECHOSTATUS CDarla24::GetCapabilities


//===========================================================================
//
// GetInputClockDetect returns a bitmask consisting of all the input
// clocks currently connected to the hardware; this changes as the user
// connects and disconnects clock inputs.  
//
// You should use this information to determine which clocks the user is
// allowed to select.
//
// Darla24 only supports Esync input clock.
//
//===========================================================================

ECHOSTATUS CDarla24::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CDarla24::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();
	
	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL;
	
	if (0 != (dwClocksFromDsp & GLDM_CLOCK_DETECT_BIT_ESYNC))
		dwClockDetectBits |= ECHO_CLOCK_BIT_ESYNC;
	
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CDarla24::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate != 8000 &&
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
			("CDarla24::QueryAudioSampleRate() - rate %ld invalid\n",
			dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CDarla24::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDarla24::QueryAudioSampleRate

void CDarla24::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 8000;
	dwMaxRate = 96000;
}



// *** CDarla24.cpp ***
