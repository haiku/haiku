// ****************************************************************************
//
//		CLayla24.h
//
//		Include file for interfacing with the CLayla24 generic driver class
//		Set editor tabs to 3 for your viewing pleasure.
//
//   Copyright Echo Digital Audio Corporation (c) 1998 - 2004
//   All rights reserved
//   www.echoaudio.com
//   
//   This file is part of Echo Digital Audio's generic driver library.
//   
//   Echo Digital Audio's generic driver library is free software; 
//   you can redistribute it and/or modify it under the terms of 
//   the GNU General Public License as published by the Free Software Foundation.
//   
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
//   MA  02111-1307, USA.
//
// ****************************************************************************

//	Prevent problems with multiple includes
#ifndef _LAYLA24OBJECT_
#define _LAYLA24OBJECT_

#include "CEchoGalsMTC.h"
#include "CLayla24DspCommObject.h"


//
//	Class used for interfacing with the Layla24 audio card.
//
class CLayla24 : public CEchoGalsMTC
{
public:
	//
	//	Construction/destruction
	//
	CLayla24( PCOsSupport pOsSupport );
	virtual ~CLayla24();

	//
	// Setup & initialization methods
	//
	virtual ECHOSTATUS InitHw();

	//
	//	Adapter information methods
	//
	virtual ECHOSTATUS GetCapabilities
	(
		PECHOGALS_CAPS	pCapabilities
	);

	//
	//	Audio interface methods
	//
	virtual ECHOSTATUS QueryAudioSampleRate
	(
		DWORD		dwSampleRate
	);

	//
	// Get a bitmask of all the clocks the hardware is currently detecting
	//
	virtual ECHOSTATUS GetInputClockDetect(DWORD &dwClockDetectBits);
	
	//
	//  Overload new & delete so memory for this object is allocated from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

protected:
	//
	//	Get access to the appropriate DSP comm object
	//
	PCLayla24DspCommObject GetDspCommObject()
		{ return( (PCLayla24DspCommObject) m_pDspCommObject ); }

};		// class CLayla24

typedef CLayla24 * PCLayla24;

#endif

// *** Layla24.H ***
