// ****************************************************************************
//
//  	CDarla24DspCommObject.cpp
//
//		Implementation file for Darla24 DSP interface class.
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
#include "CDarla24DspCommObject.h"

#include "Darla24DSP.c"


/****************************************************************************

	Magic constants for the Darla24 hardware

 ****************************************************************************/

#define GD24_96000		0x0
#define GD24_48000		0x1
#define GD24_44100		0x2
#define GD24_32000		0x3
#define GD24_22050		0x4
#define GD24_16000		0x5
#define GD24_11025		0x6
#define GD24_8000			0x7
#define GD24_88200		0x8
#define GD24_EXT_SYNC	0x9




/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CDarla24DspCommObject::CDarla24DspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGdDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Darla24" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base
	
	m_wNumPipesOut = 8;
	m_wNumPipesIn = 2;
	m_wNumBussesOut = 8;
	m_wNumBussesIn = 2;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 2;
	
	m_fHasVmixer = FALSE;	

	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels
	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );
												// Need this in case we start with ESYNC
												
	m_pwDspCodeToLoad = pwDarla24DSP;

	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;

}	// CDarla24DspCommObject::CDarla24DspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CDarla24DspCommObject::~CDarla24DspCommObject()
{
}	// CDarla24DspCommObject::~CDarla24DspCommObject()




/****************************************************************************

	Hardware config

 ****************************************************************************/

//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Darla24; this is fairly simple.  You
// just pick the right magic number.
//
//===========================================================================

DWORD CDarla24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	BYTE	bClock;

	//
	// Pick the magic number
	//
	switch ( dwNewSampleRate )
	{
		case 96000 :
			bClock = GD24_96000;
			break;
		case 88200 :
			bClock = GD24_88200;
			break;
		case 48000 : 
			bClock = GD24_48000;
			break;
		case 44100 :
			bClock = GD24_44100;
			break;		
		case 32000 :
			bClock = GD24_32000;
			break;		
		case 22050 :
			bClock = GD24_22050;
			break;
		case 16000 : 
			bClock = GD24_16000;
			break;
		case 11025 :
			bClock = GD24_11025;
			break;
		case 8000 :
		 	bClock = GD24_8000;
			break;
		default :
			ECHO_DEBUGPRINTF( ("CDarla24DspCommObject::SetSampleRate: Error, "
									 "invalid sample rate 0x%lx\n", dwNewSampleRate) );
			return 0xffffffff;
	}

	if ( !WaitForHandshake() )
		return 0xffffffff;

	//
	// Override the sample rate if this card is set to Echo sync.  
	// m_pDspCommPage->wInputClock is just being used as a parameter here;
	// the DSP ignores it.
	//	
	if ( ECHO_CLOCK_ESYNC == GetInputClock() )
		bClock = GD24_EXT_SYNC;

	m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

	//
	// Write the audio state to the comm page
	//
	m_pDspCommPage->byGDClockState = bClock;

	// Send command to DSP
	ClearHandshake();
	SendVector( DSP_VC_SET_GD_AUDIO_STATE );

	ECHO_DEBUGPRINTF( ("CDarla24DspCommObject::SetSampleRate: 0x%lx "
							 "clock %d\n", dwNewSampleRate, bClock) );

	return GetSampleRate();
	
} // DWORD CDarla24DspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
//
// Set input clock
// 
// Darla24 supports internal and Esync clock.
//
//===========================================================================

ECHOSTATUS CDarla24DspCommObject::SetInputClock(WORD wClock)
{
	if ( 	(ECHO_CLOCK_INTERNAL != wClock) &&
			(ECHO_CLOCK_ESYNC != wClock))
		return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
		
	m_wInputClock = wClock;
	
	return SetSampleRate( GetSampleRate() );
	
}	// SetInputClock


// **** Darla24DspCommObject.cpp ****
