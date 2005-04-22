// ****************************************************************************
//
//  	CMonaDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Mona DSP
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
#include "CMonaDspCommObject.h"

#include "MonaDSP.c"
#include "Mona361DSP.c"

#include "Mona1ASIC48.c"
#include "Mona1ASIC96.c"
#include "Mona1ASIC48_361.c"
#include "Mona1ASIC96_361.c"
#include "Mona2ASIC.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CMonaDspCommObject::CMonaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGMLDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Mona" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 14;
	m_wNumPipesIn = 12;
	m_wNumBussesOut = 14;
	m_wNumBussesIn = 12;
	m_wFirstDigitalBusOut = 6;
	m_wFirstDigitalBusIn = 4;
	
	m_bProfessionalSpdif = FALSE;

	m_fHasVmixer = FALSE;	

	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels
	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );
												// Need this in cse we start with ESYNC
	m_bHasASIC = TRUE;
	if ( DEVICE_ID_56361 == pOsSupport->GetDeviceId() )
		m_pwDspCodeToLoad = pwMona361DSP;
	else
		m_pwDspCodeToLoad = pwMonaDSP;

	m_byDigitalMode = DIGITAL_MODE_SPDIF_RCA;
}	// CMonaDspCommObject::CMonaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CMonaDspCommObject::~CMonaDspCommObject()
{
}	// CMonaDspCommObject::~CMonaDspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// Mona has an ASIC on the PCI card and another ASIC in the external box; 
// both need to be loaded.
//
//===========================================================================

BOOL CMonaDspCommObject::LoadASIC()
{
	DWORD	dwControlReg;
	PBYTE	pbAsic1;
	DWORD	dwSize;

	if ( m_bASICLoaded )
		return TRUE;

	m_pOsSupport->OsSnooze( 10000 );

	if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
	{
		pbAsic1 = pbMona1ASIC48_361;
		dwSize = sizeof( pbMona1ASIC48_361 );
	}
	else
	{
		pbAsic1 = pbMona1ASIC48;
		dwSize = sizeof( pbMona1ASIC48 );
	}
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_MONA_PCI_CARD_ASIC,
											  pbAsic1,
											  dwSize ) )
		return FALSE;

	m_pbyAsic = pbAsic1;	

	m_pOsSupport->OsSnooze( 10000 );

	// Do the external one
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_MONA_EXTERNAL_ASIC,
											  pbMona2ASIC,
											  sizeof( pbMona2ASIC ) ) )
		return FALSE;

	m_pOsSupport->OsSnooze( 10000 );
		
	CheckAsicStatus();

	//
	// Set up the control register if the load succeeded -
	//
	// 48 kHz, internal clock, S/PDIF RCA mode
	//	
	if ( m_bASICLoaded )
	{
		dwControlReg = GML_CONVERTER_ENABLE | GML_48KHZ;
		ECHO_DEBUGPRINTF(("CMonaDspCommObject::LoadASIC - setting control reg for 0x%lx\n",
								dwControlReg));
		WriteControlReg( dwControlReg, TRUE );	
	}
		
	return m_bASICLoaded;
	
}	// BOOL CMonaDspCommObject::LoadASIC()


//===========================================================================
//
// Depending on what digital mode you want, Mona needs different ASICs 
//	loaded.  This function checks the ASIC needed for the new mode and sees 
// if it matches the one already loaded.
//
//===========================================================================

BOOL CMonaDspCommObject::SwitchAsic( DWORD dwMask96 )
{
	BYTE *	pbyAsicNeeded;
	DWORD		dwAsicSize;
	
	//
	//	Check the clock detect bits to see if this is
	// a single-speed clock or a double-speed clock; load
	// a new ASIC if necessary.
	// 
	if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
	{
		pbyAsicNeeded = pbMona1ASIC48_361;
		dwAsicSize = sizeof( pbMona1ASIC48_361 );
		if ( 0 != ( dwMask96 & GetInputClockDetect() ) )
		{
			pbyAsicNeeded = pbMona1ASIC96_361;
			dwAsicSize = sizeof( pbMona1ASIC96_361 );
		}
	}
	else
	{
		pbyAsicNeeded = pbMona1ASIC48;
		dwAsicSize = sizeof( pbMona1ASIC48 );
		if ( 0 != ( dwMask96 & GetInputClockDetect() ) )
		{
			pbyAsicNeeded = pbMona1ASIC96;
			dwAsicSize = sizeof( pbMona1ASIC96 );
		}
	}

	if ( pbyAsicNeeded != m_pbyAsic )
	{
		//
		// Load the desired ASIC
		//
		if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_MONA_PCI_CARD_ASIC,
												  pbyAsicNeeded,
												  dwAsicSize ) )
			return FALSE;

		m_pbyAsic = pbyAsicNeeded;	
		
		m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
	}
	
	return TRUE;

}	// BOOL CMonaDspCommObject::SwitchAsic( DWORD dwMask96 )


//===========================================================================
//
// SetInputClock
//
//===========================================================================

