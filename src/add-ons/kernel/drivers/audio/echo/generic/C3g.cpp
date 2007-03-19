// ****************************************************************************
//
//		C3g.cpp
//
//		Implementation file for the C3g driver class.
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

#include "C3g.h"

#define ECHO3G_ANALOG_OUTPUT_LATENCY_1X		(1 + 32 + 12)		// ASIC + DSP + DAC
#define ECHO3G_ANALOG_OUTPUT_LATENCY_2X		(1 + 32 + 5)
#define ECHO3G_ANALOG_INPUT_LATENCY_1X			(1 + 32 + 12)
#define ECHO3G_ANALOG_INPUT_LATENCY_2X			(1 + 32 + 9)

#define ECHO3G_DIGITAL_OUTPUT_LATENCY			(1 + 32)
#define ECHO3G_DIGITAL_INPUT_LATENCY			(1 + 32)



/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID C3g::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("C3g::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID C3g::operator new( size_t Size )


VOID  C3g::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("C3g::operator delete memory free failed\n"));
	}
}	// VOID C3g::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor and destructor
//
//===========================================================================

C3g::C3g( PCOsSupport pOsSupport )
	  : CEchoGalsMTC( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "C3g::C3g() is born!\n" ) );

	m_wAnalogOutputLatency = ECHO3G_ANALOG_OUTPUT_LATENCY_1X;
	m_wAnalogInputLatency = ECHO3G_ANALOG_INPUT_LATENCY_1X;
	m_wDigitalOutputLatency = ECHO3G_DIGITAL_OUTPUT_LATENCY;
	m_wDigitalInputLatency = ECHO3G_DIGITAL_INPUT_LATENCY;
}


C3g::~C3g()
{
	ECHO_DEBUGPRINTF( ( "C3g::~C3g() is toast!\n" ) );
}




/****************************************************************************

	Setup and hardware initialization

 ****************************************************************************/

//===========================================================================
//
// Every card has an InitHw method
//
//===========================================================================

ECHOSTATUS C3g::InitHw()
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
	m_pDspCommObject = new C3gDco( (PDWORD) m_pvSharedMemory, m_pOsSupport );
	if (NULL == m_pDspCommObject)
	{
		ECHO_DEBUGPRINTF(("C3g::InitHw - could not create DSP comm object\n"));
		return ECHOSTATUS_NO_MEM;
	}

	//
	// Load the DSP
	//
	DWORD dwBoxType;
	
	GetDspCommObject()->LoadFirmware();
	
	GetDspCommObject()->Get3gBoxType(&dwBoxType,NULL);
	if (NO3GBOX == dwBoxType)
		return ECHOSTATUS_NO_3G_BOX;
	
	if ( GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	// Clear the "bad board" flag; set the flags to indicate that
	// 3G can handle super-interleave.
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
	
	ECHO_DEBUGPRINTF( ( "C3g::InitHw()\n" ) );
	return Status;

}	// ECHOSTATUS C3g::InitHw()




/****************************************************************************

	Informational methods

 ****************************************************************************/

//===========================================================================
//
// Override GetCapabilities to enumerate unique capabilties for this card
//
//===========================================================================

ECHOSTATUS C3g::GetCapabilities
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
	// Add nominal level control to all ins & outs except the universal
	//
	for (i = 0 ; i < pCapabilities->wNumBussesOut; i++)
	{
		pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	for (i = 2 ; i < pCapabilities->wNumBussesIn; i++)
	{
		pCapabilities->dwBusInCaps[i] |= ECHOCAPS_NOMINAL_LEVEL;
	}

	pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_SPDIF		|
												ECHO_CLOCK_BIT_ADAT		|
												ECHO_CLOCK_BIT_MTC;
												
	//
	// Box-specific capabilities
	//
	DWORD dwBoxType;
	
	GetDspCommObject()->Get3gBoxType(&dwBoxType,NULL);
	switch (dwBoxType)
	{
		case GINA3G :
			pCapabilities->dwBusInCaps[0] |= ECHOCAPS_PHANTOM_POWER;
			pCapabilities->dwBusInCaps[1] |= ECHOCAPS_PHANTOM_POWER;
			break;
			
		case LAYLA3G :
			pCapabilities->dwInClockTypes |= ECHO_CLOCK_BIT_WORD;
			break;
	
	}

	pCapabilities->dwOutClockTypes = 0;
	
	return Status;

}	// ECHOSTATUS C3g::GetCapabilities


//===========================================================================
//
// QueryAudioSampleRate is used to find out if this card can handle a 
// given sample rate.
//
//===========================================================================

ECHOSTATUS C3g::QueryAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	//
	// Check rates that are supported by continuous mode; only allow
	// double-speed rates if not in ADAT mode
	//
	if ((dwSampleRate >= 32000) && (dwSampleRate <= 50000))
		return ECHOSTATUS_OK;

	if (	(DIGITAL_MODE_ADAT != GetDigitalMode()) && 
			(dwSampleRate > 50000) && 
			(dwSampleRate <= 100000))
		return ECHOSTATUS_OK;

	ECHO_DEBUGPRINTF(("C3g::QueryAudioSampleRate() - rate %ld invalid\n",dwSampleRate) );

	return ECHOSTATUS_BAD_FORMAT;

}	// ECHOSTATUS C3g::QueryAudioSampleRate


