// ****************************************************************************
//
//		CEchoGalsMTC.h
//
//		CEchoGalsMTC is used to add MIDI time code sync to the base
//		CEchoGals class.  CEchoGalsMTC derives from CEchoGals; CLayla and
//		CLayla24 derive in turn from CEchoGalsMTC.
//
//		Set editor tabs to 3 for your viewing pleasure.
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
#ifndef _CECHOGALSMTC_H_
#define _CECHOGALSMTC_H_

#include "CEchoGals.h"

class CEchoGalsMTC : public CEchoGals
{
public:
	//
	//	Construction/destruction
	//
	CEchoGalsMTC( PCOsSupport pOsSupport );
	virtual ~CEchoGalsMTC();
	
	//
	// Get and set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);
	virtual ECHOSTATUS GetInputClock(WORD &wClock);	
	
	//
	// Get and set sample rate
	//
	virtual ECHOSTATUS SetAudioSampleRate
	(
		DWORD		dwSampleRate
	);
    
	virtual ECHOSTATUS GetAudioSampleRate
	(
		PDWORD	pdwSampleRate
	);
	
	//
	// Update the sample rate based on received MTC data
	//
	virtual void ServiceMtcSync();

protected:

	WORD		m_wInputClock;

};		// class CEchoGalsMTC


#endif // _CECHOGALSMTC_H_

// *** CEchoGalsMTC.H ***