ECHOSTATUS CMonaDspCommObject::SetInputClock(WORD wClock)
{
	BOOL			bSetRate;
	BOOL			bWriteControlReg;
	DWORD			dwControlReg;

	ECHO_DEBUGPRINTF( ("CMonaDspCommObject::SetInputClock: clock %d\n",wClock) );

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
			ECHO_DEBUGPRINTF( ( "\tSet Mona clock to INTERNAL\n" ) );
	
			bSetRate = TRUE;
			bWriteControlReg = FALSE;

			break;
		} // CLK_CLOCKININTERNAL

		case ECHO_CLOCK_SPDIF :
		{
			if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}

			if ( FALSE == SwitchAsic( GML_CLOCK_DETECT_BIT_SPDIF96 ) )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			ECHO_DEBUGPRINTF( ( "\tSet Mona clock to SPDIF\n" ) );
	
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
		} // CLK_CLOCKINSPDIF

		case ECHO_CLOCK_WORD : 
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mona clock to WORD\n" ) );

			if ( FALSE == SwitchAsic( GML_CLOCK_DETECT_BIT_WORD96 ) )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
		
			dwControlReg |= GML_WORD_CLOCK;
			
			if ( GML_CLOCK_DETECT_BIT_WORD96 & GetInputClockDetect() )
			{
				dwControlReg |= GML_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			}
			break;
		} // CLK_CLOCKINWORD

		case ECHO_CLOCK_ADAT :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mona clock to ADAT\n" ) );
			
			if ( DIGITAL_MODE_ADAT != GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			dwControlReg |= GML_ADAT_CLOCK;
			dwControlReg &= ~GML_DOUBLE_SPEED_MODE;
			break;
		} // CLK_CLOCKINADAT

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Mona\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}	// switch (wClock)

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

	// Set Mona sample rate to something sane if word or superword is
	// being turned off
	if ( bSetRate )
	{
		SetSampleRate( GetSampleRate() );
	}
		
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CMonaDspCommObject::SetInputClock



//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for CMona
//
//===========================================================================

DWORD CMonaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	BYTE *pbyAsicNeeded;
	DWORD	dwAsicSize, dwControlReg, dwNewClock;
	BOOL	fForceControlReg;
	
	ECHO_DEBUGPRINTF(("CMonaDspCommObject::SetSampleRate to %ld\n",dwNewSampleRate));
	
	fForceControlReg = FALSE;
	
	//
	// Only set the clock for internal mode.  If the clock is not set to
	// internal, try and re-set the input clock; this more transparently
	// handles switching between single and double-speed mode
	//
	if ( GetInputClock() != ECHO_CLOCK_INTERNAL )
	{
		ECHO_DEBUGPRINTF( ( "CMonaDspCommObject::SetSampleRate: Cannot set sample rate - "
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
	// Now, check to see if the required ASIC is loaded
	//
	if ( dwNewSampleRate >= 88200 )
	{
		if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			return( GetSampleRate() );

		if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
		{
			pbyAsicNeeded = pbMona1ASIC96_361;
			dwAsicSize = sizeof(pbMona1ASIC96_361);
		}
		else
		{
			pbyAsicNeeded = pbMona1ASIC96;
			dwAsicSize = sizeof(pbMona1ASIC96);
		}
	}
	else
	{
		if ( DEVICE_ID_56361 == m_pOsSupport->GetDeviceId() )
		{
			pbyAsicNeeded = pbMona1ASIC48_361;
			dwAsicSize = sizeof(pbMona1ASIC48_361);
		}
		else
		{
			pbyAsicNeeded = pbMona1ASIC48;
			dwAsicSize = sizeof(pbMona1ASIC48);
		}
	}

	if ( pbyAsicNeeded != m_pbyAsic )
	{
		ECHO_DEBUGPRINTF(("\tLoading a new ASIC\n"));
		//
		// Load the desired ASIC
		//
		if ( FALSE == CDspCommObject::LoadASIC
													( DSP_FNC_LOAD_MONA_PCI_CARD_ASIC,
													  pbyAsicNeeded,
													  dwAsicSize ) )
			return( GetSampleRate() );

		m_pbyAsic = pbyAsicNeeded;	
		
		fForceControlReg = TRUE;
	}

	//
	// Get the new control register value
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
			dwNewClock = GML_32KHZ | GML_SPDIF_SAMPLE_RATE0 | GML_SPDIF_SAMPLE_RATE1;
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
	}

	dwControlReg |= dwNewClock;

	//
	// Send the new value to the card
	//
	if ( ECHOSTATUS_OK == WriteControlReg( dwControlReg, fForceControlReg ) )
	{
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

		ECHO_DEBUGPRINTF( ("CMonaDspCommObject::SetSampleRate: %ld "
								 "clock %ld\n", dwNewSampleRate, dwNewClock) );
	}

	return GetSampleRate();

} // DWORD CMonaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
//
//	Set digital mode
//
//===========================================================================

ECHOSTATUS CMonaDspCommObject::SetDigitalMode
(
	BYTE	byNewMode
)
{
	ECHO_DEBUGPRINTF(("CMonaDspCommObject::SetDigitalMode %d\n",byNewMode));

	//
	// If the new mode is ADAT mode, make sure that the single speed ASIC is loaded
	//
	BYTE *pbAsic96;
	
	if (DIGITAL_MODE_ADAT == byNewMode)
	{
		switch (m_pOsSupport->GetDeviceId())
		{
			case DEVICE_ID_56301 :
				pbAsic96 = pbMona1ASIC96;
				break;
				
			case DEVICE_ID_56361 :
				pbAsic96 = pbMona1ASIC96_361;
				break;
				
			default :				// should never happen, but it's good to cover all the bases
				return ECHOSTATUS_BAD_CARDID;
		}
		if (pbAsic96 == m_pbyAsic)
			SetSampleRate( 48000 );	
	}
	
	//
	// Call the base class to tweak the input clock if necessary
	//
	return CGMLDspCommObject::SetDigitalMode(byNewMode);
	
}	// ECHOSTATUS CMonaDspCommObject::SetDigitalMode


// **** CMonaDspCommObject.cpp ****
