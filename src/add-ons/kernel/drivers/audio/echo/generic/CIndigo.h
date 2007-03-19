// ****************************************************************************
//
//		CIndigo.H
//
//		Include file for interfacing with the CIndigo generic driver class
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
#ifndef _INDIGO_OBJECT_
#define _INDIGO_OBJECT_

#include "CEchoGalsVmixer.h"
#include "CIndigoDspCommObject.h"

//
//	Class used for interfacing with the Indigo Cardbus adapter
//
class CIndigo : public CEchoGalsVmixer
{
public:
	//
	//	Construction/destruction
	//
	CIndigo( PCOsSupport pOsSupport );

	virtual ~CIndigo();

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
	// Ask if a given sample rate is supported
	//
	virtual ECHOSTATUS QueryAudioSampleRate
	(
		DWORD		dwSampleRate
	);

	virtual void QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate);

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
	PCIndigoDspCommObject GetDspCommObject()
		{ return( (PCIndigoDspCommObject) m_pDspCommObject ); }

	// 
	// No monitors for Indigo Prime, so override this function
	//
	virtual ECHOSTATUS AdjustMonitorsForBusOut(WORD wBusOut);

};		// class CIndigo

typedef CIndigo * PCIndigo;

#endif

// *** CIndigo.H ***
