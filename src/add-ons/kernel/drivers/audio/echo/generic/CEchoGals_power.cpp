// ****************************************************************************
//
//		CEchoGals_power.cpp
//
//		Power management functions for the CEchoGals driver class.
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


//===========================================================================
//
// Tell the hardware to go into a low-power state - the converters are 
// shut off and the DSP powers down.
//
//===========================================================================

ECHOSTATUS CEchoGals::GoComatose()
{
	//
	// Pass the call through to the DSP comm object
	//
	return GetDspCommObject()->GoComatose();

} // GoComatose


//===========================================================================
//
// Tell the hardware to wake up - go back to the full-power state
//
// You can call WakeUp() if you just want to be sure
// that the firmware is loaded OK
//
//===========================================================================

ECHOSTATUS CEchoGals::WakeUp()
{
	//
	// Load the firmware
	//
	ECHOSTATUS Status;
	CDspCommObject *pDCO = GetDspCommObject();
	
	Status = pDCO->LoadFirmware();

	return Status;
	

} // WakeUp


