// ****************************************************************************
//
//  	C3gDco.cpp
//
//		Implementation file for EchoGals generic driver 3G DSP
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
#include "C3gDco.h"

#include "Echo3gDSP.c"
#include "3G_ASIC.c"

/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

C3gDco::C3gDco
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base  fixme put this in base class

	m_dwOriginalBoxType = NO3GBOX;
	m_dwCurrentBoxType = m_dwOriginalBoxType;
	SetChannelCounts();
	m_bBoxTypeSet = FALSE;
	
	m_fHasVmixer = FALSE;
	
	m_wNumMidiOut = 1;					// # MIDI out channels
	m_wNumMidiIn = 1;						// # MIDI in  channels

	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 48000 );
	m_pDspCommPage->dw3gFreqReg = SWAP( (DWORD) (E3G_MAGIC_NUMBER / 48000) - 2);

	m_bHasASIC = TRUE;

	m_pwDspCodeToLoad = pwEcho3gDSP;

	m_byDigitalMode = DIGITAL_MODE_SPDIF_RCA;
	
	m_bProfessionalSpdif = FALSE;
	m_bNonAudio = FALSE;
	
}	// C3gDco::C3gDco( DWORD dwPhysRegBase )



//===========================================================================
//
// Destructor
//
//===========================================================================

C3gDco::~C3gDco()
{
}	// C3gDco::~C3gDco()



//===========================================================================
//
// Supported digital modes depend on what kind of box you have
//
//===========================================================================

DWORD C3gDco::GetDigitalModes()
{
	DWORD dwModes;

	dwModes =	ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_RCA |
					ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_OPTICAL |
					ECHOCAPS_HAS_DIGITAL_MODE_ADAT; 
	
	return dwModes;
}


/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// 3G has an ASIC in the external box
//
//===========================================================================

BOOL C3gDco::LoadASIC()
{
	DWORD	dwControlReg;

	if ( m_bASICLoaded == TRUE )
		return TRUE;
		
	ECHO_DEBUGPRINTF(("C3gDco::LoadASIC\n"));

	//
	// Give the DSP a few milliseconds to settle down
	//
	m_pOsSupport->OsSnooze( 2000 );

	//
	// Load the ASIC
	//
	if ( !CDspCommObject::LoadASIC( DSP_FNC_LOAD_3G_ASIC,
											  pb3g_asic,
											  sizeof(pb3g_asic) ) )
		return FALSE;

	//
	// Give the ASIC a whole second to set up
	//
	m_pOsSupport->OsSnooze( 1000000 );

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
		dwControlReg = E3G_48KHZ;
		WriteControlReg( dwControlReg, E3G_FREQ_REG_DEFAULT, TRUE);	// TRUE == force write
	}

	ECHO_DEBUGPRINTF(("\t3G ASIC loader finished\n"));
		
	return m_bASICLoaded;
	
}	// BOOL C3gDco::LoadASIC()



//===========================================================================
//
//	Set the input clock
//
//===========================================================================

