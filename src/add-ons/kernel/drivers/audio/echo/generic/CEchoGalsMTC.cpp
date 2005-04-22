// ****************************************************************************
//
//		CEchoGalsMTC.cpp
//
//		CEchoGalsMTC is used to add MIDI time code sync to the base
//		CEchoGals class.  CEchoGalsMTC derives from CEchoGals; CLayla and
//		CLayla24 derive in turn from CEchoGalsMTC.
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

#include "CEchoGalsMTC.h"


//****************************************************************************
//
// Constructor and destructor
//
//****************************************************************************

CEchoGalsMTC::CEchoGalsMTC( PCOsSupport pOsSupport )
		  : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CEchoGalsMTC::CEchoGalsMTC() is born!\n" ) );
	
	m_wInputClock = ECHO_CLOCK_INTERNAL;
	
}	// CEchoGalsMTC::CEchoGalsMTC()


CEchoGalsMTC::~CEchoGalsMTC()
{
	ECHO_DEBUGPRINTF( ( "CEchoGalsMTC::~CEchoGalsMTC() is toast!\n" ) );
}	// CEchoGalsMTC::~CEchoGalsMTC()




//****************************************************************************
//
// Input clock
//
//****************************************************************************

//============================================================================
//
// Set the input clock
//
// This needs to intercept the input clock value here since MTC sync is
// actually implemented in software
//
//============================================================================

ECHOSTATUS CEchoGalsMTC::SetInputClock(WORD wClock)
{
	ECHOSTATUS Status;
	
	Status = ECHOSTATUS_OK;
	
	//
	// Check for MTC clock
	//
	if (ECHO_CLOCK_MTC == wClock) 
	{
		if (ECHO_CLOCK_MTC != m_wInputClock)
		{
			//
			// Tell the MIDI input object to enable MIDI time code sync
			//
			Status = m_MidiIn.ArmMtcSync();

			if (ECHOSTATUS_OK == Status)
			{
				//
				// Store the current clock as MTC...
				//		
				m_wInputClock = ECHO_CLOCK_MTC;
				
				//
				// but set the real clock to internal.
				//
				Status = CEchoGals::SetInputClock( ECHO_CLOCK_INTERNAL );
			}
			
		}
	}
	else
	{
		//
		// Pass the clock setting to the base class
		//
		Status = CEchoGals::SetInputClock( wClock );
		if (ECHOSTATUS_OK == Status)
		{
			WORD wOldClock;
			DWORD dwRate;
			
			//
			// Get the base rate for MTC sync
			//
			m_MidiIn.GetMtcBaseRate( &dwRate );
		
			//
			// Make sure MTC sync is off
			//
			m_MidiIn.DisarmMtcSync();
			
			//
			// Store the new clock
			//
			wOldClock = m_wInputClock;
			m_wInputClock = wClock;
			
			//
			// If the previous clock was MTC, re-set the sample rate
			//
			if (ECHO_CLOCK_MTC == wOldClock)
				SetAudioSampleRate( dwRate );
		}
	}
	
	return Status;
	
}	// SetInputClock


//============================================================================
//
// Get the input clock
//
//============================================================================

ECHOSTATUS CEchoGalsMTC::GetInputClock(WORD &wClock)
{
	wClock = m_wInputClock;
	
	return ECHOSTATUS_OK;
}	


//****************************************************************************
//
// Sample rate
//
//****************************************************************************

//============================================================================
//
// Set the sample rate
//
// Again, the rate needs to be intercepted here.
//
//============================================================================

ECHOSTATUS CEchoGalsMTC::SetAudioSampleRate( DWORD dwSampleRate )
{
	ECHOSTATUS Status;

	//
	// Syncing to MTC?
	//
	if (ECHO_CLOCK_MTC == m_wInputClock)
	{
		//
		// Test the rate
		//
		Status = QueryAudioSampleRate( dwSampleRate );
		
		//
		// Set the base rate if it's OK
		//
		if (ECHOSTATUS_OK == Status)
		{
			m_MidiIn.SetMtcBaseRate( dwSampleRate );
		}
	}
	else
	{
		//
		// Call the base class
		//
		Status = CEchoGals::SetAudioSampleRate( dwSampleRate );
	}

	return Status;
	
}	// SetAudioSampleRate


//============================================================================
//
// Get the sample rate
//
//============================================================================

ECHOSTATUS CEchoGalsMTC::GetAudioSampleRate( PDWORD pdwSampleRate )
{
	ECHOSTATUS Status;
	
	if (NULL == pdwSampleRate)
		return ECHOSTATUS_INVALID_PARAM;

	//
	// Syncing to MTC?
	//
	if (ECHO_CLOCK_MTC == m_wInputClock)
	{
		//
		// Get the MTC base rate
		//	
		Status = m_MidiIn.GetMtcBaseRate( pdwSampleRate );
	}
	else
	{
		//
		// Call the base class
		//
		Status = CEchoGals::GetAudioSampleRate( pdwSampleRate );
	}

	return Status;
	
}	// GetAudioSampleRate




//****************************************************************************
//
// Call this periodically to change the sample rate based on received MTC 
// data
//
//****************************************************************************

void CEchoGalsMTC::ServiceMtcSync()
{
	m_MidiIn.ServiceMtcSync();	
}	


// *** CEchoGalsMTC.cpp ***
