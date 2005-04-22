// ****************************************************************************
//
//		CMidiInQ.cpp
//
//		Implementation file for the CMidiInQ class.
//		Use a simple fixed size queue for managing MIDI data.
//		Fill & drain pointers are maintained automatically whenever
//		an Add or Get function succeeds.
//
//		Set editor tabs to 3 for your viewing pleasure.
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

#include	"CEchoGals.h"


/****************************************************************************

	Construction and destruction, clean up and init

 ****************************************************************************/

//============================================================================
//
// Construction & destructor
//
//============================================================================

CMidiInQ::CMidiInQ()
{

	m_pBuffer = NULL;
	m_ullLastActivityTime = 0;
	
	m_wMtcState = MIDI_IN_STATE_NORMAL;

	m_pEG = NULL;	
	m_pMtcSync = NULL;
	
}	// CMidiInQ::CMidiInQ()


CMidiInQ::~CMidiInQ()
{

	Cleanup();
	
}	// CMidiInQ::~CMidiInQ()


//============================================================================
//
// Init
//
//============================================================================

ECHOSTATUS CMidiInQ::Init( CEchoGals *pEG )
{
	ECHO_DEBUGPRINTF(("CMidiInQ::Init\n"));

	m_pEG = pEG;

	m_dwFill = 0;
	m_dwBufferSizeMask =  MIDI_IN_Q_SIZE - 1;	// MIDI_IN_Q_SIZE must be a power of 2
									
	return ECHOSTATUS_OK;
									  	
}	// Init


//============================================================================
//
// Cleanup
//
//============================================================================

void CMidiInQ::Cleanup()
{
	//
	// Free the MIDI input buffer
	//
	if (m_pBuffer)
	{
		OsFreeNonPaged( m_pBuffer );
		m_pBuffer = NULL;
	}
	
	//
	// Free the MTC sync object if it exists
	//
	if (m_pMtcSync)
	{
		delete m_pMtcSync;
		m_pMtcSync = NULL;
	}
}	




/****************************************************************************

	Queue management - add and remove MIDI data

 ****************************************************************************/

//============================================================================
//
// GetMidi - get oldest MIDI input byte from the Q
//
//============================================================================

