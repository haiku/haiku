// ****************************************************************************
//
//  	CGMLDspCommObject.cpp
//
//		Implementation file for GML cards (Gina24, Mona, and Layla24).
//
// ----------------------------------------------------------------------------
//
//   Copyright Echo Digital Audio Corporation (c) 1998 - 2004
//   All rights reserved
//   www.echoaudio.com
//   
//   This file is part of Echo Digital Audio's generic driver library.
//   
//   Echo Digital Audio's generic driver library is free software; 
//   you can redistribute it and/or modify it under the terms of 
//   the GNU General Public License as published by the Free Software Foundation.
//   
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
//   MA  02111-1307, USA.
//
// ****************************************************************************

#include "CEchoGals.h"
#include "CGMLDspCommObject.h"

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
	// Handle the non-audio bit
	//
	if (m_bNonAudio)
		dwControlReg |= GML_SPDIF_NOT_AUDIO;
	
	//
	// Write the control reg
	//
	WriteControlReg( dwControlReg );

	m_bProfessionalSpdif = bNewStatus;
	
	ECHO_DEBUGPRINTF( ("CGMLDspCommObject::SetProfessionalSpdif to %s\n",
							( bNewStatus ) ? "Professional" : "Consumer") );

}	// void CGMLDspCommObject::SetProfessionalSpdif( ... )




//===========================================================================
//
// SetSpdifOutNonAudio
//
// Set the state of the non-audio status bit in the S/PDIF out status bits
// 
//===========================================================================

void CGMLDspCommObject::SetSpdifOutNonAudio(BOOL bNonAudio)
{
	DWORD		dwControlReg;

	dwControlReg = GetControlRegister();
	if (bNonAudio)
	{
		dwControlReg |= GML_SPDIF_NOT_AUDIO;
	}
	else
	{
		dwControlReg &= ~GML_SPDIF_NOT_AUDIO;
	}
	
	m_bNonAudio = bNonAudio;
	
	WriteControlReg( dwControlReg );	
}	



//===========================================================================
//
// WriteControlReg
//
// Most configuration of Gina24, Layla24, or Mona is
// accomplished by writing the control register.  WriteControlReg 
// sends the new control register value to the DSP.
// 
//===========================================================================

ECHOSTATUS CGMLDspCommObject::WriteControlReg
( 
	DWORD dwControlReg,
	BOOL 	fForceWrite
)
{
	ECHO_DEBUGPRINTF(("CGMLDspCommObject::WriteControlReg 0x%lx\n",dwControlReg));

	if ( !m_bASICLoaded )
	{
		ECHO_DEBUGPRINTF(("CGMLDspCommObject::WriteControlReg - ASIC not loaded\n"));
		return( ECHOSTATUS_ASIC_NOT_LOADED );
	}
		

	if ( !WaitForHandshake() )
	{
		ECHO_DEBUGPRINTF(("CGMLDspCommObject::WriteControlReg - no handshake\n"));
		return ECHOSTATUS_DSP_DEAD;
	}

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
	if (fForceWrite || (dwControlReg != GetControlRegister()) )
	{
		SetControlRegister( dwControlReg );

		ECHO_DEBUGPRINTF( ("CGMLDspCommObject::WriteControlReg: Setting 0x%lx\n",
								 dwControlReg) );
								 
		ClearHandshake();							 
		return SendVector( DSP_VC_WRITE_CONTROL_REG );
	}
	else
	{
		ECHO_DEBUGPRINTF( ("CGMLDspCommObject::WriteControlReg: control reg is already 0x%lx\n",
								 dwControlReg) );
	}
	
	return ECHOSTATUS_OK;
	
} // ECHOSTATUS CGMLDspCommObject::WriteControlReg( DWORD dwControlReg )

// **** CGML24DspCommObject.cpp ****
