// ****************************************************************************
//
//		CLayla.h
//
//		Include file for interfacing with the CLayla generic driver class
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
#ifndef _LAYLAOBJECT_
#define _LAYLAOBJECT_

#include "CEchoGalsMTC.h"
#include "CLaylaDspCommObject.h"


//
//	Class used for interfacing with the Layla audio card.
//
class CLayla : public CEchoGalsMTC
{
public:
	//
	//	Construction/destruction
	//
	CLayla( PCOsSupport pOsSupport );

	virtual ~CLayla();

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
	//	Audio Interface methods
	//
	virtual ECHOSTATUS QueryAudioSampleRate
	(
		DWORD		dwSampleRate
	);
	
	virtual void QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate);

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
	PCLaylaDspCommObject GetDspCommObject()
		{ return( (PCLaylaDspCommObject) m_pDspCommObject ); }

};		// class CLayla

typedef CLayla * PCLayla;

#endif

// *** CLayla.H ***
