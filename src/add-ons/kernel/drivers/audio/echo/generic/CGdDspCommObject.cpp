// ****************************************************************************
//
//  	CGdDspCommObject.cpp
//
//		Implementation file for Gina20/Darla20 DSP interface class.
//
//		Darla20 and Gina20 are very similar; this class is used for both.
//		CGinaDspCommObject and CDarlaDspCommObject dervie from this class, in
//		turn.
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
#include "CGinaDspCommObject.h"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CGdDspCommObject::CGdDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
	
	m_byGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;
	m_byGDCurrentClockState = GD_CLOCK_UNDEF;
	
}	// CGdDspCommObject::CGinaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CGdDspCommObject::~CGdDspCommObject()
{
}	// CGdDspCommObject::~CGdDspCommObject()




/****************************************************************************

	Hardware config

 ****************************************************************************/

//===========================================================================
//
//	Called after LoadFirmware to restore old gains, meters on, monitors, etc.
//	No error checking is done here.
//
//===========================================================================

void CGdDspCommObject::RestoreDspSettings()
{
	
	m_pDspCommPage->byGDClockState = GD_CLOCK_UNDEF;
	m_pDspCommPage->byGDSpdifStatus = GD_SPDIF_STATUS_UNDEF;
	
	CDspCommObject::RestoreDspSettings();
	
}	// void CGdDspCommObject::RestoreDspSettings()


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Gina/Darla
//
// For Gina and Darla, three parameters need to be set: the resampler state,
// the clock state, and the S/PDIF output status state.  The idea is that
// these parameters are changed as infrequently as possible.
//
// Resampling is no longer supported.
//
//===========================================================================

DWORD CGdDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	BYTE		byGDClockState, byGDSpdifStatus;
	DWORD		fUseSpdifInClock;
	
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	//	Pick the clock
	// 
	fUseSpdifInClock = (GetCardType() == GINA) &&
							 (GetInputClock() == ECHO_CLOCK_SPDIF);
	if (fUseSpdifInClock)
	{
		byGDClockState = GD_CLOCK_SPDIFIN;
	}		
	else
	{
		switch (dwNewSampleRate)
		{
			case 44100 :
				byGDClockState = GD_CLOCK_44;
				break;

			case 48000 :
				byGDClockState = GD_CLOCK_48;
				break;

			default :
				byGDClockState = GD_CLOCK_NOCHANGE;
				break;
		}
	}

	if ( m_byGDCurrentClockState == byGDClockState )
	{
		byGDClockState = GD_CLOCK_NOCHANGE;
	}

	//
	// Select S/PDIF output status
	//
	byGDSpdifStatus = SelectGinaDarlaSpdifStatus( dwNewSampleRate );

	//
	// Write the audio state to the comm page
	//
	m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
	m_pDspCommPage->byGDClockState = byGDClockState;
	m_pDspCommPage->byGDSpdifStatus = byGDSpdifStatus;
	m_pDspCommPage->byGDResamplerState = 3;				// magic number - 
																		// should always be 3

	//
	// Send command to DSP
	//
	ClearHandshake();
	SendVector( DSP_VC_SET_GD_AUDIO_STATE );

	//			
	// Save the new audio state
	//
	if ( byGDClockState != GD_CLOCK_NOCHANGE )
		m_byGDCurrentClockState = byGDClockState;

	if ( byGDSpdifStatus != GD_SPDIF_STATUS_NOCHANGE )
		m_byGDCurrentSpdifStatus = byGDSpdifStatus;

	return dwNewSampleRate;
	
} // DWORD CGdDspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
// 
// SelectGinaDarlaSpdifStatus
// 
//===========================================================================

BYTE CGdDspCommObject::SelectGinaDarlaSpdifStatus( DWORD dwNewSampleRate )
{
	BYTE byNewStatus = 0;
	
	switch (dwNewSampleRate)
	{
		case 44100 :
			byNewStatus = GD_SPDIF_STATUS_44;
			break;
			
		case 48000 :
			byNewStatus = GD_SPDIF_STATUS_48;
			break;
			
		default :
			byNewStatus = GD_SPDIF_STATUS_NOCHANGE;
			break;
	}
	
	if (byNewStatus == m_byGDCurrentSpdifStatus)
	{
		byNewStatus = GD_SPDIF_STATUS_NOCHANGE;
	}
	
	return byNewStatus;
	
}	// BYTE CGdDspCommObject::SelectGinaDarlaSpdifStatus( DWORD dwNewSampleRate )



/****************************************************************************

	Power management

 ****************************************************************************/

//===========================================================================
//
// Tell the DSP to go into low-power mode
//
//===========================================================================

ECHOSTATUS CGdDspCommObject::GoComatose()
{
	ECHO_DEBUGPRINTF(("CGdDspCommObject::GoComatose\n"));

  	m_byGDCurrentClockState = GD_CLOCK_UNDEF;
  	m_byGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;

	return CDspCommObject::GoComatose();
	
}	// end of GoComatose


// **** GinaDspCommObject.cpp ****
	