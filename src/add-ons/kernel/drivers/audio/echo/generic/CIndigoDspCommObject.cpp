// ****************************************************************************
//
//  	CIndigoDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Indigo DSP
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
#include "CIndigoDspCommObject.h"

#ifdef ECHO_WDM
#pragma optimize("",off)
#endif
#include "IndigoDSP.c"


//
//	Construction/destruction
//
CIndigoDspCommObject::CIndigoDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObjectVmixer( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Indigo" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 8;
	m_wNumPipesIn = 2;
	m_wNumBussesOut = 2;
	m_wNumBussesIn = m_wNumPipesIn;
	m_wFirstDigitalBusOut = m_wNumBussesOut;
	m_wFirstDigitalBusIn = m_wNumBussesIn;

	m_fHasVmixer = TRUE;
		
	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels

	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );

	m_bHasASIC = FALSE;

	m_pwDspCodeToLoad = pwIndigoDSP;
	
	m_byDigitalMode = DIGITAL_MODE_NONE;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;
	
}	// CIndigoDspCommObject::CIndigoDspCommObject( DWORD dwPhysRegBase )


CIndigoDspCommObject::~CIndigoDspCommObject()
{
}	// CIndigoDspCommObject::~CIndigoDspCommObject()


//
//	Save new clock settings and send to DSP.
//
ECHOSTATUS CIndigoDspCommObject::SetInputClock(WORD wClock)
{

	ECHO_DEBUGPRINTF( ("CIndigoDspCommObject::SetInputClock:\n") );
	return ECHOSTATUS_CLOCK_NOT_SUPPORTED;

}	// ECHOSTATUS CIndigoDspCommObject::SetInputClock


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Indigo
//
//===========================================================================

DWORD CIndigoDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
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
	// Set the control register if it has changed
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
	
} // DWORD CIndigoDspCommObject::SetSampleRate( DWORD dwNewSampleRate )

// **** CIndigoDspCommObject.cpp ****
