// ****************************************************************************
//
//  	CGina24DspCommObject.cpp
//
//		Implementation file for Gina24 DSP interface class.
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
		WriteControlReg( dwControlReg );	
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
	DWORD			dwControlReg, dwSampleRate;
	
	ECHO_DEBUGPRINTF( ("CGina24DspCommObject::SetInputClock:\n") );

	dwControlReg = GetControlRegister();

	//
	// Mask off the clock select bits
	//
	dwControlReg &= GML_CLOCK_CLEAR_MASK;
	dwSampleRate = GetSampleRate();
	
	bSetRate = FALSE;
	bWriteControlReg = TRUE;
	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
		{
			ECHO_DEBUGPRINTF( ( "\tSet Gina24 clock to INTERNAL\n" ) );
	
			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ( ( dwSampleRate < 8000  ) ||
			     ( dwSampleRate > 96000 ) )
			{
				dwSampleRate = 48000;
			}

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
		WriteControlReg( dwControlReg );
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
// Set the audio sample rate for Gina24 - fixme make this common for
// Gina24 & Mona
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


//===========================================================================
//
// SetDigitalMode
// 
//===========================================================================

ECHOSTATUS CGina24DspCommObject::SetDigitalMode
(
	BYTE	byNewMode
)
{
	DWORD		dwControlReg;
	//
	//	'361 Gina24 cards do not have the S/PDIF CD-ROM mode
	// 
	if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() &&
		 ( DIGITAL_MODE_SPDIF_CDROM == byNewMode ) )
	{
		return FALSE;
	}

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
			goto ChkClk;

		case DIGITAL_MODE_SPDIF_CDROM :

			dwControlReg |= GML_SPDIF_CDROM_MODE;

			// fall through 
		
		case DIGITAL_MODE_SPDIF_RCA :
ChkClk:
			//
			//	If the input clock is set to ADAT, set the 
			// input clock to internal and the sample rate to 48 KHz
			// 
			if ( ECHO_CLOCK_ADAT == GetInputClock() )
			{
				m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
				SetInputClock( ECHO_CLOCK_INTERNAL );
			}
		
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

			dwControlReg |= GML_ADAT_MODE;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			break;	
	}
	
	//
	// Write the control reg
	//
	WriteControlReg( dwControlReg );

	m_byDigitalMode = byNewMode;

	ECHO_DEBUGPRINTF( ("CGina24DspCommObject::SetDigitalMode to %ld\n",
							(DWORD) m_byDigitalMode) );

	return ECHOSTATUS_OK;
}	// ECHOSTATUS CGina24DspCommObject::SetDigitalMode




/****************************************************************************

	Common for Gina24, Layla24, and Mona.  These methods are from
	CGMLDspCommObject, not CGina24DspCommObject; this is to avoid code
	duplication.

 ****************************************************************************/

//===========================================================================
//
// Set the S/PDIF output format
// 
//===========================================================================

void CGMLDspCommObject::SetProfessionalSpdif
(
	BOOL bNewStatus
)
{
	DWORD		dwControlReg;

	dwControlReg = GetControlRegister();
	//
	// Clear the current S/PDIF flags
	//
	dwControlReg &= GML_SPDIF_FORMAT_CLEAR_MASK;

	//
	// Set the new S/PDIF flags depending on the mode
	//	
	dwControlReg |= 	GML_SPDIF_TWO_CHANNEL | 
							GML_SPDIF_24_BIT |
							GML_SPDIF_COPY_PERMIT;
	if ( bNewStatus )
	{
		//
		// Professional mode
		//
		dwControlReg |= GML_SPDIF_PRO_MODE;
		
		switch ( GetSampleRate() )
		{
			case 32000 : 
				dwControlReg |= GML_SPDIF_SAMPLE_RATE0 |
									 GML_SPDIF_SAMPLE_RATE1;
				break;
				
			case 44100 :
				dwControlReg |= GML_SPDIF_SAMPLE_RATE0;
				break;
			
			case 48000 :
				dwControlReg |= GML_SPDIF_SAMPLE_RATE1;
				break;
		}
	}
	else
	{
		//
		// Consumer mode
		//
		switch ( GetSampleRate() )
		{
			case 32000 : 
				dwControlReg |= GML_SPDIF_SAMPLE_RATE0 |
									 GML_SPDIF_SAMPLE_RATE1;
				break;
				
			case 48000 :
				dwControlReg |= GML_SPDIF_SAMPLE_RATE1;
				break;
		}
	}
	
	//
	// Write the control reg
	//
	WriteControlReg( dwControlReg );

	m_bProfessionalSpdif = bNewStatus;
	
	ECHO_DEBUGPRINTF( ("CGMLDspCommObject::SetProfessionalSpdif to %s\n",
							( bNewStatus ) ? "Professional" : "Consumer") );

}	// void CGina24DspCommObject::SetProfessionalSpdif( ... )




//===========================================================================
//
// WriteControlReg
//
// Most configuration of Gina24, Layla24, or Mona is
// accomplished by writing the control register.  WriteControlReg 
// sends the new control register value to the DSP.
// 
//===========================================================================

ECHOSTATUS CGMLDspCommObject::WriteControlReg( DWORD dwControlReg )
{
	if ( !m_bASICLoaded )
		return( ECHOSTATUS_ASIC_NOT_LOADED );

	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;

#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT	
	//
	// Handle the digital input auto-mute
	//	
	if (TRUE == m_fDigitalInAutoMute)
		dwControlReg |= GML_DIGITAL_IN_AUTO_MUTE;
	else
		dwControlReg &= ~GML_DIGITAL_IN_AUTO_MUTE;
#endif

	//
	// Write the control register
	//
	if (dwControlReg != GetControlRegister() )
	{
		SetControlRegister( dwControlReg );

		ECHO_DEBUGPRINTF( ("CGMLDspCommObject::WriteControlReg: 0x%lx\n",
								 dwControlReg) );
								 
		ClearHandshake();							 
		return SendVector( DSP_VC_WRITE_CONTROL_REG );
	}
	
	return ECHOSTATUS_OK;
	
} // ECHOSTATUS CGMLDspCommObject::WriteControlReg( DWORD dwControlReg )

// **** CGina24DspCommObject.cpp ****
