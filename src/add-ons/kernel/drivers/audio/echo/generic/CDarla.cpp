// ****************************************************************************
//
//		CDarla.cpp
//
//		Implementation file for the CDarla driver class; this supports 20 bit
//		Darla, not Darla24.
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

#include "CDarla.h"

#define DARLA20_ANALOG_OUTPUT_LATENCY	137
#define DARLA20_ANALOG_INPUT_LATENCY	240


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CDarla::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CDarla::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CDarla::operator new( size_t Size )


VOID  CDarla::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CDarla::operator delete memory free failed\n"));
	}
}	// VOID  CDarla::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

CDarla::CDarla( PCOsSupport pOsSupport )
		: CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CDarla::CDarla() is born!\n" ) );
	
	m_wAnalogOutputLatency = DARLA20_ANALOG_OUTPUT_LATENCY;
	m_wAnalogInputLatency = DARLA20_ANALOG_INPUT_LATENCY;
}

CDarla::~CDarla()
{
	ECHO_DEBUGPRINTF( ( "CDarla::~CDarla() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS CDarla::InitHw()
{
	ECHOSTATUS	Status;
	
	//
	// Call the base InitHw method
	//
	if ( ECHOSTATUS_OK != ( Status = CEchoGals::InitHw() ) )
		return Status;

	//
	// Create the DSP comm object 
	//
	ECHO_ASSERT(NULL == m_pDspCommObject );
	m_pDspCommObject = new CDarlaDspCommObject( (PDWORD) m_pvSharedMemory,
																m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("CDarla::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}
	
	//
	// Load the DSP
	//
	GetDspCommObject()->LoadFirmware();
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag
	//
	m_wFlags &= ~ECHOGALS_FLAG_BADBOARD;
	
	//
	//	Must call this here *after* DSP is init
	//
	Status = InitLineLevels();
	
	//
	//	Get default sample rate from DSP
	//
	m_dwSampleRate = GetDspCommObject()->GetSampleRate();
	
	ECHO_DEBUGPRINTF( ( "CDarla::InitHw()\n" ) );
	
	return Status;
	
}	// ECHOSTATUS CDarla::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS CDarla::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	if ( dwSampleRate != 44100 &&
		  dwSampleRate != 48000 )
	{
		ECHO_DEBUGPRINTF(
			("CDarla::QueryAudioSampleRate() - rate %ld invalid\n",
				dwSampleRate));
		return ECHOSTATUS_BAD_FORMAT;
	}
	
	ECHO_DEBUGPRINTF( ( "CDarla::QueryAudioSampleRate()\n" ) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDarla::QueryAudioSampleRate


void CDarla::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 44100;
	dwMaxRate = 48000;
}



// *** CDarla.cpp ***