ECHOSTATUS C3gDco::SetInputClock(WORD wClock)
{
	DWORD dwControlReg,dwSampleRate;
	ECHOSTATUS Status;

	ECHO_DEBUGPRINTF( ("C3gDco::SetInputClock:\n") );
	
	//
	// Mask off the clock select bits
	//
	dwControlReg = GetControlRegister();
	dwControlReg &= E3G_CLOCK_CLEAR_MASK;
	
	//
	// New clock
	//
	switch (wClock)
	{
		case ECHO_CLOCK_INTERNAL :
			ECHO_DEBUGPRINTF(("\tsetting internal clock\n"));
			
			m_wInputClock = ECHO_CLOCK_INTERNAL;	// prevent recursion
			
			dwSampleRate = GetSampleRate();
			if ((dwSampleRate < 32000) || (dwSampleRate > 100000))
				dwSampleRate = 48000;
				
			SetSampleRate(dwSampleRate);
			return ECHOSTATUS_OK;
			

		case ECHO_CLOCK_WORD: 
			dwControlReg |= E3G_WORD_CLOCK;

			if ( E3G_CLOCK_DETECT_BIT_WORD96 & GetInputClockDetect() )
			{
				dwControlReg |= E3G_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~E3G_DOUBLE_SPEED_MODE;
			}
			ECHO_DEBUGPRINTF( ( "\tSet 3G clock to WORD\n" ) );
			break;
			
		
		case ECHO_CLOCK_SPDIF :
			if ( DIGITAL_MODE_ADAT == GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			dwControlReg |= E3G_SPDIF_CLOCK;
			if ( E3G_CLOCK_DETECT_BIT_SPDIF96 & GetInputClockDetect() )
			{
				dwControlReg |= E3G_DOUBLE_SPEED_MODE;
			}
			else
			{
				dwControlReg &= ~E3G_DOUBLE_SPEED_MODE;
			}

			ECHO_DEBUGPRINTF( ( "\tSet 3G clock to SPDIF\n" ) );
			break;

	
		case ECHO_CLOCK_ADAT :
			if ( DIGITAL_MODE_ADAT != GetDigitalMode() )
			{
				return ECHOSTATUS_CLOCK_NOT_AVAILABLE;
			}
			
			dwControlReg |= E3G_ADAT_CLOCK;
			dwControlReg &= ~E3G_DOUBLE_SPEED_MODE;
			ECHO_DEBUGPRINTF( ( "\tSet 3G clock to ADAT\n" ) );
			break;
			

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for 3G\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}
	
	//
	// Winner! Try to write the hardware
	//
	Status = WriteControlReg( dwControlReg, Get3gFreqReg(), TRUE );
	if (ECHOSTATUS_OK == Status)
		m_wInputClock = wClock;
	
	return Status;

}	// ECHOSTATUS C3gDco::SetInputClock



//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for 3G
//
//===========================================================================

DWORD C3gDco::SetSampleRate( DWORD dwNewSampleRate )
{
	DWORD dwControlReg,dwNewClock,dwBaseRate,dwFreqReg;
	
	ECHO_DEBUGPRINTF(("3G set sample rate to %ld\n",dwNewSampleRate));
	
	//
	// Only set the clock for internal mode.  If the clock is not set to
	// internal, try and re-set the input clock; this more transparently
	// handles switching between single and double-speed mode
	//
	if ( GetInputClock() != ECHO_CLOCK_INTERNAL )
	{
		ECHO_DEBUGPRINTF( ( "C3gDco::SetSampleRate: Cannot set sample rate - "
								  "clock not set to ECHO_CLOCK_INTERNAL\n" ) );
		
		//
		// Save the rate anyhow
		//
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		
		//
		// Set the input clock to the current value
		//
		SetInputClock( m_wInputClock );
		
		return dwNewSampleRate;
	}

	//
	// Get the control register & clear the appropriate bits
	// 
	dwControlReg = GetControlRegister();
	dwControlReg &= E3G_CLOCK_CLEAR_MASK;
	
	//
	// Set the sample rate
	//
	switch ( dwNewSampleRate )
	{
		case 96000 :
			dwNewClock = E3G_96KHZ;
			break;
		
		case 88200 :
			dwNewClock = E3G_88KHZ;
			break;
		
		case 48000 : 
			dwNewClock = E3G_48KHZ;
			break;
		
		case 44100 : 
			dwNewClock = E3G_44KHZ;
			break;
		
		case 32000 :
			dwNewClock = E3G_32KHZ;
			break;

		default :
			dwNewClock = E3G_CONTINUOUS_CLOCK;
			if (dwNewSampleRate > 50000)
				dwNewClock |= E3G_DOUBLE_SPEED_MODE;
			break;
	}

	dwControlReg |= dwNewClock;
	SetSpdifBits(&dwControlReg,dwNewSampleRate);
	
	ECHO_DEBUGPRINTF(("\tdwNewClock 0x%lx  dwControlReg 0x%lx\n",dwNewClock,dwControlReg));

	//
	// Set up the frequency reg
	//
	dwBaseRate = dwNewSampleRate;
	if (dwBaseRate > 50000)
		dwBaseRate /= 2;
	
	if (dwBaseRate < 32000)
		dwBaseRate = 32000;
	
	dwFreqReg = E3G_MAGIC_NUMBER / dwBaseRate - 2;
	if (dwFreqReg > E3G_FREQ_REG_MAX)
		dwFreqReg = E3G_FREQ_REG_MAX;
		
	//
	// Tell the DSP about it - DSP reads both control reg & freq reg
	//
	if ( ECHOSTATUS_OK == WriteControlReg( dwControlReg, dwFreqReg) )
	{
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );

		ECHO_DEBUGPRINTF( ("C3gDco::SetSampleRate: %ld  clock %lx\n", dwNewSampleRate, dwControlReg) );
	}
	else
	{
		ECHO_DEBUGPRINTF( ("C3gDco::SetSampleRate: could not set sample rate %ld\n", dwNewSampleRate) );

		dwNewSampleRate = SWAP( m_pDspCommPage->dwSampleRate );
	}

	return dwNewSampleRate;
	
} // DWORD C3gDco::SetSampleRate( DWORD dwNewSampleRate )



