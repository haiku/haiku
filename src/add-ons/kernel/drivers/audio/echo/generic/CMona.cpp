// ****************************************************************************
//
//		CMona.cpp
//
//		Implementation file for the CMona driver class.
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

#include "CMona.h"


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
	ASSERT( NULL == m_pDspCommObject );
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
			("CMona::QueryAudioSampleRate() Sample rate must be "
			 " 8,000 Hz, 11,025 Hz, 16,000 Hz, 22,050 Hz, 32,000 Hz, "
			 "44,100 Hz, 48,000 Hz, 88,200 Hz or 96,000 Hz\n") );
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
