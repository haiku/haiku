// ****************************************************************************
//
//  	CLayla24DspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Layla24 DSP
//		interface class.
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

#include "CEchoGals.h"
#include "CLayla24DspCommObject.h"

#include LAYLA24_DSP_FILENAME
#include "Layla24_1ASIC.c"
#include "Layla24_2A_ASIC.c"
#include LAYLA24_2ASIC_FILENAME

//
// The ASIC files for Layla24 are always this size
//
#define LAYLA24_ASIC_SIZE		31146


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CLayla24DspCommObject::CLayla24DspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGMLDspCommObject( pdwRegBase, pOsSupport )
{

	strcpy( m_szCardName, LAYLA24_CARD_NAME);

	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base
	
	m_wNumPipesOut = 16;
	m_wNumPipesIn = 16;
	m_wNumBussesOut = 16;
	m_wNumBussesIn = 16;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 8;

	m_fHasVmixer = LAYLA24_HAS_VMIXER;

	m_wNumMidiOut = 1;					// # MIDI out channels
	m_wNumMidiIn = 1;						// # MIDI in  channels
	m_bHasASIC = TRUE;
	
	m_pwDspCodeToLoad = LAYLA24_DSP_CODE;

	m_byDigitalMode = DIGITAL_MODE_SPDIF_RCA;
	m_bProfessionalSpdif = FALSE;
	m_wMtcState = MIDI_IN_STATE_NORMAL;
	
	m_dwSampleRate = 48000;
	
}	// CLayla24DspCommObject::CLayla24DspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CLayla24DspCommObject::~CLayla24DspCommObject()
{
	ECHO_DEBUGPRINTF( ( "CLayla24DspCommObject::~CLayla24DspCommObject() "
							  "is toast!\n" ) );
}	// CLayla24DspCommObject::~CLayla24DspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// Layla24 has an ASIC on the PCI card and another ASIC in the external box; 
// both need to be loaded.
//
//===========================================================================

BOOL CLayla24DspCommObject::LoadASIC()
{
	DWORD	dwControlReg;

	if ( m_bASICLoaded == TRUE )
		return TRUE;
		
	ECHO_DEBUGPRINTF(("CLayla24DspCommObject::LoadASIC\n"));

	//
	// Give the DSP a few milliseconds to settle down
	//
	m_pOsSupport->OsSnooze( 10000 );

	//
	// Load the ASIC for the PCI card
	//
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_LAYLA24_PCI_CARD_ASIC,
											  pbLayla24_1ASIC,
											  sizeof( pbLayla24_1ASIC ) ) )
		return FALSE;

	m_pbyAsic = pbLayla24_2S_ASIC;	

	//
	// Now give the new ASIC a little time to set up
	//
	m_pOsSupport->OsSnooze( 10000 );

	//
	// Do the external one
	//
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_LAYLA24_EXTERNAL_ASIC,
											  pbLayla24_2S_ASIC,
											  sizeof( pbLayla24_2S_ASIC ) ) )
		return FALSE;

	//
	// Now give the external ASIC a little time to set up
	//
	m_pOsSupport->OsSnooze( 10000 );
		
	//
	// See if it worked
	//
	CheckAsicStatus();

	//
	// Set up the control register if the load succeeded -
	//
	// 48 kHz, internal clock, S/PDIF RCA mode
	//	
	if ( m_bASICLoaded )
	{
		dwControlReg = GML_CONVERTER_ENABLE | GML_48KHZ;
		WriteControlReg( dwControlReg, TRUE );	
		
		m_dwSampleRate = 48000;
	}

	ECHO_DEBUGPRINTF(("\tFinished\n"));
		
	return m_bASICLoaded;
}	// BOOL CLayla24DspCommObject::LoadASIC()



//===========================================================================
//
// SetSampleRate
// 
// Set the sample rate for Layla24
//
// Layla24 is simple; just send it the sampling rate (assuming that the clock
// mode is correct).
//
//===========================================================================