//===========================================================================
//
// SetDigitalMode
// 
//===========================================================================

ECHOSTATUS C3gDco::SetDigitalMode
(
	BYTE	byNewMode
)
{
	DWORD		dwControlReg;
	WORD		wInvalidClock;
	
	//
	// See if the current input clock doesn't match the new digital mode
	//
	switch (byNewMode)
	{
		case DIGITAL_MODE_SPDIF_RCA :
		case DIGITAL_MODE_SPDIF_OPTICAL :
			wInvalidClock = ECHO_CLOCK_ADAT;
			break;
			
		case DIGITAL_MODE_ADAT :
			wInvalidClock = ECHO_CLOCK_SPDIF;
			break;
			
		default :
			wInvalidClock = 0xffff;
			break;
	}

	if (wInvalidClock == GetInputClock())
	{
		SetInputClock( ECHO_CLOCK_INTERNAL );
		SetSampleRate( 48000 );
	}

	
	//
	// Clear the current digital mode
	//
	dwControlReg = GetControlRegister();
	dwControlReg &= E3G_DIGITAL_MODE_CLEAR_MASK;

	//
	// Tweak the control reg
	//
	switch ( byNewMode )
	{
		default :
			return ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED;

		case DIGITAL_MODE_SPDIF_OPTICAL :
			dwControlReg |= E3G_SPDIF_OPTICAL_MODE;
			// fall through
			
		case DIGITAL_MODE_SPDIF_RCA :
			break;
			
		case DIGITAL_MODE_ADAT :
			dwControlReg |= E3G_ADAT_MODE;
			dwControlReg &= ~E3G_DOUBLE_SPEED_MODE;
			break;	
	}

	//
	// Write the control reg
	//
	WriteControlReg( dwControlReg, Get3gFreqReg(), TRUE );

	m_byDigitalMode = byNewMode;

	ECHO_DEBUGPRINTF( ("C3gDco::SetDigitalMode to %ld\n",
							(DWORD) m_byDigitalMode) );

	return ECHOSTATUS_OK;

}	// ECHOSTATUS C3gDco::SetDigitalMode



//===========================================================================
//
// WriteControlReg
//
//===========================================================================

