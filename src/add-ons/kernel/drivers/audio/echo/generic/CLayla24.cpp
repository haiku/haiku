// ****************************************************************************
//
//		CLayla24.cpp
//
//		Implementation file for the CLayla24 driver class.
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

#include "CLayla24.h"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CLayla24::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CLayla24::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CLayla24::operator new( size_t Size )


VOID  CLayla24::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF( ("CLayla24::operator delete memory free failed\n") );
	}
}	// VOID  CLayla24::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CLayla24::CLayla24( PCOsSupport pOsSupport )
		  : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CLayla24::CLayla24() is born!\n" ) );
}	// CLayla24::CLayla24()


CLayla24::~CLayla24()
{
	ECHO_DEBUGPRINTF( ( "CLayla24::~CLayla24() is toast!\n" ) );
}	// CLayla24::~CLayla24()




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CLayla24::InitHw()
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
	m_pDspCommObject = new CLayla24DspCommObject( (PDWORD) m_pvSharedMemory,
																 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CLayla24::InitHw - could not create DSP comm object\n"));
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
	// Layla24 can handle super-interleave and supports the digital 
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
	// Set the S/PDIF output format to "professional"
	//
	SetProfessionalSpdif( TRUE );

	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();

	ECHO_DEBUGPRINTF( ( "CLayla24::InitHw()\n" ) );
	return Status;

}	// ECHOSTATUS CLayla24::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CLayla24::GetCapabilities
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

	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_WORD		|
												ECHO_CLOCK_BIT_SPDIF		|
												ECHO_CLOCK_BIT_ADAT;

	return Status;
	
}	// ECHOSTATUS CLayla24::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CLayla24::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	//
	// Standard sample rates are allowed (like Mona & Layla24)
	//
	switch ( dwSampleRate )
	{
		case 8000 :
		case 11025 :
		case 16000 :
		case 22050 :
		case 32000 :
		case 44100 :
		case 48000 :
			goto qasr_ex;

		case 88200 :
		case 96000 :
		
			//
			// Double speed sample rates not allowed in ADAT mode
			//
			if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			{
				ECHO_DEBUGPRINTF(
					("CLayla24::QueryAudioSampleRate() Sample rate must be < "
					 "50,000 Hz in ADAT mode\n") );
				return ECHOSTATUS_BAD_FORMAT;
			}
			goto qasr_ex;
	}
	
	//
	// Any rate from 25000 to 50000 is allowed
	//
	if ( ( dwSampleRate >= 25000 ) &&
	  	  ( dwSampleRate <= 50000 ) )
		goto qasr_ex;

	//
	//	If not in ADAT mode, any rate from 50,000 to 100,000 is allowed
	// 
	if ( DIGITAL_MODE_ADAT != GetDigitalMode() &&
		  ( dwSampleRate >= 50000 ) &&
		  ( dwSampleRate <= 100000 ) )
		goto qasr_ex;

	ECHO_DEBUGPRINTF(
		("CLayla24::QueryAudioSampleRate() Sample rate must be >= 50,000 Hz"
		 " and <= 100,000 Hz and NOT in ADAT mode\n") );
	return ECHOSTATUS_BAD_FORMAT;

qasr_ex:
	ECHO_DEBUGPRINTF( ( "CLayla24::QueryAudioSampleRate() %ld Hz OK\n",
							  dwSampleRate ) );
	return ECHOSTATUS_OK;
}	// ECHOSTATUS CLayla24::QueryAudioSampleRate


//===========================================================================
//
// GetInputClockDetect returns a bitmask consisting of all the input
// clocks currently connected to the hardware; this changes as the user
// connects and disconnects clock inputs.  
//
// You should use this information to determine which clocks the user is
// allowed to select.
//
// Layla24 supports S/PDIF, word, and ADAT input clocks.
//
//===========================================================================

ECHOSTATUS CLayla24::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CLayla24::GetInputClockDetect: DSP Dead!\n") );
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


// *** Layla24.cpp ***
