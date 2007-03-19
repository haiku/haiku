// ****************************************************************************
//
//		CDarla.H
//
//		Include file for interfacing with the CDarla generic driver class
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
#ifndef _DARLAOBJECT_
#define _DARLAOBJECT_

#include "CEchoGals.h"
#include "CDarlaDspCommObject.h"

//
//	Class used for interfacing with the Darla audio card.
//
class CDarla : public CEchoGals
{
public:
	//
	//	Construction/destruction
	//
	CDarla( PCOsSupport pOsSupport );

	virtual ~CDarla();

	//
	// Setup & initialization methods
	//

	virtual ECHOSTATUS InitHw();

	//
	//	Audio Interface methods
	//

	virtual ECHOSTATUS QueryAudioSampleRate
	(
		DWORD		dwSampleRate
	);
	
	virtual void QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate);
	
	//
	//  Overload new & delete so memory for this object is allocated from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

protected:
	//
	//	Get access to the appropriate DSP comm object
	//
	PCDarlaDspCommObject GetDspCommObject()
		{ return( (PCDarlaDspCommObject) m_pDspCommObject ); }
};		// class CDarla

typedef CDarla * PCDarla;

#endif

// *** CDarla.H ***
