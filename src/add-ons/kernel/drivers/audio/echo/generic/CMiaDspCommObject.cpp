// ****************************************************************************
//
//  	CMiaDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Mia DSP
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
#include "CMiaDspCommObject.h"

#include "MiaDSP.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CMiaDspCommObject::CMiaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObjectVmixer( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Mia" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 8;
	m_wNumPipesIn = 4;
	m_wNumBussesOut = 4;
	m_wNumBussesIn = 4;
	m_wFirstDigitalBusOut = 2;
	m_wFirstDigitalBusIn = 2;

	m_fHasVmixer = TRUE;

	if (MIA_MIDI_REV == pOsSupport->GetCardRev())
	{
		m_wNumMidiOut = 1;					// # MIDI out channels
		m_wNumMidiIn = 1;						// # MIDI in  channels
	}

	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );

	m_bHasASIC = FALSE;

	m_pwDspCodeToLoad = pwMiaDSP;

	m_byDigitalMode = DIGITAL_MODE_NONE;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;
	
}	// CMiaDspCommObject::CMiaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CMiaDspCommObject::~CMiaDspCommObject()
{
}	// CMiaDspCommObject::~CMiaDspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
//	Set the input clock
//
//===========================================================================

ECHOSTATUS CMiaDspCommObject::SetInputClock(WORD wClock)
{
	DWORD	dwSampleRate = GetSampleRate();

	ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetInputClock:\n") );

	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mia clock to INTERNAL\n" ) );
	
			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ( ( dwSampleRate < 32000  ) ||
			     ( dwSampleRate > 96000 ) )
			{
				dwSampleRate = 48000;
			}

			break;
		} // CLK_CLOCKININTERNAL

		case ECHO_CLOCK_SPDIF :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mia clock to SPDIF\n" ) );
			break;
		} // CLK_CLOCKINSPDIF

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Mia\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}	// switch (wInputClock)
	
	m_wInputClock = wClock;

	SetSampleRate( dwSampleRate );

	return ECHOSTATUS_OK;
}	// ECHOSTATUS CMiaDspCommObject::SetInputClock


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Mia
//
//===========================================================================

DWORD CMiaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	//
	// Set the sample rate
	//
	DWORD dwControlReg = MIA_48000;

	switch ( dwNewSampleRate )
	{
		case 96000 :
			dwControlReg = MIA_96000;
			break;
			
		case 88200 :
			dwControlReg = MIA_88200;
			break;
			
		case 44100 : 
			dwControlReg = MIA_44100;
			break;
			
		case 32000 :
			dwControlReg = MIA_32000;
			break;
	}

	//
	// Override the clock setting if this Mia is set to S/PDIF clock
	//	
	if ( ECHO_CLOCK_SPDIF == GetInputClock() )
		dwControlReg |= MIA_SPDIF;
	
	//
	//	Set the control register if it has changed
	//
	if (dwControlReg != GetControlRegister())
	{
		if ( !WaitForHandshake() )
			return 0xffffffff;

		//
		// Set the values in the comm page; the dwSampleRate
		// field isn't used by the DSP, but is read by the call
		// to GetSampleRate below
		//		
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		SetControlRegister( dwControlReg );

		//
		//	Poke the DSP
		// 
		ClearHandshake();
		SendVector( DSP_VC_UPDATE_CLOCKS );
	}
	
	return GetSampleRate();
		
} // DWORD CMiaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )

// **** CMiaDspCommObject.cpp ****
