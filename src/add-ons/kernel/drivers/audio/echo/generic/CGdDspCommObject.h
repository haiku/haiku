// ****************************************************************************
//
//		CGdDspCommObject.H
//
//		Darla20 and Gina20 are very similar; this class is used for both.
//		CGinaDspCommObject and CDarlaDspCommObject dervie from this class, in
//		turn.
//
// ----------------------------------------------------------------------------
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

#ifndef	_GDDSPCOMMOBJECT_
#define	_GDDSPCOMMOBJECT_

#include "CDspCommObject.h"


class CGdDspCommObject : public CDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	CGdDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport  );
	virtual ~CGdDspCommObject();

	//
	//	Set the DSP sample rate.
	//	Return rate that was set, -1 if error
	//
	virtual DWORD SetSampleRate( DWORD dwNewSampleRate );
	//
	//	Send current setting to DSP & return what it is
	//
	virtual DWORD SetSampleRate()
	{ return SetSampleRate( GetSampleRate() ); }

	//
	// Put the card to sleep
	//
	virtual ECHOSTATUS GoComatose();

protected:
	BYTE 	m_byGDCurrentSpdifStatus;
	BYTE 	m_byGDCurrentClockState;

	//
	//	Called after load firmware to restore old gains, meters on, monitors, etc.
	//	No error checking is done here.
	//
	virtual void RestoreDspSettings();

	// SelectGinaDarlaSpdifStatus
	BYTE SelectGinaDarlaSpdifStatus( DWORD dwNewSampleRate );
	
};		// class CGdDspCommObject

typedef CGdDspCommObject * PCGdDspCommObject;

#endif

// **** GdDspCommObject.h ****