ECHOSTATUS CMidiInQ::GetMidi
(
	ECHOGALS_MIDI_IN_CONTEXT	*pContext,
	DWORD								&dwMidiByte,
	LONGLONG							&llTimestamp
)
{
	DWORD dwDrain;

	if (NULL == m_pBuffer)
		return ECHOSTATUS_CHANNEL_NOT_OPEN;
		
	dwDrain = pContext->dwDrain;
	if ( m_dwFill == dwDrain)
		return ECHOSTATUS_NO_DATA;
	
	dwMidiByte = m_pBuffer[ dwDrain ].dwMidi;
	llTimestamp = m_pBuffer[ dwDrain ].llTimestamp;
	
	//ECHO_DEBUGPRINTF( ("CMidiInQ::GetMidi 0x%lx\n", dwMidiByte) );

	dwDrain++;
	dwDrain &= m_dwBufferSizeMask;
	pContext->dwDrain = dwDrain;
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CMidiInQ::GetMidi


//============================================================================
//
// AddMidi - add new MIDI input byte to the Q
//
//============================================================================

ECHOSTATUS CMidiInQ::AddMidi
(
		DWORD		dwMidiByte,
		LONGLONG	llTimestamp
)
{
	DWORD dwFill;

	//ECHO_DEBUGPRINTF( ("CMidiInQ::AddMidi 0x%lx\n", dwMidiByte) );
	
	dwFill = m_dwFill;
	m_pBuffer[ dwFill ].dwMidi = dwMidiByte;
	m_pBuffer[ dwFill ].llTimestamp = llTimestamp;

	dwFill++;
	dwFill &= m_dwBufferSizeMask;
	m_dwFill = dwFill;

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CMidiInQ::AddMidi


//============================================================================
//
// Reset
//
//============================================================================

void CMidiInQ::Reset(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{
	pContext->dwDrain = m_dwFill;
	
}	// void CMidiInQ::Reset()




/****************************************************************************

	Arm and disarm

 ****************************************************************************/
 

//============================================================================
//
// Arm - resets the Q and enables the Q to start receiving MIDI input data
//
//============================================================================

ECHOSTATUS CMidiInQ::Arm(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{
	ECHOSTATUS Status;

	pContext->fOpen = FALSE;

	//
	// Return if there is no Echogals pointer
	// 	
	if (NULL == m_pEG)
	{
		return ECHOSTATUS_CANT_OPEN;
	}
		
	//-------------------------------------------------------------------------	
	//
	// Set up the MIDI input buffer
	//
	//-------------------------------------------------------------------------	
	
	if (NULL == m_pBuffer)
	{
		//
		// Reset the buffer pointers
		//
		m_dwFill = 0;

		//
		// Allocate
		//
		Status = OsAllocateNonPaged( MIDI_IN_Q_SIZE * sizeof(MIDI_DATA),
												(PPVOID) &m_pBuffer);
		if (Status != ECHOSTATUS_OK)
		{
			ECHO_DEBUGPRINTF(("CMidiInQ::Arm - Could not allocate MIDI input buffer\n"));
			return Status;
		}
	}

	Reset(pContext);
	m_dwNumClients++;

	//-------------------------------------------------------------------------	
	//
	// Enable MIDI input
	//
	//-------------------------------------------------------------------------	

	m_pEG->GetDspCommObject()->SetMidiOn( TRUE );

	pContext->fOpen = TRUE;	
	return ECHOSTATUS_OK;

} // Arm


//============================================================================
//
// Disarm - surprisingly, does the opposite of Arm
//
//============================================================================

ECHOSTATUS CMidiInQ::Disarm(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{
	//
	// Did this client open the MIDI input?
	//
	if (FALSE == pContext->fOpen)
	{
		ECHO_DEBUGPRINTF(("CMidiInQ::Disarm - trying to disarm with client that isn't open\n"));
		return ECHOSTATUS_CHANNEL_NOT_OPEN;
	}
	
	pContext->fOpen = FALSE;
	
	//
	// Last client?
	//
	if (0 == m_dwNumClients)
		return ECHOSTATUS_OK;

	m_dwNumClients--;

	if (0 == m_dwNumClients)
	{
		//
		// Tell the DSP to disable MIDI input
		//
		if (m_pEG)
			m_pEG->GetDspCommObject()->SetMidiOn( FALSE );
		
		//
		// Free the MIDI input buffer
		//
		if (m_pBuffer)
		{
			OsFreeNonPaged( m_pBuffer );
			m_pBuffer = NULL;
		}
	}
	
	return ECHOSTATUS_OK;
		
}	// Disarm


//============================================================================
//
// ArmMtcSync - turns on MIDI time code sync
//
//============================================================================

ECHOSTATUS CMidiInQ::ArmMtcSync()
{
	if (NULL == m_pEG)
		return ECHOSTATUS_DSP_DEAD;
		
	if (NULL != m_pMtcSync)
		return ECHOSTATUS_OK;
	
	//
	// Create the MTC sync object
	//	
	m_pMtcSync = new CMtcSync( m_pEG );
	if (NULL == m_pMtcSync)
		return ECHOSTATUS_NO_MEM;
		
	//
	// Tell the DSP to enable MIDI input
	//		
	m_pEG->GetDspCommObject()->SetMidiOn( TRUE );
		
	return ECHOSTATUS_OK;
		
}	// ArmMtcSync


//============================================================================
//
// DisarmMtcSync - turn off MIDI time code sync
//
//============================================================================

ECHOSTATUS CMidiInQ::DisarmMtcSync()
{
	if (NULL == m_pMtcSync)
		return ECHOSTATUS_OK;
	
	if (NULL == m_pEG)
		return ECHOSTATUS_DSP_DEAD;
		
	//
	// Tell the DSP to disable MIDI input
	//
	m_pEG->GetDspCommObject()->SetMidiOn( FALSE );
	
	//
	// Free m_pMtcSync
	//
	CMtcSync *pTemp;	// Use temp variable to avoid killing the object
							// while the ISR is using it	
							
	pTemp = m_pMtcSync;
	m_pMtcSync = NULL;
	delete pTemp;
	
	return ECHOSTATUS_OK;
		
}	// DisarmMtcSync




/****************************************************************************

	Detect MIDI input activity - see if the driver has recently received
	any MIDI input

 ****************************************************************************/
 
BOOL CMidiInQ::IsActive()
{
	ULONGLONG	ullCurTime,ullDelta;

	//
	// See if anything has happened recently
	//
	m_pEG->m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullDelta = ullCurTime - m_ullLastActivityTime;
	
	if (ullDelta > MIDI_ACTIVITY_TIMEOUT_USEC)
		return FALSE;
	
	return TRUE;	
	
}	// IsActive




/****************************************************************************

	MIDI time code

 ****************************************************************************/

//===========================================================================
//
// Get and set the base MTC sample rate
//
//===========================================================================


ECHOSTATUS CMidiInQ::GetMtcBaseRate(DWORD *pdwBaseRate)
{
	ECHOSTATUS Status;

	if (m_pMtcSync)
	{
		*pdwBaseRate = m_pMtcSync->m_dwBaseSampleRate;
		Status = ECHOSTATUS_OK;
	}
	else
	{
		*pdwBaseRate = 0;
		Status = ECHOSTATUS_NO_DATA;
	}
	
	return Status;
	
}	// GetMtcBaseRate


ECHOSTATUS CMidiInQ::SetMtcBaseRate(DWORD dwBaseRate)  
{
	ECHOSTATUS Status;
	
	if (m_pMtcSync)
	{
		m_pMtcSync->m_dwBaseSampleRate = dwBaseRate;
		Status = ECHOSTATUS_OK;
	}
	else
	{
		Status = ECHOSTATUS_NO_DATA;
	}
	
	return Status;
	
}	// SetMtcBaseRate


//===========================================================================
//
// Run the state machine for MIDI input data
//
// You need this state machine to parse the incoming
// MIDI data stream.  Every time the DSP sees a 0xF1 byte come in,
// it adds the DSP sample position to the MIDI data stream.
// The DSP sample position is represented as a 32 bit unsigned
// value, with the high 16 bits first, followed by the low 16 bits.
//
// The following logic parses the incoming MIDI data.
//
//===========================================================================

DWORD CMidiInQ::MtcProcessData( DWORD dwMidiData )
{
	DWORD dwRval;

	dwRval = 0;

	switch ( m_wMtcState )
	{

		case MIDI_IN_STATE_NORMAL :

			if ( dwMidiData == 0xF1L )
			{
				m_wMtcState = MIDI_IN_STATE_TS_HIGH;
			}
			break;


		case MIDI_IN_STATE_TS_HIGH :
		
			if (m_pMtcSync)
				m_pMtcSync->StoreTimestampHigh( dwMidiData );

			m_wMtcState = MIDI_IN_STATE_TS_LOW;
			dwRval = MIDI_IN_SKIP_DATA;
			break;


		case MIDI_IN_STATE_TS_LOW :

			if (m_pMtcSync)
				m_pMtcSync->StoreTimestampLow( dwMidiData );

			m_wMtcState = MIDI_IN_STATE_F1_DATA;
			dwRval = MIDI_IN_SKIP_DATA;
			break;


		case MIDI_IN_STATE_F1_DATA :
		
			if (m_pMtcSync)
				m_pMtcSync->StoreMtcData( dwMidiData );

			m_wMtcState = MIDI_IN_STATE_NORMAL;
			break;

	}

	return dwRval;
	
}	// DWORD CMidiInQ::MtcProcessData


//===========================================================================
// 
// ServiceMtcSync
//
//===========================================================================

void CMidiInQ::ServiceMtcSync()
{
	if (m_pMtcSync)
		m_pMtcSync->Sync();
	
}	// ServiceMtcSync




/****************************************************************************

	Interrupt service

 ****************************************************************************/
 
ECHOSTATUS CMidiInQ::ServiceIrq()
{
	DWORD				dwMidiCount;
	WORD				wIndex;
	ECHOSTATUS 		Status;
	CDspCommObject	*pDCO;
	LONGLONG			llTimestamp;

	//
	// Store the time for the activity detector
	//	
	m_pEG->m_pOsSupport->OsGetSystemTime( &m_ullLastActivityTime );

	//
	// Get the MIDI count
	//
	pDCO = m_pEG->GetDspCommObject();
	pDCO->ReadMidi( 0, dwMidiCount );	  // The count is at index 0

#ifdef ECHO_DEBUG
	//ECHO_DEBUGPRINTF( ("\tMIDI interrupt (%ld MIDI bytes)\n", dwMidiCount) );
	if ( dwMidiCount == 0 )
	{
		ECHO_DEBUGBREAK();
	}
#endif

	//
	// Get the timestamp
	//
	llTimestamp = m_pEG->m_pOsSupport->GetMidiInTimestamp();

	//
	// Get the MIDI data from the comm page
	//
	wIndex = 1;		// First MIDI byte is at index 1
	while ( dwMidiCount-- > 0 )
	{
		DWORD	dwMidiByte;

		//
		// Get the MIDI byte
		//
		Status = pDCO->ReadMidi( wIndex++, dwMidiByte );
		if ( ECHOSTATUS_OK != Status )
		{
			ECHO_DEBUGPRINTF(("Failed on ReadMidi\n"));
			ECHO_DEBUGBREAK();	// should never happen...
			break;
		}

		//
		// Parse the incoming MIDI stream.  The incoming MIDI data consists
		// of MIDI bytes and timestamps for the MIDI time code 0xF1 bytes.
		// MtcProcessData is a little state machine that parses the stream.
		//
		// If you get MIDI_IN_SKIP_DATA back, then this is a timestamp byte,
		// not a MIDI byte, so don't store it in the MIDI input buffer.
		//
		if ( MIDI_IN_SKIP_DATA == MtcProcessData( dwMidiByte ) )
			continue;
			
		//
		// Only store the MIDI data if there is at least one 
		// client
		//
		if (0 != m_dwNumClients)
		{
			//
			// Stash the MIDI data and check for overflow
			//
			if ( ECHOSTATUS_BUFFER_OVERFLOW == AddMidi( dwMidiByte, llTimestamp ) )
			{
				break;
			}
			
		}
	}		// while there is more MIDI data to read
	
	return ECHOSTATUS_OK;
	
} // ServiceIrq


// *** CMidiInQ.cpp ***
