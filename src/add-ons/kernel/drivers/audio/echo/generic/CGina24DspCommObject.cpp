// ****************************************************************************
//
//  	CGina24DspCommObject.cpp
//
//		Implementation file for Gina24 DSP interface class.
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
#include "CGina24DspCommObject.h"

#include "Gina24DSP.c"
#include "Gina24_361DSP.c"

#include "Gina24ASIC.c"
#include "Gina24ASIC_361.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CGina24DspCommObject::CGina24DspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGMLDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Gina24" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base
	
	m_wNumPipesOut = 16;
	m_wNumPipesIn = 10;
	m_wNumBussesOut = 16;
	m_wNumBussesIn = 10;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 2;
	
	m_fHasVmixer = FALSE;	

	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels
	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );
												// Need this in cse we start with ESYNC
	m_bHasASIC = TRUE;

	//
	//	Gina24 comes in both '301 and '361 flavors; pick the correct one.
 	// 
	if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
		m_pwDspCodeToLoad = pwGina24_361DSP;
	else
		m_pwDspCodeToLoad = pwGina24DSP;

	m_byDigitalMode = DIGITAL_MODE_SPDIF_RCA;
	m_bProfessionalSpdif = FALSE;
}	// CGina24DspCommObject::CGina24DspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CGina24DspCommObject::~CGina24DspCommObject()
{
	ECHO_DEBUGPRINTF(("CGina24DspCommObject::~CGina24DspCommObject - "
							"hasta la vista!\n"));
}	// CGina24DspCommObject::~CGina24DspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// Gina24 has an ASIC on the PCI card which must be loaded for anything
// interesting to happen.
//
//===========================================================================

BOOL CGina24DspCommObject::LoadASIC()
{
	DWORD	dwControlReg, dwSize;
	PBYTE	pbAsic;

	if ( m_bASICLoaded )
		return TRUE;

	//
	// Give the DSP a few milliseconds to settle down
	//
	m_pOsSupport->OsSnooze( 10000 );

	//
	// Pick the correct ASIC for '301 or '361 Gina24
	//
	if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
	{
		pbAsic = pbGina24ASIC_361;
		dwSize = sizeof( pbGina24ASIC_361 );
	}
	else
	{
		pbAsic = pbGina24ASIC;
		dwSize = sizeof( pbGina24ASIC );
	}
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_GINA24_ASIC,
											  pbAsic,
											  dwSize ) )
			return FALSE;

	m_pbyAsic = pbAsic;

	//
	// Now give the new ASIC a little time to set up
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
	}
		
	return m_bASICLoaded;
	
}	// BOOL CGina24DspCommObject::LoadASIC()


//===========================================================================
//
// Set the input clock to internal, S/PDIF, ADAT
//
//===========================================================================

ECHOSTATUS CGina24DspCommObject::SetInputClock(WORD wClock)
{
	BOOL			bSetRate;
	BOOL			bWriteControlReg;
	DWORD			dwControlReg;
	
	ECHO_DEBUGPRINTF( ("CGina24DspCommObject::SetInputClock:\n") );

	dwControlReg = GetControlRegister();

	//
	// Mask off the clock select bits
	//
	dwControlReg &= GML_CLOCK_CLEAR_MASK;
	
	bSetRate = FALSE;
	bWriteControlReg = TRUE;
	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
		{
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to INTERNAL\n" ) );
	
			bSetRate = TRUE;
			bWriteControlReg = FALSE;
			break;
		} // ECHO_CLOCK_INTERNAL

		case ECHO_CLOCK_SPDIF :
		{
			if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to SPDIF\n" ) );
	
			dwControlReg |= GML_SPDIF_CLOCK;

			if ( GML_CLOCK_DETECT_BIT_SPDIF96 & GetInputClockDetect() )
			{
				dwControlReg |= GML_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			}
			break;
		} // ECHO_CLOCK_SPDIF

		case ECHO_CLOCK_ADAT :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to ADAT\n" ) );
			
			if ( DIGITAL_MODE_ADAT != GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			dwControlReg |= GML_ADAT_CLOCK;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			break;
		} // ECHO_CLOCK_ADAT

		case ECHO_CLOCK_ESYNC :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to ESYNC\n" ) );
			
			dwControlReg |= GML_ESYNC_CLOCK;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			break;
		} // ECHO_CLOCK_ESYNC

		case ECHO_CLOCK_ESYNC96 :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to ESYNC96\n" ) );
			
			dwControlReg |= GML_ESYNC_CLOCK | GML_DOUBLE_SPEED_MODE;
			break;
		} // ECHO_CLOCK_ESYNC96

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Gina24\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}	// switch (wInputClock)
	

	//
	// Winner! Save the new input clock.
	//
	m_wInputClock = wClock;

	//
	// Write the control reg if that's called for
	//
	if ( bWriteControlReg )
	{
		WriteControlReg( dwControlReg, TRUE );
	}
	
	// Set Gina24 sample rate to something sane if word or superword is
	// being turned off
	if ( bSetRate )
	{
		SetSampleRate( GetSampleRate() );
	}
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CGina24DspCommObject::SetInputClock


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Gina24
//
//===========================================================================

DWORD CGina24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	DWORD	dwControlReg, dwNewClock;

	//
	// Only set the clock for internal mode.  If the clock is not set to
	// internal, try and re-set the input clock; this more transparently
	// handles switching between single and double-speed mode
	//
	if ( GetInputClock() != ECHO_CLOCK_INTERNAL )
	{
		ECHO_DEBUGPRINTF( ( "CGina24DspCommObject::SetSampleRate: Cannot set sample rate - "
								  "clock not set to CLK_CLOCKININTERNAL\n" ) );
		
		//
		// Save the rate anyhow
		//
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		
		//
		// Set the input clock to the current value
		//
		SetInputClock( m_wInputClock );
		
		return GetSampleRate();
	}

	//
	// Set the sample rate
	//
	dwNewClock = 0;
	
	dwControlReg = GetControlRegister();
	dwControlReg &= GML_CLOCK_CLEAR_MASK;
	dwControlReg &= GML_SPDIF_RATE_CLEAR_MASK;

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
			ECHO_DEBUGPRINTF( ("CGina24DspCommObject::SetSampleRate: %ld "
									 "invalid!\n", dwNewSampleRate) );
			ECHO_DEBUGBREAK();
			return( GetSampleRate() );
	}

	dwControlReg |= dwNewClock;

	//
	// Send the new value to the DSP
	//
	if ( ECHOSTATUS_OK == WriteControlReg( dwControlReg ) )
	{
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

		ECHO_DEBUGPRINTF( ("CGina24DspCommObject::SetSampleRate: %ld "
								 "clock %ld\n", dwNewSampleRate, dwNewClock) );
	}

	return GetSampleRate();

} // DWORD CGina24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )



// **** CGina24DspCommObject.cpp ****
