// ****************************************************************************
//
//  	CLaylaDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Layla DSP
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
#include "CLaylaDspCommObject.h"

#include "Layla20DSP.c"
#include "LaylaAsic.c"

//
// The ASIC files for Layla20 are always this size
//
#define LAYLA_ASIC_SIZE		32385


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CLaylaDspCommObject::CLaylaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Layla" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 12;
	m_wNumPipesIn = 10;
	m_wNumBussesOut = 12;
	m_wNumBussesIn = 10;
	m_wFirstDigitalBusOut = 10;
	m_wFirstDigitalBusIn = 8;

	m_fHasVmixer = FALSE;

	m_wNumMidiOut = 1;					// # MIDI out channels
	m_wNumMidiIn = 1;						// # MIDI in  channels

	m_bHasASIC = TRUE;

	m_pwDspCodeToLoad = pwLayla20DSP;
	
}	// CLaylaDspCommObject::CLaylaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CLaylaDspCommObject::~CLaylaDspCommObject()
{
	ECHO_DEBUGPRINTF( ( "CLaylaDspCommObject::~CLaylaDspCommObject() is toast!\n" ) );
}	// CLaylaDspCommObject::~CLaylaDspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// Layla20 has an ASIC in the external box
//
//===========================================================================

BOOL CLaylaDspCommObject::LoadASIC()
{
	if ( m_bASICLoaded == TRUE )
		return TRUE;

	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_LAYLA_ASIC,
											  pbLaylaASIC,
											  LAYLA_ASIC_SIZE ) )
		return FALSE;
	//
	// Check if ASIC is alive and well.
	//
	return( CheckAsicStatus() );
}	// BOOL CLaylaDspCommObject::LoadASIC()


//===========================================================================
//
// SetSampleRate
// 
// Set the sample rate for Layla
//
// Layla is simple; just send it the sampling rate (assuming that the clock
// mode is correct).
//
//===========================================================================

DWORD CLaylaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	//
	// Only set the clock for internal mode
	//	Do not return failure, simply treat it as a non-event.
	//
	if ( GetInputClock() != ECHO_CLOCK_INTERNAL )
	{
		ECHO_DEBUGPRINTF( ( "SetSampleRate: Cannot set sample rate because "
								  "Layla clock NOT set to CLK_CLOCKININTERNAL\n" ) );
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		return GetSampleRate();
	}
	
	//
	// Sanity check - check the sample rate
	//
	if ( ( dwNewSampleRate < 8000 ) ||
		  ( dwNewSampleRate > 50000 ) )
	{
		ECHO_DEBUGPRINTF( ( "SetSampleRate: Layla sample rate %ld out of range, "
								  "no change made\n",
								  dwNewSampleRate) );
		return 0xffffffff;
	}

	if ( !WaitForHandshake() )
		return 0xffffffff;

	m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
	
	ClearHandshake();
	SendVector( DSP_VC_SET_LAYLA_SAMPLE_RATE );
	ECHO_DEBUGPRINTF( ( "SetSampleRate: Layla sample rate changed to %ld\n",
							  dwNewSampleRate ) );
	return( dwNewSampleRate );
}	// DWORD CLaylaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
//
// Send new input clock setting to DSP
//
//===========================================================================

ECHOSTATUS CLaylaDspCommObject::SetInputClock(WORD wClock)
{
	BOOL			bSetRate;
	//BOOL			bWriteControlReg;
	//DWORD			dwSampleRate;
	WORD			wNewClock;

	ECHO_DEBUGPRINTF( ( "CLaylaDspCommObject::SetInputClock:\n" ) );

	bSetRate = FALSE;
	
	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
			ECHO_DEBUGPRINTF( ( "\tSet Layla24 clock to INTERNAL\n" ) );

			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ( ( GetSampleRate() < 8000 ) ||
			     ( GetSampleRate() > 50000 ) )
			{
				m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
			}

			bSetRate = TRUE;
			wNewClock = LAYLA20_CLOCK_INTERNAL;
			break;

		case ECHO_CLOCK_SPDIF:
			ECHO_DEBUGPRINTF( ( "\tSet Layla20 clock to SPDIF\n" ) );
			
			wNewClock = LAYLA20_CLOCK_SPDIF;
			break;

		case ECHO_CLOCK_WORD: 
			ECHO_DEBUGPRINTF( ( "\tSet Layla20 clock to WORD\n" ) );
			
			wNewClock = LAYLA20_CLOCK_WORD;
			
			break;

		case ECHO_CLOCK_SUPER: 
			ECHO_DEBUGPRINTF( ( "\tSet Layla20 clock to SUPER\n" ) );
			
			wNewClock = LAYLA20_CLOCK_SUPER;
			
			break;

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Layla24\n", wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;

	}		// switch (wClock)

	//
	// Winner! Save the new input clock.
	//
	m_wInputClock = wClock;

	//
	// Send the new clock to the DSP
	//
	m_pDspCommPage->wInputClock = SWAP(wNewClock);
	ClearHandshake();
	SendVector( DSP_VC_UPDATE_CLOCKS );
	
	if ( bSetRate )
		SetSampleRate();
		
	return ECHOSTATUS_OK;
}		// ECHOSTATUS CLaylaDspCommObject::SetInputClock()


