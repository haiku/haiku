// ****************************************************************************
//
//		CIndigoDJ.H
//
//		Include file for interfacing with the CIndigoDJ generic driver class
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
#ifndef _INDIGO_DJ_OBJECT_
#define _INDIGO_DJ_OBJECT_

#include "CIndigo.h"
#include "CIndigoDJDspCommObject.h"

//
//	Class used for interfacing with the Indigo DJ Cardbus adapter
//
class CIndigoDJ : public CIndigo
{
public:
	//
	//	Construction/destruction
	//
	CIndigoDJ( PCOsSupport pOsSupport );

	virtual ~CIndigoDJ();

	//
	//	Overload new & delete so memory for this object is allocated from 
	//	non-paged memory.	// fixme do I need this for each derived class?
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 
	
	//
	// Setup & initialization methods
	//
	virtual ECHOSTATUS InitHw();

	//
	// Get the audio latency for a single pipe
	//
	virtual void GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency);

protected:
	//
	//	Get access to the appropriate DSP comm object
	//
	PCIndigoDJDspCommObject GetDspCommObject()
		{ return( (PCIndigoDJDspCommObject) m_pDspCommObject ); }
};		// class CIndigoDJ

typedef CIndigoDJ * PCIndigoDJ;

#endif

// *** CIndigoDJ.H ***
