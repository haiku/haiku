// ****************************************************************************
//
//  	CLayla24DspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Layla24 DSP
//		interface class.
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

#include "CEchoGals.h"
#include "CLayla24DspCommObject.h"

#include "Layla24DSP.c"		// regular DSP code

#include "Layla24_1ASIC.c"
#include "Layla24_2A_ASIC.c"
#include "Layla24_2S_ASIC.c"

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

	strcpy( m_szCardName, "Layla24" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base
	
	m_wNumPipesOut = 16;
	m_wNumPipesIn = 16;
	m_wNumBussesOut = 16;
	m_wNumBussesIn = 16;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 8;
	m_fHasVmixer = FALSE;

	m_wNumMidiOut = 1;					// # MIDI out channels
	m_wNumMidiIn = 1;						// # MIDI in  channels
	m_bHasASIC = TRUE;
	
	m_pwDspCodeToLoad = pwLayla24DSP;

	m_byDigitalMode = DIGITAL_MODE_SPDIF_RCA;
	m_bProfessionalSpdif = FALSE;
	m_wMtcState = MTC_STATE_NORMAL;
	
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
		WriteControlReg( dwControlReg );	
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
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		
		//
		// Set the input clock to the current value
		//
		SetInputClock( m_wInputClock );
		
		return GetSampleRate();
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
								 "clock %ld\n", dwNewSampleRate, dwControlReg) );
	}

	return GetSampleRate();

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
	DWORD		dwControlReg;

	dwControlReg = GetControlRegister();
	//
	// Clear the current digital mode
	//
	dwControlReg &= GML_DIGITAL_MODE_CLEAR_MASK;

	//
	// Tweak the control reg
	//
	switch ( byNewMode )
	{
		default :
			return ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED;

		case DIGITAL_MODE_SPDIF_OPTICAL :

			dwControlReg |= GML_SPDIF_OPTICAL_MODE;

			// fall through 
		
		case DIGITAL_MODE_SPDIF_RCA :
			//
			//	If the input clock is set to ADAT, set the 
			// input clock to internal and the sample rate to 48 KHz
			// 
			if ( ECHO_CLOCK_ADAT == GetInputClock() )
			{
				m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
				SetInputClock( ECHO_CLOCK_INTERNAL );
			}
			//
			// If the currently loaded ASIC is the S/PDIF ASIC, switch
			// to the ADAT ASIC
			//
			if ( !SwitchAsic( pbLayla24_2S_ASIC, sizeof( pbLayla24_2S_ASIC ) ) )
				SetInputClock( ECHO_CLOCK_INTERNAL );

			break;
			
		case DIGITAL_MODE_ADAT :
			//
			//	If the input clock is set to S/PDIF, set the 
			// input clock to internal and the sample rate to 48 KHz
			// 
			if ( ECHO_CLOCK_SPDIF == GetInputClock() )
			{
				m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
				SetInputClock( ECHO_CLOCK_INTERNAL );
			}
			//
			// If the currently loaded ASIC is the S/PDIF ASIC, switch
			// to the ADAT ASIC
			//
			if ( !SwitchAsic( pbLayla24_2A_ASIC, sizeof( pbLayla24_2A_ASIC ) ) )
				return( ECHOSTATUS_ASIC_NOT_LOADED );

			dwControlReg |= GML_ADAT_MODE;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			break;	
	}

	//
	// Write the control reg
	//
	WriteControlReg( dwControlReg );

	m_byDigitalMode = byNewMode;

	ECHO_DEBUGPRINTF( ("CLayla24DspCommObject::SetDigitalMode to %ld\n",
							(DWORD) m_byDigitalMode) );

	return ECHOSTATUS_OK;

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
	//
	//	Check to see if this is already loaded
	// 
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
		if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_LAYLA24_EXTERNAL_ASIC,
												  pbyAsicNeeded,
												  dwAsicSize ) )
		{
			memmove( m_pDspCommPage->byMonitors, byMonitors, MONITOR_ARRAY_SIZE );
			return FALSE;
		}
		m_pbyAsic = pbyAsicNeeded;	
		memmove( m_pDspCommPage->byMonitors, byMonitors, MONITOR_ARRAY_SIZE );
	}
	
	return TRUE;

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
		WriteControlReg( dwControlReg );
	}
	
	//
	// If the user just switched to internal clock,
	// set the sample rate
	//
	if ( bSetRate )
		SetSampleRate();

	return ECHOSTATUS_OK;

}		// ECHOSTATUS CLayla24DspCommObject::SetInputClock()



//===========================================================================
//
// Detect MIDI output activity
//
//===========================================================================

BOOL CLayla24DspCommObject::IsMidiOutActive()
{
	ULONGLONG	ullCurTime;

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	return( ( ( ullCurTime - m_ullMidiOutTime ) > MIDI_ACTIVITY_TIMEOUT_USEC ) ? FALSE : TRUE );
	
}	// BOOL CLayla24DspCommObject::IsMidiOutActive()

// **** Layla24DspCommObject.cpp ****