DWORD CLayla24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	DWORD		dwControlReg, dwNewClock, dwBaseRate;
	BOOL		bSetFreqReg = FALSE;
	
	//
	// Only set the clock for internal mode.  If the clock is not set to
	// internal, try and re-set the input clock; this more transparently
	// handles switching between single and double-speed mode
	//
	if ( GetInputClock() != ECHO_CLOCK_INTERNAL )
	{
		ECHO_DEBUGPRINTF( ( "CLayla24DspCommObject::SetSampleRate: Cannot set sample rate - "
								  "clock not set to CLK_CLOCKININTERNAL\n" ) );
		
		//
		// Save the rate anyhow
		//
		m_dwSampleRate = SWAP( dwNewSampleRate );
		
		//
		// Set the input clock to the current value
		//
		SetInputClock( m_wInputClock );
		
		return dwNewSampleRate;
	}

	//
	// Get the control register & clear the appropriate bits
	// 
	dwControlReg = GetControlRegister();
	dwControlReg &= GML_CLOCK_CLEAR_MASK;
	dwControlReg &= GML_SPDIF_RATE_CLEAR_MASK;
	
	//
	// Set the sample rate
	//
	bSetFreqReg = FALSE;

	switch ( dwNewSampleRate )
	{
		case 96000 :
			dwNewClock = GML_96KHZ;
			break;
		
		case 88200 :
			dwNewClock = GML_88KHZ;
			break;
		
		case 48000 : 
			dwNewClock = GML_48KHZ | GML_SPDIF_SAMPLE_RATE1;
			break;
		
		case 44100 : 
			dwNewClock = GML_44KHZ;

			//
			// Professional mode
			//
			if ( dwControlReg & GML_SPDIF_PRO_MODE )
			{
				dwNewClock |= GML_SPDIF_SAMPLE_RATE0;
			}
			break;
		
		case 32000 :
			dwNewClock = GML_32KHZ | 
							 GML_SPDIF_SAMPLE_RATE0 | 
							 GML_SPDIF_SAMPLE_RATE1;
			break;
		
		case 22050 :
			dwNewClock = GML_22KHZ;
			break;
		
		case 16000 :
			dwNewClock = GML_16KHZ;
			break;
		
		case 11025 :
			dwNewClock = GML_11KHZ;
			break;
		
		case 8000 :
			dwNewClock = GML_8KHZ;
			break;

		default :
			//
			// Set flag to write the frequency register
			//
			bSetFreqReg = TRUE;

			//
			// Set for continuous mode
			//
			dwNewClock = LAYLA24_CONTINUOUS_CLOCK;
	}

	dwControlReg |= dwNewClock;

	//
	// If this is a non-standard rate, then the driver
	// needs to use Layla24's special "continuous frequency" mode
	//
	if ( bSetFreqReg )
	{
		if ( dwNewSampleRate > 50000 )
		{
			dwBaseRate = dwNewSampleRate >> 1;
			dwControlReg |= GML_DOUBLE_SPEED_MODE;
		}
		else
		{
			dwBaseRate = dwNewSampleRate;
		}
		
		if ( dwBaseRate < 25000 )
			dwBaseRate = 25000;

		if ( !WaitForHandshake() )
			return 0xffffffff;

		m_pDspCommPage->dwSampleRate = 
			SWAP( LAYLA24_MAGIC_NUMBER / dwBaseRate - 2 );

		ClearHandshake();			
		SendVector( DSP_VC_SET_LAYLA24_FREQUENCY_REG );
	}
	
	//
	// Tell the DSP about it
	//
	if ( ECHOSTATUS_OK == WriteControlReg( dwControlReg ) )
	{
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

		ECHO_DEBUGPRINTF( ("CLayla24DspCommObject::SetSampleRate: %ld "
								 "clock %lx\n", dwNewSampleRate, dwControlReg) );
	}
	
	m_dwSampleRate = dwNewSampleRate;

	return dwNewSampleRate;

}	// DWORD CLayla24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
//
// SetDigitalMode
// 
//===========================================================================

ECHOSTATUS CLayla24DspCommObject::SetDigitalMode
(
	BYTE	byNewMode
)
{
	BOOL		AsicOk;
	
	//
	// Change the ASIC
	//
	switch ( byNewMode )
	{
		case DIGITAL_MODE_SPDIF_RCA :
		case DIGITAL_MODE_SPDIF_OPTICAL :
			//
			// If the currently loaded ASIC is the S/PDIF ASIC, switch
			// to the ADAT ASIC
			//
			AsicOk = SwitchAsic( pbLayla24_2S_ASIC, sizeof( pbLayla24_2S_ASIC ) );
			break;
			
		case DIGITAL_MODE_ADAT :
			//
			// If the currently loaded ASIC is the S/PDIF ASIC, switch
			// to the ADAT ASIC
			//
			AsicOk = SwitchAsic( pbLayla24_2A_ASIC, sizeof( pbLayla24_2A_ASIC ) );
			break;	

		default :
			return ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED;
	}
	
	if (FALSE == AsicOk)
		return ECHOSTATUS_ASIC_NOT_LOADED;

	//
	// Call the base class to tweak the input clock if necessary
	//
	return CGMLDspCommObject::SetDigitalMode(byNewMode);

}	// ECHOSTATUS CLaya24DspCommObject::SetDigitalMode