ECHOSTATUS C3gDco::WriteControlReg
( 
	DWORD dwControlReg,
	DWORD	dwFreqReg,
	BOOL 	fForceWrite
)
{
	ECHOSTATUS Status;

	ECHO_DEBUGPRINTF(("C3gDco::WriteControlReg 0x%lx 0x%lx\n",dwControlReg,dwFreqReg));
	
	//
	// New value OK?
	//
	Status = ValidateCtrlReg(dwControlReg);
	if (ECHOSTATUS_OK != Status)
		return Status;

	//
	// Ready to go?
	//
	if ( !m_bASICLoaded )
	{
		ECHO_DEBUGPRINTF(("C3gDco::WriteControlReg - ASIC not loaded\n"));
		return( ECHOSTATUS_ASIC_NOT_LOADED );
	}
		
	if ( !WaitForHandshake() )
	{
		ECHO_DEBUGPRINTF(("C3gDco::WriteControlReg - no handshake\n"));
		return ECHOSTATUS_DSP_DEAD;
	}
	
	//
	// Write the control register
	//
	if (	fForceWrite || 
			(dwControlReg != GetControlRegister()) ||
			(dwFreqReg != Get3gFreqReg())
		)
	{
		m_pDspCommPage->dw3gFreqReg = SWAP( dwFreqReg );
		SetControlRegister( dwControlReg );

		ECHO_DEBUGPRINTF( ("C3gDco::WriteControlReg: Setting 0x%lx, 0x%lx\n",
								 dwControlReg,dwFreqReg) );
								 
		ClearHandshake();							 
		return SendVector( DSP_VC_WRITE_CONTROL_REG );
	}
	else
	{
		ECHO_DEBUGPRINTF( ("C3gDco::WriteControlReg: not written, no change\n") );
	}
	
	return ECHOSTATUS_OK;
	
} // ECHOSTATUS C3gDco::WriteControlReg


//===========================================================================
//
// SetSpdifBits
//
//===========================================================================

void C3gDco::SetSpdifBits(DWORD *pdwCtrlReg,DWORD dwSampleRate)
{
	DWORD dwCtrlReg;
	
	dwCtrlReg = *pdwCtrlReg;
	
	//
	// Clean out the old status bits
	//
	dwCtrlReg &= E3G_SPDIF_FORMAT_CLEAR_MASK;

	//
	// Sample rate
	// 
	switch (dwSampleRate)
	{
		case 32000 :
			dwCtrlReg |= E3G_SPDIF_SAMPLE_RATE0 |
							 E3G_SPDIF_SAMPLE_RATE1;
			break;
			
		case 44100 :
			if (m_bProfessionalSpdif)
				dwCtrlReg |= E3G_SPDIF_SAMPLE_RATE0;
			break;
			
		case 48000 :
			dwCtrlReg |= E3G_SPDIF_SAMPLE_RATE1;
			break;
	}
	
	//
	// Professional mode?
	//
	if (m_bProfessionalSpdif)
		dwCtrlReg |= E3G_SPDIF_PRO_MODE;
		
	//
	// Non-audio data?
	//
	if (m_bNonAudio)
		dwCtrlReg |= E3G_SPDIF_NOT_AUDIO;
		
	//
	// Always stereo, 24 bit, copy permit
	//
	dwCtrlReg |= E3G_SPDIF_24_BIT | E3G_SPDIF_TWO_CHANNEL | E3G_SPDIF_COPY_PERMIT;

	*pdwCtrlReg = dwCtrlReg;

} // SetSpdifBits


//===========================================================================
//
// SetSpdifOutNonAudio
//
// Set the state of the non-audio status bit in the S/PDIF out status bits
// 
//===========================================================================

void C3gDco::SetSpdifOutNonAudio(BOOL bNonAudio)
{
	DWORD		dwControlReg;

	m_bNonAudio = bNonAudio;

	dwControlReg = GetControlRegister();
	SetSpdifBits( &dwControlReg, SWAP(	m_pDspCommPage->dwSampleRate ));
	WriteControlReg( dwControlReg, Get3gFreqReg() );	
}	


//===========================================================================
//
// Set the S/PDIF output format
// 
//===========================================================================

void C3gDco::SetProfessionalSpdif
(
	BOOL bNewStatus
)
{
	DWORD		dwControlReg;

	m_bProfessionalSpdif = bNewStatus;

	dwControlReg = GetControlRegister();
	SetSpdifBits( &dwControlReg, SWAP(	m_pDspCommPage->dwSampleRate ));
	WriteControlReg( dwControlReg, Get3gFreqReg() );

}	// void C3gDco::SetProfessionalSpdif( ... )


//===========================================================================
//
// ASIC status check
//
// 3G ASIC status check returns different values depending on what kind of box
// is hooked up
//
//===========================================================================

