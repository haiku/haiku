// ****************************************************************************
//
//		CMidiQueue.cpp
//
//		Implementation file for the CMidiQueue class.
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
#include	"CMidiQueue.h"

/****************************************************************************

	Construction and destruction

 ****************************************************************************/

CMidiQueue::CMidiQueue()
{
	Reset();
}	// CMidiQueue::CMidiQueue()


CMidiQueue::~CMidiQueue()
{
}	// CMidiQueue::~CMidiQueue()




/****************************************************************************

	Add and retrieve data

 ****************************************************************************/

//===========================================================================
//
// Get the oldest byte out of the buffer
//
//===========================================================================

ECHOSTATUS CMidiQueue::GetMidi
(
	MIDI_DATA &Midi
)
{
	if ( m_pFill == m_pDrain )
		return( ECHOSTATUS_NO_DATA );
	
	Midi = *m_pDrain;
	
	ECHO_DEBUGPRINTF( ("CMidiQueue::GetMidi 0x%x", Midi) );
	
	m_pDrain = ComputeNext( m_pDrain );
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CMidiQueue::GetMidi


//===========================================================================
//
// Put a new byte into the buffer
//
//===========================================================================

ECHOSTATUS CMidiQueue::AddMidi
(
	MIDI_DATA Midi
)
{
	if ( ComputeNext( m_pFill ) == m_pDrain )
	{
		ECHO_DEBUGPRINTF( ("CMidiQueue::AddMidi buffer overflow\n") );
		ECHO_DEBUGBREAK();
		return( ECHOSTATUS_BUFFER_OVERFLOW );
	}

	ECHO_DEBUGPRINTF( ("CMidiQueue::AddMidi 0x%x\n", Midi) );

	*m_pFill = Midi;
	m_pFill = ComputeNext( m_pFill );

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CMidiQueue::AddMidi


//===========================================================================
//
// Move the head or tail pointer, wrapping at the end of the buffer
//
//===========================================================================

PMIDI_DATA CMidiQueue::ComputeNext( PMIDI_DATA pCur )
{
	if ( ++pCur >= m_pEnd )
		pCur = m_Buffer;
		
	return pCur;
		
}	// PMIDI_DATA CMidiQueue::ComputeNext( PMIDI_DATA pCur )


//===========================================================================
//
// Reset the MIDI input buffer
//
//===========================================================================

void CMidiQueue::Reset()
{
	//
	//	Midi buffer pointer initialization
	//
	m_pEnd = m_Buffer + ECHO_MIDI_QUEUE_SZ;
	m_pFill = m_Buffer;
	m_pDrain = m_Buffer;
}	// void CMidiQueue::Reset()


typedef CMidiQueue * PCMidiQueue;

// *** CMidiQueue.cpp ***
