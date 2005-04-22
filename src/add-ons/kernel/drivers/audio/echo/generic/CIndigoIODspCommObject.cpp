// ****************************************************************************
//
//  	CIndigoIODspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Indigo io DSP
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
#include "CIndigoIODspCommObject.h"

#ifdef ECHO_WDM
#pragma optimize("",off)
#endif
#include "IndigoIODSP.c"


//
//	Construction/destruction
//
CIndigoIODspCommObject::CIndigoIODspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObjectVmixer( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Indigo io" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 8;
	m_wNumPipesIn = 2;
	m_wNumBussesOut = 2;
	m_wNumBussesIn = 2;
	m_wFirstDigitalBusOut = m_wNumBussesOut;
	m_wFirstDigitalBusIn = m_wNumBussesIn;

	m_fHasVmixer = TRUE;
		
	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels

	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );

	m_bHasASIC = FALSE;

	m_pwDspCodeToLoad = pwIndigoioDSP;
	
	m_byDigitalMode = DIGITAL_MODE_NONE;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;
	
}	// CIndigoIODspCommObject::CIndigoIODspCommObject( DWORD dwPhysRegBase )


CIndigoIODspCommObject::~CIndigoIODspCommObject()
{
}	// CIndigoIODspCommObject::~CIndigoIODspCommObject()


//
//	Save new clock settings and send to DSP.
//
ECHOSTATUS CIndigoIODspCommObject::SetInputClock(WORD wClock)
{

	ECHO_DEBUGPRINTF( ("CIndigoIODspCommObject::SetInputClock:\n") );
	return ECHOSTATUS_CLOCK_NOT_SUPPORTED;

}	// ECHOSTATUS CIndigoIODspCommObject::SetInputClock


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for IndigoIO
//
//===========================================================================

DWORD CIndigoIODspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	if ( !WaitForHandshake() )
		return 0xffffffff;
	
	//
	// Set the value in the comm page
	//		
	m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

	//
	//	Poke the DSP
	// 
	ClearHandshake();
	SendVector( DSP_VC_UPDATE_CLOCKS );

	return GetSampleRate();
	
} // DWORD CIndigoIODspCommObject::SetSampleRate( DWORD dwNewSampleRate )

// **** CIndigoIODspCommObject.cpp ****
