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

	Reset();
	
	m_pBuffer = NULL;
	m_ullLastActivityTime = 0;
	
	m_wMtcState = MTC_STATE_NORMAL;

	m_pEG = NULL;	
	
	m_fArmed = FALSE;
	
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
	ECHOSTATUS Status;

	ECHO_DEBUGPRINTF(("CMidiInQ::Init\n"));

	m_pEG = pEG;
	
	Status = OsAllocateNonPaged(	sizeof(MIDI_DATA) * ECHO_MIDI_QUEUE_SZ,
											(PPVOID) &m_pBuffer);
									
	if (ECHOSTATUS_OK == Status)
		Reset();											
									
	return Status;											
									  	
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
	if (NULL != m_pBuffer)
	{
		if (ECHOSTATUS_OK != OsFreeNonPaged( m_pBuffer ))
		{
			ECHO_DEBUGPRINTF(("CMidiInQ::Cleanup - Failed to free MIDI input bufer\n"));
		}
	}
	
	m_pBuffer = NULL;
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
	MIDI_DATA &Midi
)
{
	if (NULL == m_pBuffer)
		return ECHOSTATUS_CHANNEL_NOT_OPEN;

	if ( m_pFill == m_pDrain )
		return( ECHOSTATUS_NO_DATA );
	
	Midi = *m_pDrain;
	
	ECHO_DEBUGPRINTF( ("CMidiInQ::GetMidi 0x%x\n", Midi) );
	
	m_pDrain = ComputeNext( m_pDrain );
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CMidiInQ::GetMidi


//============================================================================
//
// AddMidi - add new MIDI input byte to the Q
//
//============================================================================

ECHOSTATUS CMidiInQ::AddMidi
(
	MIDI_DATA Midi
)
{
	if ( ComputeNext( m_pFill ) == m_pDrain )
	{
		ECHO_DEBUGPRINTF( ("CMidiInQ::AddMidi buffer overflow\n") );
		ECHO_DEBUGBREAK();
		return ECHOSTATUS_BUFFER_OVERFLOW;
	}

	ECHO_DEBUGPRINTF( ("CMidiInQ::AddMidi 0x%x\n", Midi) );

	*m_pFill = Midi;
	m_pFill = ComputeNext( m_pFill );

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CMidiInQ::AddMidi


//============================================================================
//
// ComputeNext - protected routine used to wrap the read and write pointers
// around the circular buffer
//
//============================================================================

PMIDI_DATA CMidiInQ::ComputeNext( PMIDI_DATA pCur )
{
	if ( ++pCur >= m_pEnd )
		pCur = m_pBuffer;
		
	return pCur;
		
}	// PMIDI_DATA CMidiInQ::ComputeNext( PMIDI_DATA pCur )


//============================================================================
//
// Reset
//
//============================================================================

void CMidiInQ::Reset()
{
	//
	//	Midi buffer pointer initialization
	//
	m_pEnd = m_pBuffer + ECHO_MIDI_QUEUE_SZ;
	m_pFill = m_pBuffer;
	m_pDrain = m_pBuffer;
	
}	// void CMidiInQ::Reset()




/****************************************************************************

	Arm and disarm

 ****************************************************************************/
 

//============================================================================
//
// Arm - resets the Q and enables the Q to start receiving MIDI input data
//
//============================================================================

ECHOSTATUS CMidiInQ::Arm()
{
	//
	// Return if the MIDI input buffer is not allocated
	// 	
	if ( 	(NULL == m_pBuffer) ||
			(NULL == m_pEG)
		)
		return ECHOSTATUS_CANT_OPEN;
	
	//
	// Reset the buffer pointers
	//
	Reset();

	//
	// Tell the DSP to enable MIDI input
	//		
	m_pEG->GetDspCommObject()->SetMidiOn( TRUE );
	
	//
	// Set the flag
	//
	m_fArmed = TRUE;

	return ECHOSTATUS_OK;

} // Arm


//============================================================================
//
// Disarm - surprisingly, does the opposite of Arm
//
//============================================================================

void CMidiInQ::Disarm()
{
	//
	// Tell the DSP to disable MIDI input
	//
	if (m_pEG)
		m_pEG->GetDspCommObject()->SetMidiOn( FALSE );
	
	//
	// Clear the flag
	//
	m_fArmed = FALSE;

}	// Disarm




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
// Run the state machine for MIDI input data
//
// MIDI time code sync isn't supported by this code right now,
// but you still need this state machine to parse the incoming
// MIDI data stream.  Every time the DSP sees a 0xF1 byte come in,
// it adds the DSP sample position to the MIDI data stream.
// The DSP sample position is represented as a 32 bit unsigned
// value, with the high 16 bits first, followed by the low 16 bits.
//
// Since these aren't real MIDI bytes, the following logic is
// needed to skip them.
//
//===========================================================================

DWORD CMidiInQ::MtcProcessData( DWORD dwMidiData )
{
	switch ( m_wMtcState )
	{
		case MTC_STATE_NORMAL :
			if ( dwMidiData == 0xF1L )
				m_wMtcState = MTC_STATE_TS_HIGH;
			break;
		case MTC_STATE_TS_HIGH :
			m_wMtcState = MTC_STATE_TS_LOW;
			return MTC_SKIP_DATA;
			break;
		case MTC_STATE_TS_LOW :
			m_wMtcState = MTC_STATE_F1_DATA;
			return MTC_SKIP_DATA;
			break;
		case MTC_STATE_F1_DATA :
			m_wMtcState = MTC_STATE_NORMAL;
			break;
	}
	return 0;
	
}	// DWORD CMidiInQ::MtcProcessData




/****************************************************************************

	Interrupt service

 ****************************************************************************/
 
ECHOSTATUS CMidiInQ::ServiceIrq()
{
	DWORD				dwMidiCount;
	WORD				wIndex;
	ECHOSTATUS 		Status;
	CDspCommObject	*pDCO;

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
	ECHO_DEBUGPRINTF( ("\tMIDI interrupt (%ld MIDI bytes)\n", dwMidiCount) );
	if ( dwMidiCount == 0 )
	{
		ECHO_DEBUGBREAK();
	}
#endif

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
		// MtcProcessData is a little state machine that parses the strem.
		//
		// If you get MTC_SKIP_DATA back, then this is a timestamp byte,
		// not a MIDI byte, so don't store it in the MIDI input buffer.
		//
		if ( MTC_SKIP_DATA == MtcProcessData( dwMidiByte ) )
			continue;
			
		//
		// Only store the MIDI data if MIDI input is armed
		//
		if (m_fArmed)
		{
			//
			// Stash the MIDI and timestamp data and check for overflow
			//
			if ( ECHOSTATUS_BUFFER_OVERFLOW == AddMidi( (MIDI_DATA) dwMidiByte ) )
			{
				break;
			}
			
		}
	}		// while there is more MIDI data to read
	
	return ECHOSTATUS_OK;
	
} // ServiceIrq


// *** CMidiInQ.cpp ***
