// ****************************************************************************
//
//		CEchoGals_midi.cpp
//
//		Implementation file for the CEchoGals driver class (midi functions).
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

#include "CEchoGals.h"


#ifdef MIDI_SUPPORT

/****************************************************************************

	MIDI output

 ****************************************************************************/

//===========================================================================
//
// Write a bunch of MIDI data to the MIDI output
//
// The DSP only buffers up 64 bytes internally for MIDI output; if you try
// to send more than the DSP can handle, the actual count sent will be returned
// to you.  ECHOSTATUS_BUSY is returned if the DSP is still processing the 
// last driver command.
//
//===========================================================================

ECHOSTATUS CEchoGals::WriteMidi
(
	DWORD		dwExpectedCt,
	PBYTE		pBuffer,
	PDWORD	pdwActualCt
)
{
	return GetDspCommObject()->WriteMidi( 	pBuffer,
														dwExpectedCt,
														pdwActualCt );
}	// ECHOSTATUS CLayla24::WriteMidi




/****************************************************************************

	MIDI input

 ****************************************************************************/


//===========================================================================
//
//	Read a single MIDI byte from the circular MIDI input buffer
// 
//===========================================================================

ECHOSTATUS CEchoGals::ReadMidiByte
(
	ECHOGALS_MIDI_IN_CONTEXT	*pContext,
	DWORD								&dwMidiData,
	LONGLONG							&llTimestamp
)
{
	
	return m_MidiIn.GetMidi(pContext,dwMidiData,llTimestamp);
	
}	// ReadMidiByte


//===========================================================================
//
//	Open and enable the MIDI input
//
// The context struct should be set to zero before calling OpenMidiInput
// 
//===========================================================================

ECHOSTATUS CEchoGals::OpenMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{

	return m_MidiIn.Arm(pContext);
	
}	// OpenMidiInput


//===========================================================================
//
//	Close and disable the MIDI input
// 
//===========================================================================

ECHOSTATUS CEchoGals::CloseMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{
	return m_MidiIn.Disarm(pContext);
}	


//===========================================================================
//
//	Reset the MIDI input, but leave it open and enabled
// 
//===========================================================================

ECHOSTATUS CEchoGals::ResetMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext)
{
	m_MidiIn.Reset(pContext);
	
	return ECHOSTATUS_OK;
}	



#endif // MIDI_SUPPORT

