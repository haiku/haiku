// ****************************************************************************
//
//		CMia.cpp
//
//		Implementation file for the CMia driver class.
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

#include "CMia.h"


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
	  : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CMia::CMia() is born!\n" ) );
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
	ASSERT( NULL == m_pDspCommObject );
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
	// Add meters & pans to output pipes
	//
	for (i = 0 ; i < GetNumPipesOut(); i++)
	{
		pCapabilities->dwPipeOutCaps[i] |= ECHOCAPS_PEAK_METER |	
														ECHOCAPS_PAN;
	}
	
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
	if ( dwSampleRate !=  8000 &&
		  dwSampleRate != 11025 &&
		  dwSampleRate != 12000 &&
		  dwSampleRate != 16000 &&
		  dwSampleRate != 22050 &&
		  dwSampleRate != 24000 &&
		  dwSampleRate != 32000 &&
		  dwSampleRate != 44100 &&
		  dwSampleRate != 48000 &&
		  dwSampleRate != 88200 &&
		  dwSampleRate != 96000 )
	{
		ECHO_DEBUGPRINTF(
			("CMia::QueryAudioSampleRate() Sample rate must be "
			 " 8,000 Hz, 11,025 Hz, 12,000 Hz, 16,000 Hz, 22,050 Hz, "
			 "24,000 Hz, 32,000 Hz, 44,100 Hz, 48,000 Hz, 88,200 Hz "
			 "or 96,000 Hz\n") );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CMia::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CMia::QueryAudioSampleRate


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
	ECHO_DEBUGPRINTF(("CMia::GetInputClockDetect\n"));

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("CMia::GetInputClockDetect: DSP Dead!\n") );
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


//===========================================================================
//
// Most of the cards don't have an actual output bus gain; they only
// have gain controls for output pipes; the output bus gain is implemented
// as a logical control.  Mia (and any other card with a vmixer) works
// differently; it does have a physical output bus gain control, so 
// just pass the gain down to the DSP comm object.
//
//===========================================================================

ECHOSTATUS CMia::AdjustPipesOutForBusOut(WORD wBusOut,int iBusOutGain)
{

	GetDspCommObject()->SetBusOutGain(wBusOut,iBusOutGain);
	
	return ECHOSTATUS_OK;
	
}	// AdjustPipesOutForBusOut


// *** Mia.cpp ***