//===========================================================================
//
// Set new output clock
//
//===========================================================================

ECHOSTATUS CLaylaDspCommObject::SetOutputClock(WORD wClock)
{
	if (FALSE == m_bASICLoaded)
		return ECHOSTATUS_ASIC_NOT_LOADED;
		
	if (!WaitForHandshake())
		return ECHOSTATUS_DSP_DEAD;

	ECHO_DEBUGPRINTF( ("CDspCommObject::SetOutputClock:\n") );
		
	m_pDspCommPage->wOutputClock = SWAP(wClock);
	m_wOutputClock = wClock;
	
	ClearHandshake();
	ECHOSTATUS Status = SendVector(DSP_VC_UPDATE_CLOCKS);
	
	return Status;
}	// ECHOSTATUS CLaylaDspCommObject::SetOutputClock


//===========================================================================
//
// Detect MIDI output activity
//
//===========================================================================

BOOL CLaylaDspCommObject::IsMidiOutActive()
{
	ULONGLONG	ullCurTime;

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	return( ( ( ullCurTime - m_ullMidiOutTime ) > MIDI_ACTIVITY_TIMEOUT_USEC ) ? FALSE : TRUE );
}	// BOOL CLaylaDspCommObject::IsMidiOutActive()


//===========================================================================
//
// Input bus gain - iGain is in units of .5 dB
//
//===========================================================================

ECHOSTATUS CLaylaDspCommObject::SetBusInGain( WORD wBusIn, int iGain)
{
	//
	// Store the gain for later use
	//
	m_byInputTrims[wBusIn] = (BYTE) iGain;

	//
	// Adjust the input gain depending on the nominal level switch
	//
	BYTE byMinus10;
	GetNominalLevel( wBusIn + m_wNumBussesOut, &byMinus10);
	
	if (0 == byMinus10)
	{
		//
		// This channel is in +4 mode; subtract 12 dB from the input gain
		// (note that iGain is in units of .5 dB)
		//
		iGain -= 12 << 1;
	}
	
	return CDspCommObject::SetBusInGain(wBusIn,iGain);

}	

ECHOSTATUS CLaylaDspCommObject::GetBusInGain( WORD wBusIn, int &iGain)
{
	//ECHOSTATUS Status;
	
	if (wBusIn > m_wNumBussesIn)
		return ECHOSTATUS_INVALID_CHANNEL;
	
	iGain = (int) m_byInputTrims[wBusIn];	
	
	return ECHOSTATUS_OK;
}	


//===========================================================================
//
// Set the nominal level for an input or output bus
//
//	Set bState to TRUE for -10, FALSE for +4 
//
// Layla20 sets the input nominal level by adjusting the 
// input trim
//
//===========================================================================

ECHOSTATUS CLaylaDspCommObject::SetNominalLevel
(
	WORD	wBus,
	BOOL	bState
)
{
	if (wBus < m_wNumBussesOut)
	{
		//
		//	This is an output bus; call the base class routine to service it
		//
		return CDspCommObject::SetNominalLevel(wBus,bState);
	}

	//
	// Check the bus number
	//
	if (wBus < (m_wNumBussesOut + m_wNumBussesIn))
	{
		//
		// Set the nominal bit in the comm page
		//		
		if ( bState )
			m_pDspCommPage->cmdNominalLevel.SetIndexInMask( wBus );
		else
			m_pDspCommPage->cmdNominalLevel.ClearIndexInMask( wBus );
		
		//
		// Set the input trim, using the current gain
		//	
		return SetBusInGain(wBus - m_wNumBussesOut,(int) m_byInputTrims[wBus]);
	}

	ECHO_DEBUGPRINTF( ("CLaylaDspCommObject::SetNominalOutLineLevel Invalid "
							 "index %d\n",
							 wBus ) );
	return ECHOSTATUS_INVALID_CHANNEL;

}	// ECHOSTATUS CLaylaDspCommObject::SetNominalLevel


// **** LaylaDspCommObject.cpp ****