BOOL C3gDco::CheckAsicStatus()
{
	DWORD	dwBoxStatus,dwBoxType;
	
	if ( !WaitForHandshake() )
	{
		ECHO_DEBUGPRINTF(("CheckAsicStatus - no handshake!\n"));
		return FALSE;
	}
		
	//
	// Send the vector command
	//
	m_pDspCommPage->dwExtBoxStatus = SWAP( (DWORD) E3G_ASIC_NOT_LOADED);
	m_bASICLoaded = FALSE;
	ClearHandshake();
	SendVector( DSP_VC_TEST_ASIC );	

	//
	// Wait for return from DSP
	//
	if ( !WaitForHandshake() )
	{
		ECHO_DEBUGPRINTF(("CheckAsicStatus - no handshake after VC\n"));
		m_pwDspCode = NULL;
		m_ullLastLoadAttemptTime = 0;	// so LoadFirmware will try again right away
		return FALSE;
	}
		
	//
	// What box type was set?
	//
	dwBoxStatus = SWAP(m_pDspCommPage->dwExtBoxStatus);
	if (E3G_ASIC_NOT_LOADED == dwBoxStatus)
	{
		ECHO_DEBUGPRINTF(("CheckAsicStatus - ASIC not loaded\n"));
		dwBoxType = NO3GBOX;
	}
	else
	{
		dwBoxType = dwBoxStatus & E3G_BOX_TYPE_MASK;
		m_bASICLoaded = TRUE;
		ECHO_DEBUGPRINTF(("CheckAsicStatus - read box type %x\n",dwBoxType));
	}
		
	m_dwCurrentBoxType = dwBoxType;

	//
	// Has the box type already been set?
	//
	if (m_bBoxTypeSet)
	{
		//
		// Did the ASIC load?
		// Was the box type correct?
		//
		if (	(NO3GBOX == dwBoxType) ||
				(dwBoxType != m_dwOriginalBoxType) )
		{
			ECHO_DEBUGPRINTF(("CheckAsicStatus - box type mismatch - original %x, got %x\n",m_dwOriginalBoxType,dwBoxType));
			return FALSE;
		}
			
		ECHO_DEBUGPRINTF(("CheckAsicStatus - ASIC ok\n"));
		m_bASICLoaded = TRUE;
		return TRUE;
	}
	
	//
	// First ASIC load - determine the box type and set up for that kind of box
	//		
	m_dwOriginalBoxType = dwBoxType;
	m_bBoxTypeSet = TRUE;

	SetChannelCounts();
	
	//
	// Set the bad board flag if no external box
	//
	if (NO3GBOX == dwBoxType)
	{
		ECHO_DEBUGPRINTF(("CheckAsicStatus - no external box\n"));
		m_bBadBoard = TRUE;
	}

	return m_bASICLoaded;

}	// BOOL C3gDco::CheckAsicStatus()


//===========================================================================
//
// SetPhantomPower
//
//===========================================================================

void C3gDco::SetPhantomPower(BOOL fPhantom)
{
	DWORD		dwControlReg;

	dwControlReg = GetControlRegister();
	if (fPhantom)
	{
		dwControlReg |= E3G_PHANTOM_POWER;
	}
	else
	{
		dwControlReg &= ~E3G_PHANTOM_POWER;
	}
	
	WriteControlReg( dwControlReg, Get3gFreqReg() );
}


//===========================================================================
//
// Set channel counts for the current box type
//
//===========================================================================