void C3g::QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate)
{
	dwMinRate = 32000;
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
//===========================================================================

ECHOSTATUS C3g::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("C3g::GetInputClockDetect: DSP Dead!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}
					 
	//
	// Map the DSP clock detect bits to the generic driver clock detect bits
	//
	DWORD dwClocksFromDsp = GetDspCommObject()->GetInputClockDetect();	

	dwClockDetectBits = ECHO_CLOCK_BIT_INTERNAL | ECHO_CLOCK_BIT_MTC;
	
	if (0 != (dwClocksFromDsp & E3G_CLOCK_DETECT_BIT_WORD))
		dwClockDetectBits |= ECHO_CLOCK_BIT_WORD;

	switch (GetDigitalMode())
	{
		case DIGITAL_MODE_SPDIF_RCA :
		case DIGITAL_MODE_SPDIF_OPTICAL :
			if (0 != (dwClocksFromDsp & E3G_CLOCK_DETECT_BIT_SPDIF))
				dwClockDetectBits |= ECHO_CLOCK_BIT_SPDIF;
			break;

		case DIGITAL_MODE_ADAT :	
			if (0 != (dwClocksFromDsp & E3G_CLOCK_DETECT_BIT_ADAT))
				dwClockDetectBits |= ECHO_CLOCK_BIT_ADAT;
			break;
	}	
		
	return ECHOSTATUS_OK;
	
}	// GetInputClockDetect


//===========================================================================
//
// Get the external box type
//
//===========================================================================

void C3g::Get3gBoxType(DWORD *pOriginalBoxType,DWORD *pCurrentBoxType)
{
	GetDspCommObject()->Get3gBoxType(pOriginalBoxType,pCurrentBoxType);
}


//===========================================================================
//
// Get the external box name
//
//===========================================================================

char *C3g::Get3gBoxName()
{
	char *pszName;
	DWORD dwBoxType;
	
	GetDspCommObject()->Get3gBoxType(&dwBoxType,NULL);
	switch (dwBoxType)
	{
		case GINA3G :
			pszName = "Gina3G";
			break;
		
		case LAYLA3G :
			pszName = "Layla3G";
			break;
			
		case NO3GBOX :
		default :
			pszName = "Echo3G";
			break;
	}
	
	return pszName;
}


//===========================================================================
//
// Get phantom power state for Gina3G
//
//===========================================================================

void C3g::GetPhantomPower(BOOL *pfPhantom)
{
	*pfPhantom = m_fPhantomPower;
}


//===========================================================================
//
// Set phantom power state for Gina3G
//
//===========================================================================

void C3g::SetPhantomPower(BOOL fPhantom)
{
	DWORD dwBoxType;
	
	GetDspCommObject()->Get3gBoxType(&dwBoxType,NULL);
	if (GINA3G == dwBoxType)
	{
		GetDspCommObject()->SetPhantomPower( fPhantom );
		m_fPhantomPower = fPhantom;
	}
}


//===========================================================================
//
// GetAudioLatency - returns the latency for a single pipe
//
//===========================================================================

void C3g::GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency)
{
	DWORD dwSampleRate;

	//
	// Adjust the stored latency values based on the sample rate
	//
	dwSampleRate = GetDspCommObject()->GetSampleRate();
	if (dwSampleRate <= 50000)
	{
		m_wAnalogOutputLatency = ECHO3G_ANALOG_OUTPUT_LATENCY_1X;
		m_wAnalogInputLatency = ECHO3G_ANALOG_INPUT_LATENCY_1X;
	}
	else
	{
		m_wAnalogOutputLatency = ECHO3G_ANALOG_OUTPUT_LATENCY_2X;
		m_wAnalogInputLatency = ECHO3G_ANALOG_INPUT_LATENCY_2X;
	}

	//
	// Let the base class worry about analog vs. digital
	//
	CEchoGals::GetAudioLatency(pLatency);
	
}	// GetAudioLatency



//===========================================================================
//
//	Start transport for a group of pipes
//
// Use this to make sure no one tries to start digital channels 3-8 
// with the hardware in double speed mode.
//
//===========================================================================

ECHOSTATUS C3g::Start
(
	PCChannelMask	pChannelMask
)
{
	PC3gDco pDCO;

	//
	// Double speed mode?
	//
	pDCO = GetDspCommObject();
	if (pDCO->DoubleSpeedMode())
	{
		BOOL intersect;
		
		//
		// See if ADAT in 3-8 or out 3-8 are being started
		//		
		intersect = pChannelMask->IsIntersectionOf( pDCO->m_Adat38Mask );
		if (intersect)
		{
			ECHO_DEBUGPRINTF(("Cannot start ADAT channels 3-8 in double speed mode\n"));
			return ECHOSTATUS_INVALID_CHANNEL;
		}
	}

	return CEchoGals::Start(pChannelMask);
}

// *** C3g.cpp ***
