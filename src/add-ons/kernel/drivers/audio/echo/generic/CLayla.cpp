// ****************************************************************************
//
//		CLayla.cpp
//
//		Implementation file for the CLayla driver class; this is for 20-bit 
//		Layla.
//		
//		Set editor tabs to 3 for your viewing pleasure.
//
//		Copyright Echo Digital Audio Corporation (c) 1998 - 2002
//		All rights reserved
//		www.echoaudio.com
//		
//		Permission is hereby granted, free of charge, to any person obtaining a
//		copy of this software and associated documentation files (the
//		"Software"), to deal with the Software without restriction, including
//		without limitation the rights to use, copy, modify, merge, publish,
//		distribute, sublicense, and/or sell copies of the Software, and to
//		permit persons to whom the Software is furnished to do so, subject to
//		the following conditions:
//		
//		- Redistributions of source code must retain the above copyright
//		notice, this list of conditions and the following disclaimers.
//		
//		- Redistributions in binary form must reproduce the above copyright
//		notice, this list of conditions and the following disclaimers in the
//		documentation and/or other materials provided with the distribution.
//		
//		- Neither the name of Echo Digital Audio, nor the names of its
//		contributors may be used to endorse or promote products derived from
//		this Software without specific prior written permission.
//
//		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//		IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
//		ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//		TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//		SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//
// ****************************************************************************

#include "CLayla.h"


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
		 : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CLayla::CLayla() is born!\n" ) );
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
	ASSERT( NULL == m_pDspCommObject );
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

	//
	// Set defaults for +4/-10
	//
	for (i = 0; i < GetFirstDigitalBusOut(); i++ )
	{
		GetDspCommObject()->SetNominalLevel( i, TRUE );	// TRUE is -10 here
	}
	for ( i = 0; i < GetFirstDigitalBusIn(); i++ )
	{
		GetDspCommObject()->SetNominalLevel( GetNumBussesOut() + i, TRUE );
	}

	//
	// Set the S/PDIF output format to "professional"
	//
	SetProfessionalSpdif( TRUE );

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	ECHO_DEBUGPRINTF( ( "CLayla::InitHw()\n" ) );

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
													ECHO_CLOCK_BIT_SPDIF;

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
			("CLayla::QueryAudioSampleRate() Sample rate must be >= 8,000 Hz"
			 " and <= 50,000 Hz\n") );
		return ECHOSTATUS_BAD_FORMAT;
	}	
	ECHO_DEBUGPRINTF( ( "CLayla::QueryAudioSampleRate() %ld Hz OK\n",
							  dwSampleRate ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CLayla::QueryAudioSampleRate


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
	
	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL;
	
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
