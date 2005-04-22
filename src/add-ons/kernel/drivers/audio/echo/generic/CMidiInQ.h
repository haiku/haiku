// ****************************************************************************
//
//		CMidiInQ.h
// 	
// 	This class manages MIDI input.
//
//		Use a simple fixed size queue for storing MIDI data.
//
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

//	Prevent problems with multiple includes
#ifndef _MIDIQUEUEOBJECT_
#define _MIDIQUEUEOBJECT_

#include "CMtcSync.h"

typedef struct tMIDI_DATA
{
	DWORD		dwMidi;	
	LONGLONG	llTimestamp;
}	
MIDI_DATA;

typedef MIDI_DATA *PMIDI_DATA;

// 
// Default to one MIDI input client
//
#ifndef MAX_MIDI_IN_CLIENTS
#define MAX_MIDI_IN_CLIENTS		1
#endif

//
// MIDI in queue size
//
// Total buffer size in bytes will be MIDI_IN_Q_SIZE * sizeof(MIDI_DATA)
//
// Buffer size must be a power of 2.  This is the default size; to change it,
// add #define MIDI_IN_Q_SIZE to your OsSupport??.h file
//
#ifndef MIDI_IN_Q_SIZE
#define MIDI_IN_Q_SIZE	128
#endif

//
//	Class used for simple MIDI byte queue
//
class CMidiInQ
{
public:
	//
	//	Construction/destruction
	//
	CMidiInQ();
	~CMidiInQ();
	
	//
	// Init
	//
	ECHOSTATUS Init(CEchoGals *pEG);

	//
	//	Get the oldest byte from the circular buffer
	//
	ECHOSTATUS GetMidi
	(
		ECHOGALS_MIDI_IN_CONTEXT	*pContext,
		DWORD 							&dwMidiByte,
		LONGLONG							&llTimestamp
	);

	//
	// Enable and disable MIDI input and MTC sync
	//
	ECHOSTATUS Arm(ECHOGALS_MIDI_IN_CONTEXT *pContext);
	ECHOSTATUS Disarm(ECHOGALS_MIDI_IN_CONTEXT *pContext);
	ECHOSTATUS ArmMtcSync();	
	ECHOSTATUS DisarmMtcSync();
	
	//
	// Get and set MTC base rate
	//
	ECHOSTATUS GetMtcBaseRate(DWORD *pdwBaseRate);
	ECHOSTATUS SetMtcBaseRate(DWORD dwBaseRate);
	
	//
	// If MTC sync is turned on, process received MTC data
	// and update the sample rate
	//
	void ServiceMtcSync();

	//
	// See if there has been any recent MIDI input activity
	//
	BOOL IsActive();

	//
	//	Reset the in/out ptrs
	//
	void Reset(ECHOGALS_MIDI_IN_CONTEXT *pContext);
	
	//
	// Service a MIDI input interrupt
	//
	ECHOSTATUS ServiceIrq();


protected:

	//
	// Add a MIDI byte to the circular buffer
	//
	inline ECHOSTATUS AddMidi
	(
		DWORD		dwMidiByte,
		LONGLONG	llTimestamp
	);
	
	//
	// Parse MIDI time code data
	//
	DWORD MtcProcessData( DWORD dwMidiData );
	
	//
	// Cleanup
	//
	void Cleanup();
	
	//
	//	Midi buffer management
	//
	PMIDI_DATA 	m_pBuffer;
	DWORD			m_dwFill;
	DWORD			m_dwBufferSizeMask;
	DWORD			m_dwNumClients;

	//
	// Most recent MIDI input time - used for activity detector
	//	
	ULONGLONG	m_ullLastActivityTime;
	
	//
	// MIDI time code
	//
	WORD			m_wMtcState;
	CMtcSync		*m_pMtcSync;
	
	//
	// Other objects
	//
	CEchoGals		*m_pEG;
	
};		// class CMidiInQ

typedef CMidiInQ * PCMidiInQ;

#endif

// *** CMidiInQ.H ***