//===========================================================================
//
// Depending on what digital mode you want, Layla24 needs different ASICs 
//	loaded.  This function checks the ASIC needed for the new mode and sees 
// if it matches the one already loaded.
//
//===========================================================================

BOOL CLayla24DspCommObject::SwitchAsic
(
	BYTE *	pbyAsicNeeded,
	DWORD		dwAsicSize
)
{
	BOOL rval;

	//
	//	Check to see if this is already loaded
	// 
	rval = TRUE;
	if ( pbyAsicNeeded != m_pbyAsic )
	{
		BYTE	byMonitors[ MONITOR_ARRAY_SIZE ];
		memmove( byMonitors, m_pDspCommPage->byMonitors, MONITOR_ARRAY_SIZE );
		memset( m_pDspCommPage->byMonitors,
				  GENERIC_TO_DSP(ECHOGAIN_MUTED),
				  MONITOR_ARRAY_SIZE );

		//
		// Load the desired ASIC
		//
		rval = CDspCommObject::LoadASIC(DSP_FNC_LOAD_LAYLA24_EXTERNAL_ASIC,
												  pbyAsicNeeded,
												  dwAsicSize );
		if (FALSE != rval)
		{
			m_pbyAsic = pbyAsicNeeded;	
		}
		memmove( m_pDspCommPage->byMonitors, byMonitors, MONITOR_ARRAY_SIZE );
	}
	
	return rval;

}	// BOOL CLayla24DspCommObject::SwitchAsic( DWORD dwMask96 )


//===========================================================================
//
// SetInputClock
//
//===========================================================================

ECHOSTATUS CLayla24DspCommObject::SetInputClock(WORD wClock)
{
	BOOL			bSetRate;
	BOOL			bWriteControlReg;
	DWORD			dwControlReg, dwSampleRate;

	ECHO_DEBUGPRINTF( ( "CLayla24DspCommObject::SetInputClock:\n" ) );

	dwControlReg = GetControlRegister();

	//
	// Mask off the clock select bits
	//
	dwControlReg &= GML_CLOCK_CLEAR_MASK;
	dwSampleRate = GetSampleRate();
	
	bSetRate = FALSE;
	bWriteControlReg = TRUE;

	//
	// Pick the new clock
	//
	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to INTERNAL\n" ) );

			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ( ( GetSampleRate() < 8000 ) ||
			     ( GetSampleRate() > 100000 ) )
			{
				m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
				m_dwSampleRate = 48000;
			}

			bSetRate = TRUE;
			bWriteControlReg = FALSE;
			break;

		case ECHO_CLOCK_SPDIF:
			if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to SPDIF\n" ) );
	
			dwControlReg |= GML_SPDIF_CLOCK;

/* 
	Since Layla24 doesn't support 96 kHz S/PDIF, this can be ignored
			if ( GML_CLOCK_DETECT_BIT_SPDIF96 & GetInputClockDetect() )
			{
				dwControlReg |= GML_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			}
*/
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to SPDIF\n" ) );
			break;

		case ECHO_CLOCK_WORD: 
			dwControlReg |= GML_WORD_CLOCK;
			
			if ( GML_CLOCK_DETECT_BIT_WORD96 & GetInputClockDetect() )
			{
				dwControlReg |= GML_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			}
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to WORD\n" ) );
			break;


		case ECHO_CLOCK_ADAT :
			if ( DIGITAL_MODE_ADAT != GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			dwControlReg |= GML_ADAT_CLOCK;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to ADAT\n" ) );
			break;

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Layla24\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;

	}		// switch (wClock)

	//
	// Winner! Save the new input clock.
	//
	m_wInputClock = wClock;

	//
	// Do things according to the flags
	//
	if ( bWriteControlReg )
	{
		WriteControlReg( dwControlReg, TRUE );
	}
	
	//
	// If the user just switched to internal clock,
	// set the sample rate
	//
	if ( bSetRate )
		SetSampleRate( m_dwSampleRate );

	return ECHOSTATUS_OK;

}		// ECHOSTATUS CLayla24DspCommObject::SetInputClock()


// **** Layla24DspCommObject.cpp ****
