// ****************************************************************************
//
//		CEchoGalsVmixer.cpp
//
//		CEchoGalsVmixer is used to add virtual output mixing to the base
//		CEchoGals class.
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

#include "CEchoGalsVmixer.h"


//****************************************************************************
//
// Constructor and destructor
//
//****************************************************************************

CEchoGalsVmixer::CEchoGalsVmixer( PCOsSupport pOsSupport )
		  : CEchoGals( pOsSupport )
{
	ECHO_DEBUGPRINTF( ( "CEchoGalsVmixer::CEchoGalsVmixer() is born!\n" ) );
	
}	// CEchoGalsVmixer::CEchoGalsVmixer()


CEchoGalsVmixer::~CEchoGalsVmixer()
{
	ECHO_DEBUGPRINTF( ( "CEchoGalsVmixer::~CEchoGalsVmixer() is toast!\n" ) );
}	// CEchoGalsVmixer::~CEchoGalsVmixer()




//****************************************************************************
//
// Vmixer
//
//****************************************************************************

//===========================================================================
//
// Most of the cards don't have an actual output bus gain; they only
// have gain controls for output pipes; the output bus gain is implemented
// as a logical control.  Vmixer cards work fifferently; it does have 
// a physical output bus gain control, so just pass the gain down to the 
// DSP comm object.
//
//===========================================================================

ECHOSTATUS CEchoGalsVmixer::AdjustPipesOutForBusOut(WORD wBusOut,INT32 iBusOutGain)
{
	WORD wPipe;
	
	ECHO_DEBUGPRINTF(("CEchoGalsVmixer::AdjustPipesOutForBusOut wBusOut %d  iBusOutGain %ld\n",
							wBusOut,
							iBusOutGain));

	wBusOut &= 0xfffe;					
	for (wPipe = 1; wPipe < GetNumPipesOut(); wPipe++)
	{
		m_PipeOutCtrl.SetGain( wPipe, wBusOut, ECHOGAIN_UPDATE, FALSE);
	}

	m_PipeOutCtrl.SetGain( 0, wBusOut, ECHOGAIN_UPDATE, TRUE);

	return ECHOSTATUS_OK;
	
}	// AdjustPipesOutForBusOut


//===========================================================================
//
// GetBaseCapabilities is used by the CEchoGals-derived classes to
// help fill out the ECHOGALS_CAPS struct.  Override this here to add 
// the vmixer stuff.
//
//===========================================================================

ECHOSTATUS CEchoGalsVmixer::GetBaseCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	DWORD i;
	ECHOSTATUS Status;
	
	//
	// Base class
	//
	Status = CEchoGals::GetBaseCapabilities(pCapabilities);

	//
	// Add meters & pans to output pipes
	//
	for (i = 0 ; i < GetNumPipesOut(); i++)
	{
		pCapabilities->dwPipeOutCaps[i] |= ECHOCAPS_PEAK_METER |	
														ECHOCAPS_PAN;
	}
	
	return Status;
		
}	// GetBaseCapabilities


//===========================================================================
//
// Adjust all the monitor levels for a particular output bus
//
// For vmixer cards, need to force the "update monitors" command
// for the DSP 'cos it doesn't get sent otherwise.
//
//===========================================================================

ECHOSTATUS CEchoGalsVmixer::AdjustMonitorsForBusOut(WORD wBusOut)
{
	ECHOSTATUS Status;

	//
	// Call the base class
	//	
	Status = CEchoGals::AdjustMonitorsForBusOut( wBusOut );
	if (ECHOSTATUS_OK == Status)
	{
		GetDspCommObject()->UpdateAudioOutLineLevel();		
	}

	return Status;
		
}	// AdjustMonitorsForBusOut


// *** CEchoGalsVmixer.cpp ***