void C3gDco::SetChannelCounts()
{
	char *pszName;
	WORD ch,i;
	
	switch (m_dwOriginalBoxType)
	{
		case GINA3G :
			m_wNumPipesOut = 14;
			m_wNumPipesIn = 10;
			m_wFirstDigitalBusOut = 6;
			m_wFirstDigitalBusIn = 2;
			
			pszName = "Gina3G";
			break;
			
		
		case NO3GBOX :
		case LAYLA3G :
		default :
			m_wNumPipesOut = 16;
			m_wNumPipesIn = 16;
			m_wFirstDigitalBusOut = 8;
			m_wFirstDigitalBusIn = 8;
			
			pszName = "Layla3G";
			break;
	}

	m_wNumBussesOut = m_wNumPipesOut;
	m_wNumBussesIn = m_wNumPipesIn;
	strcpy( m_szCardName, pszName);
	
	//
	// Build a channel mask for ADAT inputs & outputs 3-8
	// OK to use bus # here since this hardware has no virtual outputs
	//
	m_Adat38Mask.Clear(); 
	ch = m_wFirstDigitalBusOut + 2;
	for (i = 0; i < 6; i++)
	{
		m_Adat38Mask.SetIndexInMask(ch);
		ch++;
	}
	
	ch += m_wFirstDigitalBusIn + 2;
	for (i = 0; i < 6; i++)
	{
		m_Adat38Mask.SetIndexInMask(ch);
		ch++;
	}
}


//===========================================================================
//
// Return the 3G box type
//
//===========================================================================

void C3gDco::Get3gBoxType(DWORD *pOriginalBoxType,DWORD *pCurrentBoxType)
{
	if (NULL != pOriginalBoxType)
		*pOriginalBoxType = m_dwOriginalBoxType;
		
	if (NULL != pCurrentBoxType)
	{
		CheckAsicStatus();
		
		*pCurrentBoxType = m_dwCurrentBoxType;
	}

} // Get3gBoxType



//===========================================================================
//
// Fill out an ECHOGALS_METERS struct using the current values in the 
// comm page.  This method is overridden for vmixer cards.
//
//===========================================================================

ECHOSTATUS C3gDco::GetAudioMeters
(
	PECHOGALS_METERS	pMeters
)
{
	pMeters->iNumPipesOut = 0;
	pMeters->iNumPipesIn = 0;

	//
	//	Output 
	// 
	DWORD dwCh = 0;
	WORD 	i;

	pMeters->iNumBussesOut = (INT32) m_wNumBussesOut;
	for (i = 0; i < m_wNumBussesOut; i++)
	{
		pMeters->iBusOutVU[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->VUMeter[ dwCh ]) );

		pMeters->iBusOutPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	pMeters->iNumBussesIn = (INT32) m_wNumBussesIn;	
	dwCh = E3G_MAX_OUTPUTS;
	for (i = 0; i < m_wNumBussesIn; i++)
	{
		pMeters->iBusInVU[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->VUMeter[ dwCh ]) );
		pMeters->iBusInPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}
	
	return ECHOSTATUS_OK;
	
} // GetAudioMeters



//===========================================================================
//
// Utility function; returns TRUE if double speed mode is set
//
//===========================================================================

BOOL C3gDco::DoubleSpeedMode(DWORD *pdwNewCtrlReg)
{
	DWORD dwControlReg;
	
	if (NULL == pdwNewCtrlReg)
		dwControlReg = GetControlRegister();
	else
		dwControlReg = *pdwNewCtrlReg;
	
	if (0 != (dwControlReg & E3G_DOUBLE_SPEED_MODE))
		return TRUE;
		
	return FALSE;
}


//===========================================================================
//
// Utility function; validates a new control register value.  Prevents
// speed change while transport is running
//
//===========================================================================

ECHOSTATUS C3gDco::ValidateCtrlReg(DWORD dwNewControlReg)
{
	BOOL fCurrDoubleSpeed,fNewDoubleSpeed;
	
	//
	// Return OK if transport is off
	//
	if (m_cmActive.IsEmpty())
		return ECHOSTATUS_OK;

	//
	// Get the new and current state of things
	//
	fNewDoubleSpeed = DoubleSpeedMode(&dwNewControlReg);
	fCurrDoubleSpeed = DoubleSpeedMode(NULL);	

	//
	// OK to change?
	//
	if (fCurrDoubleSpeed != fNewDoubleSpeed)
	{
		ECHO_DEBUGPRINTF(("Can't switch to speeds with transport active\n"));
		return ECHOSTATUS_INVALID_CHANNEL;
	}

	return ECHOSTATUS_OK;
}

// **** C3gDco.cpp ****
