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

//	Prevent problems with multiple includes
#ifndef _MIDIQUEUEOBJECT_
#define _MIDIQUEUEOBJECT_

typedef BYTE MIDI_DATA;
typedef MIDI_DATA *PMIDI_DATA;

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
		MIDI_DATA &Midi
	);

	//
	// Enable and disable MIDI input
	//
	ECHOSTATUS Arm();
	void Disarm();

	//
	// See if there has been any recent MIDI input activity
	//
	BOOL IsActive();

	//
	//	Reset the in/out ptrs
	//
	void Reset();
	
	//
	// Service a MIDI input interrupt
	//
	ECHOSTATUS ServiceIrq();


protected:

	//
	//	Find the next offset into the circular buffer
	// 
	PMIDI_DATA ComputeNext( PMIDI_DATA pCur );

	//
	// Add a MIDI byte to the circular buffer
	//
	inline ECHOSTATUS AddMidi
	(
		MIDI_DATA Midi
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
	PMIDI_DATA	m_pEnd;
	PMIDI_DATA	m_pFill;
	PMIDI_DATA	m_pDrain;
	
	BOOL			m_fArmed;

	//
	// Most recent MIDI input time - used for activity detector
	//	
	ULONGLONG	m_ullLastActivityTime;
	
	//
	// MIDI time code
	//
	WORD			m_wMtcState;
	
	//
	// Other objects
	//
	CEchoGals		*m_pEG;
	
};		// class CMidiInQ

typedef CMidiInQ * PCMidiInQ;

#endif

// *** CMidiInQ.H ***
