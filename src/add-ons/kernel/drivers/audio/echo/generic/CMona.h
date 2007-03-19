// ****************************************************************************
//
//		CMona.H
//
//		Include file for interfacing with the CMona generic driver class
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
#ifndef _MONAOBJECT_
#define _MONAOBJECT_

#include "CEchoGals.h"
#include "CMonaDspCommObject.h"

//
//	Class used for interfacing with the Darla audio card.
//
class CMona : public CEchoGals
{
public:
	//
	//	Construction/destruction
	//
	CMona( PCOsSupport pOsSupport );

	virtual ~CMona();

	//
	// Setup & initialization methods
	//

	virtual ECHOSTATUS InitHw();

	//
	//	Adapter information methods
	//

	//
	//	Return the capabilities of this card; card type, card name,
	//	# analog inputs, # analog outputs, # digital channels,
	//	# MIDI ports and supported clocks.
	//	See ECHOGALS_CAPS definition above.
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
	//	Overload new & delete so memory for this object is allocated from 
	//	non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

protected:
	//
	//	Get access to the appropriate DSP comm object
	//
	PCMonaDspCommObject GetDspCommObject()
		{ return( (PCMonaDspCommObject) m_pDspCommObject ); }
};		// class CMona

typedef CMona * PCMona;

#endif

// *** CMona.H ***
