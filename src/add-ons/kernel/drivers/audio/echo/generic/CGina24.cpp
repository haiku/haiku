// ****************************************************************************
//
//		CGina24.cpp
//
//		Implementation file for the CGina24 driver class.
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

#include "CGina24.h"


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
	ASSERT( NULL == m_pDspCommObject );
	m_pDspCommObject = new CGina24DspCommObject( (PDWORD) m_pvSharedMemory,
																 m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CGina24::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	ASSERT( GetDspCommObject() );

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
			("CGina24::QueryAudioSampleRate() Sample rate must be "
			 " 8,000 Hz, 11,025 Hz, 16,000 Hz, 22,050 Hz, 32,000 Hz, "
			 "44,100 Hz, 48,000 Hz, 88,200 Hz or 96,000 Hz\n") );
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
