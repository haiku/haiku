// ****************************************************************************
//
//		CIndigoIO.cpp
//
//		Implementation file for the CIndigoIO driver class.
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

#include "CIndigoIO.h"
#include "CIndigoDJ.h"

#define INDIGO_IO_OUTPUT_LATENCY_SINGLE_SPEED	44
#define INDIGO_IO_OUTPUT_LATENCY_DOUBLE_SPEED	37
#define INDIGO_IO_INPUT_LATENCY_SINGLE_SPEED		44
#define INDIGO_IO_INPUT_LATENCY_DOUBLE_SPEED		41


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CIndigoIO::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CIndigoIO::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CIndigoIO::operator new( size_t Size )


VOID  CIndigoIO::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CIndigoIO::operator delete memory free failed\n"));
	}
}	// VOID CIndigoIO::operator delete( PVOID pVoid )



//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CIndigoIO::CIndigoIO( PCOsSupport pOsSupport )
	  : CEchoGalsVmixer( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CIndigoIO::CIndigoIO() is born!\n" ) );
}

CIndigoIO::~CIndigoIO()
{
	ECHO_DEBUGPRINTF( ( "CIndigoIO::~CIndigoIO() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CIndigoIO::InitHw()
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
	m_pDspCommObject = new CIndigoIODspCommObject( (PDWORD) m_pvSharedMemory,
															 m_pOsSupport );
 	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CIndigoIO::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Do flags
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
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();

	ECHO_DEBUGPRINTF( ( "CIndigoIO::InitHw() complete\n" ) );
	return Status;

}	// ECHOSTATUS CIndigoIO::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS CIndigoIO::GetCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	ECHOSTATUS Status;

	Status = GetBaseCapabilities(pCapabilities);
	if ( ECHOSTATUS_OK != Status )
		return Status;
		
	pCapabilities->dwOutClockTypes = 0;

	return Status;

}	// ECHOSTATUS CIndigoIO::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CIndigoIO::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate != 32000 &&
		  dwSampleRate != 44100 &&
		  dwSampleRate != 48000 &&
		  dwSampleRate != 64000 &&
		  dwSampleRate != 88200 &&
		  dwSampleRate != 96000
		)
	{
		ECHO_DEBUGPRINTF(
			("CIndigoIO::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );
		return ECHOSTATUS_BAD_FORMAT;
	}

	ECHO_DEBUGPRINTF( ( "CIndigoIO::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CIndigoIO::QueryAudioSampleRate


void CIndigoIO::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 32000;
	dwMaxRate = 96000;
}


//===========================================================================
//
// GetAudioLatency - returns the latency for a single pipe
//
//===========================================================================

void CIndigoIO::GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency)
{
	DWORD dwLatency;
	DWORD dwSampleRate;

	//
	// The latency depends on the current sample rate
	//
	dwSampleRate = GetDspCommObject()->GetSampleRate();
	if (FALSE == pLatency->wIsInput)
	{
		if (dwSampleRate < 50000)
			dwLatency = INDIGO_IO_OUTPUT_LATENCY_SINGLE_SPEED;
		else
			dwLatency = INDIGO_IO_OUTPUT_LATENCY_DOUBLE_SPEED;	
	}
	else
	{
		if (dwSampleRate < 50000)
			dwLatency = INDIGO_IO_INPUT_LATENCY_SINGLE_SPEED;
		else
			dwLatency = INDIGO_IO_INPUT_LATENCY_DOUBLE_SPEED;
	}
	
	pLatency->dwLatency = dwLatency;
	
}	// GetAudioLatency

// *** CIndigoIO.cpp ***
